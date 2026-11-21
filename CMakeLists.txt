cmake_minimum_required(VERSION 3.24)
project(bminor-tools LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(bminor_core
    src/ast.cpp
    src/ast_printer.cpp
    src/diagnostic.cpp
    src/highlighter.cpp
    src/lexer.cpp
    src/parser.cpp
    src/render.cpp
    src/token.cpp)
target_include_directories(bminor_core PUBLIC src)

add_executable(bminor src/main.cpp)
target_link_libraries(bminor PRIVATE bminor_core)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
