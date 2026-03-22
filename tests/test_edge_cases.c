#include "greatest.h"
#include "ztr.h"

#include <stdint.h>
#include <string.h>

/* ---------------------------------------------------------------------------
 * NULL argument safety
 * ------------------------------------------------------------------------- */

/* ztr_free(NULL) must be a safe no-op — it must not crash or invoke UB. */
TEST free_null_is_safe(void) {
    ztr_free(NULL);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Double-free safety
 * ------------------------------------------------------------------------- */

/* Calling ztr_free twice on the same ztr must be safe.  The implementation
 * zeros the struct on the first call so the second call sees an SSO/empty
 * string with no heap pointer. */
TEST double_free_sso_is_safe(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "short");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&s));

    ztr_free(&s);
    ztr_free(&s); /* must not crash */
    PASS();
}

TEST double_free_heap_is_safe(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "a string long enough to force heap allocation here");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&s));

    ztr_free(&s);
    /* After the first free, the struct is zeroed — the heap bit is clear.
     * The second free must recognize this and not attempt to free again. */
    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Self-referential append
 * ------------------------------------------------------------------------- */

/* ztr_append(&s, ztr_cstr(&s)) must double the content correctly even if a
 * reallocation occurs during the call. */
TEST self_append_sso_doubles_content(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "ab");
    ASSERT_EQ(ZTR_OK, err);

    err = ztr_append(&s, ztr_cstr(&s));
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("abab", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST self_append_heap_doubles_content(void) {
    ztr s;
    /* 20 bytes — guaranteed heap on 64-bit (ZTR_SSO_CAP == 15). */
    ztr_err err = ztr_from(&s, "12345678901234567890");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&s));

    err = ztr_append(&s, ztr_cstr(&s));
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)40, ztr_len(&s));
    ASSERT_STR_EQ("1234567890123456789012345678901234567890", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* Self-append on an empty string should leave it empty. */
TEST self_append_empty_stays_empty(void) {
    ztr s;
    ztr_init(&s);

    ztr_err err = ztr_append(&s, ztr_cstr(&s));
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Self-referential assign
 * ------------------------------------------------------------------------- */

/* ztr_assign(&s, ztr_cstr(&s)) must be a no-op: the string keeps its exact
 * content and the call succeeds. */
TEST self_assign_sso_is_noop(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "hello");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&s));

    err = ztr_assign(&s, ztr_cstr(&s));
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST self_assign_heap_is_noop(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "a heap string with more than fifteen characters");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&s));

    size_t original_len = ztr_len(&s);
    err = ztr_assign(&s, ztr_cstr(&s));
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(original_len, ztr_len(&s));
    ASSERT_STR_EQ("a heap string with more than fifteen characters", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * SSO boundary
 * ------------------------------------------------------------------------- */

/* A string of exactly ZTR_SSO_CAP bytes must stay in SSO storage. */
TEST sso_boundary_exact_sso_cap_stays_sso(void) {
    /* ZTR_SSO_CAP == 15 on 64-bit. */
    char buf[ZTR_SSO_CAP + 1];
    memset(buf, 'x', ZTR_SSO_CAP);
    buf[ZTR_SSO_CAP] = '\0';

    ztr s;
    ztr_err err = ztr_from(&s, buf);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&s));
    ASSERT_EQ((size_t)ZTR_SSO_CAP, ztr_len(&s));
    ASSERT_STR_EQ(buf, ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* A string of ZTR_SSO_CAP + 1 bytes must be promoted to heap storage. */
TEST sso_boundary_one_over_sso_cap_goes_heap(void) {
    char buf[ZTR_SSO_CAP + 2];
    memset(buf, 'y', ZTR_SSO_CAP + 1);
    buf[ZTR_SSO_CAP + 1] = '\0';

    ztr s;
    ztr_err err = ztr_from(&s, buf);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&s));
    ASSERT_EQ((size_t)(ZTR_SSO_CAP + 1), ztr_len(&s));
    ASSERT_STR_EQ(buf, ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* Zero-length string must use SSO and have capacity ZTR_SSO_CAP. */
TEST sso_boundary_empty_string_is_sso(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_FALSE(ztr_p_is_heap(&s));
    ASSERT_EQ((size_t)ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

/* One-byte string must use SSO. */
TEST sso_boundary_one_byte_string_is_sso(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "a");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&s));
    ASSERT_EQ((size_t)1, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

/* from_buf with len == ZTR_SSO_CAP must stay SSO; len == ZTR_SSO_CAP+1 must
 * go heap, and the content must round-trip correctly in both cases. */
TEST sso_boundary_from_buf_at_boundary(void) {
    /* Build a source buffer longer than SSO_CAP. */
    char src[ZTR_SSO_CAP + 2];
    for (size_t i = 0; i < ZTR_SSO_CAP + 1; i++) {
        src[i] = (char)('A' + (i % 26));
    }
    src[ZTR_SSO_CAP + 1] = '\0';

    /* Exactly at cap. */
    ztr a;
    ztr_err err = ztr_from_buf(&a, src, ZTR_SSO_CAP);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&a));
    ASSERT_EQ((size_t)ZTR_SSO_CAP, ztr_len(&a));
    ASSERT_EQ(0, memcmp(ztr_cstr(&a), src, ZTR_SSO_CAP));

    /* One over cap. */
    ztr b;
    err = ztr_from_buf(&b, src, ZTR_SSO_CAP + 1);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&b));
    ASSERT_EQ((size_t)(ZTR_SSO_CAP + 1), ztr_len(&b));
    ASSERT_EQ(0, memcmp(ztr_cstr(&b), src, ZTR_SSO_CAP + 1));

    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

