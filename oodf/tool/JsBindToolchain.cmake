set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_C_COMPILER clang)
#set(CMAKE_CXX_FLAGS "-fplugin=@JSBIND_PLUGIN_PATH@ --target=wasm32-unknown-wasi -Wl,--unresolved-symbols=report-all -Wl,--export-table -fno-exceptions")
set(CMAKE_CXX_FLAGS "-fplugin=@JSBIND_PLUGIN_PATH@ -fplugin-arg-jsbind-import-mod=jsbind --target=wasm32-unknown-wasi -fno-exceptions")
set(CMAKE_C_FLAGS "--target=wasm32-unknown-wasi")
set (CMAKE_CXX_LINKER_WRAPPER_FLAG "-Wl,")
set (CMAKE_CXX_LINKER_WRAPPER_FLAG_SEP ",")
set (CMAKE_C_LINKER_WRAPPER_FLAG "-Wl,")
set (CMAKE_C_LINKER_WRAPPER_FLAG_SEP ",")
#add_link_options(-Wl--unresolved-symbols=report-all)
add_link_options(-Wl,--export-table)
#set(CMAKE_MAKE_PROGRAM make)
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR wasm32)
set(CMAKE_SYSROOT /usr/share/wasi-sysroot)
