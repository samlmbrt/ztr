# ZTR Design Discussion Record

## Overview

- **Library**: `ztr` — a modern, ergonomic, secure C string library
- **Panel**: 5 engineers (Principal, Senior Embedded, Security, Performance, API/UX)
- **Date**: 2026-03-21
- **Process**: Three rounds of review were conducted. Round 1 established initial design proposals and resolved contested points by vote. Round 2 reviewed the resulting draft spec and identified critical issues, naming problems, and security/embedded accommodations. Round 3 reviewed the complete spec incorporating all Round 2 fixes and resolved remaining edge-case and documentation issues.

## Panel Composition

1. **Principal Engineer** — 20+ years of systems programming. Designed multiple production C libraries. Focuses on architectural coherence, ABI stability, and long-term maintainability.

2. **Senior Embedded Engineer** — Background in firmware for resource-constrained targets (ARM Cortex-M, RISC-V). Prioritizes minimal footprint, deterministic behavior, zero-allocation paths, and compatibility with non-mainstream toolchains (IAR, Keil).

3. **Security Engineer** — Specializes in memory safety, exploit mitigation, and secure API design. Reviews code for buffer overflows, integer overflows, format-string vulnerabilities, and use-after-free patterns.

4. **Performance Engineer** — Expert in cache behavior, branch prediction, SIMD, and micro-benchmarking. Profiles at the instruction level. Prioritizes hot-path efficiency over cold-path convenience.

5. **API/UX Engineer** — Focuses on developer experience, naming consistency, discoverability, and how the API "feels" to a new user. Draws on experience with C++ STL, Rust's standard library, and modern language string APIs.

---

## Round 1: Initial Design Proposals

### Design Constraints (from the library creator)

The following constraints were provided to the panel before deliberation:

- Name: `ztr`, prefix `ztr_`
- Modern ergonomics (inspired by C++ STL, Kotlin, Rust)
- Small, focused v1
- Performant for embedded
- Secure by default
- General purpose
- v1 uses malloc/free
- Interested in SSO (Small String Optimization)
- Must interop with `const char*`
- Error handling: return codes or Rust-like Result

### Unanimous Decisions (all 5 agreed immediately)

The following decisions required no debate. All five engineers independently arrived at the same conclusion:

1. **C11 target** — unanimous. C11 provides `_Static_assert`, anonymous unions, `_Alignof`, and enjoys universal toolchain support across GCC, Clang, MSVC, IAR, and Keil. C99 lacks critical features (no `_Static_assert`, no anonymous unions). C17 adds nothing substantive beyond defect resolutions. C23 adoption is too nascent for a library targeting broad compatibility today.

2. **Mutable in-place** — unanimous. Immutability without garbage collection creates a memory management nightmare in C. Every "modification" would require a separate allocation and corresponding free, leading to allocation storms and fragmented ownership. Mutable-in-place semantics with `const ztr*` parameters for read-only access is the only sane model for C.

3. **UTF-8 storage, byte-indexed, validation on demand** — unanimous. UTF-8 has won as the dominant encoding. Byte-level indexing is O(1) and maps directly to the underlying storage. Forced validation on every construction is wasted work for the common case (data already known to be valid, or validity irrelevant for the operation). Codepoint indexing is O(n) and is not even the right abstraction for user-visible text (grapheme clusters are what humans perceive as "characters"). The library stores bytes, indexes by bytes, and provides `ztr_is_valid_utf8()` for opt-in validation when the caller needs it.

4. **SSO included in v1** — unanimous. Adding SSO after v1 is an ABI break (the struct layout changes). The majority of real-world strings are short (identifiers, keys, status codes, HTTP methods, country codes). SSO eliminates malloc/free for these strings entirely. The implementation complexity is contained and well-understood.

