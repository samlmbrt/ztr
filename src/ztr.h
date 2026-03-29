/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Samuel Lambert */

#ifndef ZTR_H
#define ZTR_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define ZTR_PRINTF_FMT(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define ZTR_PRINTF_FMT(fmt_idx, arg_idx)
#endif

/* --- Compile-time configuration --- */

#ifndef ZTR_MALLOC
#define ZTR_MALLOC(size) malloc(size)
#endif
#ifndef ZTR_REALLOC
#define ZTR_REALLOC(ptr, size) realloc(ptr, size)
#endif
#ifndef ZTR_FREE
#define ZTR_FREE(ptr) free(ptr)
#endif

#ifndef ZTR_GROWTH_NUM
#define ZTR_GROWTH_NUM 3
#endif
#ifndef ZTR_GROWTH_DEN
#define ZTR_GROWTH_DEN 2
#endif

#ifndef ZTR_MIN_HEAP_CAP
#define ZTR_MIN_HEAP_CAP 64
#endif

/* --- Constants --- */

#define ZTR_HEAP_BIT ((size_t)1 << (sizeof(size_t) * CHAR_BIT - 1))
#define ZTR_MAX_LEN (SIZE_MAX >> 1)
#define ZTR_SSO_CAP (sizeof(char *) + sizeof(size_t) - 1)
#define ZTR_NPOS ((size_t)-1)

/* Printf helpers for ztr (mirrors ZTR_VIEW_FMT/ZTR_VIEW_ARG below). */
#define ZTR_FMT "%.*s"
#define ZTR_ARG(s) (int)ztr_len(s), ztr_cstr(s)

/* --- Data structure --- */

typedef struct ztr {
    size_t len; /* High bit: 1 = heap, 0 = SSO. Remaining bits: byte length. */
    union {
        struct {
            char *data; /* Heap-allocated buffer (always null-terminated). */
            size_t cap; /* Capacity in bytes (excludes null terminator). */
        } heap;
        char sso[sizeof(char *) + sizeof(size_t)]; /* SSO inline buffer. */
    };
} ztr;

_Static_assert(sizeof(ztr) == sizeof(size_t) * 3, "unexpected ztr struct size");
_Static_assert(sizeof(size_t) >= 2, "ztr requires size_t of at least 16 bits");

/* --- Error handling --- */

typedef enum {
    ZTR_OK = 0,
    ZTR_ERR_ALLOC,
    ZTR_ERR_NULL_ARG,
    ZTR_ERR_OUT_OF_RANGE,
    ZTR_ERR_INVALID_UTF8,
    ZTR_ERR_OVERFLOW,
} ztr_err;

/* --- Internal helpers --- */

static inline bool ztr_p_is_heap(const ztr *s) { return (s->len & ZTR_HEAP_BIT) != 0; }

/* --- Accessors (static inline, infallible) --- */

static inline size_t ztr_len(const ztr *s) { return s->len & ~ZTR_HEAP_BIT; }

static inline const char *ztr_cstr(const ztr *s) {
    return ztr_p_is_heap(s) ? s->heap.data : s->sso;
}

static inline bool ztr_is_empty(const ztr *s) { return ztr_len(s) == 0; }

static inline size_t ztr_capacity(const ztr *s) {
    return ztr_p_is_heap(s) ? s->heap.cap : ZTR_SSO_CAP;
}

static inline char ztr_at(const ztr *s, size_t i) {
    return (i < ztr_len(s)) ? ztr_cstr(s)[i] : '\0';
}

/* --- Lifecycle --- */

void ztr_init(ztr *s);
ztr_err ztr_from(ztr *s, const char *cstr);
ztr_err ztr_from_buf(ztr *s, const char *buf, size_t len);
ztr_err ztr_with_cap(ztr *s, size_t cap);
ztr_err ztr_clone(ztr *dst, const ztr *src);
ztr_err ztr_assign(ztr *s, const char *cstr);
ztr_err ztr_assign_buf(ztr *s, const char *buf, size_t len);
void ztr_move(ztr *dst, ztr *src);
void ztr_free(ztr *s);

