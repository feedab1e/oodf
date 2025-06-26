all: frontend

test: frontend
	cmake --build build/frontend --target test_basic
	ctest --test-dir build/frontend --rerun-failed --output-on-failure

frontend: tool
	cmake --toolchain ${PWD}/build/tool/tool/JsBindToolchain.cmake -B build/frontend -S oodf
	cmake --build build/frontend --target main

tool:
	cmake -B build/tool -S oodf
	cmake --build build/tool --target jsbind
