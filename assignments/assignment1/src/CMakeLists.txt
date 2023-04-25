cmake_minimum_required(VERSION 3.8)

project(assignment1)

set(CMAKE_C_COMPILER "clang")

set(CMAKE_C_STANDARD 11)

add_executable(server server.c)

add_executable(client client.c)
