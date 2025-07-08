#pragma once
#include "../common_types.hpp"
#include "oodf/util/property.hpp"

namespace oodf::js::impl {
struct externref_action {
  template<auto ptrmem>
  static constexpr std::string get_name() {
    return std::string{[](std::string_view v){
      v.remove_suffix(1);
      v.remove_prefix(v.find_last_of(':')+1);
      return v;
    }(__PRETTY_FUNCTION__)};
  }
  template<auto ptrmem>
  static constexpr auto get_fn_text() {
    return util::static_str_from([]{
      return "()=>(obj)=>{ return obj." + get_name<ptrmem>() + " }";
    });
  }
  template<auto ptrmem>
  static constexpr auto set_fn_text() {
    return util::static_str_from([]{
      return "()=>(obj, v)=>{ obj." + get_name<ptrmem>() + " = v }";
    });
  }
  template<auto ptrmem, js::binds::js_type R, auto data = get_fn_text<ptrmem>()>
  [[jsbind::jsbind((&*data.data))]]
  static R get_fn(__externref_t);
  template<auto ptrmem, js::binds::js_type R, auto data = set_fn_text<ptrmem>()>
  [[jsbind::jsbind((&*data.data))]]
  static void set_fn(__externref_t, R);

  template<class T, auto ptrmem>
  T read(const js::externref &r, util::value_t<ptrmem>, util::type_t<T>) const {
    return {get_fn<ptrmem, decltype(js::make_raw(std::declval<T>()))>(r)};
  }
  template<class T, auto ptrmem>
  T write(const js::externref &r, util::value_t<ptrmem>, T v) const {
    return set_fn<ptrmem>(r, js::make_raw(std::declval<T>()));
  }
};

template<class T, auto uniq = []{}>
using property = util::property<externref_action{}, T, uniq>;

}
