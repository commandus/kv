cmake_minimum_required(VERSION 3.9)
project(kv VERSION 1.0)

INCLUDE_DIRECTORIES("D:/s/loracli/libs/include")
LINK_DIRECTORIES("D:/s/loracli/libs/32" "D:/git/openssl/build/static/x86/lib")

set(TEST_VEGA_CLASS_SRC tests/test-vega-class.cpp	vega.cpp lora-device.cpp lora-packet.cpp lora-device-list.cpp search-options.cpp packet-vega-sh-2.cpp 
	packet-vega-si-13.cpp utilstring.cpp
)
add_executable(test-vega-class ${TEST_VEGA_CLASS_SRC})
target_link_libraries(test-vega-class ev uwsc ssl crypto Ws2_32 Crypt32)

target_include_directories(test-vega-class PUBLIC "${PROJECT_BINARY_DIR}")
target_include_directories(test-vega-class PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(test-vega-class PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
target_include_directories(test-vega-class PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/tests")

INSTALL(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/test-vega-class DESTINATION tests)
INSTALL(FILES README.md LICENSE HISTORY COPYING TODO CODE_OF_CONDUCT.md CONTRIBUTING.md DESTINATION doc)

include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${loracli_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${loracli_VERSION_MINOR}")
include(CPack)