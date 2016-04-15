cmake_minimum_required (VERSION 3.4)
project (c-rrb)

SET (CMAKE_C_FLAGS "-Wall -Wextra -pedantic -std=c99")

INCLUDE (CheckIncludeFiles)
check_include_files (gc.h HAVE_GC)

include_directories ("${PROJECT_SOURCE_DIR}/src")
add_library(rrb src/rrb.c)

install(TARGETS rrb
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib)

enable_testing()

function(add_rrb_test target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} gc rrb)

    add_test(${target} ${target})
endfunction()


include_directories ("${PROJECT_SOURCE_DIR}/test-suite")
add_rrb_test(catslice test-suite/test_catslice.c)
add_rrb_test(concat test-suite/test_concat.c)
add_rrb_test(fibocat test-suite/test_fibocat.c)
add_rrb_test(peek test-suite/test_peek.c)
add_rrb_test(pop test-suite/test_pop.c)
add_rrb_test(push test-suite/test_push.c)
add_rrb_test(slice test-suite/test_slice.c)
add_rrb_test(transient-pop test-suite/test_transient_pop.c)
add_rrb_test(transient-push test-suite/test_transient_push.c)
add_rrb_test(transient-push-2 test-suite/test_transient_push_2.c)
add_rrb_test(transient-update test-suite/test_transient_update.c)
add_rrb_test(update test-suite/test_update.c)
