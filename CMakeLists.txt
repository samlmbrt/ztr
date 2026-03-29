cmake_minimum_required(VERSION 3.20)
project(ztr LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# --- Library target ---

add_library(ztr STATIC src/ztr.c)
target_include_directories(ztr PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)
if(MSVC)
    target_compile_options(ztr PRIVATE /W4 /WX)
else()
    target_compile_options(ztr PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
    )
endif()

# --- Sanitizer flags (Sanitize build type) ---

if(CMAKE_BUILD_TYPE STREQUAL "Sanitize")
    add_compile_options(-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# --- Release optimizations ---

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(ztr PRIVATE -O2 -march=native -flto)
    target_link_options(ztr PRIVATE -flto)
endif()

# --- Tests ---

option(ZTR_BUILD_TESTS "Build ztr test suite" ON)
if(ZTR_BUILD_TESTS)
    enable_testing()

    add_executable(ztr_tests
        tests/test_main.c
        tests/test_lifecycle.c
        tests/test_accessors.c
        tests/test_comparison.c
        tests/test_search.c
        tests/test_mutation.c
        tests/test_transform.c
        tests/test_split_join.c
        tests/test_utf8.c
        tests/test_interop.c
        tests/test_edge_cases.c
        tests/test_view.c
    )
    target_include_directories(ztr_tests PRIVATE tests)
    target_link_libraries(ztr_tests PRIVATE ztr)
    if(MSVC)
        target_compile_options(ztr_tests PRIVATE /W4 /WX)
    else()
        target_compile_options(ztr_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Werror
        )
    endif()

    add_test(NAME ztr_tests COMMAND ztr_tests)
endif()

# --- Benchmarks ---

option(ZTR_BUILD_BENCHMARKS "Build ztr benchmarks" OFF)
if(ZTR_BUILD_BENCHMARKS)
    add_executable(ztr_bench benchmarks/bench_main.c)
    target_link_libraries(ztr_bench PRIVATE ztr)
    target_compile_options(ztr_bench PRIVATE -O2 -march=native -flto)
    target_link_options(ztr_bench PRIVATE -flto)
endif()

# --- Fuzz ---

option(ZTR_BUILD_FUZZ "Build fuzz harness" OFF)
if(ZTR_BUILD_FUZZ)
    add_executable(ztr_fuzz fuzz/fuzz_parse.c)
    target_link_libraries(ztr_fuzz PRIVATE ztr)
    target_compile_options(ztr_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
    target_link_options(ztr_fuzz PRIVATE -fsanitize=fuzzer,address,undefined)
endif()
