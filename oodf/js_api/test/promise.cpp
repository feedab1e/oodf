#include<harness.hpp>
#include<oodf/js_api/common_types.hpp>

template<class = void>
[[jsbind::jsbind("()=>(ms)=>new Promise(res=>setTimeout(()=>res(1), ms))")]]
__externref_t get_delay(int);

oodf::js::promise<void> delay(int ms) {
  return get_delay(ms);
}

oodf::js::coro_promise<void> test_stuff() {
  future_assert a("co_await returns");
  co_await delay(0);
  a(true);
  co_return;
}

int main() {
  test_stuff();
}
