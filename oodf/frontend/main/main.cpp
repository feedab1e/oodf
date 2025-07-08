#include <string_view>
#include "oodf/js_api/common_types.hpp"
#include "oodf/util/property.hpp"
#include "oodf/js_api/impl/externref_action.hpp"

struct element: oodf::js::externref {
  using externref::externref;
};

struct html_element: element {
  using element::element;
};

struct html_canvas: html_element {
  using html_element::html_element;
};


struct document: oodf::js::externref {
  template<class = int>
  [[jsbind::jsbind("()=>(document)=>document.children()")]]
  static __externref_t children(__externref_t);
  template<class = int>
  [[jsbind::jsbind("()=>(document, elem)=>document.appendChild(elem)")]]
  static __externref_t appendChild(__externref_t, __externref_t);
  template<class = int>
  [[jsbind::jsbind("()=>(document, elem)=>document.createElement(elem)")]]
  static __externref_t createElement(__externref_t, __externref_t);
  oodf::js::array<element> children() { return children(*this); }
  oodf::js::array<element> appendChild(element e) { return appendChild(*this, e); }
  template<class Elem>
  Elem createElement(oodf::js::string s) {
    return {createElement(*this, s)};
  }
  template<class Elem>
  Elem createElement(std::string_view s) {
    return createElement<Elem>((oodf::js::string)s);
  }
  [[no_unique_address]]oodf::js::impl::property<document> body;
  static_assert(oodf::util::inject_memptr<&document::body>{});
};

static_assert(sizeof(document) == sizeof(oodf::js::externref));

template<class = int>
[[jsbind::jsbind("()=>()=>document")]]
extern __externref_t js_fn();
document this_document {[]{
    return js_fn();
}()};

[[jsbind::jsbind("()=>console.log")]]
void print(oodf::js::binds::js_type auto ...);

void console_log(auto ...xs) {
  print(oodf::js::make_raw(xs)...);
}

using namespace oodf::js::literal;

int main() {
  oodf::js::function say_success = []{console_log("success"_js);};
  say_success();
  
  console_log(oodf::js::array<>::of("foobar"_js, 2));
  auto canvas = this_document.createElement<html_canvas>("canvas");
  this_document.body->appendChild(canvas);
}
