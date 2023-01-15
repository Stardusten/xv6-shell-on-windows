cmake_minimum_required(VERSION 3.21)
project(design_new C)

set(CMAKE_C_STANDARD 11)

add_executable(sh sh.c sh.h debug.c mem.c parser.c runcmd.c)

add_executable(cat cat.c)