#ifndef ZTR_NO_FMT
ztr_err ztr_fmt(ztr *s, const char *fmt, ...) ZTR_PRINTF_FMT(2, 3);
#endif

/* --- Comparison --- */

bool ztr_eq(const ztr *a, const ztr *b);
bool ztr_eq_cstr(const ztr *s, const char *cstr);
int ztr_cmp(const ztr *a, const ztr *b);
int ztr_cmp_cstr(const ztr *s, const char *cstr);
bool ztr_eq_ascii_nocase(const ztr *a, const ztr *b);

/* --- Search --- */

size_t ztr_find(const ztr *s, const char *needle, size_t start);
size_t ztr_rfind(const ztr *s, const char *needle, size_t start);
bool ztr_contains(const ztr *s, const char *needle);
bool ztr_starts_with(const ztr *s, const char *prefix);
bool ztr_ends_with(const ztr *s, const char *suffix);
size_t ztr_count(const ztr *s, const char *needle);

/* --- Mutation --- */

ztr_err ztr_append(ztr *s, const char *cstr);
ztr_err ztr_append_buf(ztr *s, const char *buf, size_t len);
ztr_err ztr_append_ztr(ztr *s, const ztr *other);

/* Outlined slow path for append_byte when growth is needed. */
ztr_err ztr_p_append_byte_slow(ztr *s, char c);

/* Append a single byte. Inlined fast path: if capacity is sufficient,
   write the byte directly and update the length. No self-ref check needed
   (the address of a local char can never alias the internal buffer). */
static inline ztr_err ztr_append_byte(ztr *s, char c) {
    size_t len = ztr_len(s);
    if (len < ztr_capacity(s)) {
        char *buf = ztr_p_is_heap(s) ? s->heap.data : s->sso;
        buf[len] = c;
        buf[len + 1] = '\0';
        s->len = (s->len & ZTR_HEAP_BIT) | (len + 1);
        return ZTR_OK;
    }
    return ztr_p_append_byte_slow(s, c);
}
ztr_err ztr_insert(ztr *s, size_t pos, const char *cstr);
ztr_err ztr_insert_buf(ztr *s, size_t pos, const char *buf, size_t len);
void ztr_erase(ztr *s, size_t pos, size_t count);
ztr_err ztr_replace_first(ztr *s, const char *needle, const char *replacement);
ztr_err ztr_replace_all(ztr *s, const char *needle, const char *replacement);
void ztr_clear(ztr *s);
void ztr_truncate(ztr *s, size_t new_len);
ztr_err ztr_reserve(ztr *s, size_t cap);
void ztr_shrink_to_fit(ztr *s);

#ifndef ZTR_NO_FMT
ztr_err ztr_append_fmt(ztr *s, const char *fmt, ...) ZTR_PRINTF_FMT(2, 3);
#endif

/* --- Transformation (in-place) --- */

void ztr_trim(ztr *s);
void ztr_trim_start(ztr *s);
void ztr_trim_end(ztr *s);
void ztr_to_ascii_upper(ztr *s);
void ztr_to_ascii_lower(ztr *s);

/* --- Extraction --- */

ztr_err ztr_substr(ztr *out, const ztr *s, size_t pos, size_t count);

/* --- Split and join --- */

typedef struct {
    const char *ztr_p_src;
    size_t ztr_p_src_len;
    const char *ztr_p_delim;
    size_t ztr_p_delim_len;
    size_t ztr_p_pos;
    bool ztr_p_done;
} ztr_split_iter;

void ztr_split_begin(ztr_split_iter *it, const ztr *s, const char *delim);
bool ztr_split_next(ztr_split_iter *it, const char **part, size_t *part_len);

ztr_err ztr_split_alloc(const ztr *s, const char *delim, ztr **parts, size_t *count,
                        size_t max_parts);
void ztr_split_free(ztr *parts, size_t count);

ztr_err ztr_join(ztr *out, const ztr *parts, size_t count, const char *sep);
ztr_err ztr_join_cstr(ztr *out, const char *const *parts, size_t count, const char *sep);

