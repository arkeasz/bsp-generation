cmake_minimum_required(VERSION 3.20)

project("bsp")

add_subdirectory(raylib)

add_executable(bsp main.c)

target_link_libraries(bsp PRIVATE raylib)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)