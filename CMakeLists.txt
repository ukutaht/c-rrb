cmake_minimum_required (VERSION 3.4)
project (c-rrb)

INCLUDE (CheckIncludeFiles)
check_include_files (gc.h HAVE_GC)

include_directories ("${PROJECT_SOURCE_DIR}/src")
add_library(rrb src/rrb.c)

install (FILES src/rrb.h DESTINATION include)

install (TARGETS rrb
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib)
