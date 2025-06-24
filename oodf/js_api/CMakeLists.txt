cmake_minimum_required(VERSION 4.0)
project(oodf)

add_library(js_api)
target_link_libraries(js_api util)
target_compile_features(js_api INTERFACE cxx_std_23)
target_compile_features(js_api PUBLIC cxx_std_23)

file(GLOB_RECURSE hdrs CONFIGURE_DEPENDS *.hpp)
target_sources(js_api
  INTERFACE
  FILE_SET headers
  TYPE HEADERS
  BASE_DIRS ../..
  FILES ${hdrs}
)

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS *.cpp)
target_sources(js_api PUBLIC ${srcs})
