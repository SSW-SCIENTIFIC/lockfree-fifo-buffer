cmake_minimum_required(VERSION 3.8)

include(FindThreads)
include(CheckCCompilerFlag)

if(NOT Threads_FOUND)
    message(FATAL_ERROR "No thread library found.")
endif()

add_library(lockfree_queue)
target_sources(lockfree_queue PRIVATE
    lockfree_fifo_buffer.c
    multiwriter_fifo_buffer.c
)

target_include_directories(lockfree_queue PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/>
    $<INSTALL_INTERFACE:include/>
)

check_c_compiler_flag(-Wextra HAS_WEXTRA)
check_c_compiler_flag(-Weverything HAS_WEVERYTHING)
check_c_compiler_flag(-Werror HAS_WERROR)
target_compile_options(lockfree_queue PRIVATE
    $<$<BOOL:${HAS_WEXTRA}>:-Wextra>
    $<$<BOOL:${HAS_WEVERYTHING}>:-Weverything>
    $<$<BOOL:${HAS_WERROR}>:-Werror=pedantic>
    -Wall
)

set_target_properties(lockfree_queue PROPERTIES
    C_STANDARD 11
    C_EXTENSION off
)

target_link_libraries(lockfree_queue Threads::Threads)
