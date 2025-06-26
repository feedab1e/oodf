add_library(js_api)
target_link_libraries(js_api util)
target_compile_features(js_api INTERFACE cxx_std_23)
target_compile_features(js_api PUBLIC cxx_std_23)

file(GLOB_RECURSE hdrs CONFIGURE_DEPENDS impl/*.hpp dom/*.hpp)
target_sources(js_api
  INTERFACE
  FILE_SET headers
  TYPE HEADERS
  BASE_DIRS ../..
  FILES ${hdrs}  common_types.hpp
)

file(GLOB_RECURSE srcs CONFIGURE_DEPENDS impl/*.cpp dom/*.cpp)
target_sources(js_api PUBLIC ${srcs})

add_subdirectory(test)
