cmake_minimum_required(VERSION 3.14)

project(user C ASM)

set(CROSS_COMPILE "aarch64-linux-gnu-")

set(CMAKE_C_COMPILER "${CROSS_COMPILE}gcc")
set(CMAKE_ASM_COMPILER "${CROSS_COMPILE}gcc")

set(C_COMPILER_NOSTDINC "-nostdinc -nostdlib -nostartfiles")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    ${C_COMPILER_NOSTDINC}")

message(${CMAKE_C_FLAGS})

set(LINK_FLAGS "-static")

add_subdirectory(lab3)
include_directories(".")