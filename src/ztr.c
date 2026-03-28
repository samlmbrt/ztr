/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Samuel Lambert */

#include "ztr.h"

#include <stdlib.h>
#include <string.h>

#ifndef ZTR_NO_FMT
#include <stdarg.h>
#include <stdio.h>
#endif

/* ---- Internal helpers ---- */

/* Portable memmem: find needle in haystack. Returns NULL if not found.
   Uses memchr to skip to candidate positions (SIMD-accelerated on most
   platforms), then memcmp to verify. This outperforms both naive byte-by-byte
   scanning and some system memmem implementations (e.g., Apple's). */
static const void *ztr_p_memmem(const void *haystack, size_t haystacklen, const void *needle,
                                size_t needlelen) {
    if (needlelen == 0) {
        return haystack;
    }
    if (needlelen > haystacklen) {
        return NULL;
    }
    if (needlelen == 1) {
        return memchr(haystack, *(const unsigned char *)needle, haystacklen);
    }

    const unsigned char *h = (const unsigned char *)haystack;
    const unsigned char *n = (const unsigned char *)needle;
    size_t last = haystacklen - needlelen;

    /* Use memchr to jump to the next occurrence of the first byte,
       then memcmp to check the full needle. This leverages the platform's
       SIMD-optimized memchr to skip large regions of non-matching bytes. */
    for (size_t i = 0; i <= last;) {
        const unsigned char *found = (const unsigned char *)memchr(h + i, n[0], last - i + 1);
        if (!found) {
            return NULL;
        }
        if (memcmp(found, n, needlelen) == 0) {
            return found;
        }
        i = (size_t)(found - h) + 1;
    }
    return NULL;
}

static inline void ztr_p_set_len_sso(ztr *s, size_t len) {
    s->len = len; /* High bit clear = SSO mode. */
}

static inline void ztr_p_set_len_heap(ztr *s, size_t len) { s->len = len | ZTR_HEAP_BIT; }

static inline char *ztr_p_buf(ztr *s) { return ztr_p_is_heap(s) ? s->heap.data : s->sso; }

/* Ensure capacity for at least new_cap bytes of content. May transition SSO → heap.
   On failure, s is unmodified. */
static ztr_err ztr_p_grow(ztr *s, size_t new_cap) {
    size_t cur_cap = ztr_capacity(s);
    if (new_cap <= cur_cap) {
        return ZTR_OK;
    }

    /* Overflow check: need new_cap + 1 for null terminator. */
    if (new_cap > ZTR_MAX_LEN) {
        return ZTR_ERR_OVERFLOW;
    }

    /* Apply growth factor. */
    size_t grown = cur_cap + cur_cap * (ZTR_GROWTH_NUM - ZTR_GROWTH_DEN) / ZTR_GROWTH_DEN;
    if (grown < new_cap) {
        grown = new_cap;
    }
    if (grown < ZTR_MIN_HEAP_CAP) {
        grown = ZTR_MIN_HEAP_CAP;
    }
    if (grown > ZTR_MAX_LEN) {
        grown = ZTR_MAX_LEN;
    }

    size_t alloc_size = grown + 1; /* +1 for null terminator */

    if (ztr_p_is_heap(s)) {
        char *new_data = (char *)ZTR_REALLOC(s->heap.data, alloc_size);
        if (!new_data) {
            return ZTR_ERR_ALLOC;
        }
        s->heap.data = new_data;
        s->heap.cap = grown;
    } else {
        /* SSO → heap transition. */
        char *new_data = (char *)ZTR_MALLOC(alloc_size);
        if (!new_data) {
            return ZTR_ERR_ALLOC;
        }
        size_t cur_len = ztr_len(s);
        memcpy(new_data, s->sso, cur_len + 1); /* Copy content + null terminator. */
        s->heap.data = new_data;
        s->heap.cap = grown;
        ztr_p_set_len_heap(s, cur_len);
    }

    return ZTR_OK;
}

static inline bool ztr_p_is_ascii_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

/* ---- Lifecycle ---- */

void ztr_init(ztr *s) { memset(s, 0, sizeof(*s)); }

ztr_err ztr_from(ztr *s, const char *cstr) {
    if (!cstr) {
        ztr_init(s);
        return ZTR_OK;
    }
    return ztr_from_buf(s, cstr, strlen(cstr));
}

ztr_err ztr_from_buf(ztr *s, const char *buf, size_t len) {
    if (len > ZTR_MAX_LEN) {
        return ZTR_ERR_OVERFLOW;
    }

    if (!buf) {
        if (len == 0) {
            ztr_init(s);
            return ZTR_OK;
        }
        return ZTR_ERR_NULL_ARG;
    }

    ztr_init(s);

    if (len <= ZTR_SSO_CAP) {
        memcpy(s->sso, buf, len);
        s->sso[len] = '\0';
        ztr_p_set_len_sso(s, len);
    } else {
        char *data = (char *)ZTR_MALLOC(len + 1);
        if (!data) {
            return ZTR_ERR_ALLOC;
        }
        memcpy(data, buf, len);
        data[len] = '\0';
        s->heap.data = data;
        s->heap.cap = len;
        ztr_p_set_len_heap(s, len);
    }

    return ZTR_OK;
}