5. **Error handling via `ztr_err` enum** — unanimous. `ZTR_OK = 0` enables the idiomatic `if (ztr_func(...))` error-check pattern. Zero overhead on the success path. No global state (unlike `errno`). No allocation (unlike Rust's `Result`). Functions that cannot fail return their value directly (e.g., `ztr_len` returns `size_t`).

6. **Zero-initialization `{0}` must be a valid empty string** — unanimous. This eliminates an entire class of "forgot to initialize" bugs. Arrays of `ztr` can be allocated with `calloc`. Global/static `ztr` variables in BSS are automatically valid. No constructor call required for the empty case.

7. **Off-limits for v1** — unanimous. The following are explicitly excluded: no global state, no locale-dependent behavior, no regex engine, no `wchar_t` support, no copy-on-write (COW), no threading primitives, no custom allocator function pointers in the v1 API surface (compile-time macros for malloc/free/realloc are acceptable).

### Contested Decisions

#### Header-only vs .h/.c split

- **Principal Engineer**: `.h/.c` with optional amalgamation generation via a build script. Separate compilation is the correct model for a C library. Compile time and binary size matter, especially in large projects that include the header in many translation units.

- **Senior Embedded Engineer**: `.h/.c`. Compile time and binary size are critical on embedded toolchains. IAR and Keil handle separate compilation well but choke on large inline expansions.

- **Security Engineer**: `.h/.c`. A single-header model with `#define ZTR_IMPLEMENTATION` creates ODR-like risks when different translation units compile with different defines (e.g., one TU defines `ZTR_NO_FMT` and another does not). Separate compilation eliminates this class of bugs.

- **Performance Engineer**: **Single-header (stb-style).** Full inlining opportunity across all call sites. Zero build integration friction — just drop the file in. Link-time optimization (LTO) can recover some of this for `.h/.c`, but not all compilers/toolchains support LTO equally.

- **API/UX Engineer**: `.h/.c`. The correct compilation model for C. Single-header libraries create confusion about where to put the `#define IMPLEMENTATION` and lead to subtle bugs in multi-TU projects.

- **Vote: 4-1 for `.h/.c`.**
- **Compromise**: Hot accessors (`ztr_len`, `ztr_cstr`, `ztr_is_empty`, `ztr_cap`) are declared `static inline` in the header, giving the Performance Engineer's inlining benefit where it matters most.

#### Struct size and SSO capacity

- **Principal Engineer**: 24 bytes (3 words on 64-bit), 22-byte SSO capacity. Minimal footprint. Two or more structs fit per cache line. Natural alignment.

- **Senior Embedded Engineer**: 24 bytes, 22-byte SSO. Every byte matters on constrained targets. 32 bytes is a 33% overhead increase for no functional gain on the strings embedded systems handle.

- **Security Engineer**: **32 bytes, 30-byte SSO.** More SSO capacity means fewer heap allocations. Fewer heap allocations means a smaller attack surface (fewer opportunities for heap overflow, use-after-free, double-free). The 8 extra bytes per string are worth the security benefit.

- **Performance Engineer**: 24 bytes, 22-byte SSO. Cache-friendly. More strings per cache line. The hot path (accessor functions) benefits from the smaller struct.

- **API/UX Engineer**: 24 bytes, 22-byte SSO. Natural 3-word alignment. Predictable `sizeof`.

- **Vote: 4-1 for 24 bytes.** The Security Engineer's concern was noted but the panel judged that the cache and memory benefits of 24 bytes outweigh the marginal security gain of 32 bytes.

#### Mutable data pointer (`ztr_data_mut`)

- **Principal Engineer**: Yes. Needed for zero-copy I/O (read directly into the string buffer), FFI with C libraries that take `char*`, and interop with POSIX APIs.

- **Senior Embedded Engineer**: Yes. DMA transfers, UART reads, and other hardware interfaces require writable buffers.

- **Performance Engineer**: Yes. Avoiding a copy on the I/O path is essential for high-throughput scenarios.

- **API/UX Engineer**: Yes. Without it, users will cast away const anyway, which is worse.

- **Security Engineer**: **No.** Exposing a writable interior pointer enables buffer overflows (writing past capacity), length/data mismatch (modifying data without updating length), and use-after-free (holding the pointer across a realloc). This is the single most dangerous function in the API.

- **Vote: 4-1 to include.**
- **Compromise**: Pair `ztr_data_mut` with `ztr_set_len()`, which validates that the new length does not exceed capacity and null-terminates the buffer. Document the contract clearly: after modifying data through the raw pointer, the caller must call `ztr_set_len` before any other `ztr_` function.

#### Printf-style `ztr_fmt()`

- **Principal Engineer**: Include. Wraps `vsnprintf` safely with automatic buffer management. Add `__attribute__((format(printf, ...)))` for compile-time format string checking.

- **Senior Embedded Engineer**: Include, but gate behind a compile-time flag. Some embedded targets do not link printf at all.

- **Performance Engineer**: Include. String formatting is a fundamental operation. Without it, users will write their own (worse) version.

- **Security Engineer**: **Hard no.** Format strings are the number one source of format-string vulnerabilities. Even with `__attribute__((format))`, not all compilers enforce it, and runtime-constructed format strings bypass compile-time checks entirely.

- **API/UX Engineer**: No for v1. Keep the API surface small. Users can call `snprintf` into a buffer and use `ztr_from_buf` for now.

- **Vote: 3-2 to include.**
- **Compromise**: Gate behind `ZTR_NO_FMT` compile-time flag per the Embedded Engineer's request. This also addresses the Security Engineer's concern for deployments that want to eliminate format-string risk entirely.

#### `ztr_split()` approach

- **Principal Engineer**: Full split returning an array, with dry-run mode (`max_parts=0` returns count only, no allocation). Two-pass pattern common in C APIs.

- **Senior Embedded Engineer**: Full split with caller-provided array and maximum size. No internal allocation.

- **Security Engineer**: Full split with a `max_splits` cap to prevent denial-of-service via pathological input (e.g., splitting a 1GB string on every byte).

- **Performance Engineer**: Full split with a zero-allocation iterator as the primary API. The iterator avoids allocating an array of results when the caller only needs to process them sequentially.

- **API/UX Engineer**: **Only `ztr_split_once()` in v1.** A full split requires deciding on an array/vector return type, which pulls in container design decisions that are out of scope for a string library.

- **Resolution**: After discussion, the panel agreed to include BOTH:
  - `ztr_split_iter` — a zero-allocation iterator (all five engineers agreed this was the right primary API)
  - `ztr_split_alloc` — an allocating convenience function for cases where the caller needs all results at once
  - The iterator is the primary, recommended API. The allocating version is a convenience.

#### Search return type

- **Principal Engineer**: Return `size_t` with `ZTR_NPOS` sentinel (modeled on C++ `std::string::npos`).

- **Senior Embedded Engineer**: Return `size_t` with `ZTR_NPOS`. Familiar pattern for C/C++ developers.

- **Performance Engineer**: Return `size_t` with `ZTR_NPOS`. No indirection through an out-parameter.

- **Security Engineer**: Return `size_t` with `ZTR_NPOS`. The sentinel is `(size_t)-1`, which is an impossible valid index.

- **API/UX Engineer**: Return `ztr_err` with an out-parameter for the position. Consistent with the error-handling model used elsewhere in the API.

- **Vote: 4-1 for sentinel.** The `if (ztr_find(...) != ZTR_NPOS)` pattern is more ergonomic and familiar than checking an error code and then reading an out-parameter.

---

## Round 2: Spec Review and Critique

After Round 1 decisions were compiled into a draft specification, the panel reconvened for a critical review. Engineers were asked to rate their confidence in the spec on a scale of 1-10.

### Confidence Scores

- Principal Engineer: 5/10
- Senior Embedded Engineer: 4/10
- Security Engineer: 4/10
- Performance Engineer: 5/10
- API/UX Engineer: 5/10

These low-to-moderate scores reflected significant issues discovered during review, detailed below.

### Critical Issues Identified

#### 1. Endianness Bug in Struct Layout (Principal, Embedded, Security)

The original struct layout placed the SSO/heap discriminator tag in the most significant byte (MSB) of the `cap` field, relying on the MSB being the last byte in memory. On big-endian architectures, the MSB of `cap` is the *first* byte in memory, not the last. This means the discriminator position changes with endianness, breaking the struct layout on big-endian targets (PowerPC, some ARM configurations, network processors).

**Resolution**: Move the discriminator to byte 0 of the struct (fixed memory position regardless of endianness). In SSO mode, byte 0 is a tag byte with bit 7 set. In heap mode, byte 0 is part of the data pointer, which always has bit 7 clear on user-space virtual addresses (true on all modern architectures: x86-64, AArch64, RISC-V). This makes the discriminator endian-safe.

#### 2. Self-Referential Aliasing (Principal, Security)

`ztr_append(&s, ztr_cstr(&s))` is a natural, likely usage pattern. But if `ztr_append` triggers a `realloc`, the `const char*` returned by `ztr_cstr` becomes a dangling pointer *before* the append reads from it. This is a use-after-free bug.

**Resolution**: The specification mandates that all mutation functions handle self-referential input safely. Implementations must either snapshot the source data before any potential reallocation, or detect aliasing (check whether the source pointer falls within the destination's buffer) and handle it appropriately.

#### 3. Branching Accessors (Performance)

Every call to `ztr_len` branches on the SSO/heap tag to determine where to read the length from (inline field in SSO mode vs. explicit `len` field in heap mode). The Performance Engineer profiled this and found that on collections of mixed SSO/heap strings, the branch misprediction cost dominates the SSO capacity benefit. `ztr_len` is called on virtually every operation, making this a hot-path penalty.

**Resolution**: Move `len` outside the SSO/heap union to a fixed offset in the struct. `ztr_len` becomes a single load instruction with zero branches. This reduces SSO capacity from 22 bytes to 15 bytes (on 64-bit). The panel accepted the trade-off: branch-free accessors benefit every operation on every string, while the SSO capacity reduction only affects strings in the 16-22 byte range.

#### 4. Integer Overflow (Security)

`ztr_from_buf(out, buf, SIZE_MAX)` causes `len + 1` (for null terminator) to wrap to 0, leading to a zero-size allocation followed by a full-length memcpy — a catastrophic heap overflow.

**Resolution**: All size computations must check for overflow before performing arithmetic. A new error code `ZTR_ERR_OVERFLOW` was added to the `ztr_err` enum. Any operation where a size computation would overflow returns this error immediately.

#### 5. Missing `ztr_append_fmt` (API/UX)

The most common real-world operation — format-and-append for HTTP headers, log lines, file paths, SQL fragments — has no direct API support. Users would have to format into a temporary buffer, then append, requiring manual buffer management that the library exists to eliminate.

**Resolution**: Add `ztr_append_fmt()` with the same `__attribute__((format))` protection as `ztr_fmt()`. Gated behind the same `ZTR_NO_FMT` flag.

#### 6. Double-Init Memory Leak (Principal, API/UX)

Calling `ztr_from` on a `ztr` that already holds heap-allocated data leaks the previous allocation. This is an easy mistake: `ztr_from(&s, "hello"); ztr_from(&s, "world");` leaks the buffer for "hello".

**Resolution**: Add `ztr_assign()` for overwriting an existing string (frees the old buffer if necessary, then sets new content). Document `ztr_from` as a constructor-only function that must be called on uninitialized or zero-initialized memory.

#### 7. Parameter Ordering Inconsistency (API/UX)

Constructors place the output parameter first (`ztr_from(out, cstr)`), following the `memcpy(dst, src, n)` convention. But extractors place it last (`ztr_substr(s, pos, len, out)`). This inconsistency forces users to check documentation for every function.

**Resolution**: Output-first everywhere, following the `memcpy` convention. All functions that produce a `ztr` take the output pointer as their first parameter.

#### 8. `ztr_split` Allocating Design (All 5)

Every engineer identified problems with the allocating `ztr_split` design during independent review:
- Double-pointer pattern (`ztr **out`) is error-prone
- Caller must free both the array and each individual `ztr` in it
- `max_splits` semantics are ambiguous (does it count separators or results?)
- No way to limit memory consumption

**Resolution**: `ztr_split_iter` is the primary, recommended API for splitting. It requires zero allocation and processes results one at a time. `ztr_split_alloc` is retained as a convenience function with clear documentation of cleanup requirements.

### Naming Issues Resolved

The following renames were adopted to improve clarity and consistency:

- `ztr_push` → `ztr_append_byte` — consistent with the `ztr_append` family of functions. "Push" implies stack semantics.
- `ztr_shrink` → `ztr_shrink_to_fit` — avoids confusion with content shrinking (truncation). Matches the C++ `shrink_to_fit` convention.
- `ztr_to_lower` / `ztr_to_upper` → `ztr_to_ascii_lower` / `ztr_to_ascii_upper` — makes it explicit that only ASCII bytes are affected. Prevents users from assuming Unicode case mapping.
- `ztr_from_len` → `ztr_from_buf` — describes what the caller passes (a buffer), not the name of a parameter. More discoverable.
- `ztr_eq_nocase` → `ztr_eq_ascii_nocase` — explicit that comparison is ASCII-only.
- Internal/private prefix: `ztr_p_` — the double-underscore prefix `ztr__` is reserved by C11 section 7.1.3 (identifiers beginning with underscore followed by underscore or uppercase letter are reserved). Using `ztr_p_` (for "private") avoids undefined behavior.

### Security Mandates Adopted

The Security Engineer's review produced the following mandates, all adopted by the panel:

1. **Integer overflow checks** on all size computations. No arithmetic on sizes without prior overflow validation.
2. **Self-referential aliasing** must be safe for all mutation functions. No function may read from a pointer that it might invalidate.
3. **`ztr_set_len` validates `len <= capacity`** and null-terminates. This is the safety net for `ztr_data_mut` usage.
4. **Strong guarantee**: on error, the string is unmodified. No partial mutations. The caller can always retry or continue using the string after a failed operation.
5. **`ztr_fmt` requires `__attribute__((format(printf,...)))`** on compilers that support it. Format string errors are caught at compile time.
6. **All case conversion and trim functions are explicitly ASCII-only.** Function names include "ascii" to prevent misuse on UTF-8 multibyte sequences.
7. **Empty needle behavior specified** for find, replace, and split. Empty needle in find returns 0 (found at start). Empty needle in replace is a no-op. Empty needle in split yields one result (the whole string).
8. **Thread safety documented**: no thread safety for concurrent access to the same `ztr` object. Concurrent access to different `ztr` objects is safe. This matches the C standard library's thread safety model.

### Embedded Accommodations Adopted for v1

The Embedded Engineer's review produced the following accommodations, all adopted for v1:

1. **`ZTR_MALLOC` / `ZTR_FREE` / `ZTR_REALLOC`** compile-time macros. Define these before including the header to redirect all allocation to custom allocators (e.g., FreeRTOS `pvPortMalloc`, pool allocators, or instrumented allocators).
2. **`ZTR_GROWTH_NUM` / `ZTR_GROWTH_DEN`** configurable growth factor as a rational number (default 3/2 = 1.5x). Allows embedded targets to use conservative growth (e.g., 5/4 = 1.25x) to reduce memory waste.
3. **`ZTR_NO_FMT`** compile-time gate. Define this to exclude all printf-dependent code, eliminating the dependency on `vsnprintf` and reducing code size.
4. **`ztr_split_iter`** zero-allocation iterator. Splitting can be performed without any heap allocation, which is critical for embedded targets with limited or no heap.

### Embedded Items Deferred to v2

The following embedded-focused features were discussed but deferred to a future version:

1. **`ZTR_NO_ALLOC` mode** — SSO-only operation with no heap allocation. Strings longer than SSO capacity would return an error instead of allocating. Deferred because it changes the API contract significantly.
2. **Full feature gates** (`ZTR_NO_UTF8`, `ZTR_NO_SEARCH`, etc.) — fine-grained compile-time flags to exclude entire subsystems. Deferred because the v1 API surface is small enough that the code size impact is minimal.
3. **Fixed-buffer mode** — a separate `ztr_fixed` type backed by a caller-provided buffer with no allocation. Deferred with a recommendation to implement as a separate, complementary type rather than a mode flag on `ztr`.

---

## Round 3: Final Spec Review

After the spec was written incorporating all Round 2 fixes, the panel reconvened for a final review. Each engineer was given the complete SPEC.md and asked to find any remaining issues.

### Confidence Scores

- Principal Engineer: ~8/10
- Senior Embedded Engineer: 8/10
- Security Engineer: 8/10
- Performance Engineer: 8/10
- API/UX Engineer: 9/10
- Average: 8.2/10 (up from 4.6 in Round 2)

The substantial confidence increase reflects the impact of the Round 2 fixes — the spec moved from a rough draft with structural problems to a polished document with only edge-case issues remaining.

### Issues Identified and Resolved

#### 1. `ztr_set_len` Must Reject Lengths with High Bit Set (Security, P0)

The high-bit-of-len discriminator introduced a subtle new failure mode: if `ztr_set_len(s, huge_value)` is called with a value where the high bit is set, it flips the discriminator — turning an SSO string into "heap mode" with a garbage pointer, or vice versa. Any subsequent operation on the string would read from or write to an arbitrary memory address.

**Resolution**: `ztr_set_len` rejects `len > ZTR_MAX_LEN` and returns `ZTR_ERR_OVERFLOW`. A new constant `ZTR_MAX_LEN = SIZE_MAX >> 1` was added, representing the largest string length the library can represent without colliding with the discriminator bit. This is a hard, architectural limit — not merely a safety check.

#### 2. `ztr_detach` Heap Behavior Unspecified and Inconsistent Return Convention (Security, Principal)

The original `char *ztr_detach(ztr *s)` returned a pointer (NULL on failure), which is inconsistent with every other allocating function in the library (all of which return `ztr_err`). Additionally, the behavior for heap vs. SSO strings was unclear — does it always allocate a new buffer, or does it transfer ownership of the existing heap buffer?

**Resolution**: Changed to `ztr_err ztr_detach(ztr *s, char **out)`, bringing it in line with the library's error-handling convention. For heap strings, the existing pointer is transferred directly (no copy, no allocation). For SSO strings, a new buffer is allocated and the SSO content is copied into it. In both cases, `s` is reset to the empty state on success. On allocation failure (SSO case), `s` is unmodified and `ZTR_ERR_ALLOC` is returned.

#### 3. Empty Needle Behavior Undefined Across Multiple Functions (Principal, P0)

Nine functions accept a needle or delimiter parameter, and none had specified behavior for the empty-string case. Of particular concern: `ztr_replace_all(s, "", "x")` would enter an infinite loop in any naive implementation, since an empty needle matches at every position and replacing it does not advance past the match.

**Resolution**: Fully specified for all 9 affected functions. Key decisions:
- `ztr_count(s, "")` returns 0.
- `ztr_replace_first` / `ztr_replace_all` with empty needle is a no-op (returns `ZTR_OK`, string unmodified).
- `ztr_split_begin` with empty delimiter yields the entire string as a single token.
- `ztr_find(s, "")` returns 0 (found at position 0).
- `ztr_rfind(s, "")` returns `ztr_len(s)`.

#### 4. `ztr_rfind` Start Semantics Ambiguous (Principal, API/UX)

The `start` parameter of `ztr_rfind` was underspecified. It was unclear whether `start` means "begin searching leftward from this byte offset" or "only consider matches whose start position is at or before this offset."

**Resolution**: `start` is documented as the rightmost byte position to consider as a match start. The search proceeds rightward from position 0 to `start`, returning the last match found. `ZTR_NPOS` means search from the end (equivalent to `start = ztr_len(s) - needle_len`).

#### 5. `ztr_split_alloc` `max_parts=0` Ambiguous (API/UX)

The `max_parts` parameter of `ztr_split_alloc` had no documented behavior for the value 0. It could reasonably mean "no results" (literal zero) or "unlimited" (no cap).

**Resolution**: 0 means unlimited. This follows the convention established by Python's `str.split()` and Go's `strings.SplitN()`, where a non-positive limit means no limit.

#### 6. `ztr_data_mut` Pointer Invalidation (Security)

The pointer returned by `ztr_data_mut` is invalidated by any mutation that triggers an SSO-to-heap transition. In SSO mode, the pointer points into the struct's inline `sso` array. After a transition to heap mode, that same memory region is reinterpreted as heap metadata (`data` pointer and `cap`), meaning the held pointer now aliases struct internals. Writing through it corrupts the string's metadata.

**Resolution**: Expanded documentation with an explicit warning about SSO-to-heap invalidation. The contract is stated as: the pointer returned by `ztr_data_mut` is valid until the next call to any `ztr_` function on the same string (other than `ztr_len`, `ztr_cstr`, `ztr_cap`, and `ztr_is_empty`, which are read-only).

#### 7. Split Iterator Lifetime Contract (Embedded, Principal)

The `ztr_split_iter` API stores a pointer to the source `ztr` internally. If the source string is modified or freed during iteration, the iterator holds a dangling or stale pointer. This is particularly dangerous because the iterator is a stack-allocated struct with no ownership — the failure mode is silent data corruption.

**Resolution**: Explicit documentation that the source `ztr` must not be modified or freed during iteration. A `_Static_assert` on the iterator struct size ensures ABI stability and catches accidental layout changes during development.

#### 8. Security Notes Section Added

A new "Security Notes" section was added to the spec covering three topics:
- **Memory zeroing**: `ztr_free` does NOT zero memory before freeing. Applications handling secrets should use `explicit_bzero` or `SecureZeroMemory` on the buffer (obtained via `ztr_data_mut`) before calling `ztr_free`.
- **Debug invariant checks**: Debug builds (`NDEBUG` not defined) assert that the null terminator is present at `data[len]` on entry to every function. This catches bugs where `ztr_data_mut` / `ztr_set_len` usage violates the null-terminator invariant.
- **Discriminator safety**: Internal helpers enforce correct high-bit state. The `ztr_p_set_heap_bit` and `ztr_p_clear_heap_bit` helpers are the only code paths that modify the discriminator, centralizing this safety-critical operation.

#### 9. `ZTR_MAX_LEN` Constant Added

The new constant `ZTR_MAX_LEN = SIZE_MAX >> 1` was added with a `_Static_assert(sizeof(size_t) >= 2)` guard. The static assert protects against exotic platforms where `size_t` is a single byte (maximum representable length would be 127 bytes — likely too small to be useful, and worth failing loudly at compile time rather than silently producing a broken library).

---

## Embedded Complexity Assessment

A separate assessment was conducted on the complexity cost of supporting embedded targets in the v1 library. The question: how much harder does the implementation become when the four v1 embedded accommodations are included?

**If building a general-purpose-only library is difficulty 1.0x, adding the v1 embedded accommodations is difficulty 1.15x.**

The four v1 embedded items — custom allocator macros, configurable growth factor, `ZTR_NO_FMT` gate, and `ztr_split_iter` — are all additive, isolated changes that do not interact with each other or complicate the core data structure. Custom allocator macros are a search-and-replace of `malloc`/`free`/`realloc` with macro calls. The growth factor is a single arithmetic expression change. `ZTR_NO_FMT` wraps two functions in an `#ifndef` guard. The split iterator is a self-contained addition with no impact on existing code.

Full embedded support — `ZTR_NO_ALLOC` mode, fixed-buffer type, full feature gates — would be 1.8–2.0x difficulty and is deferred to v2. These features require pervasive changes: `ZTR_NO_ALLOC` alters the return contract of every allocating function, the fixed-buffer type introduces a second data structure with its own API surface, and full feature gates create a combinatorial testing matrix.

The v1 embedded items move the embedded confidence score from 4/10 to approximately 6.5–7/10, covering Cortex-M4+ with RTOS — the embedded segment most likely to adopt a third-party string library. Bare-metal Cortex-M0 and hard-real-time systems without a heap remain out of scope until v2.

---

## Final Struct Layout Decision

After extensive debate across all three rounds, the panel adopted the **high-bit-of-len discriminator** layout. This final design was discovered during the spec writing process following Round 2, when analysis revealed it to be strictly superior to the byte-0-tag approach originally adopted as the Round 2 endianness fix.

```c
typedef struct ztr {
    size_t len;   // High bit: 1=heap, 0=SSO. Remaining bits: byte length.
    union {
        struct { char *data; size_t cap; } heap;
        char sso[sizeof(char*) + sizeof(size_t)];
    };
} ztr;  // 24 bytes on 64-bit, 12 on 32-bit
```

The high-bit-of-len approach is strictly better than the byte-0-tag approach for the following reasons:

1. **No pointer assumptions.** The byte-0 discriminator relied on user-space virtual addresses having bit 7 clear — a property that holds on current mainstream architectures but is not guaranteed by any standard. The high-bit-of-len approach makes no assumptions about pointer values whatsoever.

2. **No endianness concerns.** The discriminator is the high bit of a `size_t` field, accessed as a `size_t` value. There is no byte-level layout dependence. The same code works identically on little-endian and big-endian architectures without any conditional compilation.

3. **More SSO capacity.** The byte-0-tag approach reserved one byte of the struct for the tag, leaving 14 bytes of SSO capacity on 64-bit (after accounting for the null terminator). The high-bit-of-len approach reclaims that byte, yielding 15 bytes of SSO capacity — one additional byte, which covers common cases like 15-character identifiers.

4. **Same branchless `ztr_len`.** Length extraction is a single bitwise operation: `return s->len & ~ZTR_HEAP_BIT;`. This is the same cost as the byte-0 approach — one load, one AND — with zero branches.

5. **Zero-init remains valid.** `len = 0` has the high bit clear, meaning SSO mode. `sso[0] = 0` provides the null terminator. A zero-initialized `ztr` is a valid empty string, preserving the unanimous Round 1 decision.

6. **`ZTR_MAX_LEN` is architecturally natural.** The maximum representable length is `SIZE_MAX >> 1`, which is 2^63 - 1 on 64-bit — far beyond any practical string length. The Round 3 fix to `ztr_set_len` (rejecting lengths with the high bit set) follows directly from this layout.

The Performance Engineer's original argument from Round 2 — that `ztr_len` must be branch-free — remains the foundational motivation for placing `len` outside the union. The high-bit-of-len discriminator preserves this property while eliminating every portability concern raised by the byte-0-tag alternative.

Trade-off accepted: 15 bytes of SSO capacity covers single-word identifiers, short dictionary keys, status codes, country codes, HTTP methods (GET, POST, PUT, DELETE), and similar short strings. Strings of 16+ bytes spill to the heap (a one-time allocation cost), but every subsequent access to those strings is faster due to the branch-free accessor.

---

*This document is the historical record of the design process. For the authoritative specification, see SPEC.md.*
