
cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0077 NEW)

set(CMAKE_EXPORT_COMPILE_COMMANDS true)

project(coretorio C CXX ASM)

set(ZYDIS_BUILD_SHARED_LIB ON)
add_subdirectory(zydis)

add_library(coretorio SHARED
    src/injection_x64.cpp
    src/injection.cpp
    src/main.cpp
    src/object.cpp
)
target_include_directories(coretorio PRIVATE priv_include)
target_include_directories(coretorio PUBLIC include)
target_link_libraries(coretorio PRIVATE Zydis)
target_link_libraries(coretorio PRIVATE -ldl)
target_compile_options(coretorio PRIVATE -O2 -ggdb)
