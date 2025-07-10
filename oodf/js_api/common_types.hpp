#pragma once
#include <new>
#include <coroutine>
#include <cstdlib>
#include <string>
#include <concepts>
#include "oodf/util/types.hpp"
namespace oodf::js {

struct externref;

namespace binds {
template<class T>
concept js_regular_primitive = std::is_pointer_v<T>
  || std::integral<T>
  || std::floating_point<T>;
template<class T>
concept js_primitive = std::same_as<T, __externref_t>
  || js_regular_primitive<T>;

template<class T>
concept js_object = std::derived_from<std::remove_cvref_t<T>, externref>
  && std::convertible_to<std::remove_cvref_t<T>, __externref_t>
  && std::constructible_from<std::remove_cvref_t<T>, __externref_t>;

template<class T>
concept js_type = js_primitive<T> || js_object<T>;
}

namespace impl {
int allocate_number(__externref_t) noexcept;
void free_number(int) noexcept;
__externref_t get_number(int) noexcept;
void set_number(int, __externref_t) noexcept;
}

struct string;
struct number;
struct externref {
  int index;
  externref(__externref_t r): index{impl::allocate_number(r)} {}
  externref(const externref &other) noexcept: index{impl::allocate_number(other)} {}
  externref(externref &&other) noexcept:
    index{std::exchange(other.index, -1)} {}
  externref &operator=(const externref &other) {
    impl::set_number(index, other);
    return *this;
  }
  externref &operator=(externref &&other) {
    std::swap(other.index, index);
    return *this;
  }
  externref(): index{-1}{}

  operator bool() const {
    return index != -1;
  }

  operator __externref_t() const {
    return impl::get_number(index);
  }
  ~externref() {
    impl::free_number(index);
  }

