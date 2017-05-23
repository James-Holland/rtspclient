cmake_minimum_required(VERSION 3.6)
project(rtsptc2 C CXX)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp)
add_executable(testrtsp ${HEADER_FILES} ${SOURCE_FILES} )

# CMake links -lpthread correctly i.e after everything else
find_package(Threads REQUIRED)
target_link_libraries ( testrtsp ${CMAKE_THREAD_LIBS_INIT} )

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB_PKG glib-2.0)
message( "Glib found = ${GLIB_PKG_FOUND}" )
message( "Glib flags = ${GLIB_PKG_CFLAGS}" )
message( "Glib flags = ${GLIB_PKG_LDFLAGS}" )
target_include_directories(testrtsp PUBLIC ${GLIB_PKG_INCLUDE_DIRS})
target_link_libraries(testrtsp ${GLIB_PKG_LIBRARIES} ${GLIB_PKG_LDFLAGS})
target_compile_options(testrtsp PUBLIC ${GLIB_PKG_CFLAGS})

# Add the gstreamer modules.
pkg_check_modules(GST REQUIRED gstreamer-1.0
        REQUIRED gstreamer-base-1.0
        REQUIRED gstreamer-controller-1.0
        REQUIRED gstreamer-net-1.0
        REQUIRED gstreamer-rtp-1.0
        REQUIRED gstreamer-rtsp-1.0
        REQUIRED gstreamer-sdp-1.0)
add_definitions(${GST_DEFINITIONS})
target_include_directories(testrtsp PUBLIC ${GST_INCLUDE_DIRS} )
target_link_libraries(testrtsp ${GST_LIBRARIES} ${GST_LDFLAGS} )
target_compile_options(testrtsp PUBLIC ${GST_CFLAGS})

# Boost dependencies
find_package(Boost COMPONENTS system filesystem REQUIRED)
target_link_libraries(testrtsp ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_REGEX_LIBRARY})


#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++11")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -std=c++11")

