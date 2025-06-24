project(oodf)
cmake_minimum_required(VERSION 4.0)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL wasm32)
else()
  find_package(Clang REQUIRED)

  add_library(jsbind_plugin SHARED)
  target_link_libraries(jsbind_plugin clang)
  target_compile_features(jsbind_plugin INTERFACE cxx_std_23)
  target_compile_features(jsbind_plugin PUBLIC cxx_std_23)
  file(GLOB_RECURSE srcs CONFIGURE_DEPENDS *.cpp)
  target_sources(jsbind_plugin PUBLIC ${srcs})

  set(JSBIND_PLUGIN_PATH $<TARGET_FILE:jsbind_plugin>)
  set(JSBIND_COMPILER_WRAPPER $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/clang.py>)

  configure_file(
    JsBindToolchain.cmake
    JsBindToolchain.cmake.temp
    @ONLY
  )
  file(GENERATE
    OUTPUT JsBindToolchain.cmake
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/JsBindToolchain.cmake.temp
  )
  configure_file(
    clang.py
    clang.py.temp
    FILE_PERMISSIONS OWNER_READ OWNER_EXECUTE
    @ONLY
  )
  file(GENERATE
    OUTPUT clang.py
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/clang.py.temp
    FILE_PERMISSIONS
      OWNER_READ OWNER_WRITE OWNER_EXECUTE
      GROUP_READ GROUP_WRITE GROUP_EXECUTE
      WORLD_READ WORLD_WRITE WORLD_EXECUTE
  )

  add_custom_target(jsbind 
    chmod +x clang.py
    DEPENDS jsbind_plugin clang.py JsBindToolchain.cmake
  )
endif()
