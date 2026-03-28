# ZTR v1 Specification

## 1. Overview

`ztr` is a modern, ergonomic, secure string library for C. It provides a mutable, owning string type with small string optimization (SSO), UTF-8 awareness, and safe-by-default error handling.

### Design Goals

- Modern ergonomics inspired by C++ `std::string`, Rust `String`, Kotlin `String`
- Small and focused — essential operations done right
- Performant enough for embedded and constrained devices (Cortex-M4+)
- Secure by default — safe APIs, no hidden undefined behavior
- Seamless interop with `const char*` / existing C APIs
- General purpose

### Target

- **Language:** C11 (requires `_Static_assert`, anonymous unions, `<stdalign.h>`)
- **Platforms:** Any with standard user-space virtual memory
- **Dependencies:** C11 standard library only (`<string.h>`, `<stdlib.h>`, `<stdint.h>`, `<stddef.h>`, `<stdbool.h>`)
- **Optional dependency:** `<stdio.h>` (only when `ZTR_NO_FMT` is not defined)

### Library Structure

- `ztr.h` — public API, struct definition, and `static inline` hot accessors
- `ztr.c` — implementation

The following functions are inlined in the header for performance: `ztr_len`, `ztr_cstr`, `ztr_is_empty`, `ztr_capacity`, `ztr_at`, `ztr_append_byte` (fast path only), `ztr_append_buf` (fast path only), `ztr_clear`, and `ztr_data_mut`.

---

## 2. Compile-Time Configuration

```c
// Custom allocator (default: malloc/realloc/free)
#ifndef ZTR_MALLOC
  #define ZTR_MALLOC(size)           malloc(size)
#endif
#ifndef ZTR_REALLOC
  #define ZTR_REALLOC(ptr, size)     realloc(ptr, size)
#endif
#ifndef ZTR_FREE
  #define ZTR_FREE(ptr)              free(ptr)
#endif

// Growth factor (default: 1.5x = 3/2)
#ifndef ZTR_GROWTH_NUM
  #define ZTR_GROWTH_NUM  3
#endif
#ifndef ZTR_GROWTH_DEN
  #define ZTR_GROWTH_DEN  2
#endif

// Feature gates
// Define ZTR_NO_FMT to EXCLUDE printf-based functions (ztr_fmt, ztr_append_fmt).
// When not defined, formatting functions are available (default).
// This saves 8-20KB of flash on embedded toolchains that link printf eagerly.
// #define ZTR_NO_FMT
```

---

## 3. Data Structure

### Layout

```c
#include <limits.h>

#define ZTR_HEAP_BIT   ((size_t)1 << (sizeof(size_t) * CHAR_BIT - 1))
#define ZTR_MAX_LEN    (SIZE_MAX >> 1)                      // max representable length
#define ZTR_SSO_CAP    (sizeof(char*) + sizeof(size_t) - 1)
// 15 on 64-bit (LP64), 7 on 32-bit (ILP32)

_Static_assert(sizeof(size_t) >= 2, "ztr requires size_t of at least 16 bits");

typedef struct ztr {
    size_t len;   // High bit: 1 = heap mode, 0 = SSO mode. Remaining bits: byte length.
    union {
        struct {
            char   *data;   // heap-allocated buffer (always null-terminated)
            size_t  cap;    // capacity in bytes (excludes null terminator)
        } heap;
        char sso[sizeof(char*) + sizeof(size_t)];
        // SSO inline buffer. 16 bytes on 64-bit, 8 on 32-bit.
    };
} ztr;

_Static_assert(sizeof(ztr) == sizeof(size_t) * 3, "unexpected ztr struct size");
```

### Properties

| Property              | Value                                                                |
| --------------------- | -------------------------------------------------------------------- |
| `sizeof(ztr)`         | 24 on 64-bit, 12 on 32-bit                                           |
| SSO capacity          | 15 bytes (64-bit), 7 bytes (32-bit), excluding null terminator       |
| Maximum string length | `ZTR_MAX_LEN` = `SIZE_MAX >> 1` (~4.6 EB on 64-bit, ~1 GB on 32-bit) |
| Zero-initialized      | `ztr s = {0}` is a valid empty SSO string                            |
| `ztr_len` cost        | 1 load + 1 AND, zero branches                                        |
| Pointer assumptions   | None                                                                 |
| Endianness concerns   | None                                                                 |

