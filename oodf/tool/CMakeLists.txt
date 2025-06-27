if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL wasm32)
else()

  add_library(jsbind_plugin SHARED)
  target_link_libraries(jsbind_plugin clang)
  target_compile_features(jsbind_plugin INTERFACE cxx_std_23)
  target_compile_features(jsbind_plugin PUBLIC cxx_std_23)
  file(GLOB_RECURSE srcs CONFIGURE_DEPENDS *.cpp)
  target_sources(jsbind_plugin PUBLIC ${srcs})

  set(JSBIND_PLUGIN_PATH $<TARGET_FILE:jsbind_plugin>)

  configure_file(
    JsBindToolchain.cmake
    JsBindToolchain.cmake.temp
    @ONLY
  )
  file(GENERATE
    OUTPUT JsBindToolchain.cmake
    INPUT ${CMAKE_CURRENT_BINARY_DIR}/JsBindToolchain.cmake.temp
  )

  add_custom_target(jsbind 
    true
    DEPENDS jsbind_plugin JsBindToolchain.cmake
  )
endif()
