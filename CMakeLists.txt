cmake_minimum_required(VERSION 4.0.0)

project(
    getp
    DESCRIPTION "This is a project description."
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)

add_executable(
    main
    example/main.cpp
)

target_include_directories(
    main
    PRIVATE
    ~/.local/include
    src/
)

target_compile_options(
    main
    PRIVATE
    -fdiagnostics-color=always
    -Wall
    -Wextra
    -Og
    -march=native
)

target_link_directories(
    main
    PRIVATE
    ~/.local/lib
)

target_link_libraries(
    main
    PRIVATE
    -pthread
)

target_link_options(
    main
    PRIVATE
)