### Discriminator

The high bit of the `len` field serves as the mode discriminator:

- `len & ZTR_HEAP_BIT` is set: **heap mode**. String data lives at `heap.data`, capacity is tracked in `heap.cap`.
- `len & ZTR_HEAP_BIT` is clear: **SSO mode**. String data lives inline in `sso[]`.

The actual byte length is always `len & ~ZTR_HEAP_BIT`.

This design avoids all pointer-value assumptions and endianness concerns. The discriminator is accessed via a simple bitwise AND on a native-width integer.

### SSO Details

- SSO buffer: `sso[0]` through `sso[sizeof(char*) + sizeof(size_t) - 1]`
- String content occupies `sso[0]` through `sso[len - 1]`
- Null terminator at `sso[len]`
- Maximum SSO string length: `ZTR_SSO_CAP` (15 bytes on 64-bit, 7 bytes on 32-bit). This is the buffer size minus one byte reserved for the null terminator.

### Invariants

1. **Always null-terminated.** In SSO mode: `sso[len] == '\0'`. In heap mode: `data[len] == '\0'`.
2. **`ztr_free()` resets to `{0}`.** A freed `ztr` is a valid empty SSO string. Double-free is safe. Use-after-free reads an empty string.
3. **Strong error guarantee.** On error, the `ztr` is unmodified.
4. **Null terminator maintained.** Every public API call preserves the null terminator invariant.

### Internal Helpers

```c
static inline bool ztr_p_is_heap(const ztr *s) {
    return s->len & ZTR_HEAP_BIT;
}
```

---

## 4. Error Handling

```c
typedef enum {
    ZTR_OK = 0,           // success
    ZTR_ERR_ALLOC,        // malloc/realloc failed
    ZTR_ERR_NULL_ARG,     // NULL passed where non-NULL required
    ZTR_ERR_OUT_OF_RANGE, // index or range out of bounds
    ZTR_ERR_INVALID_UTF8, // byte sequence is not valid UTF-8
    ZTR_ERR_OVERFLOW,     // size computation would overflow
} ztr_err;
```

### Rules

- Functions that **cannot fail** return their value directly (`size_t`, `bool`, `const char*`, `void`).
- Functions that **can fail** return `ztr_err`. Output goes through pointer parameters.
- On error, output parameters are untouched. The `ztr` object is unmodified (strong guarantee).
- `ZTR_OK == 0`, so `if (ztr_func(...))` checks for error.
- In debug builds (`#ifndef NDEBUG`), contract violations (NULL passed to infallible functions) trigger `assert`.
- In release builds, infallible functions return safe defaults (`0`, `""`, `false`) for NULL input.

---

## 5. Constants

```c
#define ZTR_SSO_CAP     (sizeof(char*) + sizeof(size_t) - 1)  // 15 or 7
#define ZTR_MAX_LEN     (SIZE_MAX >> 1)                        // max representable string length
#define ZTR_NPOS        ((size_t)-1)                           // "not found" sentinel
```

---

## 6. Complete API Surface

### 6.1 Lifecycle

```c
// Initialize to empty string (SSO, no allocation). Equivalent to = {0}.
void ztr_init(ztr *s);
```

```c
// Construct from null-terminated C string. Copies data.
// Precondition: s must be uninitialized or freed. cstr must be null-terminated or NULL.
// NULL cstr produces empty string.
ztr_err ztr_from(ztr *s, const char *cstr);
```

```c
// Construct from a byte buffer with explicit length. Copies data.
// The buffer need not be null-terminated.
// Precondition: s must be uninitialized or freed.
// Overflow check: returns ZTR_ERR_OVERFLOW if len + 1 would overflow.
ztr_err ztr_from_buf(ztr *s, const char *buf, size_t len);
```

```c
// Construct with pre-allocated capacity. String is empty but can hold cap bytes
// without reallocation.
ztr_err ztr_with_cap(ztr *s, size_t cap);
```

```c
// Construct via printf-style formatting.
// Only available when ZTR_NO_FMT is not defined.
// Requires __attribute__((format(printf, 2, 3))) for compile-time format checking.
ztr_err ztr_fmt(ztr *s, const char *fmt, ...);
```

