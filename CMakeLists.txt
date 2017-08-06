cmake_minimum_required (VERSION 2.8)

project (sequence)

add_library(sequence SHARED sequence.c)

# special for mingw32
target_link_libraries(sequence PUBLIC -Wl,--add-stdcall-alias -static-libgcc)

