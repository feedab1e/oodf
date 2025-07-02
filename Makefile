all: frontend

test: frontend
	cmake --build build/frontend --target test_basic --target test_fail --target test_function
	ctest --test-dir build/frontend --rerun-failed --output-on-failure 

frontend: tool
	cmake --toolchain ${PWD}/build/tool/tool/JsBindToolchain.cmake -B build/frontend -S oodf
	cmake --build build/frontend --target main
	cp build/frontend/compile_commands.json compile_commands.json

tool:
	cmake -B build/tool -S oodf
	cmake --build build/tool --target jsbind -v

clean:
	rm -rf build