ztr_err ztr_with_cap(ztr *s, size_t cap) {
    ztr_init(s);

    if (cap <= ZTR_SSO_CAP) {
        return ZTR_OK; /* SSO already provides enough capacity. */
    }

    if (cap > ZTR_MAX_LEN) {
        return ZTR_ERR_OVERFLOW;
    }

    char *data = (char *)ZTR_MALLOC(cap + 1);
    if (!data) {
        return ZTR_ERR_ALLOC;
    }
    data[0] = '\0';
    s->heap.data = data;
    s->heap.cap = cap;
    ztr_p_set_len_heap(s, 0);

    return ZTR_OK;
}

ztr_err ztr_clone(ztr *dst, const ztr *src) {
    if (dst == src) {
        return ZTR_OK;
    }
    return ztr_from_buf(dst, ztr_cstr(src), ztr_len(src));
}

ztr_err ztr_assign(ztr *s, const char *cstr) {
    if (!cstr) {
        ztr_free(s);
        return ZTR_OK;
    }
    return ztr_assign_buf(s, cstr, strlen(cstr));
}

ztr_err ztr_assign_buf(ztr *s, const char *buf, size_t len) {
    if (len > ZTR_MAX_LEN) {
        return ZTR_ERR_OVERFLOW;
    }

    if (!buf) {
        return len == 0 ? ZTR_OK : ZTR_ERR_NULL_ARG;
    }

    /* Handle self-referential input: buf might point into s's buffer. */
    const char *cur_buf = ztr_p_buf(s);
    size_t cur_cap = ztr_capacity(s);
    bool is_self = (buf >= cur_buf && buf < cur_buf + cur_cap + 1);

    if (is_self) {
        /* Make a temporary copy. */
        ztr tmp;
        ztr_err err = ztr_from_buf(&tmp, buf, len);
        if (err) {
            return err;
        }
        ztr_free(s);
        *s = tmp;
        return ZTR_OK;
    }

    /* If the current buffer has enough capacity, reuse it. */
    if (len <= cur_cap) {
        char *dst = ztr_p_buf(s);
        memmove(dst, buf, len);
        dst[len] = '\0';
        if (ztr_p_is_heap(s)) {
            ztr_p_set_len_heap(s, len);
        } else {
            ztr_p_set_len_sso(s, len);
        }
        return ZTR_OK;
    }

    /* Need more space — build new string then swap. */
    ztr tmp;
    ztr_err err = ztr_from_buf(&tmp, buf, len);
    if (err) {
        return err;
    }
    ztr_free(s);
    *s = tmp;
    return ZTR_OK;
}

void ztr_move(ztr *dst, ztr *src) {
    if (dst == src) {
        return;
    }
    ztr_free(dst);
    *dst = *src;
    ztr_init(src);
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
    va_list ap;

    /* First pass: measure. */
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (n < 0) {
        return ZTR_ERR_OVERFLOW;
    }

    size_t len = (size_t)n;
    ztr_init(s);

    if (len <= ZTR_SSO_CAP) {
        va_start(ap, fmt);
        vsnprintf(s->sso, len + 1, fmt, ap);
        va_end(ap);
        ztr_p_set_len_sso(s, len);
    } else {
        char *data = (char *)ZTR_MALLOC(len + 1);
        if (!data) {
            return ZTR_ERR_ALLOC;
        }
        va_start(ap, fmt);
        vsnprintf(data, len + 1, fmt, ap);
        va_end(ap);
        s->heap.data = data;
        s->heap.cap = len;
        ztr_p_set_len_heap(s, len);
    }

    return ZTR_OK;
}
#endif

/* ---- Comparison ---- */

bool ztr_eq(const ztr *a, const ztr *b) {
    size_t alen = ztr_len(a);
    size_t blen = ztr_len(b);
    if (alen != blen) {
        return false;
    }
    return memcmp(ztr_cstr(a), ztr_cstr(b), alen) == 0;
}

bool ztr_eq_cstr(const ztr *s, const char *cstr) {
    if (!cstr) {
        return ztr_is_empty(s);
    }
    size_t slen = ztr_len(s);
    size_t clen = strlen(cstr);
    if (slen != clen) {
        return false;
    }
    return memcmp(ztr_cstr(s), cstr, slen) == 0;
}

int ztr_cmp(const ztr *a, const ztr *b) {
    size_t alen = ztr_len(a);
    size_t blen = ztr_len(b);
    size_t min_len = alen < blen ? alen : blen;
    int r = memcmp(ztr_cstr(a), ztr_cstr(b), min_len);
    if (r != 0) {
        return r;
    }
    if (alen < blen) {
        return -1;
    }
    if (alen > blen) {
        return 1;
    }
    return 0;
}

int ztr_cmp_cstr(const ztr *s, const char *cstr) {
    if (!cstr) {
        return ztr_is_empty(s) ? 0 : 1;
    }
    const char *sdata = ztr_cstr(s);
    size_t slen = ztr_len(s);
    size_t clen = strlen(cstr);
    size_t min_len = slen < clen ? slen : clen;
    int r = memcmp(sdata, cstr, min_len);
    if (r != 0) {
        return r;
    }
    if (slen < clen) {
        return -1;
    }
    if (slen > clen) {
        return 1;
    }
    return 0;
}

bool ztr_eq_ascii_nocase(const ztr *a, const ztr *b) {
    size_t alen = ztr_len(a);
    size_t blen = ztr_len(b);
    if (alen != blen) {
        return false;
    }
    const char *ad = ztr_cstr(a);
    const char *bd = ztr_cstr(b);
    for (size_t i = 0; i < alen; i++) {
        unsigned char ac = (unsigned char)ad[i];
        unsigned char bc = (unsigned char)bd[i];
        if (ac >= 'A' && ac <= 'Z') {
            ac = ac - 'A' + 'a';
        }
        if (bc >= 'A' && bc <= 'Z') {
            bc = bc - 'A' + 'a';
        }
        if (ac != bc) {
            return false;
        }
    }
    return true;
}

