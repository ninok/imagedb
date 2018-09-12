cmake_minimum_required (VERSION 2.6)
project (imgcmp)


include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(imgcmp main.cpp)

target_link_libraries(imgcmp ${CONAN_LIBS})
