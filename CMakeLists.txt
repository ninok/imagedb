cmake_minimum_required (VERSION 2.6)
project (imgcmp)

set(CMAKE_CXX_STANDARD 17)

include(${CMAKE_SOURCE_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(imgcmp main.cpp)
set_target_properties(imgcmp PROPERTIES CMAKE_CXX_STANDARD 17)
target_link_libraries(imgcmp ${CONAN_LIBS})
