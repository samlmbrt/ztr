.PHONY: debug sanitize release test clean format fuzz valgrind coverage bench

debug:
	@[ -d build/debug ] || cmake --preset debug
	cmake --build build/debug

sanitize:
	@[ -d build/sanitize ] || cmake --preset sanitize
	cmake --build build/sanitize

release:
	@[ -d build/release ] || cmake --preset release
	cmake --build build/release

test: sanitize
	cd build/sanitize && ctest --output-on-failure

bench:
	@[ -d build/release ] || cmake --preset release
	cmake -B build/release -DZTR_BUILD_BENCHMARKS=ON
	cmake --build build/release --target ztr_bench
	./build/release/ztr_bench

fuzz:
	@[ -d build/fuzz ] || cmake --preset fuzz \
		$(if $(shell which /opt/homebrew/opt/llvm/bin/clang 2>/dev/null),-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang) \
		$(if $(shell which /usr/local/opt/llvm/bin/clang 2>/dev/null),-DCMAKE_C_COMPILER=/usr/local/opt/llvm/bin/clang)
	cmake --build build/fuzz
	@echo "Run: ./build/fuzz/ztr_fuzz corpus/ -max_len=4096 -max_total_time=300"

valgrind: debug
	valgrind --leak-check=full --error-exitcode=1 ./build/debug/ztr_tests

coverage:
	cmake -B build/coverage -DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_C_FLAGS="--coverage" -DCMAKE_EXE_LINKER_FLAGS="--coverage"
	cmake --build build/coverage
	cd build/coverage && ctest --output-on-failure
	lcov --capture --directory build/coverage --output-file build/coverage/coverage.info
	lcov --remove build/coverage/coverage.info '*/tests/*' --output-file build/coverage/coverage.info
	genhtml build/coverage/coverage.info --output-directory build/coverage/html
	@echo "Coverage report: build/coverage/html/index.html"

format:
	find src tests fuzz benchmarks -name '*.c' -o -name '*.h' | grep -v greatest.h | xargs clang-format -i

clean:
	rm -rf build
