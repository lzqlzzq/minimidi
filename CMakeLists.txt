cmake_minimum_required(VERSION 3.20)
project(minimidi)

option(BUILD_EXAMPLES "Build examples" OFF)
option(USE_MIMALLOC "Use mimalloc" OFF)

add_library(minimidi INTERFACE)
add_library(minimidi::minimidi ALIAS minimidi)

# TODO: upgrade to C++20 temporarily for concepts
# concepts will be replaced by SFINAE in the future
target_compile_features(minimidi INTERFACE cxx_std_20)
target_include_directories(minimidi INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(USE_MIMALLOC)
    include(FetchContent)
    FetchContent_Declare(
            mimalloc
            GIT_REPOSITORY https://github.com/microsoft/mimalloc.git
            GIT_TAG        v3.0.1
    )
    set(MI_BUILD_STATIC ON CACHE BOOL "Build static library")
    set(MI_OVERRIDE ON CACHE BOOL "Override system malloc")
    set(MI_INSTALL_OFF ON CACHE BOOL "Skip installation")  # 可选，避免安装
    FetchContent_MakeAvailable(mimalloc)
    target_link_libraries(minimidi INTERFACE mimalloc-static)
endif()

if(BUILD_EXAMPLES)
    add_executable(dumpmidi example/dumpmidi.cpp)
    target_link_libraries(dumpmidi PRIVATE minimidi)

    add_executable(parsemidi example/parsemidi.cpp)
    target_link_libraries(parsemidi PRIVATE minimidi)
    target_compile_features(parsemidi PRIVATE cxx_std_20)

    add_executable(redumpmidi example/redumpmidi.cpp)
    target_link_libraries(redumpmidi PRIVATE minimidi)

    add_executable(writemidi example/writemidi.cpp)
    target_link_libraries(writemidi PRIVATE minimidi)

    include(FetchContent)

    FetchContent_Declare(
            nanobench
            GIT_REPOSITORY https://github.com/martinus/nanobench.git
            GIT_TAG v4.3.11
            GIT_SHALLOW TRUE)

    FetchContent_MakeAvailable(nanobench)
    add_executable(parsebench example/parsebench.cpp)
    target_link_libraries(parsebench PRIVATE minimidi nanobench)
endif()