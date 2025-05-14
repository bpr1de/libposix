cmake_minimum_required(VERSION 3.26)
project(posix++ VERSION 0.0.1 DESCRIPTION "POSIX for C++")

set(CMAKE_CXX_STANDARD 11)

include_directories(include)
add_compile_options(-Wall -O2)

add_library(posix++ SHARED
        auto_fd.cc
        auto_pipe.cc
        module.cc
        process.cc)

set_target_properties(posix++ PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
        PUBLIC_HEADER "include/libposix.hh")
target_include_directories(posix++ PUBLIC include)

add_library(auto_fd_test MODULE
        auto_fd.cc)
target_compile_definitions(auto_fd_test PRIVATE AUTO_FD_TEST)
add_library(auto_pipe_test MODULE
        auto_fd.cc
        auto_pipe.cc)
target_compile_definitions(auto_pipe_test PRIVATE AUTO_PIPE_TEST)
add_library(process_test MODULE
        process.cc)
target_compile_definitions(process_test PRIVATE PROCESS_TEST)

install(TARGETS posix++
        LIBRARY DESTINATION lib
        PUBLIC_HEADER DESTINATION include)

add_executable(test-runner
        unit_test.cc)
add_dependencies(test-runner
        auto_fd_test
        auto_pipe_test
        process_test)
target_link_libraries(test-runner PRIVATE posix++)

add_custom_target(test
        COMMAND test-runner ./lib*_test.so
        WORKING_DIRECTORY ${CMAKE_PROJECT_DIR})
