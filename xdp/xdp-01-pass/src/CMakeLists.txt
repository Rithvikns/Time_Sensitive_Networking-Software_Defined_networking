cmake_minimum_required(VERSION 3.8)

#set(CMAKE_VERBOSE_MAKEFILE ON)

# libbpf directories
set(LIBBPF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/libbpf)
set(LIBBPF_INCL_DIR ${LIBBPF_DIR}/src)
include_directories(${LIBBPF_INCL_DIR})

project(xdp-pass)

SET (CMAKE_C_COMPILER "clang")

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

add_subdirectory(bpf)
