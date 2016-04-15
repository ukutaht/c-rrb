cmake_minimum_required (VERSION 3.4)
project (c-rrb)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
set(CMAKE_C_FLAGS_DEBUG, "${CMAKE_C_FLAGS_DEBUG} -Wall -Weverything -Wno-cast-align")

include_directories ("${PROJECT_SOURCE_DIR}/src")
add_library(rrb src/rrb.c)
