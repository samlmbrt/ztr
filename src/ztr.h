#ifndef ZTR_H
#define ZTR_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
ztr_err ztr_append_byte(ztr *s, char c);
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

#endif /* ZTR_H */