/* ---- Search ---- */

size_t ztr_find(const ztr *s, const char *needle, size_t start) {
    size_t slen = ztr_len(s);
    size_t nlen = strlen(needle);

    if (nlen == 0) {
        return start <= slen ? start : ZTR_NPOS;
    }
    if (start > slen || nlen > slen - start) {
        return ZTR_NPOS;
    }

    const char *haystack = ztr_cstr(s);
    const char *found = (const char *)ztr_p_memmem(haystack + start, slen - start, needle, nlen);
    if (!found) {
        return ZTR_NPOS;
    }
    return (size_t)(found - haystack);
}

size_t ztr_rfind(const ztr *s, const char *needle, size_t start) {
    size_t slen = ztr_len(s);
    size_t nlen = strlen(needle);

    if (nlen == 0) {
        return (start == ZTR_NPOS || start > slen) ? slen : start;
    }
    if (nlen > slen) {
        return ZTR_NPOS;
    }

    const char *haystack = ztr_cstr(s);
    size_t last_possible = slen - nlen;
    if (start != ZTR_NPOS && start < last_possible) {
        last_possible = start;
    }

    /* Single-byte needle: scan backwards with direct byte comparison. */
    if (nlen == 1) {
        unsigned char target = (unsigned char)needle[0];
        for (size_t i = last_possible + 1; i > 0; i--) {
            if ((unsigned char)haystack[i - 1] == target) {
                return i - 1;
            }
        }
        return ZTR_NPOS;
    }

    /* Multi-byte: filter on first byte before calling memcmp. */
    unsigned char first = (unsigned char)needle[0];
    for (size_t i = last_possible + 1; i > 0; i--) {
        if ((unsigned char)haystack[i - 1] == first &&
            memcmp(haystack + i - 1, needle, nlen) == 0) {
            return i - 1;
        }
    }
    return ZTR_NPOS;
}

bool ztr_contains(const ztr *s, const char *needle) { return ztr_find(s, needle, 0) != ZTR_NPOS; }

bool ztr_starts_with(const ztr *s, const char *prefix) {
    size_t plen = strlen(prefix);
    if (plen == 0) {
        return true;
    }
    size_t slen = ztr_len(s);
    if (plen > slen) {
        return false;
    }
    return memcmp(ztr_cstr(s), prefix, plen) == 0;
}

bool ztr_ends_with(const ztr *s, const char *suffix) {
    size_t xlen = strlen(suffix);
    if (xlen == 0) {
        return true;
    }
    size_t slen = ztr_len(s);
    if (xlen > slen) {
        return false;
    }
    return memcmp(ztr_cstr(s) + slen - xlen, suffix, xlen) == 0;
}

size_t ztr_count(const ztr *s, const char *needle) {
    size_t nlen = strlen(needle);
    if (nlen == 0) {
        return 0; /* Spec: empty needle returns 0. */
    }

    size_t total = 0;
    size_t pos = 0;
    while (pos <= ztr_len(s)) {
        size_t found = ztr_find(s, needle, pos);
        if (found == ZTR_NPOS) {
            break;
        }
        total++;
        pos = found + nlen;
    }
    return total;
}

/* ---- Mutation ---- */

ztr_err ztr_append_buf(ztr *s, const char *buf, size_t len) {
    if (len == 0) {
        return ZTR_OK;
    }

    if (!buf) {
        return len == 0 ? ZTR_OK : ZTR_ERR_NULL_ARG;
    }

    size_t cur_len = ztr_len(s);

    /* Overflow check. */
    if (len > ZTR_MAX_LEN - cur_len) {
        return ZTR_ERR_OVERFLOW;
    }

    size_t new_len = cur_len + len;

    /* Handle self-referential input: buf might point into our buffer.
       If grow triggers realloc, buf would dangle. Record the offset instead. */
    const char *cur_data = ztr_p_buf(s);
    bool self_ref = (buf >= cur_data && buf < cur_data + ztr_capacity(s) + 1);
    size_t self_offset = self_ref ? (size_t)(buf - cur_data) : 0;

    ztr_err err = ztr_p_grow(s, new_len);
    if (err) {
        return err;
    }

    char *dst = ztr_p_buf(s);
    const char *src = self_ref ? dst + self_offset : buf;
    memmove(dst + cur_len, src, len);
    dst[new_len] = '\0';

    if (ztr_p_is_heap(s)) {
        ztr_p_set_len_heap(s, new_len);
    } else {
        ztr_p_set_len_sso(s, new_len);
    }

    return ZTR_OK;
}

ztr_err ztr_append(ztr *s, const char *cstr) {
    if (!cstr) {
        return ZTR_OK;
    }
    return ztr_append_buf(s, cstr, strlen(cstr));
}

ztr_err ztr_append_ztr(ztr *s, const ztr *other) {
    return ztr_append_buf(s, ztr_cstr(other), ztr_len(other));
}

ztr_err ztr_p_append_byte_slow(ztr *s, char c) { return ztr_append_buf(s, &c, 1); }