```c
// Deep copy. Precondition: dst must be uninitialized or freed.
// Aliasing: dst == src is allowed (no-op).
ztr_err ztr_clone(ztr *dst, const ztr *src);
```

```c
// Overwrite contents of an already-initialized ztr. Frees old content if needed.
// This is the safe way to reassign a string to a new value.
// Safe when cstr points into s (self-assignment).
ztr_err ztr_assign(ztr *s, const char *cstr);
```

```c
// Overwrite contents from a byte buffer with explicit length. Frees old content if needed.
// Safe when buf points into s (self-assignment).
// NULL buf with len == 0 clears the string (matches ztr_assign(s, NULL) behavior).
// NULL buf with len > 0 returns ZTR_ERR_NULL_ARG.
ztr_err ztr_assign_buf(ztr *s, const char *buf, size_t len);
```

```c
// Transfer ownership from src to dst. Frees any existing content in dst.
// After the call, src is reset to empty. If dst == src, this is a no-op.
void ztr_move(ztr *dst, ztr *src);
```

```c
// Free heap memory (if any). Reset to valid empty SSO state.
// Safe on NULL (no-op). Safe on zero-initialized. Safe to call multiple times.
void ztr_free(ztr *s);
```

### 6.2 Accessors (static inline, infallible)

```c
// Byte length, excluding null terminator. O(1), zero branches.
static inline size_t ztr_len(const ztr *s) {
    return s->len & ~ZTR_HEAP_BIT;
}
```

```c
// Pointer to null-terminated string data. O(1).
// Returned pointer valid until next mutating call on s.
static inline const char *ztr_cstr(const ztr *s) {
    return ztr_p_is_heap(s) ? s->heap.data : s->sso;
}
```

```c
// Is the string empty (length == 0)?
static inline bool ztr_is_empty(const ztr *s) {
    return ztr_len(s) == 0;
}
```

```c
// Current capacity in bytes (excludes null terminator). O(1).
// For SSO strings, returns ZTR_SSO_CAP.
static inline size_t ztr_capacity(const ztr *s) {
    return ztr_p_is_heap(s) ? s->heap.cap : ZTR_SSO_CAP;
}
```

```c
// Byte at index i. Returns '\0' if i >= len.
// Note: indistinguishable from an embedded null at position i. Use ztr_len for bounds.
static inline char ztr_at(const ztr *s, size_t i) {
    return (i < ztr_len(s)) ? ztr_cstr(s)[i] : '\0';
}
```

```c
// Human-readable error description. Returns a static string literal. Never NULL.
const char *ztr_err_str(ztr_err err);
```

### 6.3 Comparison

```c
// Content equality. Checks length first, then memcmp. Not constant-time.
bool ztr_eq(const ztr *a, const ztr *b);

// Content equality against a null-terminated C string.
bool ztr_eq_cstr(const ztr *s, const char *cstr);

// Lexicographic comparison. Returns <0, 0, or >0 (like strcmp).
int ztr_cmp(const ztr *a, const ztr *b);

// Lexicographic comparison against a null-terminated C string.
int ztr_cmp_cstr(const ztr *s, const char *cstr);

// ASCII case-insensitive equality. Folds only A-Z and a-z; non-ASCII bytes
// are compared as-is.
bool ztr_eq_ascii_nocase(const ztr *a, const ztr *b);
```

### 6.4 Search

All search functions use byte offsets and return `ZTR_NPOS` (`(size_t)-1`) when not found.

**Empty needle behavior:** Passing an empty string `""` as needle:

- `ztr_find`: returns `start` (the empty string is found at every position).
- `ztr_rfind`: returns `start` (or `ztr_len(s)` if `start == ZTR_NPOS`).
- `ztr_contains`: returns `true`.
- `ztr_starts_with` / `ztr_ends_with`: returns `true`.
- `ztr_count`: returns `0` (no meaningful occurrences to count).
- `ztr_replace_first` / `ztr_replace_all` with empty needle: no-op, returns `ZTR_OK`.
- `ztr_split_begin` with empty delimiter: yields the entire string as a single token.