  struct setter_wrapper;
  setter_wrapper operator[](std::string_view);
  setter_wrapper operator[](externref);
  setter_wrapper operator[](string);
  setter_wrapper operator[](number);
  setter_wrapper operator[](int);
private:
};
struct externref::setter_wrapper {
  externref &obj, idx;
  setter_wrapper(setter_wrapper &&other) = delete;
  setter_wrapper(const setter_wrapper &other) = delete;
  setter_wrapper(externref &obj, externref idx) noexcept:
    obj(obj), idx(std::move(idx))
  {}
  operator externref() const&& noexcept;
  operator __externref_t() const&& noexcept;
  const setter_wrapper &operator=(__externref_t) const&& noexcept;
};

__externref_t make_raw(std::derived_from<externref> auto v) {
  return v;
}
auto make_raw(binds::js_primitive auto v) {
  return v;
}

namespace impl {
template<class T>
struct make_raw_t {
  using type = decltype(make_raw(std::declval<T>()));
};
template<>
struct make_raw_t<void> {
  using type = void;
};
}
template<class T>
using make_raw_t = typename impl::make_raw_t<T>::type;

namespace binds {

template<class = int>
[[jsbind::jsbind("()=>(obj, idx)=>obj[idx]")]]
__externref_t get_externref_idx(__externref_t, __externref_t);

template<class = int>
[[jsbind::jsbind("()=>(obj, idx, value)=>{ obj[idx] = value }")]]
__externref_t set_externref_idx(__externref_t, __externref_t, __externref_t);

template<class = int>
[[jsbind::jsbind("()=>(beg, size)=>new TextDecoder().decode(new DataView(memory.data.buffer, beg, size))")]]
__externref_t decode_string(const char *, int);

template<class = int>
[[jsbind::jsbind("()=>(string)=>new TextEncoder().encode(string)")]]
__externref_t encode_string_to_buf(__externref_t str);
template<class = int>
[[jsbind::jsbind("()=>(buf)=>buf.byteLength")]]
int encode_string_buf_size(__externref_t buf);
template<class = int>
[[jsbind::jsbind("()=>(buf, ptr)=>new Uint8Array(memory.data.buffer).set(buf, ptr)")]]
void encode_string_buf_copy(__externref_t buf, char *ptr);

template<class R = int>
[[jsbind::jsbind("()=>(b)=>new Number(b)")]]
__externref_t decode_int(R);

template<class R = int>
[[jsbind::jsbind("()=>(b)=>b")]]
R encode_int(__externref_t);

template<js_primitive value_t = __externref_t, js_primitive idx_t = int>
[[jsbind::jsbind("()=>(arr, idx)=>arr[idx]")]]
value_t externref_get(__externref_t, idx_t);
template<js_primitive value_t = __externref_t, js_primitive idx_t = int>
[[jsbind::jsbind("()=>(arr, idx, v)=>{ old = arr[idx]; arr[idx]=v; return old; }")]]
void externref_set(__externref_t, idx_t, value_t);
}

inline externref::setter_wrapper::operator __externref_t() const&& noexcept {
  return binds::get_externref_idx(obj, idx);
}

inline externref::setter_wrapper::operator externref() const&& noexcept {
  return (__externref_t)std::move(*this);
}

inline const externref::setter_wrapper
&externref::setter_wrapper::operator=(__externref_t r) const&& noexcept {
  binds::set_externref_idx(obj, idx, r);
  return *this;
}
inline int a;

struct string: externref {
  string(std::string_view v): externref {
    [&]{
      return binds::decode_string(v.data(), v.size());
    }()
  }{}
  operator std::string() const {
    using namespace binds;
    __externref_t buf = encode_string_to_buf(*this);
    std::string res(encode_string_buf_size(buf), '\0');
    encode_string_buf_copy(buf, res.data());
    return res;
  }
};

struct number: externref {
  number(int v): externref{[&]{
    return binds::decode_int(v);
  }()} {}
  number(double v): externref{[&]{
    return binds::decode_int(v);
  }()} {}
  operator int() const{
    return binds::encode_int(*this);
  }
  operator double() const{
    return binds::encode_int<double>(*this);
  }
};
namespace literal {
inline number operator ""_js(long double d) { return {(double)d}; }
inline number operator ""_js(unsigned long long d) { return {(double)d}; }
inline string operator ""_js(const char *data, std::size_t l) { return {std::string_view{data, l}}; }
}

template<class T>
struct array_of {
  template<bool is_const>
  struct setter_proxy {
    const js::externref &arr;
    int idx;
    operator T() const {
      return binds::externref_get<T>(arr, idx);
    }
    T operator=(T r) {
      return binds::externref_set(arr, idx, r);
    }
    T operator=(std::derived_from<externref> auto r) {
      return binds::externref_set(arr, idx, (__externref_t)r);
    }
  };
  setter_proxy<true> operator[](int i) const {
    return {*this, i};
  }
  setter_proxy<false> operator[](int i) {
    return {*this, i};
  }
};
template<class T>
struct map_of {
  template<bool is_const>
  struct setter_proxy {
    const js::externref &arr;
    externref idx;
    operator T() const {
      return binds::externref_get<T>(arr, (__externref_t)idx);
    }
    T operator=(T r) {
      return binds::externref_set(arr, (__externref_t)idx, r);
    }
    T operator=(std::derived_from<externref> auto r) {
      return binds::externref_set(arr, (__externref_t)idx, (__externref_t)r);
    }
  };
  setter_proxy<true> operator[](std::string_view i) const {
    return {*this, i};
  }
  setter_proxy<false> operator[](std::string_view i) {
    return {*this, i};
  }
  setter_proxy<true> operator[](string i) const {
    return {*this, i};
  }
  setter_proxy<false> operator[](string i) {
    return {*this, i};
  }
};
inline externref::setter_wrapper externref::operator[](std::string_view r) {
  return (*this)[(string)r];
}
inline externref::setter_wrapper externref::operator[](externref r) {
  return {*this, std::move(r)};
}
inline externref::setter_wrapper externref::operator[](string r) {
  return (*this)[(__externref_t)std::move(r)];
}
inline externref::setter_wrapper externref::operator[](number r) {
  return (*this)[(__externref_t)std::move(r)];
}
inline externref::setter_wrapper externref::operator[](int r) {
  return (*this)[(number)r];
}
namespace binds {

template<js_primitive ...args>
[[jsbind::jsbind("()=>(...arr)=>arr")]]
__externref_t create_array(args ...);
}


template<class T = js::externref>
struct array: js::externref, js::array_of<T> {
  using externref::externref;
  static array of(auto &&...values) {
    return {js::binds::create_array(js::make_raw((decltype(values))values)...)};
  }
  array(__externref_t r): externref{r}{}
  array(auto &&...values) requires (sizeof...(values) > 1):
    array{of(((decltype(values))values)...)}
  {}
  using js::array_of<T>::operator[];
};

namespace impl {

template<class T, auto destroy = [](T *v){ delete v; }>
class destructor_registry: externref {
  template<int = 0>
  [[jsbind::jsbind(
    "()=>(r, obj, addr)=>r.register(obj, addr)"
  )]]
  static void reg(__externref_t, __externref_t, T *) noexcept;
  template<int = 0>
  [[jsbind::jsbind(
    "()=>(destroy)=>new FinalizationRegistry(memory.table.get(destroy))"
  )]]
  static __externref_t create_registry(void(*)(T *)) noexcept;
public:
  destructor_registry(): externref{create_registry([](T *v){destroy(v);})} {}
  void reg(__externref_t r, T *v) { reg(*this, r, v); }
};

template<class T>
destructor_registry<T> default_registry;

template<class R, class ...Args>
struct erased_function {
  virtual R invoke(Args ...) = 0;
  virtual ~erased_function() = default;
};

template<class Fn, class R, class ...Args>
struct concrete_function: erased_function<R, Args...> {
  concrete_function(auto &&ff): f(ff()) {}
  Fn f;
  R invoke(Args ...args) override {
    return f((Args)args...);
  }
  ~concrete_function() override = default;
};

}

template<class T>
struct function;
template<class R, class ...Args>
struct function<R (Args ...)>: externref {
  template<class F> requires std::is_invocable_r_v<R, F, Args ...>
  function (F &&f): externref{create_erased((F&&)f)} {}
  function (R (*f)(Args ...)): externref{create_js(f)} {}