/* --- UTF-8 --- */

bool ztr_is_valid_utf8(const ztr *s);
ztr_err ztr_utf8_len(size_t *out, const ztr *s);
ztr_err ztr_utf8_next(const ztr *s, size_t *pos, uint32_t *cp);
ztr_err ztr_utf8_append(ztr *s, uint32_t codepoint);

/* --- Interop --- */

static inline char *ztr_data_mut(ztr *s) { return ztr_p_is_heap(s) ? s->heap.data : s->sso; }

ztr_err ztr_set_len(ztr *s, size_t len);
ztr_err ztr_detach(ztr *s, char **out);
void ztr_swap(ztr *a, ztr *b);

/* --- Utility --- */

bool ztr_is_ascii(const ztr *s);
const char *ztr_err_str(ztr_err err);

/* ========================================================================
 * String View — non-owning, read-only reference to a contiguous byte range
 *
 * A ztr_view does NOT own its data. The caller must ensure the underlying
 * buffer outlives the view and is not mutated while the view is in use.
 * Mutating or freeing the source (ztr, buffer, etc.) invalidates all
 * views derived from it.
 * ======================================================================== */

typedef struct ztr_view {
    const char *data;
    size_t len;
} ztr_view;

_Static_assert(sizeof(ztr_view) == sizeof(size_t) * 2, "unexpected ztr_view struct size");

/* Canonical empty view. data is always non-NULL and dereferenceable. */
#ifdef __cplusplus
#define ZTR_VIEW_EMPTY (ztr_view{"", 0})
#else
#define ZTR_VIEW_EMPTY ((ztr_view){"", 0})
#endif

/* Compile-time view from a string literal. No strlen call.
   WARNING: Only use with string literals, not char* variables. On GCC/Clang,
   passing a non-array argument is a compile-time error. */
#if defined(__GNUC__) || defined(__clang__)
#define ZTR_VIEW_LIT(s)                                                                            \
    (__extension__({                                                                                \
        _Static_assert(__builtin_types_compatible_p(__typeof__(s), char[sizeof(s)]),                 \
                       "ZTR_VIEW_LIT requires a string literal");                                   \
        (ztr_view){(s), sizeof(s) - 1};                                                            \
    }))
#else
#define ZTR_VIEW_LIT(s) ((ztr_view){(s), sizeof(s) - 1})
#endif

/* Printf helpers: printf("val=" ZTR_VIEW_FMT "\n", ZTR_VIEW_ARG(v)); */
#define ZTR_VIEW_FMT "%.*s"
#define ZTR_VIEW_ARG(v) (int)(v).len, (v).data

/* --- Construction (static inline, infallible) --- */

static inline ztr_view ztr_view_from_buf(const char *buf, size_t len) {
    if (!buf) return ZTR_VIEW_EMPTY;
    ztr_view v;
    v.data = buf;
    v.len = len;
    return v;
}

static inline ztr_view ztr_view_from_cstr(const char *cstr) {
    if (!cstr) return ZTR_VIEW_EMPTY;
    ztr_view v;
    v.data = cstr;
    v.len = strlen(cstr);
    return v;
}

/* Borrow from an existing ztr. The ztr must not be mutated or freed
   while the view is in use — any mutation may reallocate, invalidating
   the view's data pointer. */
static inline ztr_view ztr_view_from_ztr(const ztr *s) {
    if (!s) return ZTR_VIEW_EMPTY;
    ztr_view v;
    v.data = ztr_cstr(s);
    v.len = ztr_len(s);
    return v;
}

/* Extract a sub-view. Clamped: pos >= len returns empty, count clamped to
   available bytes. No allocation, no UB. */
static inline ztr_view ztr_view_substr(ztr_view v, size_t pos, size_t count) {
    if (pos >= v.len) return ZTR_VIEW_EMPTY;
    size_t avail = v.len - pos;
    if (count > avail) count = avail;
    ztr_view out;
    out.data = v.data + pos;
    out.len = count;
    return out;
}

/* --- Accessors (static inline, infallible) --- */

