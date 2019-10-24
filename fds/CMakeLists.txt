cmake_minimum_required(VERSION 3.15)
project(fds C)

set(CMAKE_C_STANDARD 11)

add_executable(fds FileDataServer.c)