ztr_err ztr_insert_buf(ztr *s, size_t pos, const char *buf, size_t len) {
    if (len == 0) {
        return ZTR_OK;
    }

    if (!buf) {
        return len == 0 ? ZTR_OK : ZTR_ERR_NULL_ARG;
    }

    size_t cur_len = ztr_len(s);
    if (pos > cur_len) {
        return ZTR_ERR_OUT_OF_RANGE;
    }

    if (len > ZTR_MAX_LEN - cur_len) {
        return ZTR_ERR_OVERFLOW;
    }

    size_t new_len = cur_len + len;

    /* Handle self-referential input: record offset before potential realloc. */
    const char *cur_data = ztr_p_buf(s);
    bool self_ref = (buf >= cur_data && buf < cur_data + ztr_capacity(s) + 1);
    size_t self_offset = self_ref ? (size_t)(buf - cur_data) : 0;

    ztr_err err = ztr_p_grow(s, new_len);
    if (err) {
        return err;
    }

    char *dst = ztr_p_buf(s);
    /* Shift tail right to make room. */
    memmove(dst + pos + len, dst + pos, cur_len - pos + 1); /* +1 for null terminator */
    /* Source may have shifted due to the memmove above if self-referential. */
    const char *src = self_ref ? dst + self_offset : buf;
    /* If src is in the shifted region (at or after pos), it moved right by len. */
    if (self_ref && self_offset >= pos) {
        src += len;
    }
    memmove(dst + pos, src, len);

    if (ztr_p_is_heap(s)) {
        ztr_p_set_len_heap(s, new_len);
    } else {
        ztr_p_set_len_sso(s, new_len);
    }

    return ZTR_OK;
}

ztr_err ztr_insert(ztr *s, size_t pos, const char *cstr) {
    if (!cstr) {
        return ZTR_OK;
    }
    return ztr_insert_buf(s, pos, cstr, strlen(cstr));
}

void ztr_erase(ztr *s, size_t pos, size_t count) {
    size_t cur_len = ztr_len(s);
    if (pos >= cur_len) {
        return;
    }

    /* Clamp count. */
    if (count > cur_len - pos) {
        count = cur_len - pos;
    }

    char *buf = ztr_p_buf(s);
    size_t tail = cur_len - pos - count;
    memmove(buf + pos, buf + pos + count, tail + 1); /* +1 for null terminator */

    size_t new_len = cur_len - count;
    if (ztr_p_is_heap(s)) {
        ztr_p_set_len_heap(s, new_len);
    } else {
        ztr_p_set_len_sso(s, new_len);
    }
}

ztr_err ztr_replace_first(ztr *s, const char *needle, const char *replacement) {
    size_t nlen = strlen(needle);
    if (nlen == 0) {
        return ZTR_OK; /* Empty needle: no-op. */
    }

    size_t pos = ztr_find(s, needle, 0);
    if (pos == ZTR_NPOS) {
        return ZTR_OK;
    }

    size_t rlen = strlen(replacement);
    size_t cur_len = ztr_len(s);

    /* Snapshot replacement if it points into our buffer (may be invalidated by grow). */
    const char *cur_data = ztr_p_buf(s);
    char *rep_copy = NULL;
    if (replacement >= cur_data && replacement < cur_data + ztr_capacity(s) + 1) {
        rep_copy = (char *)ZTR_MALLOC(rlen + 1);
        if (!rep_copy) {
            return ZTR_ERR_ALLOC;
        }
        memcpy(rep_copy, replacement, rlen + 1);
        replacement = rep_copy;
    }

    if (rlen <= nlen) {
        /* Replacement fits in place — shrink or same size. */
        char *buf = ztr_p_buf(s);
        memcpy(buf + pos, replacement, rlen);
        if (rlen < nlen) {
            size_t tail = cur_len - pos - nlen;
            memmove(buf + pos + rlen, buf + pos + nlen, tail + 1);
            size_t new_len = cur_len - (nlen - rlen);
            if (ztr_p_is_heap(s)) {
                ztr_p_set_len_heap(s, new_len);
            } else {
                ztr_p_set_len_sso(s, new_len);
            }
        }
        ZTR_FREE(rep_copy);
        return ZTR_OK;
    }

    /* Replacement is longer — need to grow. */
    size_t growth = rlen - nlen; /* safe: rlen > nlen guaranteed by the if branch above */
    if (growth > ZTR_MAX_LEN - cur_len) {
        ZTR_FREE(rep_copy);
        return ZTR_ERR_OVERFLOW;
    }
    size_t new_len = cur_len + growth;

    ztr_err err = ztr_p_grow(s, new_len);
    if (err) {
        ZTR_FREE(rep_copy);
        return err;
    }

    char *buf = ztr_p_buf(s);
    size_t tail = cur_len - pos - nlen;
    memmove(buf + pos + rlen, buf + pos + nlen, tail + 1);
    memcpy(buf + pos, replacement, rlen);

    if (ztr_p_is_heap(s)) {
        ztr_p_set_len_heap(s, new_len);
    } else {
        ztr_p_set_len_sso(s, new_len);
    }

    ZTR_FREE(rep_copy);
    return ZTR_OK;
}