```c
// Find first occurrence of needle starting at byte offset start.
// Returns byte index or ZTR_NPOS.
// Algorithm: Two-Way (Crochemore-Perrin) for needles > 4 bytes. Naive for <= 4 bytes.
size_t ztr_find(const ztr *s, const char *needle, size_t start);

// Find last occurrence of needle at or before byte offset start.
// start is the rightmost byte position to consider as a match start.
// Pass ZTR_NPOS to search the entire string from the end.
size_t ztr_rfind(const ztr *s, const char *needle, size_t start);

// Does the string contain needle?
bool ztr_contains(const ztr *s, const char *needle);

// Does the string start with prefix?
bool ztr_starts_with(const ztr *s, const char *prefix);

// Does the string end with suffix?
bool ztr_ends_with(const ztr *s, const char *suffix);

// Count non-overlapping occurrences of needle (left-to-right).
size_t ztr_count(const ztr *s, const char *needle);
```

### 6.5 Mutation

Mutation functions that can fail return `ztr_err`. On error, `s` is unmodified (strong guarantee). Shrinking operations that never allocate (`ztr_erase`, `ztr_clear`, `ztr_truncate`, `ztr_shrink_to_fit`) return `void`. All mutation functions are safe when their input arguments point into the string being mutated (e.g., appending a string to itself).

```c
// Append a null-terminated C string. Safe when cstr points into s.
ztr_err ztr_append(ztr *s, const char *cstr);

// Append a byte buffer with explicit length. Safe when buf points into s.
ztr_err ztr_append_buf(ztr *s, const char *buf, size_t len);

// Append another ztr. Safe when s and other are the same object.
ztr_err ztr_append_ztr(ztr *s, const ztr *other);

// Append a single byte.
ztr_err ztr_append_byte(ztr *s, char c);

// Append via printf-style formatting.
// Only available when ZTR_NO_FMT is not defined.
// Requires __attribute__((format(printf, 2, 3))) for compile-time format checking.
ztr_err ztr_append_fmt(ztr *s, const char *fmt, ...);

// Insert a C string at byte position pos. Safe when cstr points into s.
ztr_err ztr_insert(ztr *s, size_t pos, const char *cstr);

// Insert a byte buffer with explicit length at byte position pos.
ztr_err ztr_insert_buf(ztr *s, size_t pos, const char *buf, size_t len);

// Remove count bytes starting at byte position pos.
// Clamps: if pos >= len, no-op. If pos + count > len, removes to end.
void ztr_erase(ztr *s, size_t pos, size_t count);

// Replace the first occurrence of needle with replacement.
// Safe when needle or replacement point into s.
// Returns ZTR_ERR_OUT_OF_RANGE if needle is not found.
ztr_err ztr_replace_first(ztr *s, const char *needle, const char *replacement);

// Replace all non-overlapping occurrences of needle (left-to-right) with
// replacement in a single pass, using at most one allocation.
// Safe when needle or replacement point into s.
// Empty needle is a no-op (returns ZTR_OK).
ztr_err ztr_replace_all(ztr *s, const char *needle, const char *replacement);

// Set length to 0. Does not free memory (preserves capacity). Never fails.
void ztr_clear(ztr *s);

// Truncate to new_len bytes. No-op if new_len >= current length. Never fails.
void ztr_truncate(ztr *s, size_t new_len);

// Ensure capacity for at least cap bytes of content (excluding null terminator).
ztr_err ztr_reserve(ztr *s, size_t cap);

// Reduce capacity to fit current length. May transition from heap to SSO.
// No-op if the current slack is less than 16 bytes.
void ztr_shrink_to_fit(ztr *s);
```

**Why some mutation functions return `void`:** `ztr_erase`, `ztr_clear`, and `ztr_truncate` are shrinking operations that never allocate and therefore cannot fail. `ztr_shrink_to_fit` is `void` because a failed `realloc` when shrinking leaves the original allocation intact — silently keeping the excess capacity is the correct behavior.

### 6.6 Transformation (in-place, never allocate)

