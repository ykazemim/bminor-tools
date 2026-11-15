cmake_minimum_required(VERSION 3.24)
project(bminor-tools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(bminor_core
    src/diagnostic.cpp
    src/token.cpp)
target_include_directories(bminor_core PUBLIC src)