static inline size_t ztr_view_len(ztr_view v) { return v.len; }
static inline const char *ztr_view_data(ztr_view v) { return v.data; }
static inline bool ztr_view_is_empty(ztr_view v) { return v.len == 0; }

/* Returns '\0' if i >= len (matches ztr_at behavior). */
static inline char ztr_view_at(ztr_view v, size_t i) {
    return (i < v.len) ? v.data[i] : '\0';
}

/* --- Comparison --- */

bool ztr_view_eq(ztr_view a, ztr_view b);
bool ztr_view_eq_cstr(ztr_view v, const char *cstr);
int ztr_view_cmp(ztr_view a, ztr_view b);
int ztr_view_cmp_cstr(ztr_view v, const char *cstr);
bool ztr_view_eq_ascii_nocase(ztr_view a, ztr_view b);
bool ztr_view_eq_ascii_nocase_cstr(ztr_view v, const char *cstr);

/* --- Search (view needle) --- */

size_t ztr_view_find(ztr_view v, ztr_view needle, size_t start);
size_t ztr_view_rfind(ztr_view v, ztr_view needle, size_t start);
bool ztr_view_contains(ztr_view v, ztr_view needle);
bool ztr_view_starts_with(ztr_view v, ztr_view prefix);
bool ztr_view_ends_with(ztr_view v, ztr_view suffix);
size_t ztr_view_count(ztr_view v, ztr_view needle);

/* --- Search (C string needle — convenience for runtime const char*) --- */

size_t ztr_view_find_cstr(ztr_view v, const char *needle, size_t start);
size_t ztr_view_rfind_cstr(ztr_view v, const char *needle, size_t start);
bool ztr_view_contains_cstr(ztr_view v, const char *needle);
bool ztr_view_starts_with_cstr(ztr_view v, const char *prefix);
bool ztr_view_ends_with_cstr(ztr_view v, const char *suffix);
size_t ztr_view_count_cstr(ztr_view v, const char *needle);

/* --- Search (single character — memchr fast path) --- */

size_t ztr_view_find_char(ztr_view v, char c, size_t start);
size_t ztr_view_rfind_char(ztr_view v, char c, size_t start);
bool ztr_view_contains_char(ztr_view v, char c);

/* --- Trimming (returns narrowed view, no allocation) --- */

ztr_view ztr_view_trim(ztr_view v);
ztr_view ztr_view_trim_start(ztr_view v);
ztr_view ztr_view_trim_end(ztr_view v);

/* --- Slice manipulation (clamped) --- */

static inline ztr_view ztr_view_remove_prefix(ztr_view v, size_t n) {
    if (n >= v.len) return ZTR_VIEW_EMPTY;
    ztr_view out;
    out.data = v.data + n;
    out.len = v.len - n;
    return out;
}

static inline ztr_view ztr_view_remove_suffix(ztr_view v, size_t n) {
    if (n >= v.len) return ZTR_VIEW_EMPTY;
    ztr_view out;
    out.data = v.data;
    out.len = v.len - n;
    return out;
}

/* --- Split iterator --- */

typedef struct {
    ztr_view ztr_p_remaining;
    ztr_view ztr_p_delim;
    bool ztr_p_done;
} ztr_view_split_iter;

_Static_assert(sizeof(ztr_view_split_iter) <= 48, "ztr_view_split_iter size check");

void ztr_view_split_begin(ztr_view_split_iter *it, ztr_view s, ztr_view delim);
void ztr_view_split_begin_cstr(ztr_view_split_iter *it, ztr_view s, const char *delim);
bool ztr_view_split_next(ztr_view_split_iter *it, ztr_view *token);

/* --- Utility --- */

bool ztr_view_is_ascii(ztr_view v);
bool ztr_view_is_valid_utf8(ztr_view v);

/* --- Interop --- */

/* Copy view data into an owned ztr. Works on both uninitialized (zeroed) and
   already-initialized strings (frees existing content first). Safe when v
   references s's own buffer (self-referential). */
ztr_err ztr_from_view(ztr *s, ztr_view v);

#endif /* ZTR_H */
