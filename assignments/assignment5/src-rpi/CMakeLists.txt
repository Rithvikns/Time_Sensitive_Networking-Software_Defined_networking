cmake_minimum_required (VERSION 2.6)
find_package (Threads)

add_executable (speedcmd-sender speedcmd-sender.c tty.h tty.c socketcan.h socketcan.c)
target_link_libraries (speedcmd-sender ${CMAKE_THREAD_LIBS_INIT})

add_executable (positionreq-sender positionreq-sender.c tty.h socketcan.h socketcan.c)
target_link_libraries (positionreq-sender ${CMAKE_THREAD_LIBS_INIT})
