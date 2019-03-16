cmake_minimum_required(VERSION 3.13)
project(lab_7 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(lab_7 main.c)