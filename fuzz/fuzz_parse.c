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

    /* ---- Cleanup ---- */

    ztr_free(&needle);
    ztr_free(&s);

    return 0;
}