ztr_err ztr_replace_all(ztr *s, const char *needle, const char *replacement) {
    size_t nlen = strlen(needle);
    if (nlen == 0) {
        return ZTR_OK; /* Empty needle: no-op. */
    }

    size_t rlen = strlen(replacement);
    size_t cur_len = ztr_len(s);
    const char *src = ztr_cstr(s);

    /* Two-pass: first count occurrences to compute final size. */
    size_t occ_count = 0;
    size_t scan = 0;
    while (scan + nlen <= cur_len) {
        const char *found = (const char *)ztr_p_memmem(src + scan, cur_len - scan, needle, nlen);
        if (!found) {
            break;
        }
        occ_count++;
        scan = (size_t)(found - src) + nlen;
    }

    if (occ_count == 0) {
        return ZTR_OK;
    }

    /* Compute new length with overflow checks. */
    size_t new_len;
    if (rlen >= nlen) {
        size_t growth = rlen - nlen;
        if (growth > 0 && occ_count > (ZTR_MAX_LEN - cur_len) / growth) {
            return ZTR_ERR_OVERFLOW;
        }
        new_len = cur_len + occ_count * growth;
    } else {
        size_t shrink = nlen - rlen;
        new_len = cur_len - occ_count * shrink;
    }

    /* Build new content in a temporary buffer. */
    char *new_data = (char *)ZTR_MALLOC(new_len + 1);
    if (!new_data) {
        return ZTR_ERR_ALLOC;
    }

    size_t read_pos = 0;
    size_t write_pos = 0;
    while (read_pos + nlen <= cur_len) {
        const char *found =
            (const char *)ztr_p_memmem(src + read_pos, cur_len - read_pos, needle, nlen);
        if (!found) {
            break;
        }
        size_t chunk = (size_t)(found - (src + read_pos));
        memcpy(new_data + write_pos, src + read_pos, chunk);
        write_pos += chunk;
        memcpy(new_data + write_pos, replacement, rlen);
        write_pos += rlen;
        read_pos += chunk + nlen;
    }
    /* Copy remaining tail. */
    size_t tail = cur_len - read_pos;
    memcpy(new_data + write_pos, src + read_pos, tail);
    write_pos += tail;
    new_data[write_pos] = '\0';

    /* Swap in the new buffer. */
    if (ztr_p_is_heap(s)) {
        ZTR_FREE(s->heap.data);
    }

    if (new_len <= ZTR_SSO_CAP) {
        memcpy(s->sso, new_data, new_len + 1);
        ztr_p_set_len_sso(s, new_len);
        ZTR_FREE(new_data);
    } else {
        s->heap.data = new_data;
        s->heap.cap = new_len;
        ztr_p_set_len_heap(s, new_len);
    }

    return ZTR_OK;
}

void ztr_clear(ztr *s) {
    char *buf = ztr_p_buf(s);
    buf[0] = '\0';
    if (ztr_p_is_heap(s)) {
        ztr_p_set_len_heap(s, 0);
    } else {
        ztr_p_set_len_sso(s, 0);
    }
}

void ztr_truncate(ztr *s, size_t new_len) {
    size_t cur_len = ztr_len(s);
    if (new_len >= cur_len) {
        return;
    }
    char *buf = ztr_p_buf(s);
    buf[new_len] = '\0';
    if (ztr_p_is_heap(s)) {
        ztr_p_set_len_heap(s, new_len);
    } else {
        ztr_p_set_len_sso(s, new_len);
    }
}

ztr_err ztr_reserve(ztr *s, size_t cap) { return ztr_p_grow(s, cap); }

void ztr_shrink_to_fit(ztr *s) {
    if (!ztr_p_is_heap(s)) {
        return;
    }

    size_t cur_len = ztr_len(s);
    size_t cur_cap = s->heap.cap;

    /* Don't bother if slack is small. */
    if (cur_cap - cur_len < 16) {
        return;
    }

    /* Transition back to SSO if possible. */
    if (cur_len <= ZTR_SSO_CAP) {
        char *old_data = s->heap.data;
        memcpy(s->sso, old_data, cur_len + 1);
        ztr_p_set_len_sso(s, cur_len);
        ZTR_FREE(old_data);
        return;
    }

    /* Try to shrink the heap allocation. */
    char *new_data = (char *)ZTR_REALLOC(s->heap.data, cur_len + 1);
    if (new_data) {
        s->heap.data = new_data;
        s->heap.cap = cur_len;
    }
    /* If realloc fails, keep the old (larger) buffer — that's fine. */
}

#ifndef ZTR_NO_FMT
ztr_err ztr_append_fmt(ztr *s, const char *fmt, ...) {
    va_list ap;

    /* Measure. */
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (n < 0) {
        return ZTR_ERR_OVERFLOW;
    }
    if (n == 0) {
        return ZTR_OK;
    }

    size_t add_len = (size_t)n;
    size_t cur_len = ztr_len(s);

    if (add_len > ZTR_MAX_LEN - cur_len) {
        return ZTR_ERR_OVERFLOW;
    }

    size_t new_len = cur_len + add_len;
    ztr_err err = ztr_p_grow(s, new_len);
    if (err) {
        return err;
    }

    char *dst = ztr_p_buf(s);
    va_start(ap, fmt);
    vsnprintf(dst + cur_len, add_len + 1, fmt, ap);
    va_end(ap);

    if (ztr_p_is_heap(s)) {
        ztr_p_set_len_heap(s, new_len);
    } else {
        ztr_p_set_len_sso(s, new_len);
    }

    return ZTR_OK;
}
#endif

/* ---- Transformation ---- */

void ztr_trim_start(ztr *s) {
    size_t len = ztr_len(s);
    const char *data = ztr_cstr(s);
    size_t i = 0;
    while (i < len && ztr_p_is_ascii_space(data[i])) {
        i++;
    }
    if (i > 0) {
        ztr_erase(s, 0, i);
    }
}

void ztr_trim_end(ztr *s) {
    size_t len = ztr_len(s);
    const char *data = ztr_cstr(s);
    while (len > 0 && ztr_p_is_ascii_space(data[len - 1])) {
        len--;
    }
    ztr_truncate(s, len);
}