```c
// Remove leading and trailing ASCII whitespace.
// ASCII whitespace: ' ', '\t', '\n', '\r', '\v', '\f'.
void ztr_trim(ztr *s);

// Remove leading ASCII whitespace.
void ztr_trim_start(ztr *s);

// Remove trailing ASCII whitespace.
void ztr_trim_end(ztr *s);

// Convert ASCII a-z to A-Z in place. Non-ASCII bytes are untouched.
void ztr_to_ascii_upper(ztr *s);

// Convert ASCII A-Z to a-z in place. Non-ASCII bytes are untouched.
void ztr_to_ascii_lower(ztr *s);
```

### 6.7 Extraction

Output parameters use output-first convention.

```c
// Extract substring. out receives bytes s[pos..pos+count).
// Clamps: if pos+count > len, takes to end. If pos >= len, out is empty.
ztr_err ztr_substr(ztr *out, const ztr *s, size_t pos, size_t count);
```

### 6.8 Split and Join

#### Zero-allocation split iterator (primary API)

**Lifetime contract:** The source `ztr` passed to `ztr_split_begin` must not be modified or freed for the lifetime of the iterator. The `const char*` pointers returned by `ztr_split_next` point directly into the source string's buffer — they are invalidated if the source is mutated, freed, or goes out of scope. The iterator itself is a value type (40 bytes on 64-bit) and can be stack-allocated.

```c
typedef struct {
    const char *ztr_p_src;
    size_t      ztr_p_src_len;
    const char *ztr_p_delim;
    size_t      ztr_p_delim_len;
    size_t      ztr_p_pos;
    bool        ztr_p_done;
} ztr_split_iter;

_Static_assert(sizeof(ztr_split_iter) <= 48, "ztr_split_iter size check");

// Initialize a split iterator.
void ztr_split_begin(ztr_split_iter *it, const ztr *s, const char *delim);

// Get next token. Returns false when done.
// *part points into the original string (not a copy). *part_len is the token length.
bool ztr_split_next(ztr_split_iter *it, const char **part, size_t *part_len);
```

#### Allocating split (convenience wrapper)

```c
// Split into array of ztr. Library allocates the array.
// max_parts: maximum number of parts to produce. 0 means unlimited (split on
//   every occurrence). The last part contains the remainder of the string.
// Returns count through *count. Caller must call ztr_split_free.
ztr_err ztr_split_alloc(const ztr *s, const char *delim,
                         ztr **parts, size_t *count, size_t max_parts);

// Free array returned by ztr_split_alloc.
void ztr_split_free(ztr *parts, size_t count);
```

#### Join

```c
// Join count strings with separator. Output-first convention.
ztr_err ztr_join(ztr *out, const ztr *parts, size_t count, const char *sep);

// Join C string array with separator.
ztr_err ztr_join_cstr(ztr *out, const char *const *parts, size_t count, const char *sep);
```

### 6.9 UTF-8

```c
// Validate that contents are well-formed UTF-8.
// Rejects: overlong encodings, surrogate halves (U+D800-U+DFFF),
// codepoints > U+10FFFF.
bool ztr_is_valid_utf8(const ztr *s);

// Count UTF-8 codepoints. O(n).
// Returns ZTR_ERR_INVALID_UTF8 if not valid UTF-8.
ztr_err ztr_utf8_len(size_t *out, const ztr *s);

// Decode one codepoint starting at byte position *pos. Advances *pos past it.
// On success: *cp is the codepoint, *pos advanced by 1-4 bytes.
// On error (malformed): *cp is U+FFFD, *pos advanced by 1 byte
//   (prevents infinite loops in iteration).
// Returns ZTR_ERR_OUT_OF_RANGE if *pos >= len.
// Returns ZTR_ERR_INVALID_UTF8 on malformed sequence.
ztr_err ztr_utf8_next(const ztr *s, size_t *pos, uint32_t *cp);

// Encode one codepoint as UTF-8 and append it.
// Returns ZTR_ERR_INVALID_UTF8 for surrogates or codepoints > U+10FFFF.
ztr_err ztr_utf8_append(ztr *s, uint32_t codepoint);
```

### 6.10 Interop and Advanced

