project(oodf)
cmake_minimum_required(VERSION 4.0)

add_library(util INTERFACE)
target_compile_features(util INTERFACE cxx_std_23)
target_sources(util
  INTERFACE
  FILE_SET headers
  TYPE HEADERS
  BASE_DIRS ../../
  FILES ./property.hpp ./types.hpp
)
