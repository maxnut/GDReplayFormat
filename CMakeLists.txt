cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project(GDReplayFormat)

add_executable(GDReplayFormat
    src/main.cpp
    src/Replay.cpp
)

target_include_directories(GDReplayFormat PRIVATE
    libs/json
)

target_link_libraries(GDReplayFormat
)

install(TARGETS GDReplayFormat)