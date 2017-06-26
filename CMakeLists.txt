cmake_minimum_required(VERSION 2.8)
project(ftp_client)

set(CMAKE_CXX_STANDARD 11)

include_directories(. ./include)

set(SOURCE_FILES
        ./src/connection.c
        ./src/dir_list.c
        ./src/main.c
        ./src/translation.c
   )

add_executable(ftp_client ${SOURCE_FILES})