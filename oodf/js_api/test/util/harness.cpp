#include "harness.hpp"
#include <oodf/js_api/common_types.hpp>

template<class = void>
[[jsbind::jsbind(
  "()=>(name)=>{"
  "  let v = {name, status: 'dnf'};"
  "  global.tests.push(v);"
  "  return v;"
  "}"
)]]
__externref_t register_(__externref_t name);

template<class = void>
[[jsbind::jsbind(
  "()=>(test, success)=>{"
  "  test.status = success ? 'success' : 'fail';"
  "}"
)]]
void run_(__externref_t test, bool success);

future_assert::future_assert(std::string_view name): handle(register_(oodf::js::string(name))) {}
void future_assert::operator()(bool success) const {
  if(handle)
    run_(handle, success);
}

void assert(bool b, std::string_view s) {
  future_assert{s}(b);
}
