#include "harness.hpp"
#include <oodf/js_api/common_types.hpp>

[[jsbind::jsbind("()=>(s)=> {\n"
  "console.error(s);\n"
  "process.exit(1);\n"
"}")]]
[[clang::import_module("jsbind")]]
void throw_(__externref_t);

void assert(bool b, std::string_view s) {
  if(!b)
    return throw_(oodf::js::string(s));
}
