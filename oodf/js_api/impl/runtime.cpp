#include <cstdint>

template<class = int>
[[jsbind::jsbind(("()=>()=>{throw 'employment jumpscare';}"))]]
[[clang::import_module("jsbind")]]
[[noreturn]] void bail();

extern "C" void __imported_wasi_snapshot_preview1_proc_exit(int) {
  bail();
}
extern "C" int __imported_wasi_snapshot_preview1_fd_close(int) {
  bail();
}
extern "C" void __imported_wasi_snapshot_preview1_fd_open(int) {
  bail();
}
extern "C" int __imported_wasi_snapshot_preview1_fd_write(int, int, int, int) {
  bail();
}
extern "C" int __imported_wasi_snapshot_preview1_fd_seek(int, int64_t, int, int) {
  bail();
}