/* ---------------------------------------------------------------------------
 * SSO → heap transition during append
 * ------------------------------------------------------------------------- */

/* Start with a 1-byte SSO string and append one byte at a time until the
 * string transitions from SSO to heap.  Verify that transition happens at
 * the expected boundary and the content remains correct throughout. */
TEST sso_to_heap_transition_via_append(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "a");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&s));

    /* Append up through ZTR_SSO_CAP bytes total — must stay SSO. */
    for (size_t len = 1; len < ZTR_SSO_CAP; len++) {
        err = ztr_append_byte(&s, 'b');
        ASSERT_EQ(ZTR_OK, err);
        ASSERT_EQ(len + 1, ztr_len(&s));
        /* Still SSO if we haven't crossed the boundary yet. */
        if (len + 1 <= ZTR_SSO_CAP) {
            ASSERT_FALSE(ztr_p_is_heap(&s));
        }
    }
    /* Now len == ZTR_SSO_CAP — still SSO. */
    ASSERT_EQ((size_t)ZTR_SSO_CAP, ztr_len(&s));
    ASSERT_FALSE(ztr_p_is_heap(&s));

    /* One more byte pushes over the boundary — must be on heap. */
    err = ztr_append_byte(&s, 'c');
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)(ZTR_SSO_CAP + 1), ztr_len(&s));
    ASSERT(ztr_p_is_heap(&s));

    /* Verify first and last bytes. */
    ASSERT_EQ('a', ztr_at(&s, 0));
    ASSERT_EQ('c', ztr_at(&s, ZTR_SSO_CAP));
    ASSERT_EQ('\0', ztr_cstr(&s)[ZTR_SSO_CAP + 1]);

    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Empty string operations
 * ------------------------------------------------------------------------- */

/* ztr_find on an empty string must return ZTR_NPOS for a non-empty needle
 * and return 0 (start) for an empty needle when start == 0. */
