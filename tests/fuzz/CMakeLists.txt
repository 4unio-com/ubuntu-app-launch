
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_executable (desktop-fuzz
	desktop-fuzz.cpp)
target_link_libraries (desktop-fuzz
	launcher-static
	/usr/lib/llvm-3.9/lib/libFuzzer.a)
