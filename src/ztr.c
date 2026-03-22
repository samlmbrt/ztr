#include "ztr.h"

#include <stdlib.h>
#include <string.h>

#ifndef ZTR_NO_FMT
#include <stdarg.h>
#include <stdio.h>
#endif

/* ---- Lifecycle ---- */

void ztr_init(ztr *s) { memset(s, 0, sizeof(*s)); }

ztr_err ztr_from(ztr *s, const char *cstr) {
    (void)s;
    (void)cstr;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_from_buf(ztr *s, const char *buf, size_t len) {
    (void)s;
    (void)buf;
    (void)len;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_with_cap(ztr *s, size_t cap) {
    (void)s;
    (void)cap;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_clone(ztr *dst, const ztr *src) {
    (void)dst;
    (void)src;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_assign(ztr *s, const char *cstr) {
    (void)s;
    (void)cstr;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_assign_buf(ztr *s, const char *buf, size_t len) {
    (void)s;
    (void)buf;
    (void)len;
    return ZTR_OK; /* TODO */
}

void ztr_move(ztr *dst, ztr *src) {
    (void)dst;
    (void)src;
    /* TODO */
}

void ztr_free(ztr *s) {
    if (!s) {
        return;
    }
    if (ztr_p_is_heap(s)) {
        ZTR_FREE(s->heap.data);
    }
    memset(s, 0, sizeof(*s));
}

#ifndef ZTR_NO_FMT
ztr_err ztr_fmt(ztr *s, const char *fmt, ...) {
    (void)s;
    (void)fmt;
    return ZTR_OK; /* TODO */
}
#endif

/* ---- Comparison ---- */

bool ztr_eq(const ztr *a, const ztr *b) {
    (void)a;
    (void)b;
    return false; /* TODO */
}

bool ztr_eq_cstr(const ztr *s, const char *cstr) {
    (void)s;
    (void)cstr;
    return false; /* TODO */
}

int ztr_cmp(const ztr *a, const ztr *b) {
    (void)a;
    (void)b;
    return 0; /* TODO */
}

int ztr_cmp_cstr(const ztr *s, const char *cstr) {
    (void)s;
    (void)cstr;
    return 0; /* TODO */
}

bool ztr_eq_ascii_nocase(const ztr *a, const ztr *b) {
    (void)a;
    (void)b;
    return false; /* TODO */
}

/* ---- Search ---- */

size_t ztr_find(const ztr *s, const char *needle, size_t start) {
    (void)s;
    (void)needle;
    (void)start;
    return ZTR_NPOS; /* TODO */
}

size_t ztr_rfind(const ztr *s, const char *needle, size_t start) {
    (void)s;
    (void)needle;
    (void)start;
    return ZTR_NPOS; /* TODO */
}

bool ztr_contains(const ztr *s, const char *needle) {
    (void)s;
    (void)needle;
    return false; /* TODO */
}

bool ztr_starts_with(const ztr *s, const char *prefix) {
    (void)s;
    (void)prefix;
    return false; /* TODO */
}

bool ztr_ends_with(const ztr *s, const char *suffix) {
    (void)s;
    (void)suffix;
    return false; /* TODO */
}

size_t ztr_count(const ztr *s, const char *needle) {
    (void)s;
    (void)needle;
    return 0; /* TODO */
}

/* ---- Mutation ---- */

ztr_err ztr_append(ztr *s, const char *cstr) {
    (void)s;
    (void)cstr;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_append_buf(ztr *s, const char *buf, size_t len) {
    (void)s;
    (void)buf;
    (void)len;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_append_ztr(ztr *s, const ztr *other) {
    (void)s;
    (void)other;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_append_byte(ztr *s, char c) {
    (void)s;
    (void)c;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_insert(ztr *s, size_t pos, const char *cstr) {
    (void)s;
    (void)pos;
    (void)cstr;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_insert_buf(ztr *s, size_t pos, const char *buf, size_t len) {
    (void)s;
    (void)pos;
    (void)buf;
    (void)len;
    return ZTR_OK; /* TODO */
}

void ztr_erase(ztr *s, size_t pos, size_t count) {
    (void)s;
    (void)pos;
    (void)count;
    /* TODO */
}

ztr_err ztr_replace_first(ztr *s, const char *needle, const char *replacement) {
    (void)s;
    (void)needle;
    (void)replacement;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_replace_all(ztr *s, const char *needle, const char *replacement) {
    (void)s;
    (void)needle;
    (void)replacement;
    return ZTR_OK; /* TODO */
}

void ztr_clear(ztr *s) {
    (void)s;
    /* TODO */
}

void ztr_truncate(ztr *s, size_t new_len) {
    (void)s;
    (void)new_len;
    /* TODO */
}

ztr_err ztr_reserve(ztr *s, size_t cap) {
    (void)s;
    (void)cap;
    return ZTR_OK; /* TODO */
}

void ztr_shrink_to_fit(ztr *s) {
    (void)s;
    /* TODO */
}

#ifndef ZTR_NO_FMT
ztr_err ztr_append_fmt(ztr *s, const char *fmt, ...) {
    (void)s;
    (void)fmt;
    return ZTR_OK; /* TODO */
}
#endif

/* ---- Transformation ---- */

void ztr_trim(ztr *s) {
    (void)s;
    /* TODO */
}

void ztr_trim_start(ztr *s) {
    (void)s;
    /* TODO */
}

void ztr_trim_end(ztr *s) {
    (void)s;
    /* TODO */
}

void ztr_to_ascii_upper(ztr *s) {
    (void)s;
    /* TODO */
}

void ztr_to_ascii_lower(ztr *s) {
    (void)s;
    /* TODO */
}

/* ---- Extraction ---- */

ztr_err ztr_substr(ztr *out, const ztr *s, size_t pos, size_t count) {
    (void)out;
    (void)s;
    (void)pos;
    (void)count;
    return ZTR_OK; /* TODO */
}

/* ---- Split and join ---- */

void ztr_split_begin(ztr_split_iter *it, const ztr *s, const char *delim) {
    (void)it;
    (void)s;
    (void)delim;
    /* TODO */
}

bool ztr_split_next(ztr_split_iter *it, const char **part, size_t *part_len) {
    (void)it;
    (void)part;
    (void)part_len;
    return false; /* TODO */
}

ztr_err ztr_split_alloc(const ztr *s, const char *delim, ztr **parts, size_t *count,
                        size_t max_parts) {
    (void)s;
    (void)delim;
    (void)parts;
    (void)count;
    (void)max_parts;
    return ZTR_OK; /* TODO */
}

void ztr_split_free(ztr *parts, size_t count) {
    (void)parts;
    (void)count;
    /* TODO */
}

ztr_err ztr_join(ztr *out, const ztr *parts, size_t count, const char *sep) {
    (void)out;
    (void)parts;
    (void)count;
    (void)sep;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_join_cstr(ztr *out, const char *const *parts, size_t count, const char *sep) {
    (void)out;
    (void)parts;
    (void)count;
    (void)sep;
    return ZTR_OK; /* TODO */
}

/* ---- UTF-8 ---- */

bool ztr_is_valid_utf8(const ztr *s) {
    (void)s;
    return true; /* TODO */
}

ztr_err ztr_utf8_len(size_t *out, const ztr *s) {
    (void)out;
    (void)s;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_utf8_next(const ztr *s, size_t *pos, uint32_t *cp) {
    (void)s;
    (void)pos;
    (void)cp;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_utf8_append(ztr *s, uint32_t codepoint) {
    (void)s;
    (void)codepoint;
    return ZTR_OK; /* TODO */
}

/* ---- Interop ---- */

ztr_err ztr_set_len(ztr *s, size_t len) {
    (void)s;
    (void)len;
    return ZTR_OK; /* TODO */
}

ztr_err ztr_detach(ztr *s, char **out) {
    (void)s;
    (void)out;
    return ZTR_OK; /* TODO */
}

void ztr_swap(ztr *a, ztr *b) {
    (void)a;
    (void)b;
    /* TODO */
}

/* ---- Utility ---- */

bool ztr_is_ascii(const ztr *s) {
    (void)s;
    return true; /* TODO */
}

const char *ztr_err_str(ztr_err err) {
    switch (err) {
    case ZTR_OK:
        return "ok";
    case ZTR_ERR_ALLOC:
        return "allocation failed";
    case ZTR_ERR_NULL_ARG:
        return "null argument";
    case ZTR_ERR_OUT_OF_RANGE:
        return "out of range";
    case ZTR_ERR_INVALID_UTF8:
        return "invalid UTF-8";
    case ZTR_ERR_OVERFLOW:
        return "size overflow";
    }
    return "unknown error";
}