TEST find_on_empty_string(void) {
    ztr s;
    ztr_init(&s);

    /* Non-empty needle — not found. */
    ASSERT_EQ(ZTR_NPOS, ztr_find(&s, "x", 0));
    ASSERT_EQ(ZTR_NPOS, ztr_find(&s, "hello", 0));

    /* Empty needle at start == 0 returns 0. */
    ASSERT_EQ((size_t)0, ztr_find(&s, "", 0));

    /* Empty needle with start > len returns NPOS. */
    ASSERT_EQ(ZTR_NPOS, ztr_find(&s, "", 1));

    ztr_free(&s);
    PASS();
}

/* ztr_trim, ztr_trim_start, ztr_trim_end on an empty string must be no-ops. */
TEST trim_on_empty_string(void) {
    ztr a, b, c;
    ztr_init(&a);
    ztr_init(&b);
    ztr_init(&c);

    ztr_trim(&a);
    ztr_trim_start(&b);
    ztr_trim_end(&c);

    ASSERT(ztr_is_empty(&a));
    ASSERT(ztr_is_empty(&b));
    ASSERT(ztr_is_empty(&c));
    ASSERT_STR_EQ("", ztr_cstr(&a));
    ASSERT_STR_EQ("", ztr_cstr(&b));
    ASSERT_STR_EQ("", ztr_cstr(&c));

    ztr_free(&a);
    ztr_free(&b);
    ztr_free(&c);
    PASS();
}

/* ztr_erase on an empty string with any pos and count must be a safe no-op. */
TEST erase_on_empty_string(void) {
    ztr s;
    ztr_init(&s);

    ztr_erase(&s, 0, 0);
    ASSERT(ztr_is_empty(&s));

    ztr_erase(&s, 0, 100);
    ASSERT(ztr_is_empty(&s));

    /* pos > len — must also be a no-op (out of range, not a crash). */
    ztr_erase(&s, 5, 1);
    ASSERT(ztr_is_empty(&s));

    ztr_free(&s);
    PASS();
}

/* ztr_contains, ztr_starts_with, ztr_ends_with on an empty string. */
TEST search_predicates_on_empty_string(void) {
    ztr s;
    ztr_init(&s);

    ASSERT_FALSE(ztr_contains(&s, "a"));
    /* Empty needle/prefix/suffix is always true by convention. */
    ASSERT(ztr_starts_with(&s, ""));
    ASSERT(ztr_ends_with(&s, ""));

    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * ztr_is_ascii
 * ------------------------------------------------------------------------- */

/* A pure ASCII string (all bytes 0x00–0x7F) must return true. */
TEST is_ascii_pure_ascii_string(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "Hello, World! 0123 \t\n");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_is_ascii(&s));

    ztr_free(&s);
    PASS();
}

/* An empty string must be considered ASCII (vacuously true). */
TEST is_ascii_empty_string_is_ascii(void) {
    ztr s;
    ztr_init(&s);
    ASSERT(ztr_is_ascii(&s));
    ztr_free(&s);
    PASS();
}

/* A string containing a byte with value > 0x7F must return false. */
TEST is_ascii_high_byte_returns_false(void) {
    /* Build a string manually using from_buf so we can embed a high byte. */
    const char buf[] = {'a', 'b', (char)0x80, 'c', '\0'};
    ztr s;
    ztr_err err = ztr_from_buf(&s, buf, 4);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_is_ascii(&s));

    ztr_free(&s);
    PASS();
}

/* Byte 0xFF at various positions must also return false. */
TEST is_ascii_0xff_byte_returns_false(void) {
    const char buf[] = {(char)0xFF, 'a', 'b', '\0'};
    ztr s;
    ztr_err err = ztr_from_buf(&s, buf, 3);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_is_ascii(&s));

    ztr_free(&s);
    PASS();
}

/* Byte 0x7F (DEL) is still within the ASCII range. */
TEST is_ascii_0x7f_is_ascii(void) {
    const char buf[] = {'a', (char)0x7F, 'z', '\0'};
    ztr s;
    ztr_err err = ztr_from_buf(&s, buf, 3);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_is_ascii(&s));

    ztr_free(&s);
    PASS();
}