void ztr_trim(ztr *s) {
    ztr_trim_end(s);
    ztr_trim_start(s);
}

void ztr_to_ascii_upper(ztr *s) {
    char *buf = ztr_p_buf(s);
    size_t len = ztr_len(s);
    for (size_t i = 0; i < len; i++) {
        if (buf[i] >= 'a' && buf[i] <= 'z') {
            buf[i] = buf[i] - 'a' + 'A';
        }
    }
}

void ztr_to_ascii_lower(ztr *s) {
    char *buf = ztr_p_buf(s);
    size_t len = ztr_len(s);
    for (size_t i = 0; i < len; i++) {
        if (buf[i] >= 'A' && buf[i] <= 'Z') {
            buf[i] = buf[i] - 'A' + 'a';
        }
    }
}

/* ---- Extraction ---- */

ztr_err ztr_substr(ztr *out, const ztr *s, size_t pos, size_t count) {
    if (out == (ztr *)s) {
        size_t slen = ztr_len(s);
        if (pos >= slen) {
            ztr_clear((ztr *)s);
            return ZTR_OK;
        }
        if (count > slen - pos) {
            count = slen - pos;
        }
        ztr_erase((ztr *)s, pos + count, slen - pos - count);
        ztr_erase((ztr *)s, 0, pos);
        return ZTR_OK;
    }

    size_t slen = ztr_len(s);
    if (pos >= slen) {
        ztr_init(out);
        return ZTR_OK;
    }
    if (count > slen - pos) {
        count = slen - pos;
    }
    return ztr_from_buf(out, ztr_cstr(s) + pos, count);
}

/* ---- Split and join ---- */

void ztr_split_begin(ztr_split_iter *it, const ztr *s, const char *delim) {
    it->ztr_p_src = ztr_cstr(s);
    it->ztr_p_src_len = ztr_len(s);
    it->ztr_p_delim = delim;
    it->ztr_p_delim_len = strlen(delim);
    it->ztr_p_pos = 0;
    it->ztr_p_done = false;
}

bool ztr_split_next(ztr_split_iter *it, const char **part, size_t *part_len) {
    if (it->ztr_p_done) {
        return false;
    }

    size_t dlen = it->ztr_p_delim_len;
    size_t remaining = it->ztr_p_src_len - it->ztr_p_pos;

    /* Empty delimiter or no delimiter: yield entire remainder. */
    if (dlen == 0) {
        *part = it->ztr_p_src + it->ztr_p_pos;
        *part_len = remaining;
        it->ztr_p_done = true;
        return true;
    }

    const char *haystack = it->ztr_p_src + it->ztr_p_pos;
    const char *found = (const char *)ztr_p_memmem(haystack, remaining, it->ztr_p_delim, dlen);

    if (!found) {
        /* No more delimiters — yield the rest. */
        *part = haystack;
        *part_len = remaining;
        it->ztr_p_done = true;
        return true;
    }

    *part = haystack;
    *part_len = (size_t)(found - haystack);
    it->ztr_p_pos = (size_t)(found - it->ztr_p_src) + dlen;

    /* If we just consumed to the end, the next call should yield an empty trailing part. */
    return true;
}

ztr_err ztr_split_alloc(const ztr *s, const char *delim, ztr **parts, size_t *count,
                        size_t max_parts) {
    /* First pass: count parts. */
    ztr_split_iter it;
    ztr_split_begin(&it, s, delim);
    const char *p;
    size_t plen;
    size_t n = 0;
    while (ztr_split_next(&it, &p, &plen)) {
        n++;
        if (max_parts > 0 && n >= max_parts) {
            /* Remaining goes into the last part. */
            break;
        }
    }

    if (n > SIZE_MAX / sizeof(ztr)) {
        return ZTR_ERR_OVERFLOW;
    }

    ztr *arr = (ztr *)ZTR_MALLOC(n * sizeof(ztr));
    if (!arr) {
        return ZTR_ERR_ALLOC;
    }

    /* Second pass: build parts. */
    ztr_split_begin(&it, s, delim);
    for (size_t i = 0; i < n; i++) {
        if (i == n - 1 && max_parts > 0 && n == max_parts) {
            /* Last part gets the entire remainder. */
            size_t rem_start = it.ztr_p_pos;
            size_t rem_len = it.ztr_p_src_len - rem_start;
            ztr_err err = ztr_from_buf(&arr[i], it.ztr_p_src + rem_start, rem_len);
            if (err) {
                /* Free already-built parts. */
                for (size_t j = 0; j < i; j++) {
                    ztr_free(&arr[j]);
                }
                ZTR_FREE(arr);
                return err;
            }
        } else {
            ztr_split_next(&it, &p, &plen);
            ztr_err err = ztr_from_buf(&arr[i], p, plen);
            if (err) {
                for (size_t j = 0; j < i; j++) {
                    ztr_free(&arr[j]);
                }
                ZTR_FREE(arr);
                return err;
            }
        }
    }

    *parts = arr;
    *count = n;
    return ZTR_OK;
}

void ztr_split_free(ztr *parts, size_t count) {
    if (!parts) {
        return;
    }
    for (size_t i = 0; i < count; i++) {
        ztr_free(&parts[i]);
    }
    ZTR_FREE(parts);
}

