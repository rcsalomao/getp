cmake_minimum_required(VERSION 4.0.0)

project(
    getp
    DESCRIPTION "Good Enough Thread Pool (GETP). A concise thread pool header only library for C++."
    LANGUAGES CXX
)

add_executable(
    main
)

target_sources(
    main
    PRIVATE
    example/main.cpp
)

target_compile_features(
    main
    PRIVATE
    cxx_std_23
)

target_include_directories(
    main
    PRIVATE
    ~/.local/include/
    src/
)

target_compile_options(
    main
    PRIVATE
    -Wall
    -Wextra
    -Og
    -march=native
    -fdiagnostics-color=always
    -fsanitize=address,undefined
)

target_link_directories(
    main
    PRIVATE
    ~/.local/lib/
)

target_link_libraries(
    main
    PRIVATE
    -pthread
)

target_link_options(
    main
    PRIVATE
    -fsanitize=address,undefined
)
