cmake_minimum_required(VERSION 2.8)
project(ftp_client)
set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_C_COMPILER, gcc)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -O2)
add_definitions(-w)
include_directories(. ./include)

set(SOURCE_FILES
        ./src/connection.c
        ./src/dir_list.c
        ./src/main.c
        ./src/translation.c
   )

add_executable(ftp_client ${SOURCE_FILES})