ztr_err ztr_join(ztr *out, const ztr *parts, size_t count, const char *sep) {
    ztr_init(out);
    if (count == 0) {
        return ZTR_OK;
    }

    size_t sep_len = sep ? strlen(sep) : 0;

    /* Compute total length. */
    size_t total = 0;
    for (size_t i = 0; i < count; i++) {
        size_t plen = ztr_len(&parts[i]);
        if (total > ZTR_MAX_LEN - plen) {
            return ZTR_ERR_OVERFLOW;
        }
        total += plen;
        if (i < count - 1 && sep_len > 0) {
            if (total > ZTR_MAX_LEN - sep_len) {
                return ZTR_ERR_OVERFLOW;
            }
            total += sep_len;
        }
    }

    ztr_err err = ztr_with_cap(out, total);
    if (err) {
        return err;
    }

    for (size_t i = 0; i < count; i++) {
        ztr_err err2 = ztr_append_buf(out, ztr_cstr(&parts[i]), ztr_len(&parts[i]));
        if (err2) {
            ztr_free(out);
            return err2;
        }
        if (i < count - 1 && sep_len > 0) {
            err2 = ztr_append_buf(out, sep, sep_len);
            if (err2) {
                ztr_free(out);
                return err2;
            }
        }
    }

    return ZTR_OK;
}

ztr_err ztr_join_cstr(ztr *out, const char *const *parts, size_t count, const char *sep) {
    ztr_init(out);
    if (count == 0) {
        return ZTR_OK;
    }

    size_t sep_len = sep ? strlen(sep) : 0;

    size_t total = 0;
    for (size_t i = 0; i < count; i++) {
        size_t plen = parts[i] ? strlen(parts[i]) : 0;
        if (total > ZTR_MAX_LEN - plen) {
            return ZTR_ERR_OVERFLOW;
        }
        total += plen;
        if (i < count - 1 && sep_len > 0) {
            if (total > ZTR_MAX_LEN - sep_len) {
                return ZTR_ERR_OVERFLOW;
            }
            total += sep_len;
        }
    }

    ztr_err err = ztr_with_cap(out, total);
    if (err) {
        return err;
    }

    for (size_t i = 0; i < count; i++) {
        if (parts[i]) {
            ztr_err err2 = ztr_append(out, parts[i]);
            if (err2) {
                ztr_free(out);
                return err2;
            }
        }
        if (i < count - 1 && sep_len > 0) {
            ztr_err err2 = ztr_append_buf(out, sep, sep_len);
            if (err2) {
                ztr_free(out);
                return err2;
            }
        }
    }

    return ZTR_OK;
}

/* ---- UTF-8 ---- */

bool ztr_is_valid_utf8(const ztr *s) {
    const unsigned char *data = (const unsigned char *)ztr_cstr(s);
    size_t len = ztr_len(s);
    size_t i = 0;

    while (i < len) {
        unsigned char b = data[i];
        size_t seq_len;
        uint32_t cp;

        if (b <= 0x7F) {
            i++;
            continue;
        } else if ((b & 0xE0) == 0xC0) {
            seq_len = 2;
            cp = b & 0x1F;
        } else if ((b & 0xF0) == 0xE0) {
            seq_len = 3;
            cp = b & 0x0F;
        } else if ((b & 0xF8) == 0xF0) {
            seq_len = 4;
            cp = b & 0x07;
        } else {
            return false; /* Invalid leading byte. */
        }

        if (i + seq_len > len) {
            return false; /* Truncated sequence. */
        }

        for (size_t j = 1; j < seq_len; j++) {
            if ((data[i + j] & 0xC0) != 0x80) {
                return false; /* Invalid continuation byte. */
            }
            cp = (cp << 6) | (data[i + j] & 0x3F);
        }

        /* Reject overlong encodings. */
        if (seq_len == 2 && cp < 0x80) {
            return false;
        }
        if (seq_len == 3 && cp < 0x800) {
            return false;
        }
        if (seq_len == 4 && cp < 0x10000) {
            return false;
        }

        /* Reject surrogates and out-of-range. */
        if (cp >= 0xD800 && cp <= 0xDFFF) {
            return false;
        }
        if (cp > 0x10FFFF) {
            return false;
        }

        i += seq_len;
    }

    return true;
}

ztr_err ztr_utf8_len(size_t *out, const ztr *s) {
    const unsigned char *data = (const unsigned char *)ztr_cstr(s);
    size_t len = ztr_len(s);
    size_t i = 0;
    size_t cp_count = 0;

    while (i < len) {
        unsigned char b = data[i];
        size_t seq_len;
        uint32_t cp;

        if (b <= 0x7F) {
            i++;
            cp_count++;
            continue;
        } else if ((b & 0xE0) == 0xC0) {
            seq_len = 2;
            cp = b & 0x1F;
        } else if ((b & 0xF0) == 0xE0) {
            seq_len = 3;
            cp = b & 0x0F;
        } else if ((b & 0xF8) == 0xF0) {
            seq_len = 4;
            cp = b & 0x07;
        } else {
            return ZTR_ERR_INVALID_UTF8;
        }

        if (i + seq_len > len) {
            return ZTR_ERR_INVALID_UTF8;
        }

        for (size_t j = 1; j < seq_len; j++) {
            if ((data[i + j] & 0xC0) != 0x80) {
                return ZTR_ERR_INVALID_UTF8;
            }
            cp = (cp << 6) | (data[i + j] & 0x3F);
        }

        /* Reject overlong encodings. */
        if (seq_len == 2 && cp < 0x80) {
            return ZTR_ERR_INVALID_UTF8;
        }
        if (seq_len == 3 && cp < 0x800) {
            return ZTR_ERR_INVALID_UTF8;
        }
        if (seq_len == 4 && cp < 0x10000) {
            return ZTR_ERR_INVALID_UTF8;
        }

        /* Reject surrogates and out-of-range. */
        if (cp >= 0xD800 && cp <= 0xDFFF) {
            return ZTR_ERR_INVALID_UTF8;
        }
        if (cp > 0x10FFFF) {
            return ZTR_ERR_INVALID_UTF8;
        }

        cp_count++;
        i += seq_len;
    }

    *out = cp_count;
    return ZTR_OK;
}

