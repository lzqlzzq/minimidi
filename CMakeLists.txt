cmake_minimum_required(VERSION 3.20)
project(minimidi)

option(BUILD_EXAMPLES "Build examples" OFF)

add_library(minimidi INTERFACE)
add_library(minimidi::minimidi ALIAS minimidi)
target_compile_features(minimidi INTERFACE cxx_std_17)
target_include_directories(minimidi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

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
