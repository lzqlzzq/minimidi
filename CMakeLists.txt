cmake_minimum_required(VERSION 3.20)
project(minimidi)
set(CMAKE_CXX_STANDARD 17)

option(BUILD_EXAMPLES "Build examples" OFF)

add_library(minimidi INTERFACE)
target_include_directories(minimidi INTERFACE include)

if(BUILD_EXAMPLES)
    add_executable(dumpmidi example/dumpmidi.cpp)
    target_link_libraries(dumpmidi PRIVATE minimidi)
    add_executable(parsemidi example/parsemidi.cpp)
    target_link_libraries(parsemidi PRIVATE minimidi)
    add_executable(redumpmidi example/redumpmidi.cpp)
    target_link_libraries(redumpmidi PRIVATE minimidi)
    add_executable(writemidi example/writemidi.cpp)
    target_link_libraries(writemidi PRIVATE minimidi)
endif()
