cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(libGDR)

add_library(libGDR INTERFACE)

target_include_directories(libGDR INTERFACE include/)

# Test

add_executable(libGDR_test
    test/main.cpp
)

target_link_libraries(libGDR_test PRIVATE libGDR)

add_custom_command(
    TARGET libGDR_test POST_BUILD
    COMMAND libGDR_test
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Running test..."
)
