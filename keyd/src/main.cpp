#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <iostream>
#include <print>
#include <span>
#include <string_view>
#include <sys/inotify.h>
#include <unistd.h>

constexpr std::string_view watched_dir = "/etc/keyd";
constexpr std::string_view target_name = "default.conf";

class unique_fd {
	public:
		explicit unique_fd(int fd = -1) noexcept : fd_(fd) {}

		unique_fd(const unique_fd&) = delete;
		unique_fd& operator=(const unique_fd&) = delete;

		unique_fd(unique_fd&& other) noexcept : fd_(other.fd_) {
			other.fd_ = -1;
		}

		unique_fd& operator=(unique_fd&& other) noexcept {
			if (this != &other) {
				reset();
				fd_ = other.fd_;
				other.fd_ = -1;
			}
			return *this;
		}

		~unique_fd() { reset(); }

		[[nodiscard]] int get() const noexcept { return fd_; }
		[[nodiscard]] explicit operator bool() const noexcept { return fd_ >= 0; }

		void reset(int new_fd = -1) noexcept {
			if (fd_ >= 0) {
				close(fd_);
			}
			fd_ = new_fd;
		}

	private:
		int fd_;
};

struct parsed_event {
    std::string_view name{};
    std::size_t total_size{};
    inotify_event header{};
};

[[nodiscard]] std::expected<parsed_event, std::string_view>
parse_event(std::span<const std::byte> bytes, std::size_t offset)
{
    if (offset > bytes.size()) {
        return std::unexpected("offset out of range");
    }

    if (bytes.size() - offset < sizeof(inotify_event)) {
        return std::unexpected("short inotify header");
    }

    parsed_event result{};

    auto header_src = bytes.subspan(offset, sizeof(inotify_event));
    auto header_dst = std::as_writable_bytes(std::span{&result.header, 1});
    std::ranges::copy(header_src, header_dst.begin());

    result.total_size = sizeof(inotify_event) + result.header.len;

    if (bytes.size() - offset < result.total_size) {
        return std::unexpected("short inotify payload");
    }

    auto name_bytes =
        bytes.subspan(offset + sizeof(inotify_event), result.header.len);

    auto* name_ptr = reinterpret_cast<const char*>(name_bytes.data());
    auto name_len =
        result.header.len > 0 ? strnlen(name_ptr, result.header.len) : 0;

    result.name = std::string_view{name_ptr, name_len};
    return result;
}

[[nodiscard]] bool is_target_event(const parsed_event& ev)
{
    return ev.name == target_name &&
           ((ev.header.mask & IN_CLOSE_WRITE) ||
            (ev.header.mask & IN_MOVED_TO) ||
            (ev.header.mask & IN_CREATE));
}

[[nodiscard]] int restart_keyd()
{
    return std::system("systemctl restart keyd");
}

int main()
{
    unique_fd inotify_fd{inotify_init1(IN_CLOEXEC)};
    if (!inotify_fd) {
        std::println(std::cerr, "inotify_init1 failed: {}",
                     std::strerror(errno));
        return EXIT_FAILURE;
    }

    int wd = inotify_add_watch(
        inotify_fd.get(),
        watched_dir.data(),
        IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE
    );
    if (wd < 0) {
        std::println(std::cerr, "inotify_add_watch failed: {}",
                     std::strerror(errno));
        return EXIT_FAILURE;
    }

    std::array<std::byte, 4096> buf{};

    for (;;) {
        ssize_t bytes_read_raw = read(inotify_fd.get(), buf.data(), buf.size());
        if (bytes_read_raw < 0) {
            if (errno == EINTR) {
                continue;
            }

            std::println(std::cerr, "read failed: {}", std::strerror(errno));
            break;
        }

        auto bytes_read = static_cast<std::size_t>(bytes_read_raw);
        std::span<const std::byte> bytes{buf.data(), bytes_read <= buf.size() ? bytes_read : buf.size()};

        for (std::size_t event_idx = 0; event_idx < bytes.size();) {
            auto parsed = parse_event(bytes, event_idx);
            if (!parsed) {
                std::println(std::cerr, "parse error: {}", parsed.error());
                break;
            }

            const auto& ev = *parsed;

            if (is_target_event(ev)) {
                int rc = restart_keyd();
                if (rc == 0) {
                    std::println("restarted keyd");
                } else {
                    std::println(std::cerr,
                                 "systemctl restart keyd failed, rc={}", rc);
                }
            }

            event_idx += ev.total_size;
        }
    }

    return EXIT_SUCCESS;
}
