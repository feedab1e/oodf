add_custom_target(test_harness_js 
  tsc -b ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS harness.ts
)

add_library(test_harness STATIC harness.cpp)
target_link_libraries(test_harness js_api)
target_sources(test_harness INTERFACE FILE_SET HEADERS TYPE HEADERS FILES harness.hpp)

add_dependencies(test_harness test_harness_js)

function(add_test_file TEST_FILE)
  get_filename_component(NAME_ ${TEST_FILE} NAME_WLE)
  set(TEST_NAME test_${NAME_})

  add_executable(${TEST_NAME} ${TEST_FILE})
  target_link_libraries(${TEST_NAME} test_harness)

  add_test(NAME ${TEST_NAME} COMMAND node ../../../../js/js_api/test/util/harness.js ${TEST_NAME})
  set_property(TEST ${TEST_NAME} PROPERTY ENVIRONMENT "NODE_PATH=../../../../js/frontend")
  if(${ARGC} EQUAL 2)
    set_property(TEST ${TEST_NAME} PROPERTY WILL_FAIL TRUE)
  endif()
endfunction()

add_test_file(../basic.cpp)
add_test_file(../fail.cpp FAIL)
add_test_file(../function.cpp)
add_test_file(../promise.cpp)
