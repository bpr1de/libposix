cmake_minimum_required(VERSION 3.26)
project(posix++ VERSION 0.0.1 DESCRIPTION "POSIX for C++")

set(CMAKE_CXX_STANDARD 11)

add_library(posix++ SHARED
        auto_fd.cc
        auto_pipe.cc
        posix_error.cc
        process.cc)

set_target_properties(posix++ PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER include/libposix.hh)
target_include_directories(posix++ PRIVATE include)

add_executable(test
        unit_test.cc)

target_link_libraries(test PRIVATE posix++)