  R operator()(Args ...args) {
    return invoke(*this, (Args)args...);
  }
private:
  using erased = impl::erased_function<R, Args ...>;
  template<class T, class T_ = std::remove_cvref_t<T>>
  using concrete = impl::concrete_function<T_, R, Args ...>;
  
  template<class T = void>
  [[jsbind::jsbind("()=>(addr, fn)=>(...args)=>memory.table.get(fn)(addr, ...args)")]]
  static __externref_t create_js(erased *, R(*)(erased *, Args ...));
  
  template<class T = void>
  [[jsbind::jsbind("()=>(fn)=>(...args)=>memory.table.get(fn)(addr, ...args)")]]
  static __externref_t create_js(R(*)(Args ...));
  
  static R erased_invoke(erased *f, Args ...args) {
    return f->invoke((Args)args...);
  }
  
  template<class T = void>
  [[jsbind::jsbind("()=>(fn, ...args)=>fn(...args)")]]
  static R invoke(__externref_t, Args ...);

  static __externref_t create_erased(auto &&f) {
    erased *v = new concrete<decltype(f)>([&]-> decltype(f)&& {return (decltype(f)&&)f;});
    __externref_t r = create_js(v, erased_invoke);
    impl::default_registry<erased>.reg(r, v);
    return r;
  }
};

template<class R, class ...Args>
function(R(*)(Args ...)) -> function<R(Args ...)>;
template<class Fn>
function(Fn) -> function<
  typename util::fn_traits<
    typename util::memptr_traits<decltype(&Fn::operator())>::value_type
  >::fn_type
>;

template<class R>
struct promise;

namespace impl {

template<class T>
struct converter {
  using storage_type = T *;
  using return_type = T *;
  static T get(const storage_type &s) { return *s; }
  static void set(storage_type &s, return_type r) { s = r; }
};
template<binds::js_regular_primitive T>
struct converter<T> {
  using storage_type = T;
  using return_type = T;
  static T get(const storage_type &s) { return s; }
  static void set(storage_type &s, return_type r) { s = r; }
};
template<class T> requires binds::js_object<T> || std::same_as<T, __externref_t>
struct converter<T> {
  using storage_type = externref;
  using return_type = __externref_t;
  static T get(const storage_type &s) { return (__externref_t)s; }
  static void set(storage_type &s, return_type r) { s = r; }
};
template<>
struct converter<void> {
  struct storage_type{};
  using return_type = __externref_t;
  static void get(const storage_type &s) { return; }
  static void set(storage_type &s, return_type r) {}
};

template<class T>
void resume_continuation(
    void *coro,
    typename converter<T>::storage_type *p,
    typename converter<T>::return_type r
) {
  converter<T>::set(*p, r);
  std::coroutine_handle<>::from_address(coro)();
}

template<class T>
[[jsbind::jsbind(
  "()=>(promise, coro_addr, return_value, resume_fn)=>{"
  "  promise.then((data)=>"
  "    memory.table.get(resume_fn)(coro_addr, return_value, data)"
  "  );"
  "}"
)]]
extern void promise_set_continuation(
  __externref_t,
  void *,
  typename converter<T>::storage_type *,
  decltype(resume_continuation<T>) * = resume_continuation<T>
);

}

template<class R> requires binds::js_primitive<R> || std::copyable<R> || std::same_as<R, void>
struct promise<R>: externref {
  struct awaiter {
    externref coro;
    typename impl::converter<R>::storage_type return_expr = {};
    constexpr bool await_ready() const { return false; }
    constexpr void await_suspend(std::coroutine_handle<> h) {
      impl::promise_set_continuation<R>(coro, h.address(), &return_expr);
    }
    R await_resume() const {
      return impl::converter<R>::get(return_expr);
    }
  };
  awaiter operator co_await() const {
    return {*this};
  }
  using externref::externref;
};

namespace impl {

template<class = void>
[[jsbind::jsbind("()=>()=>Promise.withResolvers()")]]
__externref_t create_promise();
template<class = void>
[[jsbind::jsbind("()=>(x)=>x.promise")]]
__externref_t get_promise(__externref_t);
template<class = void>
[[jsbind::jsbind("()=>(x)=>x.resolve")]]
__externref_t get_resolve(__externref_t);
template<class T>
[[jsbind::jsbind("()=>(resolve, v)=>resolve(v)")]]
__externref_t invoke_resolve(__externref_t, T);
template<class T = void>
[[jsbind::jsbind("()=>(resolve)=>resolve()")]]
__externref_t invoke_resolve(__externref_t);

}

template<class R>
struct coro_promise: promise<R> {
  struct promise_type;
  struct inject_void {
    void return_void() noexcept {
      auto pthis = static_cast<const promise_type *>(this);
      impl::invoke_resolve(pthis->resolve);
    }
  };
  struct inject_value {
    void return_value(R v) noexcept {
      auto pthis = static_cast<const promise_type *>(this);
      impl::invoke_resolve(pthis->resolve, v);
    }
  };
  struct promise_type: std::conditional_t<
      std::same_as<R, void>,
      inject_void,
      inject_value
    >
  {
    externref resolve;
    coro_promise get_return_object() {
      __externref_t promise = impl::create_promise();
      resolve = impl::get_resolve(promise);
      return {impl::get_promise(promise)};
    }
    constexpr std::suspend_never initial_suspend() const noexcept { return {}; }
    constexpr std::suspend_always final_suspend() const noexcept { return {}; }
    void unhandled_exception() {};
  };
};

}
