#include <vector>

static std::vector<int> externref_freelist;

namespace oodf::js::impl {
int freelist_size() noexcept {
  return externref_freelist.size();
}
void freelist_push(int i) noexcept {
  return externref_freelist.push_back(i);
}
int freelist_pop() noexcept {
  int n = externref_freelist.back();
  externref_freelist.pop_back();
  return n;
}
}
