cmake_minimum_required(VERSION 3.10)
project(ex4 C)

set(CMAKE_C_STANDARD 99)

add_executable(ex4 main.c threadPool.c threadPool.h osqueue.c osqueue.h)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")