/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Samuel Lambert
 *
 * Fuzz harness for the ztr string library.
 *
 * Exercises construction, mutation, search, replace, split, UTF-8, and interop
 * paths with arbitrary byte input. The fuzzer input is partitioned into a
 * "haystack" and a "needle" to drive two-argument operations.
 *
 * Build:  make fuzz
 * Run:    ./build/fuzz/ztr_fuzz corpus/ -max_len=4096
 */

#include "ztr.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    if (size == 0) {
        return 0;
    }

    /* Split input into two parts: haystack and needle.
       First byte selects the split point as a fraction of the remaining data. */
    uint8_t split_frac = data[0];
    const uint8_t *payload = data + 1;
    size_t payload_len = size - 1;

    size_t hay_len = (payload_len * (size_t)split_frac) / 255;
    if (hay_len > payload_len) {
        hay_len = payload_len;
    }
    size_t needle_len = payload_len - hay_len;

    const char *hay_data = (const char *)payload;
    const char *needle_data = (const char *)(payload + hay_len);

    /* ---- Construction ---- */

    ztr s = {0};
    if (ztr_from_buf(&s, hay_data, hay_len) != ZTR_OK) {
        return 0;
    }

    /* Build a null-terminated needle for functions that take const char*. */
    ztr needle = {0};
    if (ztr_from_buf(&needle, needle_data, needle_len) != ZTR_OK) {
        ztr_free(&s);
        return 0;
    }

    /* ---- Accessors ---- */

    (void)ztr_len(&s);
    (void)ztr_cstr(&s);
    (void)ztr_is_empty(&s);
    (void)ztr_capacity(&s);
    if (ztr_len(&s) > 0) {
        (void)ztr_at(&s, 0);
        (void)ztr_at(&s, ztr_len(&s) - 1);
        (void)ztr_at(&s, ztr_len(&s)); /* OOB — should return '\0' */
    }

    /* ---- Comparison ---- */

    (void)ztr_eq(&s, &needle);
    (void)ztr_eq_cstr(&s, ztr_cstr(&needle));
    (void)ztr_cmp(&s, &needle);
    (void)ztr_eq_ascii_nocase(&s, &needle);

    /* ---- Search ---- */

    (void)ztr_find(&s, ztr_cstr(&needle), 0);
    (void)ztr_rfind(&s, ztr_cstr(&needle), ZTR_NPOS);
    (void)ztr_contains(&s, ztr_cstr(&needle));
    (void)ztr_starts_with(&s, ztr_cstr(&needle));
    (void)ztr_ends_with(&s, ztr_cstr(&needle));
    (void)ztr_count(&s, ztr_cstr(&needle));

    /* ---- UTF-8 ---- */

    (void)ztr_is_valid_utf8(&s);
    (void)ztr_is_ascii(&s);

    {
        size_t cp_count = 0;
        (void)ztr_utf8_len(&cp_count, &s);
    }

    /* Iterate codepoints — exercises ztr_utf8_next error/advance paths. */
    {
        size_t pos = 0;
        uint32_t cp;
        while (pos < ztr_len(&s)) {
            ztr_err err = ztr_utf8_next(&s, &pos, &cp);
            if (err == ZTR_ERR_OUT_OF_RANGE) {
                break;
            }
            /* On ZTR_ERR_INVALID_UTF8, pos advances by 1 — loop terminates. */
        }
    }

    /* ---- Mutation ---- */

    /* Work on a clone so we can keep exercising s. */
    ztr work;
    ztr_clone(&work, &s);

    /* Append */
    ztr_append(&work, ztr_cstr(&needle));
    ztr_append_ztr(&work, &needle);
    if (needle_len > 0) {
        ztr_append_byte(&work, needle_data[0]);
    }

    /* Self-referential append — a critical path for aliasing bugs. */
    ztr_append(&work, ztr_cstr(&work));

    /* Insert at various positions. */
    if (ztr_len(&work) > 0) {
        ztr_insert(&work, 0, ztr_cstr(&needle));                  /* beginning */
        ztr_insert(&work, ztr_len(&work), ztr_cstr(&needle));     /* end */
        ztr_insert(&work, ztr_len(&work) / 2, ztr_cstr(&needle)); /* middle */
    }

    /* Erase — clamping, boundary conditions. */
    if (ztr_len(&work) > 0) {
        ztr_erase(&work, 0, 1);                  /* first byte */
        ztr_erase(&work, ztr_len(&work) / 2, 3); /* middle */
    }
    ztr_erase(&work, ztr_len(&work), 100); /* past end — should be no-op */

    /* Truncate. */
    if (ztr_len(&work) > 2) {
        ztr_truncate(&work, ztr_len(&work) - 2);
    }

    /* Clear and rebuild. */
    ztr_clear(&work);
    ztr_append_buf(&work, hay_data, hay_len > 64 ? 64 : hay_len);

    ztr_free(&work);

    /* ---- Replace ---- */

    if (needle_len > 0 && needle_len <= 32 && hay_len <= 256) {
        /* Bound inputs to keep replace_all tractable for the fuzzer. */
        ztr r;
        ztr_clone(&r, &s);
        ztr_replace_first(&r, ztr_cstr(&needle), "REPLACED");
        ztr_free(&r);

        ztr_clone(&r, &s);
        ztr_replace_all(&r, ztr_cstr(&needle), "X");
        ztr_free(&r);

        /* Replace with empty (deletion). */
        ztr_clone(&r, &s);
        ztr_replace_all(&r, ztr_cstr(&needle), "");
        ztr_free(&r);
    }

    /* ---- Split ---- */

    if (needle_len > 0 && needle_len <= 8) {
        /* Iterator split. */
        ztr_split_iter it;
        ztr_split_begin(&it, &s, ztr_cstr(&needle));
        const char *part;
        size_t part_len;
        size_t part_count = 0;
        while (ztr_split_next(&it, &part, &part_len)) {
            part_count++;
            if (part_count > 10000) {
                break; /* Safety valve. */
            }
        }

        /* Allocating split with limit. */
        ztr *parts = NULL;
        size_t count = 0;
        if (ztr_split_alloc(&s, ztr_cstr(&needle), &parts, &count, 100) == ZTR_OK) {
            ztr_split_free(parts, count);
        }
    }

    /* ---- Extraction ---- */

    if (hay_len > 4) {
        ztr sub;
        ztr_substr(&sub, &s, 1, hay_len / 2);
        ztr_free(&sub);
    }

    /* ---- Transformation ---- */

    {
        ztr t;
        ztr_clone(&t, &s);
        ztr_trim(&t);
        ztr_free(&t);

        ztr_clone(&t, &s);
        ztr_to_ascii_upper(&t);
        ztr_to_ascii_lower(&t);
        ztr_free(&t);
    }

    /* ---- Interop ---- */

    /* data_mut + set_len round-trip. */
    {
        ztr d;
        ztr_clone(&d, &s);
        size_t len = ztr_len(&d);
        if (len > 0) {
            char *buf = ztr_data_mut(&d);
            buf[0] = 'Z';         /* mutate through raw pointer */
            ztr_set_len(&d, len); /* no-op length change, but exercises the function */
        }
        ztr_free(&d);
    }

    /* detach + free round-trip. */
    {
        ztr d;
        ztr_clone(&d, &s);
        char *detached = NULL;
        if (ztr_detach(&d, &detached) == ZTR_OK) {
            free(detached);
        }
        ztr_free(&d); /* should be empty after detach */
    }

    /* swap. */
    {
        ztr a, b;
        ztr_clone(&a, &s);
        ztr_clone(&b, &needle);
        ztr_swap(&a, &b);
        ztr_free(&a);
        ztr_free(&b);
    }

    /* ---- Lifecycle edge cases ---- */

    /* assign (may free + rebuild). */
    {
        ztr a;
        ztr_clone(&a, &s);
        ztr_assign(&a, ztr_cstr(&needle));
        ztr_assign(&a, NULL); /* should reset to empty */
        ztr_free(&a);
    }

    /* move. */
    {
        ztr src_m, dst_m;
        ztr_clone(&src_m, &s);
        ztr_init(&dst_m);
        ztr_move(&dst_m, &src_m);
        ztr_free(&dst_m);
        ztr_free(&src_m); /* should be safe — src is empty after move */
    }

    /* Double-free safety. */
    {
        ztr d;
        ztr_clone(&d, &s);
        ztr_free(&d);
        ztr_free(&d); /* second free — must not crash */
    }

    /* ---- String View ---- */

    /* Build views from the same haystack/needle data. */
    ztr_view vs = ztr_view_from_ztr(&s);
    ztr_view vn = ztr_view_from_ztr(&needle);

    /* Construction variants. */
    {
        ztr_view v1 = ztr_view_from_buf(hay_data, hay_len);
        (void)ztr_view_len(v1);

        ztr_view v2 = ztr_view_from_cstr(ztr_cstr(&s));
        (void)ztr_view_data(v2);

        ztr_view v3 = ztr_view_from_ztr(&s);
        (void)ztr_view_is_empty(v3);

        /* NULL construction. */
        ztr_view v4 = ztr_view_from_cstr(NULL);
        (void)ztr_view_len(v4);
        ztr_view v5 = ztr_view_from_buf(NULL, 0);
        (void)ztr_view_len(v5);
        ztr_view v6 = ztr_view_from_ztr(NULL);
        (void)ztr_view_len(v6);
    }

    /* Accessors. */
    (void)ztr_view_len(vs);
    (void)ztr_view_data(vs);
    (void)ztr_view_is_empty(vs);
    if (ztr_view_len(vs) > 0) {
        (void)ztr_view_at(vs, 0);
        (void)ztr_view_at(vs, ztr_view_len(vs) - 1);
    }
    (void)ztr_view_at(vs, ztr_view_len(vs)); /* OOB — should return '\0' */

    /* Comparison. */
    (void)ztr_view_eq(vs, vn);
    (void)ztr_view_eq_cstr(vs, ztr_cstr(&needle));
    (void)ztr_view_eq_cstr(vs, NULL);
    (void)ztr_view_cmp(vs, vn);
    (void)ztr_view_cmp_cstr(vs, ztr_cstr(&needle));
    (void)ztr_view_cmp_cstr(vs, NULL);
    (void)ztr_view_eq_ascii_nocase(vs, vn);
    (void)ztr_view_eq_ascii_nocase_cstr(vs, ztr_cstr(&needle));
    (void)ztr_view_eq_ascii_nocase_cstr(vs, NULL);

    /* Search (view needle). */
    (void)ztr_view_find(vs, vn, 0);
    (void)ztr_view_rfind(vs, vn, ZTR_NPOS);
    (void)ztr_view_contains(vs, vn);
    (void)ztr_view_starts_with(vs, vn);
    (void)ztr_view_ends_with(vs, vn);
    (void)ztr_view_count(vs, vn);

    /* Search with offset. */
    if (ztr_view_len(vs) > 2) {
        (void)ztr_view_find(vs, vn, ztr_view_len(vs) / 2);
        (void)ztr_view_rfind(vs, vn, ztr_view_len(vs) / 2);
    }

    /* Search (empty needle). */
    (void)ztr_view_find(vs, ZTR_VIEW_EMPTY, 0);
    (void)ztr_view_contains(vs, ZTR_VIEW_EMPTY);

    /* Search (cstr needle). */
    (void)ztr_view_find_cstr(vs, ztr_cstr(&needle), 0);
    (void)ztr_view_rfind_cstr(vs, ztr_cstr(&needle), ZTR_NPOS);
    (void)ztr_view_contains_cstr(vs, ztr_cstr(&needle));
    (void)ztr_view_starts_with_cstr(vs, ztr_cstr(&needle));
    (void)ztr_view_ends_with_cstr(vs, ztr_cstr(&needle));
    (void)ztr_view_count_cstr(vs, ztr_cstr(&needle));
    (void)ztr_view_find_cstr(vs, NULL, 0);

    /* Search (single character). */
    if (needle_len > 0) {
        (void)ztr_view_find_char(vs, needle_data[0], 0);
        (void)ztr_view_rfind_char(vs, needle_data[0], ZTR_NPOS);
        (void)ztr_view_contains_char(vs, needle_data[0]);
    }
    (void)ztr_view_find_char(vs, '\0', 0);
    (void)ztr_view_contains_char(ZTR_VIEW_EMPTY, 'x');

    /* Trimming. */
    {
        ztr_view t1 = ztr_view_trim(vs);
        (void)ztr_view_len(t1);
        ztr_view t2 = ztr_view_trim_start(vs);
        (void)ztr_view_len(t2);
        ztr_view t3 = ztr_view_trim_end(vs);
        (void)ztr_view_len(t3);

        /* Trim on empty/zero-init. */
        ztr_view empty = {0};
        (void)ztr_view_trim(empty);
        (void)ztr_view_trim(ZTR_VIEW_EMPTY);
    }

    /* Substr and slice. */
    if (ztr_view_len(vs) > 4) {
        ztr_view sub = ztr_view_substr(vs, 1, ztr_view_len(vs) / 2);
        (void)ztr_view_len(sub);
    }
    (void)ztr_view_substr(vs, 0, 0);
    (void)ztr_view_substr(vs, ztr_view_len(vs), 5);        /* pos == len */
    (void)ztr_view_substr(vs, ztr_view_len(vs) + 10, 5);   /* pos > len */
    (void)ztr_view_substr(vs, 0, ZTR_NPOS);                /* count = SIZE_MAX */

    if (ztr_view_len(vs) > 2) {
        (void)ztr_view_remove_prefix(vs, 1);
        (void)ztr_view_remove_suffix(vs, 1);
    }
    (void)ztr_view_remove_prefix(vs, ztr_view_len(vs));     /* exact */
    (void)ztr_view_remove_suffix(vs, ztr_view_len(vs));     /* exact */
    (void)ztr_view_remove_prefix(vs, ztr_view_len(vs) + 1); /* past end */
    (void)ztr_view_remove_suffix(vs, ztr_view_len(vs) + 1); /* past end */

    /* Split iterator. */
    if (needle_len > 0 && needle_len <= 8) {
        ztr_view_split_iter vit;
        ztr_view_split_begin(&vit, vs, vn);
        ztr_view token;
        size_t vpart_count = 0;
        while (ztr_view_split_next(&vit, &token)) {
            (void)ztr_view_len(token);
            vpart_count++;
            if (vpart_count > 10000) {
                break; /* Safety valve. */
            }
        }

        /* cstr delimiter variant. */
        ztr_view_split_begin_cstr(&vit, vs, ztr_cstr(&needle));
        vpart_count = 0;
        while (ztr_view_split_next(&vit, &token)) {
            vpart_count++;
            if (vpart_count > 10000) {
                break;
            }
        }
    }

    /* Empty delimiter split. */
    {
        ztr_view_split_iter vit;
        ztr_view_split_begin(&vit, vs, ZTR_VIEW_EMPTY);
        ztr_view token;
        (void)ztr_view_split_next(&vit, &token);
    }

    /* NULL delimiter via cstr. */
    {
        ztr_view_split_iter vit;
        ztr_view_split_begin_cstr(&vit, vs, NULL);
        ztr_view token;
        (void)ztr_view_split_next(&vit, &token);
    }

    /* Utility. */
    (void)ztr_view_is_ascii(vs);
    (void)ztr_view_is_valid_utf8(vs);
    (void)ztr_view_is_ascii(ZTR_VIEW_EMPTY);
    (void)ztr_view_is_valid_utf8(ZTR_VIEW_EMPTY);

    /* Interop — materialize view into owned ztr. */
    {
        ztr m;
        ztr_init(&m);
        ztr_from_view(&m, vs);
        ztr_free(&m);
    }

    /* Self-referential: view into s, then from_view back into s. */
    if (hay_len > 4) {
        ztr self;
        ztr_clone(&self, &s);
        ztr_view sv = ztr_view_substr(ztr_view_from_ztr(&self), 1, hay_len / 2);
        ztr_from_view(&self, sv);
        ztr_free(&self);
    }

    /* ---- Cleanup ---- */

    ztr_free(&needle);
    ztr_free(&s);

    return 0;
}