/* A high byte in the last position of a heap string must be detected. */
TEST is_ascii_high_byte_at_end_heap_string(void) {
    /* Construct a 20-byte string (heap) with 0x81 as the final byte. */
    char buf[21];
    memset(buf, 'a', 19);
    buf[19] = (char)0x81;
    buf[20] = '\0';

    ztr s;
    ztr_err err = ztr_from_buf(&s, buf, 20);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&s));
    ASSERT_FALSE(ztr_is_ascii(&s));

    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * ztr_from_buf with len close to ZTR_MAX_LEN (overflow)
 * ------------------------------------------------------------------------- */

/* from_buf with len == ZTR_MAX_LEN + 1 must return ZTR_ERR_OVERFLOW without
 * allocating or modifying the output ztr. */
TEST from_buf_len_just_over_max_returns_overflow(void) {
    /* We only pass the length — we don't need a real buffer of that size
     * because the overflow check happens before any dereference. */
    const char *dummy = "x";
    ztr s;
    /* Deliberately leave s uninitialised to confirm it is not touched. */
    ztr_init(&s);

    ztr_err err = ztr_from_buf(&s, dummy, ZTR_MAX_LEN + 1);
    ASSERT_EQ(ZTR_ERR_OVERFLOW, err);

    /* The ztr must still be in a valid (empty) state because from_buf calls
     * ztr_init before any allocation. */
    ASSERT(ztr_is_empty(&s));

    ztr_free(&s);
    PASS();
}

/* set_len with ZTR_MAX_LEN + 1 must return ZTR_ERR_OVERFLOW even if the
 * string happens to have a large-enough capacity (which it never will in
 * practice — but the overflow check must fire first). */
TEST set_len_size_t_max_overflow(void) {
    ztr s;
    ztr_init(&s);

    /* SIZE_MAX itself wraps around to 0 when interpreted as a length, but
     * ZTR_MAX_LEN + 1 == ZTR_HEAP_BIT which is clearly a sentinel. */
    ztr_err err = ztr_set_len(&s, ZTR_MAX_LEN + 1);
    ASSERT_EQ(ZTR_ERR_OVERFLOW, err);

    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Additional boundary and correctness checks
 * ------------------------------------------------------------------------- */

/* Appending to a string that is exactly at ZTR_SSO_CAP must succeed and
 * produce the correct heap string. */
TEST append_from_full_sso(void) {
    /* Fill SSO to capacity. */
    char buf[ZTR_SSO_CAP + 1];
    memset(buf, 'z', ZTR_SSO_CAP);
    buf[ZTR_SSO_CAP] = '\0';

    ztr s;
    ztr_err err = ztr_from(&s, buf);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&s));
    ASSERT_EQ((size_t)ZTR_SSO_CAP, ztr_len(&s));

    err = ztr_append(&s, "!");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&s));
    ASSERT_EQ((size_t)(ZTR_SSO_CAP + 1), ztr_len(&s));
    ASSERT_EQ('z', ztr_at(&s, 0));
    ASSERT_EQ('!', ztr_at(&s, ZTR_SSO_CAP));
    ASSERT_EQ('\0', ztr_cstr(&s)[ZTR_SSO_CAP + 1]);

    ztr_free(&s);
    PASS();
}

/* ztr_erase with count larger than the remaining tail must clamp and not
 * read out of bounds. */
