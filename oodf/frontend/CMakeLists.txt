add_subdirectory(test)

add_executable(main main/main.cpp)
target_link_libraries(main js_api)