```c
// Mutable pointer to internal buffer. O(1).
//
// WARNING — ADVANCED/UNSAFE:
//   - Caller must not write past ztr_capacity(s) bytes.
//   - Caller must not corrupt the null terminator.
//   - Caller MUST call ztr_set_len() after writing to update the length.
//   - The returned pointer is INVALIDATED by ANY mutating call on s
//     (including ztr_append, ztr_reserve, etc.). This is because mutations
//     may trigger an SSO-to-heap transition or reallocation, making the
//     old pointer dangling. Hold the pointer only for the duration of
//     the direct write, then call ztr_set_len and discard the pointer.
//   - For SSO strings, the pointer points into the ztr struct itself.
//     An SSO-to-heap transition makes this pointer point at reinterpreted
//     heap metadata — using it after transition is a memory corruption bug.
static inline char *ztr_data_mut(ztr *s) {
    return ztr_p_is_heap(s) ? s->heap.data : s->sso;
}
```

```c
// Update length after external mutation via ztr_data_mut.
// Validates: len must be <= capacity AND len must be <= ZTR_MAX_LEN.
// Writes null terminator at buf[len] on success.
// Returns ZTR_ERR_OUT_OF_RANGE if len > capacity.
// Returns ZTR_ERR_OVERFLOW if len > ZTR_MAX_LEN (high bit set — would corrupt
//   the discriminator and cause memory corruption).
ztr_err ztr_set_len(ztr *s, size_t len);
```

```c
// Transfer ownership of internal buffer to caller via out-parameter.
// For heap strings: transfers the existing heap pointer (no copy, no allocation).
// For SSO strings: allocates a new buffer and copies (may fail with ZTR_ERR_ALLOC).
// On success: *out receives a malloc'd, null-terminated pointer. Caller must
//   free it with ZTR_FREE (or free() by default). s is reset to empty.
// On failure: *out is untouched, s is unmodified.
ztr_err ztr_detach(ztr *s, char **out);
```

```c
// Swap contents of two strings. Never fails, never allocates.
void ztr_swap(ztr *a, ztr *b);
```

### 6.11 Utility

```c
// True if all bytes are in the ASCII range (0x00-0x7F).
bool ztr_is_ascii(const ztr *s);
```

---

## 7. API Summary Table