TEST erase_count_clamped_to_tail(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "abcdef");
    ASSERT_EQ(ZTR_OK, err);

    /* Erase from index 2 with a huge count — must clamp to 4. */
    ztr_erase(&s, 2, 1000);
    ASSERT_EQ((size_t)2, ztr_len(&s));
    ASSERT_STR_EQ("ab", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* ztr_erase at pos == len is a no-op (pos is out of range for erase). */
TEST erase_at_len_is_noop(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "hello");
    ASSERT_EQ(ZTR_OK, err);

    ztr_erase(&s, 5, 1);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* ztr_at out-of-bounds index must return '\0' without crashing. */
TEST at_out_of_bounds_returns_nul(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "hi");
    ASSERT_EQ(ZTR_OK, err);

    ASSERT_EQ('\0', ztr_at(&s, 2));
    ASSERT_EQ('\0', ztr_at(&s, 100));
    ASSERT_EQ('\0', ztr_at(&s, ZTR_NPOS));

    ztr_free(&s);
    PASS();
}

/* ztr_clear on a heap string must preserve the heap allocation (capacity)
 * while producing a zero-length string. */
TEST clear_heap_string_preserves_capacity(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "this string is longer than fifteen bytes total");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&s));

    size_t cap_before = ztr_capacity(&s);
    ztr_clear(&s);

    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    /* Capacity must be at least as large as it was before clear. */
    ASSERT_GTE(ztr_capacity(&s), cap_before);
    /* The string must still be on the heap (clear does not free). */
    ASSERT(ztr_p_is_heap(&s));

    ztr_free(&s);
    PASS();
}

/* ztr_truncate to the same length is a no-op. */
TEST truncate_to_same_length_is_noop(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "same");
    ASSERT_EQ(ZTR_OK, err);

    ztr_truncate(&s, 4);
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("same", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* ztr_truncate to a length greater than the current length is a no-op. */
TEST truncate_to_longer_is_noop(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "abc");
    ASSERT_EQ(ZTR_OK, err);

    ztr_truncate(&s, 100);
    ASSERT_EQ((size_t)3, ztr_len(&s));
    ASSERT_STR_EQ("abc", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* ztr_cstr always returns a null-terminated string, even after operations
 * that modify the string in place. */
TEST cstr_always_null_terminated(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "hello world");
    ASSERT_EQ(ZTR_OK, err);

    /* Truncate and verify null-terminator. */
    ztr_truncate(&s, 5);
    ASSERT_EQ('\0', ztr_cstr(&s)[5]);

    /* Append and verify null-terminator at new end. */
    err = ztr_append(&s, "!");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ('\0', ztr_cstr(&s)[6]);

    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * ztr_substr
 * ------------------------------------------------------------------------- */

/* Basic substring from the middle of a string. */
TEST substr_from_middle(void) {
    ztr s, out;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(ZTR_OK, ztr_substr(&out, &s, 6, 5));
    ASSERT_EQ((size_t)5, ztr_len(&out));
    ASSERT_STR_EQ("world", ztr_cstr(&out));
    ztr_free(&out);
    ztr_free(&s);
    PASS();
}

/* Substring from the start (pos=0). */
TEST substr_from_start(void) {
    ztr s, out;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(ZTR_OK, ztr_substr(&out, &s, 0, 5));
    ASSERT_EQ((size_t)5, ztr_len(&out));
    ASSERT_STR_EQ("hello", ztr_cstr(&out));
    ztr_free(&out);
    ztr_free(&s);
    PASS();
}

/* Substring to end: count exceeds remaining characters, must clamp. */
TEST substr_to_end_count_exceeds_remaining(void) {
    ztr s, out;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(ZTR_OK, ztr_substr(&out, &s, 6, 1000));
    ASSERT_EQ((size_t)5, ztr_len(&out));
    ASSERT_STR_EQ("world", ztr_cstr(&out));
    ztr_free(&out);
    ztr_free(&s);
    PASS();
}

/* pos >= len: output must be empty. */
TEST substr_pos_ge_len_gives_empty(void) {
    ztr s, out;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_substr(&out, &s, 5, 3));
    ASSERT(ztr_is_empty(&out));
    ASSERT_STR_EQ("", ztr_cstr(&out));
    ztr_free(&out);

    ASSERT_EQ(ZTR_OK, ztr_substr(&out, &s, 100, 1));
    ASSERT(ztr_is_empty(&out));
    ztr_free(&out);
    ztr_free(&s);
    PASS();
}

/* Full string: pos=0, count=len. */
TEST substr_full_string(void) {
    ztr s, out;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_substr(&out, &s, 0, 5));
    ASSERT_EQ((size_t)5, ztr_len(&out));
    ASSERT_STR_EQ("hello", ztr_cstr(&out));
    ASSERT(ztr_eq(&s, &out));
    ztr_free(&out);
    ztr_free(&s);
    PASS();
}

