cmake_minimum_required(VERSION 3.16)

project(HexEditor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)

add_executable(hexeditor
    main.cpp
)

target_link_libraries(hexeditor
    PRIVATE
    SDL3_ttf::SDL3_ttf
)