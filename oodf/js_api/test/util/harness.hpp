#include <string_view>
#include <oodf/js_api/common_types.hpp>
void assert(bool, std::string_view = "");
struct future_assert {
  oodf::js::externref handle;
  future_assert(std::string_view name);
  void operator()(bool success) const;
};
