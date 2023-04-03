cmake_minimum_required(VERSION 3.8)

#set(CMAKE_VERBOSE_MAKEFILE ON)

# libbpf directories
set(LIBBPF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/libbpf)
set(LIBBPF_INCL ${LIBBPF_DIR}/src/root/usr/include)
set(LIBBPF_INCL2 ${LIBBPF_DIR}/include)
set(LIBBPF_LIB ${LIBBPF_DIR}/src/root/usr/lib64)

# xdp-tutorial directories
set(XDPTUTORIAL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../dependencies/xdp-tutorial)
set(XDPTUTORIAL_COMMON ${XDPTUTORIAL_DIR}/common)

link_directories(${LIBBPF_LIB})

project(xdp-drop_and_count)

set(CMAKE_C_COMPILER "clang")

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

ADD_LIBRARY(xdp-tutorial-commons ${XDPTUTORIAL_COMMON}/common_user_bpf_xdp.c ${XDPTUTORIAL_COMMON}/common_libbpf.c ${XDPTUTORIAL_COMMON}/common_params.c)
target_include_directories(xdp-tutorial-commons PRIVATE ${LIBBPF_INCL} ${LIBBPF_INCL2} ${XDPTUTORIAL_COMMON})

# The user-space program interacting with the BPF program in kernel
add_executable(xdp-drop_and_count-user xdp-drop_and_count-user.c)
target_include_directories(xdp-drop_and_count-user PRIVATE ${LIBBPF_INCL} ${XDPTUTORIAL_COMMON})
target_link_libraries(xdp-drop_and_count-user xdp-tutorial-commons bpf elf)

# The BPF program executing in kernel
add_subdirectory(bpf)
