static __externref_t externref_table[0];

namespace oodf::js::impl {
int freelist_size() noexcept;
void freelist_push(int) noexcept;
int freelist_pop() noexcept;
int allocate_number(__externref_t r) noexcept {
  if(!freelist_size())
    return __builtin_wasm_table_grow(externref_table, r, 1);
  int n = freelist_pop();
  __builtin_wasm_table_set(externref_table, n, r);
  return n;
}
void free_number(int index) noexcept {
  if (index == -1)
    return;
  freelist_push(index);
}
__externref_t get_number(int index) noexcept {
  return __builtin_wasm_table_get(externref_table, index);
}
void set_number(int index, __externref_t r) noexcept {
  return __builtin_wasm_table_set(externref_table, index, r);
}
}