| Function              | Returns       | Can Allocate?  | Category       |
| --------------------- | ------------- | -------------- | -------------- |
| `ztr_init`            | `void`        | No             | Lifecycle      |
| `ztr_from`            | `ztr_err`     | Yes            | Lifecycle      |
| `ztr_from_buf`        | `ztr_err`     | Yes            | Lifecycle      |
| `ztr_with_cap`        | `ztr_err`     | Yes            | Lifecycle      |
| `ztr_fmt`             | `ztr_err`     | Yes            | Lifecycle      |
| `ztr_clone`           | `ztr_err`     | Yes            | Lifecycle      |
| `ztr_assign`          | `ztr_err`     | Yes            | Lifecycle      |
| `ztr_assign_buf`      | `ztr_err`     | Yes            | Lifecycle      |
| `ztr_move`            | `void`        | No             | Lifecycle      |
| `ztr_free`            | `void`        | No (frees)     | Lifecycle      |
| `ztr_len`             | `size_t`      | No             | Accessor       |
| `ztr_cstr`            | `const char*` | No             | Accessor       |
| `ztr_is_empty`        | `bool`        | No             | Accessor       |
| `ztr_capacity`        | `size_t`      | No             | Accessor       |
| `ztr_at`              | `char`        | No             | Accessor       |
| `ztr_err_str`         | `const char*` | No             | Accessor       |
| `ztr_eq`              | `bool`        | No             | Comparison     |
| `ztr_eq_cstr`         | `bool`        | No             | Comparison     |
| `ztr_cmp`             | `int`         | No             | Comparison     |
| `ztr_cmp_cstr`        | `int`         | No             | Comparison     |
| `ztr_eq_ascii_nocase` | `bool`        | No             | Comparison     |
| `ztr_find`            | `size_t`      | No             | Search         |
| `ztr_rfind`           | `size_t`      | No             | Search         |
| `ztr_contains`        | `bool`        | No             | Search         |
| `ztr_starts_with`     | `bool`        | No             | Search         |
| `ztr_ends_with`       | `bool`        | No             | Search         |
| `ztr_count`           | `size_t`      | No             | Search         |
| `ztr_append`          | `ztr_err`     | Yes            | Mutation       |
| `ztr_append_buf`      | `ztr_err`     | Yes            | Mutation       |
| `ztr_append_ztr`      | `ztr_err`     | Yes            | Mutation       |
| `ztr_append_byte`     | `ztr_err`     | Yes            | Mutation       |
| `ztr_append_fmt`      | `ztr_err`     | Yes            | Mutation       |
| `ztr_insert`          | `ztr_err`     | Yes            | Mutation       |
| `ztr_insert_buf`      | `ztr_err`     | Yes            | Mutation       |
| `ztr_erase`           | `void`        | No             | Mutation       |
| `ztr_replace_first`   | `ztr_err`     | Yes            | Mutation       |
| `ztr_replace_all`     | `ztr_err`     | Yes            | Mutation       |
| `ztr_clear`           | `void`        | No             | Mutation       |
| `ztr_truncate`        | `void`        | No             | Mutation       |
| `ztr_reserve`         | `ztr_err`     | Yes            | Mutation       |
| `ztr_shrink_to_fit`   | `void`        | No             | Mutation       |
| `ztr_trim`            | `void`        | No             | Transformation |
| `ztr_trim_start`      | `void`        | No             | Transformation |
| `ztr_trim_end`        | `void`        | No             | Transformation |
| `ztr_to_ascii_upper`  | `void`        | No             | Transformation |
| `ztr_to_ascii_lower`  | `void`        | No             | Transformation |
| `ztr_substr`          | `ztr_err`     | Yes            | Extraction     |
| `ztr_split_begin`     | `void`        | No             | Split/Join     |
| `ztr_split_next`      | `bool`        | No             | Split/Join     |
| `ztr_split_alloc`     | `ztr_err`     | Yes            | Split/Join     |
| `ztr_split_free`      | `void`        | No (frees)     | Split/Join     |
| `ztr_join`            | `ztr_err`     | Yes            | Split/Join     |
| `ztr_join_cstr`       | `ztr_err`     | Yes            | Split/Join     |
| `ztr_is_valid_utf8`   | `bool`        | No             | UTF-8          |
| `ztr_utf8_len`        | `ztr_err`     | No             | UTF-8          |
| `ztr_utf8_next`       | `ztr_err`     | No             | UTF-8          |
| `ztr_utf8_append`     | `ztr_err`     | Yes            | UTF-8          |
| `ztr_data_mut`        | `char*`       | No             | Interop        |
| `ztr_set_len`         | `ztr_err`     | No             | Interop        |
| `ztr_detach`          | `ztr_err`     | Yes (SSO case) | Interop        |
| `ztr_swap`            | `void`        | No             | Interop        |
| `ztr_is_ascii`        | `bool`        | No             | Utility        |

**Total: 62 functions** (6 `static inline` in header, 56 in `ztr.c`).

---

## 8. Growth Strategy

When heap capacity is exceeded:

```c
new_cap = old_cap * ZTR_GROWTH_NUM / ZTR_GROWTH_DEN;
if (new_cap < required) new_cap = required;
if (new_cap < 64) new_cap = 64;  // minimum first heap allocation
```

The 64-byte minimum on the first heap allocation avoids repeated small reallocations when a string slightly exceeds SSO capacity and continues to grow.

---

## 9. Thread Safety

- **Concurrent reads** of the same `ztr` from multiple threads are safe.
- **Concurrent read and write**, or concurrent writes, to the same `ztr` are undefined behavior. The caller must provide external synchronization.
- **Distinct `ztr` instances** are fully independent. No shared global state exists, and no synchronization is needed between threads operating on different strings.
- **Signal safety:** No `ztr` function is safe to call from a signal handler (because `malloc`, `realloc`, and `free` are not async-signal-safe).

---

## 10. Embedded Null Bytes

`ztr` can contain embedded null bytes when constructed via `ztr_from_buf`. The library tracks length explicitly and does not rely on null termination for internal operations. However, there are important interop implications:

- `ztr_cstr()` returns a pointer to the internal buffer. Standard C functions that use `strlen` (such as `printf("%s", ...)`, `strcmp`, etc.) will see the string as truncated at the first embedded null.
- `ztr_append(s, cstr)` uses `strlen(cstr)` to determine the input length. To append data that may contain embedded nulls, use `ztr_append_buf`.
- `ztr_append_ztr` correctly handles embedded nulls because it uses the tracked length rather than `strlen`.
- The null terminator maintained by the library is always at position `buf[len]`, even if earlier bytes are also null.

