# ztr

[![CI](https://github.com/samlmbrt/ztr/actions/workflows/ci.yml/badge.svg)](https://github.com/samlmbrt/ztr/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A C11 string library with small string optimization, UTF-8 support, and safe-by-default mutation.

`ztr` provides a mutable, owning string type (`ztr`) and a non-owning string view (`ztr_view`). Both target C11 with no dependencies beyond the standard library.

## Why ztr?

C's standard string handling (`char*` + `strlen` + `strcat` + manual `malloc`) is error-prone and verbose. `ztr` fixes this with a small, focused API that makes correct usage easy and incorrect usage hard:

- **Small string optimization** — strings of 15 bytes or fewer (7 on 32-bit) live inline in the struct. No heap allocation, no pointer chasing.
- **Branchless `ztr_len`** — length is always a single load + mask. No SSO/heap branching on the hottest accessor.
- **Safe by default** — all mutation functions handle self-referential input (e.g., `ztr_append(&s, ztr_cstr(&s))`), check for integer overflow, and provide a strong guarantee (unmodified on error).
- **Zero-init is valid** — `ztr s = {0}` is a valid empty string. No constructor call required.
- **`const char*` interop** — `ztr_cstr()` is always O(1) and null-terminated. Pass it directly to `printf`, `fopen`, `strcmp`, or any C API.
- **Non-owning views** — `ztr_view` references slices of existing buffers without allocating. Zero-copy parsing, searching, and trimming.

## Quick start

### Drop-in (no build system required)

Copy `src/ztr.h` and `src/ztr.c` into your project. Compile `ztr.c` with your existing build:

```sh
cc -std=c11 -c ztr.c -o ztr.o
ar rcs libztr.a ztr.o
```

### With CMake

```sh
git clone https://github.com/samlmbrt/ztr.git
cd ztr
make test    # build with sanitizers + run all tests
```

Or add to your project via `add_subdirectory`:

```cmake
add_subdirectory(ztr)
target_link_libraries(your_target PRIVATE ztr)
```

## Usage

```c
#include "ztr.h"
#include <stdio.h>

int main(void) {
    // Construction (error checks omitted for brevity)
    ztr greeting = {0};
    ztr_from(&greeting, "hello");
    ztr_append(&greeting, ", world!");
    printf("%s\n", ztr_cstr(&greeting));  // "hello, world!"

    // Formatting
    ztr msg = {0};
    ztr_fmt(&msg, "count: %d", 42);
    printf("%s (len=%zu)\n", ztr_cstr(&msg), ztr_len(&msg));

    // Search and replace
    ztr path = {0};
    ztr_from(&path, "/usr/local/bin");
    if (ztr_starts_with(&path, "/usr")) {
        ztr_replace_first(&path, "local", "share");
    }
    printf("%s\n", ztr_cstr(&path));  // "/usr/share/bin"

    // Split (zero allocation)
    ztr csv = {0};
    ztr_from(&csv, "alice,bob,charlie");
    ztr_split_iter it;
    ztr_split_begin(&it, &csv, ",");
    const char *part;
    size_t part_len;
    while (ztr_split_next(&it, &part, &part_len)) {
        printf("%.*s\n", (int)part_len, part);
    }

    // Cleanup
    ztr_free(&greeting);
    ztr_free(&msg);
    ztr_free(&path);
    ztr_free(&csv);
    return 0;
}
```

## String views

`ztr_view` is a non-owning, read-only reference to a contiguous byte range. It holds a pointer and a length, never allocates, and never frees. Use it for zero-copy parsing, matching, and passing substrings without copying.

```c
#include "ztr.h"
#include <stdio.h>

int main(void) {
    ztr header = {0};
    ztr_from(&header, "  Content-Type: text/html  ");

    // Create a view, trim whitespace (no allocation)
    ztr_view v = ztr_view_trim(ztr_view_from_ztr(&header));

    // Compile-time literal views — no strlen
    if (ztr_view_starts_with(v, ZTR_VIEW_LIT("Content-Type"))) {
        ztr_view value = ztr_view_remove_prefix(v, 14);  // skip "Content-Type: "
        printf("value: " ZTR_VIEW_FMT "\n", ZTR_VIEW_ARG(value));
    }

    // Split a view into tokens (zero allocation)
    ztr_view csv = ztr_view_from_cstr("alice,bob,charlie");
    ztr_view_split_iter it;
    ztr_view_split_begin(&it, csv, ZTR_VIEW_LIT(","));
    ztr_view token;
    while (ztr_view_split_next(&it, &token)) {
        printf(ZTR_VIEW_FMT "\n", ZTR_VIEW_ARG(token));
    }

    // Materialize a view into an owned string when needed
    ztr owned = {0};
    ztr_from_view(&owned, ztr_view_from_cstr("hello"));

    ztr_free(&owned);
    ztr_free(&header);
    return 0;
}
```

A `ztr_view` is invalidated if the underlying buffer is mutated or freed. Do not hold views across mutations of the source.

## API overview

102 functions — 62 owning (`ztr`) and 40 non-owning (`ztr_view`):

### `ztr` — owning string

| Category | Functions | Description |
|---|---|---|
| Lifecycle | `ztr_init`, `ztr_from`, `ztr_from_buf`, `ztr_with_cap`, `ztr_fmt`, `ztr_clone`, `ztr_assign`, `ztr_assign_buf`, `ztr_move`, `ztr_free` | Create, copy, reassign, and destroy strings |
| Accessors | `ztr_len`, `ztr_cstr`, `ztr_is_empty`, `ztr_capacity`, `ztr_at` | Read string properties (inline, branchless where possible) |
| Comparison | `ztr_eq`, `ztr_eq_cstr`, `ztr_cmp`, `ztr_cmp_cstr`, `ztr_eq_ascii_nocase` | Equality and ordering |
| Search | `ztr_find`, `ztr_rfind`, `ztr_contains`, `ztr_starts_with`, `ztr_ends_with`, `ztr_count` | Byte-offset search with `ZTR_NPOS` sentinel |
| Mutation | `ztr_append`, `ztr_append_buf`, `ztr_append_ztr`, `ztr_append_byte`, `ztr_append_fmt`, `ztr_insert`, `ztr_insert_buf`, `ztr_erase`, `ztr_replace_first`, `ztr_replace_all`, `ztr_clear`, `ztr_truncate`, `ztr_reserve`, `ztr_shrink_to_fit` | Modify content in place |
| Transformation | `ztr_trim`, `ztr_trim_start`, `ztr_trim_end`, `ztr_to_ascii_upper`, `ztr_to_ascii_lower` | In-place ASCII transformations |
| Extraction | `ztr_substr` | Extract substrings |
| Split/Join | `ztr_split_begin`, `ztr_split_next`, `ztr_split_alloc`, `ztr_split_free`, `ztr_join`, `ztr_join_cstr` | Zero-allocation iterator or allocating split; join with separator |
| UTF-8 | `ztr_is_valid_utf8`, `ztr_utf8_len`, `ztr_utf8_next`, `ztr_utf8_append` | Validation, codepoint counting, iteration, and encoding |
| Interop | `ztr_data_mut`, `ztr_set_len`, `ztr_detach`, `ztr_swap` | Advanced: mutable buffer access, ownership transfer |
| Utility | `ztr_is_ascii`, `ztr_err_str` | Character classification, error messages |

### `ztr_view` — non-owning string view

| Category | Functions | Description |
|---|---|---|
| Construction | `ztr_view_from_buf`, `ztr_view_from_cstr`, `ztr_view_from_ztr`, `ztr_view_substr` | Build a view from a buffer, C string, or `ztr`; extract a sub-view |
| Accessors | `ztr_view_len`, `ztr_view_data`, `ztr_view_is_empty`, `ztr_view_at` | Read view properties (all inline) |
| Comparison | `ztr_view_eq`, `ztr_view_eq_cstr`, `ztr_view_cmp`, `ztr_view_cmp_cstr`, `ztr_view_eq_ascii_nocase`, `ztr_view_eq_ascii_nocase_cstr` | Equality and ordering |
| Search (view) | `ztr_view_find`, `ztr_view_rfind`, `ztr_view_contains`, `ztr_view_starts_with`, `ztr_view_ends_with`, `ztr_view_count` | Search with `ztr_view` needle — zero-copy, no `strlen` |
| Search (cstr) | `ztr_view_find_cstr`, `ztr_view_rfind_cstr`, `ztr_view_contains_cstr`, `ztr_view_starts_with_cstr`, `ztr_view_ends_with_cstr`, `ztr_view_count_cstr` | Convenience wrappers for `const char*` needles |
| Search (char) | `ztr_view_find_char`, `ztr_view_rfind_char`, `ztr_view_contains_char` | Single-byte search via `memchr` |
| Trimming | `ztr_view_trim`, `ztr_view_trim_start`, `ztr_view_trim_end` | Return narrowed view, no allocation |
| Slice | `ztr_view_remove_prefix`, `ztr_view_remove_suffix` | Advance or shrink bounds (clamped) |
| Split | `ztr_view_split_begin`, `ztr_view_split_begin_cstr`, `ztr_view_split_next` | Zero-allocation iterator over delimited tokens |
| Utility | `ztr_view_is_ascii`, `ztr_view_is_valid_utf8` | Classification and validation |
| Interop | `ztr_from_view` | Copy view data into an owned `ztr` |

### Printf helpers

```c
// Owned strings:
printf("s=" ZTR_FMT "\n", ZTR_ARG(&s));

// Views (not null-terminated — always use these macros, never %s):
printf("v=" ZTR_VIEW_FMT "\n", ZTR_VIEW_ARG(v));

// Compile-time view from a literal (no strlen):
ztr_view sep = ZTR_VIEW_LIT(",");

// Canonical empty view:
ztr_view empty = ZTR_VIEW_EMPTY;
```

See [docs/SPEC.md](docs/SPEC.md) for the complete API reference with signatures, preconditions, edge cases, and error behavior.

## Error handling

Functions that can fail return `ztr_err`:

```c
ztr s = {0};
ztr_err err = ztr_from(&s, "hello");
if (err) {
    fprintf(stderr, "error: %s\n", ztr_err_str(err));
}
```

| Error code | Meaning |
|---|---|
| `ZTR_OK` | Success (always 0) |
| `ZTR_ERR_ALLOC` | `malloc` / `realloc` failed |
| `ZTR_ERR_NULL_ARG` | `NULL` passed where non-NULL required |
| `ZTR_ERR_OUT_OF_RANGE` | Index or range out of bounds |
| `ZTR_ERR_INVALID_UTF8` | Byte sequence is not valid UTF-8 |
| `ZTR_ERR_OVERFLOW` | Size computation would overflow |

Functions that cannot fail return their value directly (`size_t`, `bool`, `const char*`, `void`). On error, the `ztr` object is always left unmodified (strong guarantee).

## Build targets

| Command | Description |
|---|---|
| `make test` | Build with AddressSanitizer + UBSan, run all tests |
| `make debug` | Build debug (no sanitizers) |
| `make release` | Build optimized (`-O2 -march=native -flto`) |
| `make bench` | Build and run benchmarks |
| `make fuzz` | Build libFuzzer harness |
| `make valgrind` | Run tests under Valgrind |
| `make coverage` | Generate code coverage report |
| `make format` | Run clang-format on all source files |
| `make clean` | Remove all build artifacts |

## Compile-time configuration

Define these before including `ztr.h` or pass them as compiler flags:

```c
// Custom allocator (default: malloc/realloc/free)
#define ZTR_MALLOC(size)        my_alloc(size)
#define ZTR_REALLOC(ptr, size)  my_realloc(ptr, size)
#define ZTR_FREE(ptr)           my_free(ptr)

// Growth factor (default: 1.5x = 3/2). Override for constrained systems:
#define ZTR_GROWTH_NUM  5
#define ZTR_GROWTH_DEN  4   // 1.25x

// Minimum heap allocation (default: 64 bytes)
#define ZTR_MIN_HEAP_CAP  32

// Exclude printf-based functions (saves 8-20KB flash on embedded)
#define ZTR_NO_FMT
```

## Data structure

```c
typedef struct ztr {
    size_t len;   // high bit = heap flag, remaining bits = byte length
    union {
        struct { char *data; size_t cap; } heap;
        char sso[sizeof(char *) + sizeof(size_t)];
    };
} ztr;  // 24 bytes on 64-bit, 12 bytes on 32-bit

typedef struct ztr_view {
    const char *data;
    size_t len;
} ztr_view;  // 16 bytes on 64-bit, 8 bytes on 32-bit. Passed by value.
```

- **SSO capacity:** 15 bytes (64-bit), 7 bytes (32-bit)
- **`ztr_len`:** single load + mask, zero branches
- **`ztr s = {0}`:** valid empty string, no constructor needed
- **`ztr_free`:** resets to empty, double-free safe
- **`ztr_view`:** non-owning, read-only, fits in 2 registers

## Documentation

- [docs/SPEC.md](docs/SPEC.md) — complete API specification (signatures, edge cases, guarantees)
- [docs/DESIGN_DISCUSSION.md](docs/DESIGN_DISCUSSION.md) — design panel record (rationale, alternatives considered, trade-offs)

## License

MIT