/* Empty source string: output must be empty regardless of pos/count. */
TEST substr_empty_source(void) {
    ztr s, out;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_substr(&out, &s, 0, 5));
    ASSERT(ztr_is_empty(&out));
    ASSERT_STR_EQ("", ztr_cstr(&out));
    ztr_free(&out);
    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Suite
 * ------------------------------------------------------------------------- */

SUITE(edge_cases) {
    /* NULL safety */
    RUN_TEST(free_null_is_safe);

    /* Double-free safety */
    RUN_TEST(double_free_sso_is_safe);
    RUN_TEST(double_free_heap_is_safe);

    /* Self-referential append */
    RUN_TEST(self_append_sso_doubles_content);
    RUN_TEST(self_append_heap_doubles_content);
    RUN_TEST(self_append_empty_stays_empty);

    /* Self-referential assign */
    RUN_TEST(self_assign_sso_is_noop);
    RUN_TEST(self_assign_heap_is_noop);

    /* SSO boundary */
    RUN_TEST(sso_boundary_exact_sso_cap_stays_sso);
    RUN_TEST(sso_boundary_one_over_sso_cap_goes_heap);
    RUN_TEST(sso_boundary_empty_string_is_sso);
    RUN_TEST(sso_boundary_one_byte_string_is_sso);
    RUN_TEST(sso_boundary_from_buf_at_boundary);

    /* SSO → heap transition via append */
    RUN_TEST(sso_to_heap_transition_via_append);

    /* Empty string operations */
    RUN_TEST(find_on_empty_string);
    RUN_TEST(trim_on_empty_string);
    RUN_TEST(erase_on_empty_string);
    RUN_TEST(search_predicates_on_empty_string);

    /* ztr_is_ascii */
    RUN_TEST(is_ascii_pure_ascii_string);
    RUN_TEST(is_ascii_empty_string_is_ascii);
    RUN_TEST(is_ascii_high_byte_returns_false);
    RUN_TEST(is_ascii_0xff_byte_returns_false);
    RUN_TEST(is_ascii_0x7f_is_ascii);
    RUN_TEST(is_ascii_high_byte_at_end_heap_string);

    /* Overflow / ztr_from_buf with extreme lengths */
    RUN_TEST(from_buf_len_just_over_max_returns_overflow);
    RUN_TEST(set_len_size_t_max_overflow);

    /* Additional boundary checks */
    RUN_TEST(append_from_full_sso);
    RUN_TEST(erase_count_clamped_to_tail);
    RUN_TEST(erase_at_len_is_noop);
    RUN_TEST(at_out_of_bounds_returns_nul);
    RUN_TEST(clear_heap_string_preserves_capacity);
    RUN_TEST(truncate_to_same_length_is_noop);
    RUN_TEST(truncate_to_longer_is_noop);
    RUN_TEST(cstr_always_null_terminated);

    /* ztr_substr */
    RUN_TEST(substr_from_middle);
    RUN_TEST(substr_from_start);
    RUN_TEST(substr_to_end_count_exceeds_remaining);
    RUN_TEST(substr_pos_ge_len_gives_empty);
    RUN_TEST(substr_full_string);
    RUN_TEST(substr_empty_source);
}
