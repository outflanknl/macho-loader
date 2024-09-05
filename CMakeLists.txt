cmake_minimum_required(VERSION 3.11)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)

project(macho-loader LANGUAGES CXX)

set(TARGET_ARCH "x86_64" CACHE STRING "Target architecture")
string(TOUPPER ${TARGET_ARCH} TARGET_ARCH_UPPER)
set_property(CACHE TARGET_ARCH PROPERTY STRINGS "x86_64")

file(GLOB_RECURSE SOURCES "src/*.cpp")
include_directories("src")

set(RELEASE_FLAGS "-Os -ffunction-sections -v -fdata-sections -fno-exceptions -fPIC -fno-stack-protector -fno-common -nostdlib -nodefaultlibs")
set(DEBUG_FLAGS "-O0 -D_DEBUG -g")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${RELEASE_FLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${RELEASE_FLAGS}")

elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${DEBUG_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "${DEBUG_FLAGS}")

else()
    message(FATAL_ERROR "Unsupported build type: `${CMAKE_BUILD_TYPE}`")
endif()

string(APPEND MACOS_TARGET_FLAGS " -arch x86_64")
string(APPEND CMAKE_EXE_LINKER_FLAGS " ${MACOS_TARGET_FLAGS}")
string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${MACOS_TARGET_FLAGS}")
string(APPEND CMAKE_CXX_FLAGS " ${MACOS_TARGET_FLAGS}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(${PROJECT_NAME} ${SOURCES})

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(MACOS_RELEASE_LINKER_FLAGS "-static -Wl,-no_data_const -Wl,-no_function_starts -image_base 0 -Wl,-order_file,\"${CMAKE_SOURCE_DIR}/src/order.ld\"")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${MACOS_RELEASE_LINKER_FLAGS}")

    add_library(${PROJECT_NAME} MODULE ${SOURCES})
endif()
