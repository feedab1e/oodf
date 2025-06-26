cmake_minimum_required(VERSION 4.0)
project(oodf CXX)

enable_testing()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(tool)
add_subdirectory(util)
add_subdirectory(js_api)
add_subdirectory(frontend)
