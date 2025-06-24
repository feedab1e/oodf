#pragma once
#include <new>
#include <cstdlib>
#include <string>
#include <concepts>
#include "oodf/util/types.hpp"
namespace oodf::js {

namespace binds {
template<class T>
concept js_type = std::integral<T> || std::same_as<T, __externref_t> || std::is_pointer_v<T>;
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
    index = std::exchange(other.index, -1);
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
auto make_raw(binds::js_type auto v) {
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
[[clang::import_module("jsbind")]]
__externref_t get_externref_idx(__externref_t, __externref_t);

template<class = int>
[[jsbind::jsbind("()=>(obj, idx, value)=>{ obj[idx] = value }")]]
[[clang::import_module("jsbind")]]
__externref_t set_externref_idx(__externref_t, __externref_t, __externref_t);

template<class = int>
[[jsbind::jsbind("()=>(beg, size)=>new TextDecoder().decode(new DataView(memory.data.buffer, beg, size))")]]
[[clang::import_module("jsbind")]]
__externref_t decode_string(const char *, int);

template<class = int>
[[jsbind::jsbind("()=>(string)=>new TextEncoder().encode(string)")]]
[[clang::import_module("jsbind")]]
__externref_t encode_string_to_buf(__externref_t str);
template<class = int>
[[jsbind::jsbind("()=>(buf)=>buf.byteLength")]]
[[clang::import_module("jsbind")]]
int encode_string_buf_size(__externref_t buf);
template<class = int>
[[jsbind::jsbind("()=>(buf, ptr)=>new Uint8Array(memory.data.buffer).set(buf, ptr)")]]
[[clang::import_module("jsbind")]]
void encode_string_buf_copy(__externref_t buf, char *ptr);

template<class R = int>
[[jsbind::jsbind("()=>(b)=>new Number(b)")]]
[[clang::import_module("jsbind")]]
__externref_t decode_int(R);

template<class R = int>
[[jsbind::jsbind("()=>(b)=>b")]]
[[clang::import_module("jsbind")]]
R encode_int(__externref_t);

template<js_type value_t = __externref_t, js_type idx_t = int>
[[jsbind::jsbind("()=>(arr, idx)=>arr[idx]")]]
value_t externref_get(__externref_t, idx_t);
template<js_type value_t = __externref_t, js_type idx_t = int>
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

template<js_type ...args>
[[jsbind::jsbind("()=>(...arr)=>arr")]]
[[clang::import_module("jsbind")]]
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

class destructible_externref;

namespace impl {
struct shared_externref_state {
  char shared;
  char *allocation;
  destructible_externref *object;
  char data[];

  void patch() noexcept;

  template<class T>
  static shared_externref_state *create(auto &&...args) noexcept {
    constexpr auto max = [](auto a, auto b) { return a > b ? a : b; };
    constexpr auto roundup = [](auto value, auto to) { return (((value - 1) / to) + 1) * to; };

    constexpr auto aligned_state_size = roundup(sizeof(shared_externref_state), alignof(T));
    constexpr auto aligned_t_size = roundup(sizeof(T), alignof(shared_externref_state));
    constexpr auto offset = aligned_state_size - sizeof(shared_externref_state);

    constexpr auto alloc_align = max(alignof(T), alignof(shared_externref_state));
    constexpr auto alloc_size = aligned_state_size + aligned_t_size;

    char *store = (char *)aligned_alloc(alloc_align, alloc_size);
    store += offset;

    auto *state = new(store) shared_externref_state {
      .shared = 2,
      .allocation = store,
    };
    state->object = static_cast<destructible_externref *>(new(state->data) T((decltype(args))args...));
    state->patch();
    return state;
  }
  void destroy_obj() noexcept;
  void shared_dec() noexcept { if(!--shared) destroy_obj(); }
};

}

template<class T>
class shared_externref {
  impl::shared_externref_state *state;
  T *pointee;
  template<class U>
  friend class shared_externref;

public:
  shared_externref(std::in_place_t, auto &&...args):
    state(impl::shared_externref_state::create<T>((decltype(args))args...)),
    pointee(std::launder((T *)state->data))
  {}
  shared_externref(std::in_place_type_t<T>, auto &&...args):
    shared_externref(std::in_place, (decltype(args))args...)
  {}
  shared_externref(): state(0), pointee(0) {}
  shared_externref(auto &&arg, auto &&...args)
    requires (!std::same_as<shared_externref, std::remove_cvref_t<decltype(arg)>>):
    shared_externref(std::in_place, (decltype(arg))arg, (decltype(args))args...)
  {}
  template<class U> requires std::derived_from<U, T>
  shared_externref(shared_externref<U> &&other):
    state{std::exchange(other.state, nullptr)},
    pointee{std::exchange(other.pointee, nullptr)}
  {}
  template<class U> requires std::derived_from<U, T>
  shared_externref(const shared_externref<U> &other):
    state{impl::shared_externref_state::create(*other.pointee)},
    pointee{state->object}
  {}
  shared_externref &operator=(shared_externref other) {
    std::swap(state, other.state);
    std::swap(pointee, other.pointee);
    return *this;
  }
  ~shared_externref() {
    state->shared_dec();
  }
  operator bool() const noexcept { return pointee && *(externref *)pointee; }
  T *operator->() const noexcept { return pointee; }
  T &operator*() const noexcept { return *pointee; }
};
namespace binds {
  void destroy(destructible_externref *r) noexcept;
}
class destructible_externref: public externref {
  impl::shared_externref_state *state;
  friend void binds::destroy(destructible_externref *r) noexcept;
  friend struct impl::shared_externref_state;
  static constexpr auto dtry = binds::destroy;
  template<int = 0>
  [[jsbind::jsbind(
    "()=>(r, obj, addr)=>r.register(obj, addr)"
  )]]
  [[clang::import_module("jsbind")]]
  static void reg(__externref_t, __externref_t, destructible_externref *) noexcept;
  template<int = 0>
  [[jsbind::jsbind(
    "()=>(destroy)=>new FinalizationRegistry(memory.table.get(destroy))"
  )]]
  [[clang::import_module("jsbind")]]
  static __externref_t create_registry(void(*)(destructible_externref *)) noexcept;
  static externref registry;
public:
  destructible_externref(__externref_t r) noexcept: externref(r) {
    reg(registry, *this, this);
  }
  destructible_externref(const destructible_externref &r) noexcept:
    externref{(const externref &)r} {
    reg(registry, *this, this);
  }
  destructible_externref(destructible_externref &&r) noexcept:
    externref{(externref &&)r} {
  }
  externref &operator=(const externref &other) noexcept {
    *(externref *)this = (const externref &)other;
    reg(registry, *this, this);
    return *this;
  }
  externref &operator=(externref &&other) noexcept {
    *(externref *)this = (externref &&)other;
    return *this;
  }
protected:
  virtual ~destructible_externref(){};
};
inline externref destructible_externref::registry = create_registry(binds::destroy);
inline void impl::shared_externref_state::destroy_obj() noexcept {
  object->~destructible_externref();
  free(allocation);
}
inline void impl::shared_externref_state::patch() noexcept {
  object->state = this;
}

namespace binds {
[[gnu::used, gnu::visibility("default")]]
void destroy(destructible_externref *r) noexcept {
  r->state->shared_dec();
}
}

namespace impl {
template<class R, class ...Args>
struct function_state_base: destructible_externref {
  inline static make_raw_t<R> invoke(
    function_state_base *fn,
    make_raw_t<Args> ...args
  ) { return make_raw(fn->call({args}...)); }
  virtual R call(Args ...) = 0;
  function_state_base(__externref_t r): destructible_externref(r) {}
};
template<class F, class R, class ...Args>
struct function_state: function_state_base<R, Args...> {
  F f;
  constexpr static auto invk = function_state_base<R, Args...>::invoke;
  template<int = 0>
  [[jsbind::jsbind(
    "()=>(invoke, obj)=>(...args)=>(memory.table.get(invoke)(obj, ...args))"
  )]]
  [[clang::import_module("jsbind")]]
  inline static __externref_t create(decltype(invk), function_state *);
  function_state(auto &&f):
    function_state_base<R, Args...>{create(invk, this)},
    f{(decltype(f))f}
  {}
  R call(Args ...args) override {
    return f(args...);
  }
};

template<class ...Args>
struct function_state_base<void, Args...>: destructible_externref {
  inline static void invoke(
    function_state_base *fn,
    make_raw_t<Args> ...args
  ) { fn->call({args}...); }
  virtual void call(Args ...) = 0;
  function_state_base(__externref_t r): destructible_externref(r) {}
};
template<class F, class ...Args>
struct function_state<F, void, Args...>: function_state_base<void, Args...> {
  F f;
  constexpr static auto *invk = function_state_base<void, Args...>::invoke;
  template<int = 0>
  [[jsbind::jsbind(
    "()=>(invoke, obj)=>(...args)=>(memory.table.get(invoke)(obj, ...args))"
  )]]
  [[clang::import_module("jsbind")]]
  inline static __externref_t create(decltype(invk), function_state *);
  function_state(auto &&f):
    function_state_base<void, Args...>{create(invk, this)},
    f{(decltype(f))f}
  {}
  void call(Args ...args) override {
    return f(args...);
  }
};

template<class R, class ...Args>
struct erased_function {
  shared_externref<function_state_base<R, Args...>> state;
  erased_function(auto &&f):
    state{
      shared_externref<
        function_state<std::remove_cvref_t<decltype(f)>, R, Args...>
      >((decltype(f))f)
    }
  {}
  erased_function() = default;
  erased_function(const erased_function &) = delete;
  erased_function(erased_function &&) = default;
  operator bool() const {
    return state;
  }
  operator __externref_t() const {
    return *state;
  }
};
template<class R, class ...Args>
struct stateless_function: externref {
  template<class R2 = R>
  static auto invoke(
    R2 (*fn)(Args ...),
    make_raw_t<Args> ...args
  )
  { return make_raw(fn(Args{args}...)); }
  static constexpr void (*fn)(
    R (*fn)(Args ...),
    make_raw_t<Args> ...args
  ) = invoke;

  template<int = 0>
  [[jsbind::jsbind("()=>(invoke, obj)=>(...args)=>(memory.table.get(invoke)(obj, ...args))",(fn))]]
  [[clang::import_module("jsbind")]]
  static __externref_t create(decltype(fn), R (*)(Args ...));
  
  stateless_function(R(*fn)(Args ...)): externref{create(invoke, fn)} {}
  stateless_function(): externref{} {}
};
template<class ...Args>
struct stateless_function<void, Args ...>: externref {
  static void invoke_void(
    void (*fn)(Args ...),
    make_raw_t<Args> ...args
  )
  { fn(Args{args}...); }
  static constexpr void (*fn_void)(
    void (*fn)(Args ...),
    make_raw_t<Args> ...args
  ) = invoke_void;

  template<int = 0>
  [[jsbind::jsbind("()=>(invoke, obj)=>(...args)=>(memory.table.get(invoke)(obj, ...args))")]]
  [[clang::import_module("jsbind")]]
  static __externref_t create_void(decltype(fn_void), void (*)(Args ...));
  
  stateless_function(void(*fn)(Args ...)): externref{create_void(fn_void, fn)} {}
  stateless_function(): externref{} {}
};

}

template<int = 0>
[[jsbind::jsbind("()=>(str)=>console.log(str)")]]
[[clang::import_module("jsbind")]]
void assert(bool);


template<class F>
struct function;
template<class R, class ...Args>
struct function<R(Args ...) const>: function<R(Args ...)> {
  using function<R(Args ...)>::function;
};
template<class R, class ...Args>
struct function<R(Args ...)> {
  [[jsbind::jsbind("()=>(obj, ...args)=>obj(...args)")]]
  [[clang::import_module("jsbind")]]
  static R call(js::binds::js_type auto ...);
  [[jsbind::jsbind("()=>(obj, ...args)=>obj(...args)")]]
  [[clang::import_module("jsbind")]]
  static void call_void(js::binds::js_type auto ...);


  impl::stateless_function<R, Args...> stateless;
  impl::erased_function<R, Args...> stateful = {};
  operator bool() const {
    return stateful || stateless;
  }
  operator __externref_t() const {
    if(stateful) return stateful;
    else return stateless;
  }
  function(externref x): stateless(std::move(x)) {}
  function(R(*fn)(Args...)): stateless(fn){}
  function(auto &&fn): stateful((decltype(fn))fn){}
  R operator()(Args ...args) {
    if constexpr (std::same_as<R, void>) {
      if (stateful) call_void(__externref_t(stateful), make_raw(args)...);
      else call_void(make_raw(stateless), make_raw(args)...);
    }
    else {
      if (stateful) return {call(make_raw(stateful), make_raw(args)...)};
      else return {call(__externref_t(stateless), make_raw(args)...)};
    }
  }
};

template<class R, class ...Args>
function(R(*)(Args ...))->function<R(Args ...)>;
template<
  class functor,
  class fn_type = util::memptr_traits<
    std::remove_cvref_t<decltype(&functor::operator())>
  >::value_type
>
function(functor)->function<fn_type>;

}
