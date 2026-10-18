cmake_minimum_required(VERSION 3.20)
project(libGDR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(libGDR INTERFACE)
target_include_directories(libGDR INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Test

option(LIBGDR_BUILD_TESTS "Build libGDR tests" ${PROJECT_IS_TOP_LEVEL})

if(LIBGDR_BUILD_TESTS)
    enable_testing()

    add_executable(libGDR_test test/main.cpp)
    target_link_libraries(libGDR_test PRIVATE libGDR)

    add_test(NAME libGDR_test COMMAND libGDR_test)
endif()