---

## 11. Security Notes

### Memory Zeroing

`ztr_free()` does **not** zero the buffer contents before freeing. For security-sensitive data (passwords, tokens, keys), the old buffer contents persist in freed memory. Applications handling sensitive data should overwrite the buffer before freeing:

```c
// Manual secure erase before free
char *buf = ztr_data_mut(&secret);
explicit_bzero(buf, ztr_len(&secret));  // or memset_s, SecureZeroMemory
ztr_free(&secret);
```

A dedicated `ztr_free_secure()` using `explicit_bzero` / `memset_s` is planned for v2.

### Debug Invariant Checks

In debug builds (`#ifndef NDEBUG`), the implementation should assert the null-terminator invariant on every accessor: in SSO mode, `assert(sso[ztr_len(s)] == '\0')`; in heap mode, `assert(data[ztr_len(s)] == '\0')`. This catches internal bugs where a mutation path fails to maintain the null terminator.

### Discriminator Safety

Any function that writes the `len` field must ensure the high bit is set correctly for the current mode. `ztr_set_len()` rejects lengths >= `ZTR_HEAP_BIT` to prevent discriminator corruption. Internally, the library uses `ztr_p_set_len_sso()` and `ztr_p_set_len_heap()` helpers that always set the discriminator bit correctly.

---

## 12. Design Rationale

### Why mutable in-place?

Immutability without garbage collection creates memory management nightmares in C. Every modification requires a separate allocation and free. Mutable in-place is what C programmers expect and avoids hidden copies.

### Why not header-only?

Compile time and binary size. Hot accessors are inlined in the header; everything else lives in `ztr.c`. This keeps compile times low for projects that include `ztr.h` in many translation units.

### Why the high-bit-of-len discriminator?

This design maximizes SSO capacity (15 bytes on 64-bit, 7 on 32-bit) with no pointer assumptions, no endianness concerns, and a branchless `ztr_len` (one load + one AND). An alternative byte-0 tag approach was considered but sacrifices one byte of SSO capacity and requires assumptions about pointer bit patterns.

### Why branchless `ztr_len`?

`ztr_len` is the most frequently called function — used internally by virtually every operation. Making it branchless (one load + one mask) benefits every code path, especially in loops over mixed SSO and heap strings where branch prediction is unreliable.

### Why 1.5x growth?

A 1.5x growth factor allows allocator block reuse: the sum of previously freed blocks exceeds the current allocation after approximately 3 growths. This wastes less memory than 2x on constrained systems while maintaining amortized O(1) appends.

### Why return codes over Result structs?

Returning a struct containing a `ztr` (24 bytes) plus an error code causes ABI issues on some platforms (hidden pointer parameters, register pressure). Return codes are zero-overhead on success and idiomatic C.

### Why ASCII-only case conversion?

Unicode case mapping requires approximately 200 KB of lookup tables and can change string length (e.g., German sharp s uppercases to SS). This belongs in a dedicated Unicode library, not a string container.

### Why a zero-allocation split iterator as the primary API?

Most split use cases process tokens sequentially without needing random access. The iterator avoids all allocation and works on constrained devices. The convenience wrapper `ztr_split_alloc` is available when a materialized array is needed.

---

## 13. Future Directions (v2)

The following features are intentionally deferred from v1. The v1 data structure and API are designed to accommodate all of them without breaking changes:

- Custom allocator per-instance via function pointers in a config struct
- String views and borrowed slices (`ztr_view` type, non-owning)
- Codepoint iterator struct for UTF-8 traversal
- Additional feature gates (`ZTR_NO_UTF8`, `ZTR_NO_SPLIT`)
- `ZTR_NO_ALLOC` compile-time mode for SSO-only, zero-heap environments
- Fixed-buffer mode as a separate `ztr_fixed` type
- File I/O integration (`ztr_getline`, `ztr_read_file`), gated behind `<stdio.h>`
- Case-insensitive search (`ztr_find_ascii_nocase`)
- Secure erasure via `ztr_free_secure()` using `explicit_bzero` or `memset_s`

---

_This specification is the authoritative reference for implementing ztr v1._
