cmake_minimum_required (VERSION 2.6)
project (imgcmp)

set(Boost_USE_STATIC_LIBS        ON) # only find static libs
set(Boost_USE_MULTITHREADED      ON)

find_package(Boost
    1.63.0
    REQUIRED
    COMPONENTS filesystem)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIR_RELEASE})
    
#set (OPENSSL_USE_STATIC_LIBS TRUE )
# find_package(OpenSSL
    # 1.1.0
    # REQUIRED)

#FIXME: Don't use ENV
include_directories($ENV{CIMG_ROOT_DIR})

#FIXME: Don't use ENV
include_directories( $ENV{JPEG_ROOT}/include )
link_directories( $ENV{JPEG_ROOT}/lib )

add_executable(imgcmp main.cpp)

target_link_libraries(imgcmp ${Boost_LIBRARIES})
# target_link_libraries(imgcmp "turbojpeg.lib")
target_link_libraries(imgcmp "jpeg-static.lib")
