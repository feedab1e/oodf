#pragma once
#include <cstddef>
#include <algorithm>
namespace oodf::util {

template<class T>
struct memptr_traits;
template<class C, class R>
struct memptr_traits<R C::*> {
  typedef C class_type;
  typedef R value_type;
};
template<class C, class R>
struct memptr_traits<R C::* const> {
  typedef C class_type;
  typedef R value_type;
};

template<class T>
struct fn_traits;
template<class R, class ...Args>
struct fn_traits<R(Args ...)> {
  typedef R fn_type(Args ...);
  constexpr static auto arg_types = [](auto fs){
    return fs(std::declval<Args>()...);
  };
  typedef R value_type;
};
template<class R, class ...Args>
struct fn_traits<R(Args ...) const> {
  typedef R fn_type(Args ...);
  constexpr static auto arg_types = [](auto fs){
    return fs(std::declval<Args>()...);
  };
  typedef R value_type;
};

template<size_t size>
struct static_str {
    char data[size+1]{};
    constexpr static_str(const char *in) {
        std::copy_n(in, size, data);
    }
    constexpr operator const char *() const {
      return data;
    }
};

constexpr auto static_str_from(auto f) {
    constexpr auto size = f().size();
    return static_str<size>{f().data()};
}

template<auto x>
struct value_t {
    constexpr operator decltype(x)() const { return x; }
    constexpr auto operator *() const { return x; }
};
template<class x>
struct type_t {
    constexpr x operator *() const;
};


}
