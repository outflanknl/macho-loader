cmake_minimum_required(VERSION 3.11)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_STANDARD 11)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")

project(test LANGUAGES C)

add_executable(${PROJECT_NAME} main.c)
