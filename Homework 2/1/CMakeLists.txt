cmake_minimum_required(VERSION 3.22)
project(Paral)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

add_executable(mult mult_mv.cpp)