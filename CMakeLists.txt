cmake_minimum_required(VERSION 3.19)
project(thread_pool)

set(CMAKE_CXX_STANDARD 14)

add_executable(thread_pool main.cpp thread_pool.h thread_pool.cpp)