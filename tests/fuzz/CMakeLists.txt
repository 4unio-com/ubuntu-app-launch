
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fsanitize=address -fsanitize-coverage=trace-pc-guard")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fsanitize=address -fsanitize-coverage=edge")

add_executable (desktop-fuzz
	desktop-fuzz.cpp)
target_link_libraries (desktop-fuzz
	launcher-static
	-fsanitize=address
	-fsanitize-coverage=edge
	/usr/lib/llvm-3.9/lib/libFuzzer.a)

add_custom_target(format-fuzz
	COMMAND clang-format -i -style=file
	desktop-fuzz.cpp
)

add_test (NAME desktop-fuzz-test
	  COMMAND find ${CMAKE_CURRENT_SOURCE_DIR}/desktop-corpus/ -exec ${CMAKE_CURRENT_BINARY_DIR}/desktop-fuzz "{}" ";")
