cmake_minimum_required(VERSION 3.8)

#set(CMAKE_VERBOSE_MAKEFILE ON)

# libbpf directories
set(LIBBPF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/libbpf)
set(LIBBPF_INCL_DIR ${LIBBPF_DIR}/src/root/usr/include)

# xdp-tutorial directories
#set(XDPTUTORIAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/xdp-tutorial)

# common files used by all XDP projects
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../common)

include_directories(${LIBBPF_INCL_DIR} ${XDPTUTORIAL_DIR} ${COMMON_DIR})

project(xdp-drop_and_count)

set(CMAKE_C_COMPILER "clang")

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# The user-space program interacting with the BPF program in kernel
add_executable(xdp-drop_and_count-user xdp-drop_and_count-user.c ${COMMON_DIR}/xdp-common.c)

# The BPF program executing in kernel
add_subdirectory(bpf)
