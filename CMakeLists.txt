cmake_minimum_required(VERSION 3.16)
project(HexEditor)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(SDL3 CONFIG REQUIRED)
find_package(SDL3_ttf CONFIG REQUIRED)
add_executable(hexeditor
    main.cpp
    editor.cpp
)
target_link_libraries(hexeditor
    PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
)
add_custom_command(TARGET hexeditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/SpaceMono.ttf
    $<TARGET_FILE_DIR:hexeditor>
)