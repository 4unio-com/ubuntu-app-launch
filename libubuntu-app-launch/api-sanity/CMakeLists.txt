
set(ABI_BUILD_HEADERS ${LAUNCHER_HEADERS})
list(APPEND ABI_BUILD_HEADERS ${LAUNCHER_CPP_HEADERS})
list_prefix(ABI_BUILD_HEADERS_PATH ABI_BUILD_HEADERS "${CMAKE_SOURCE_DIR}/libubuntu-app-launch/")

set(HEADER_AS_LIST "")
foreach(HEADER IN LISTS ABI_BUILD_HEADERS_PATH)
	set(HEADER_AS_LIST "${HEADER_AS_LIST}\n${HEADER}")
endforeach()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/build.xml.in ${CMAKE_CURRENT_BINARY_DIR}/build.xml @ONLY)

add_custom_target (gen-api-fuzz
	COMMAND api-sanity-checker -l ubuntu-app-launch -d ${CMAKE_CURRENT_BINARY_DIR}/build.xml -gen
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/build.xml
)

add_custom_target (build-api-fuzz ALL
	COMMAND api-sanity-checker -l ubuntu-app-launch -d ${CMAKE_CURRENT_BINARY_DIR}/build.xml -build
	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/build.xml gen-api-fuzz
)

add_test (NAME api-fuzz
	COMMAND api-sanity-checker -l ubuntu-app-launch -d ${CMAKE_CURRENT_BINARY_DIR}/build.xml -run)
