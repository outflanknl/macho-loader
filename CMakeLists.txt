cmake_minimum_required(VERSION 3.11)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE INTERNAL "" FORCE)

project(macho-loader LANGUAGES CXX)

set(TARGET_ARCH "arm64" CACHE STRING "Target architecture")
string(TOUPPER ${TARGET_ARCH} TARGET_ARCH_UPPER)
set_property(CACHE TARGET_ARCH PROPERTY STRINGS "arm64")

file(GLOB_RECURSE SOURCES "src/*.cpp")
include_directories("src")

set(RELEASE_FLAGS "-Os -ffunction-sections -v -fdata-sections -fno-exceptions -fPIC -fvisibility=hidden -fno-ident -fvisibility-inlines-hidden -fno-common -fno-stack-protector -fno-builtin -nostdlib -nostdlib++ -nodefaultlibs")
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

string(APPEND MACOS_TARGET_FLAGS " -arch arm64 -lSystem")

if(LINUX)
    set(OSXCROSS_TARGET "darwin23")
    set(CMAKE_C_COMPILER "${CMAKE_OSX_ARCHITECTURES}-apple-${OSXCROSS_TARGET}-clang")
    set(CMAKE_CXX_COMPILER "${CMAKE_OSX_ARCHITECTURES}-apple-${OSXCROSS_TARGET}-clang++")
    set(CMAKE_LINKER "${CMAKE_OSX_ARCHITECTURES}-apple-${OSXCROSS_TARGET}-ld")
    set(CMAKE_AR "${CMAKE_OSX_ARCHITECTURES}-apple-${OSXCROSS_TARGET}-ar")
    set(CMAKE_RANLIB "${CMAKE_OSX_ARCHITECTURES}-apple-${OSXCROSS_TARGET}-ranlib")
    set(CMAKE_NM "${CMAKE_OSX_ARCHITECTURES}-apple-${OSXCROSS_TARGET}-nm")
    set(CMAKE_STRIP "${CMAKE_OSX_ARCHITECTURES}-apple-${OSXCROSS_TARGET}-strip")

    set(CMAKE_LINK_DEPENDS_USE_LINKER FALSE)
endif()

string(APPEND CMAKE_EXE_LINKER_FLAGS " ${MACOS_TARGET_FLAGS}")
string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${MACOS_TARGET_FLAGS}")
string(APPEND CMAKE_CXX_FLAGS " ${MACOS_TARGET_FLAGS}")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_executable(${PROJECT_NAME} ${SOURCES})

elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(MACOS_RELEASE_LINKER_FLAGS "-static -Wl,-no_data_const -Wl,-no_function_starts -pie -image_base 0 -Wl,-order_file,\"${CMAKE_SOURCE_DIR}/src/order.ld\"")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${MACOS_RELEASE_LINKER_FLAGS}")

    add_library(${PROJECT_NAME} MODULE ${SOURCES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} -x $<TARGET_FILE:${PROJECT_NAME}>)

    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
    set_target_properties(${PROJECT_NAME} PROPERTIES C_VISIBILITY_PRESET hidden)
    set_target_properties(${PROJECT_NAME} PROPERTIES VISIBILITY_INLINES_HIDDEN TRUE)
    set_target_properties(${PROJECT_NAME} PROPERTIES IMPORTED_NO_SONAME TRUE)
    set_target_properties(${PROJECT_NAME} PROPERTIES BUILD_WITH_INSTALL_RPATH TRUE)
    set_target_properties(${PROJECT_NAME} PROPERTIES NO_SONAME TRUE)
endif()
