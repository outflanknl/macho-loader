cmake_minimum_required(VERSION 3.11)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_C_STANDARD 11)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")

project(example LANGUAGES C)
set_property(CACHE TARGET_ARCH PROPERTY STRINGS "x86_64")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)

set(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE INTERNAL "" FORCE)
string(APPEND MACOS_TARGET_FLAGS " -arch x86_64")

string(APPEND CMAKE_C_FLAGS " ${MACOS_TARGET_FLAGS}")
string(APPEND CMAKE_EXE_LINKER_FLAGS " ${MACOS_TARGET_FLAGS}")
string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${MACOS_TARGET_FLAGS}")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(RELEASE_FLAGS "-ffunction-sections -fdata-sections")
    set(MACOS_LINKER_FLAGS "-flto -Wl,-dead_strip_dylibs -Wl,-install_name,0 -Wl,-function_starts -Wl,-no_adhoc_codesign -Wl,-data_in_code_info -Wl,-fixup_chains -Wl,-no_function_starts -Wl,-no_uuid -Wl,-no_data_in_code_info -Wl,-no_source_version -Wl,-dead_strip")
    string(APPEND CMAKE_C_FLAGS "-Os ${RELEASE_FLAGS}")
    string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${RELEASE_FLAGS} ${MACOS_LINKER_FLAGS}")
endif()

add_library(${PROJECT_NAME} MODULE main.c)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".dylib")
endif()
