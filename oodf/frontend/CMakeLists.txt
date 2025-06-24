project(oodf)
cmake_minimum_required(VERSION 4.0)

add_executable(main main/main.cpp)
target_link_libraries(main js_api)
