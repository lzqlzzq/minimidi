cmake_minimum_required(VERSION 3.20)
project(minimidi)
set(CMAKE_CXX_STANDARD 20)

option(BUILD_EXAMPLES "Build examples" OFF)

# for using minimidi, you could include ./include directory derectly
# without concerning this CMakeLists.txt
# this is for developing minimidi itself
include_directories(include)

if(BUILD_EXAMPLES)
    add_executable(dumpmidi example/dumpmidi.cpp)
    add_executable(parsemidi example/parsemidi.cpp)
    add_executable(redumpmidi example/redumpmidi.cpp)
    add_executable(writemidi example/writemidi.cpp)
endif()