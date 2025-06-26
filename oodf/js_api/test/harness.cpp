#include "harness.hpp"
#include <oodf/js_api/common_types.hpp>

[[jsbind::jsbind("()=>(s)=> {\n"
  "console.error(e);\n"
  "process.exit(1);\n"
"}")]]
[[clang::import_module("jsbind")]]
void throw_(__externref_t);

void assert(bool, std::string_view s) {
  return throw_(oodf::js::string(s));
}
