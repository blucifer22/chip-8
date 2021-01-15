cmake_minimum_required(VERSION 3.7)

project(chip8)

set (CMAKE_CXX_STANDARD 11)

find_package(SDL2 REQUIRED)
include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

add_executable(
    chip8
    src/chip_8.hpp
    src/chip_8.cpp
    src/platform.cpp
    src/main.cpp
    )
target_link_libraries(chip8 ${SDL2_LIBRARIES})