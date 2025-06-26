
add_custom_target(test_harness 
  tsc -b ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS harness.ts
)
add_executable(test_basic basic.cpp)
target_link_libraries(test_basic js_api)
add_dependencies(test_basic test_harness)
add_test(NAME test_basic COMMAND node ../../../js/test/harness.js test_basic)
set_property(TEST test_basic PROPERTY ENVIRONMENT "NODE_PATH=../../../js")
