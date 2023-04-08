cmake_minimum_required(VERSION 3.8)

#set(CMAKE_VERBOSE_MAKEFILE ON)

# libbpf directories
set(LIBBPF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/libbpf)
set(LIBBPF_INCL_DIR ${LIBBPF_DIR}/src/root/usr/include)
include_directories(${LIBBPF_INCL_DIR})

project(xdp-pass)

set(CMAKE_C_COMPILER "clang")

set(CMAKE_C_STANDARD 11)

add_library(xdp-pass-bpf OBJECT xdp-pass-bpf.c)

target_compile_options(xdp-pass-bpf PRIVATE -target bpf -Wall)

add_custom_target(copy_bpf_objects
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_OBJECTS:xdp-pass-bpf> "${CMAKE_BINARY_DIR}/"
	COMMAND_EXPAND_LISTS
	)