ztr_err ztr_utf8_next(const ztr *s, size_t *pos, uint32_t *cp) {
    size_t len = ztr_len(s);
    if (*pos >= len) {
        return ZTR_ERR_OUT_OF_RANGE;
    }

    const unsigned char *data = (const unsigned char *)ztr_cstr(s);
    unsigned char b = data[*pos];
    size_t seq_len;
    uint32_t codepoint;

    if (b <= 0x7F) {
        *cp = b;
        (*pos)++;
        return ZTR_OK;
    } else if ((b & 0xE0) == 0xC0) {
        seq_len = 2;
        codepoint = b & 0x1F;
    } else if ((b & 0xF0) == 0xE0) {
        seq_len = 3;
        codepoint = b & 0x0F;
    } else if ((b & 0xF8) == 0xF0) {
        seq_len = 4;
        codepoint = b & 0x07;
    } else {
        *cp = 0xFFFD; /* Replacement character. */
        (*pos)++;     /* Advance by 1 to prevent infinite loops. */
        return ZTR_ERR_INVALID_UTF8;
    }

    if (*pos + seq_len > len) {
        *cp = 0xFFFD;
        (*pos)++;
        return ZTR_ERR_INVALID_UTF8;
    }

    for (size_t j = 1; j < seq_len; j++) {
        if ((data[*pos + j] & 0xC0) != 0x80) {
            *cp = 0xFFFD;
            (*pos)++;
            return ZTR_ERR_INVALID_UTF8;
        }
        codepoint = (codepoint << 6) | (data[*pos + j] & 0x3F);
    }

    /* Reject overlong, surrogates, out-of-range. */
    bool invalid = false;
    if (seq_len == 2 && codepoint < 0x80) {
        invalid = true;
    }
    if (seq_len == 3 && codepoint < 0x800) {
        invalid = true;
    }
    if (seq_len == 4 && codepoint < 0x10000) {
        invalid = true;
    }
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
        invalid = true;
    }
    if (codepoint > 0x10FFFF) {
        invalid = true;
    }

    if (invalid) {
        *cp = 0xFFFD;
        (*pos)++;
        return ZTR_ERR_INVALID_UTF8;
    }

    *cp = codepoint;
    *pos += seq_len;
    return ZTR_OK;
}

ztr_err ztr_utf8_append(ztr *s, uint32_t codepoint) {
    /* Reject surrogates and out-of-range. */
    if (codepoint >= 0xD800 && codepoint <= 0xDFFF) {
        return ZTR_ERR_INVALID_UTF8;
    }
    if (codepoint > 0x10FFFF) {
        return ZTR_ERR_INVALID_UTF8;
    }

    char buf[4];
    size_t len;

    if (codepoint <= 0x7F) {
        buf[0] = (char)codepoint;
        len = 1;
    } else if (codepoint <= 0x7FF) {
        buf[0] = (char)(0xC0 | (codepoint >> 6));
        buf[1] = (char)(0x80 | (codepoint & 0x3F));
        len = 2;
    } else if (codepoint <= 0xFFFF) {
        buf[0] = (char)(0xE0 | (codepoint >> 12));
        buf[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (codepoint & 0x3F));
        len = 3;
    } else {
        buf[0] = (char)(0xF0 | (codepoint >> 18));
        buf[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buf[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buf[3] = (char)(0x80 | (codepoint & 0x3F));
        len = 4;
    }

    return ztr_append_buf(s, buf, len);
}

/* ---- Interop ---- */

ztr_err ztr_set_len(ztr *s, size_t len) {
    if (len > ZTR_MAX_LEN) {
        return ZTR_ERR_OVERFLOW;
    }
    if (len > ztr_capacity(s)) {
        return ZTR_ERR_OUT_OF_RANGE;
    }
    char *buf = ztr_p_buf(s);
    buf[len] = '\0';
    if (ztr_p_is_heap(s)) {
        ztr_p_set_len_heap(s, len);
    } else {
        ztr_p_set_len_sso(s, len);
    }
    return ZTR_OK;
}

ztr_err ztr_detach(ztr *s, char **out) {
    size_t len = ztr_len(s);

    if (ztr_p_is_heap(s)) {
        /* Transfer ownership directly. */
        *out = s->heap.data;
        ztr_init(s);
    } else {
        /* SSO: must allocate a copy. */
        char *copy = (char *)ZTR_MALLOC(len + 1);
        if (!copy) {
            return ZTR_ERR_ALLOC;
        }
        memcpy(copy, s->sso, len + 1);
        *out = copy;
        ztr_init(s);
    }

    return ZTR_OK;
}

void ztr_swap(ztr *a, ztr *b) {
    ztr tmp = *a;
    *a = *b;
    *b = tmp;
}

/* ---- Utility ---- */

bool ztr_is_ascii(const ztr *s) {
    const unsigned char *data = (const unsigned char *)ztr_cstr(s);
    size_t len = ztr_len(s);
    for (size_t i = 0; i < len; i++) {
        if (data[i] > 0x7F) {
            return false;
        }
    }
    return true;
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
