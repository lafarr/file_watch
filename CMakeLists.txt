cmake_minimum_required(VERSION 3.10)

project(file_watch)

file(GLOB_RECURSE KEYD_SOURCES "keyd/*.cpp")

set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(settings INTERFACE)

target_compile_options(settings INTERFACE
	-Wall
	-Wextra
	-Wpedantic
	-Wno-unused-parameter
	-Wunused
	-Wunused-variable
	-Wunused-result
	-Winline                       
	-Wdisabled-optimization        
	-Wmissing-noreturn
	-Wpessimizing-move             
	-Wredundant-move               
	-Wmove                         
	-Wshift-overflow
	-Wshift-sign-overflow
	-Winteger-overflow
	-Wdivision-by-zero
	-Wnull-dereference
	-Wstrict-overflow=5            
	-Warray-bounds
	-Wsequence-point               
	-Wfree-nonheap-object
	-Waddress                      
	-Wmismatched-new-delete
	-Wcast-align                   
	-Wcast-qual                    
	-Wold-style-cast               
	-Wsign-compare
	-Wsign-conversion
	-Wdouble-promotion             
	-Wfloat-conversion
	-Wnarrowing
	-Wconversion                   
	-Wint-to-pointer-cast
	-Wpointer-arith
	-Wshadow                       
	-Wshadow-all                   
	-Wswitch-enum                  
	-Wswitch-default               
	-Wimplicit-fallthrough         
	-Wdeprecated                   
	-Wdeprecated-copy              
	-Wdeprecated-copy-dtor
	-Wzero-as-null-pointer-constant
	-Wnon-virtual-dtor             
	-Woverloaded-virtual           
	-Wextra-semi
	-Wunreachable-code
	-Wunreachable-code-return
	-Wmissing-field-initializers
	-Wredundant-decls
	-Wctor-dtor-privacy            
	-Wnoexcept-type
	-Wregister                     
	-Wcomma                        
	-Wformat=2                     
	-Wformat-security
	-Wformat-overflow
	-Wformat-truncation
	-fstrict-aliasing                 
	-Wpadded                          
	-Wnoexcept-type                   
	-fno-rtti                         
	-Wmissing-noreturn                

	$<$<CONFIG:Debug>:
		-fsanitize=undefined           
		-fsanitize=address             
		-fsanitize=leak                
		-Og
		-ggdb3
	>

	$<$<CONFIG:Release>:
        -O3
        -march=native
        -fno-exceptions
        -fno-rtti
    >
)

target_link_options(settings INTERFACE
	$<$<CONFIG:Debug>:
		-fsanitize=address
		-fsanitize=undefined
		-fsanitize=leak
	>
)

add_executable(keyd_watch ${KEYD_SOURCES})
target_link_libraries(keyd_watch PRIVATE settings)
