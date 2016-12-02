
add_executable (desktop-fuzz
	desktop-fuzz.cpp)
target_link_libraries (desktop-fuzz
	launcher-static)
