#include<harness.hpp>
#include<oodf/js_api/common_types.hpp>

using namespace oodf::js;

[[jsbind::jsbind("()=>()=>({})")]]
__externref_t get();

int main() {
  function([] -> __externref_t{return get();})();
  function([](int a){ assert(a == 1, "simple passing"); })(1);
  assert(function([]{return 1;})() == 1, "simple return");
  assert(function([](int a){return a;})(1) == 1, "arg return");
  function([s=raii_assert{"function state is destroyed on garbage collection"}]{});
}
