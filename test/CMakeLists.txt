cmake_minimum_required(VERSION 3.14)

include(${PROJECT_SOURCE_DIR}/CMake/DownloadProject/DownloadProject.cmake)
include(GoogleTest)

download_project(
    PROJ googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.10.0
    UPDATE_DISCONNECTED
)
add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})

include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(lockfree_fifo_buffer_test)
target_sources(lockfree_fifo_buffer_test PRIVATE
    lockfree_fifo_buffer_test.cpp
)
target_include_directories(lockfree_fifo_buffer_test PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(lockfree_fifo_buffer_test lockfree_queue gtest_main)
gtest_discover_tests(lockfree_fifo_buffer_test)

add_executable(multiwriter_fifo_buffer_test)
target_sources(multiwriter_fifo_buffer_test PRIVATE
    multiwriter_fifo_buffer_test.cpp
)
target_include_directories(multiwriter_fifo_buffer_test PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(multiwriter_fifo_buffer_test lockfree_queue gtest_main)
gtest_discover_tests(multiwriter_fifo_buffer_test)

set_target_properties(
    lockfree_fifo_buffer_test
    multiwriter_fifo_buffer_test
    PROPERTIES
        C_STANDARD 11
        C_EXTENSION off
        CXX_STANDARD 17
        CXX_EXTENSION off
)
