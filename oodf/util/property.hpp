#pragma once
#include "types.hpp"

namespace oodf::util {

template<auto action, class pt, auto = []{}>
struct property{
    constexpr friend auto assoc_field(type_t<property>);
    auto &&get_class() const {
        constexpr auto ptr = assoc_field(type_t<property>{});
        using class_type = memptr_traits<decltype(*ptr)>::class_type;
        void *cl = (char *)this - std::bit_cast<size_t>(*ptr);
        return *(class_type *)cl;
    }

    template<auto = 0>
    auto operator *() const {
        return action.read(get_class(), assoc_field(type_t<property>{}), type_t<pt>{});
    }
    template<auto = 0>
    auto operator ->() const {
      using return_type = decltype(this->operator*());
        struct access {
          return_type val;
          return_type *operator->() { return &val; }
          const return_type *operator->() const { return &val; }
        };
        return access{this->operator*()};
    }
    operator auto() const {
        return this->operator*();
    }
    template<auto = 0>
    property operator=(pt &&v) requires requires {
      action.write(get_class(), assoc_field(type_t<property>{}), v);
      requires !std::is_const_v<pt>;
    } {
      action.write(get_class(), assoc_field(type_t<property>{}), v);
      return *this;
    }
};
template<auto x>
struct inject_memptr {
    constexpr friend auto assoc_field(type_t<typename memptr_traits<decltype(x)>::value_type>) {
        return value_t<x>{};
    }
    constexpr operator bool() const { return true; }
};

}
