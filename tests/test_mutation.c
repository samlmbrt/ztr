#include "greatest.h"
#include "ztr.h"

/* ZTR_SSO_CAP is 15 on 64-bit platforms:
 *   sizeof(char*) + sizeof(size_t) - 1  =  8 + 8 - 1  = 15
 *
 * A string of length <= 15 lives in SSO; length > 15 lives on the heap.
 * The growth policy always allocates at least 64 bytes, so after any
 * SSO-to-heap transition the capacity is >= 64.
 */

/* ================================================================
 * ztr_append
 * ================================================================ */

TEST append_basic(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_append(&s, ", world"));
    ASSERT_EQ((size_t)12, ztr_len(&s));
    ASSERT_STR_EQ("hello, world", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_to_empty(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_append(&s, "abc"));
    ASSERT_EQ((size_t)3, ztr_len(&s));
    ASSERT_STR_EQ("abc", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_null_cstr_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_append(&s, NULL));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_self(void) {
    /* ztr_append(&s, ztr_cstr(&s)) — self-referential append */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "ab"));
    ASSERT_EQ(ZTR_OK, ztr_append(&s, ztr_cstr(&s)));
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("abab", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_self_sso_boundary(void) {
    /* A 7-char string, doubling it stays <= 15 (SSO stays in SSO). */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcdefg")); /* len=7, SSO */
    ASSERT(!ztr_p_is_heap(&s));
    ASSERT_EQ(ZTR_OK, ztr_append(&s, ztr_cstr(&s)));
    ASSERT_EQ((size_t)14, ztr_len(&s));
    ASSERT_STR_EQ("abcdefgabcdefg", ztr_cstr(&s));
    ASSERT(!ztr_p_is_heap(&s)); /* still SSO */
    ztr_free(&s);
    PASS();
}

TEST append_sso_to_heap_transition(void) {
    /* Build a 15-byte string (full SSO capacity), then append 1 byte to force
     * a move to the heap. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "123456789012345")); /* len=15, SSO */
    ASSERT_EQ((size_t)15, ztr_len(&s));
    ASSERT(!ztr_p_is_heap(&s));

    ASSERT_EQ(ZTR_OK, ztr_append(&s, "X"));
    ASSERT_EQ((size_t)16, ztr_len(&s));
    ASSERT_STR_EQ("123456789012345X", ztr_cstr(&s));
    ASSERT(ztr_p_is_heap(&s)); /* now on the heap */
    ASSERT(ztr_capacity(&s) >= (size_t)16);
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_append_buf
 * ================================================================ */

TEST append_buf_basic(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "foo"));
    ASSERT_EQ(ZTR_OK, ztr_append_buf(&s, "barbaz", 3)); /* only "bar" */
    ASSERT_EQ((size_t)6, ztr_len(&s));
    ASSERT_STR_EQ("foobar", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_buf_zero_len_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_append_buf(&s, "IGNORED", 0));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_buf_with_embedded_null(void) {
    /* The library is byte-oriented; embedded nulls inside the buffer are kept.
     * The resulting string has an embedded null but ztr_len tracks real length. */
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_append_buf(&s, "ab\0cd", 5));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    /* Only the first byte after 'b' is '\0', but len is 5. */
    ASSERT_EQ('a', ztr_at(&s, 0));
    ASSERT_EQ('b', ztr_at(&s, 1));
    ASSERT_EQ('\0', ztr_at(&s, 2));
    ASSERT_EQ('c', ztr_at(&s, 3));
    ASSERT_EQ('d', ztr_at(&s, 4));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_append_ztr
 * ================================================================ */

TEST append_ztr_basic(void) {
    ztr s, t;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_from(&t, " world"));
    ASSERT_EQ(ZTR_OK, ztr_append_ztr(&s, &t));
    ASSERT_EQ((size_t)11, ztr_len(&s));
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ztr_free(&s);
    ztr_free(&t);
    PASS();
}

TEST append_ztr_self(void) {
    /* s == other — appending a string to itself */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "xy"));
    ASSERT_EQ(ZTR_OK, ztr_append_ztr(&s, &s));
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("xyxy", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_ztr_empty_other(void) {
    ztr s, t;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ztr_init(&t);
    ASSERT_EQ(ZTR_OK, ztr_append_ztr(&s, &t));
    ASSERT_STR_EQ("abc", ztr_cstr(&s));
    ztr_free(&s);
    ztr_free(&t);
    PASS();
}

TEST append_ztr_to_empty(void) {
    ztr s, t;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_from(&t, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_append_ztr(&s, &t));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    ztr_free(&t);
    PASS();
}

/* ================================================================
 * ztr_append_byte
 * ================================================================ */

TEST append_byte_basic(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hel"));
    ASSERT_EQ(ZTR_OK, ztr_append_byte(&s, 'l'));
    ASSERT_EQ(ZTR_OK, ztr_append_byte(&s, 'o'));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_byte_sso_boundary(void) {
    /* Fill SSO to capacity byte-by-byte, then push one more onto the heap. */
    ztr s;
    ztr_init(&s);
    for (int i = 0; i < (int)ZTR_SSO_CAP; i++) {
        ASSERT_EQ(ZTR_OK, ztr_append_byte(&s, 'a'));
        ASSERT(!ztr_p_is_heap(&s));
    }
    ASSERT_EQ(ZTR_SSO_CAP, ztr_len(&s));

    /* One more byte forces heap allocation. */
    ASSERT_EQ(ZTR_OK, ztr_append_byte(&s, 'z'));
    ASSERT_EQ((size_t)(ZTR_SSO_CAP + 1), ztr_len(&s));
    ASSERT(ztr_p_is_heap(&s));
    ASSERT_EQ('z', ztr_at(&s, ZTR_SSO_CAP));
    ztr_free(&s);
    PASS();
}

TEST append_byte_to_empty(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_append_byte(&s, 'X'));
    ASSERT_EQ((size_t)1, ztr_len(&s));
    ASSERT_EQ('X', ztr_at(&s, 0));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_append_fmt
 * ================================================================ */

#ifndef ZTR_NO_FMT

TEST append_fmt_basic(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "val="));
    ASSERT_EQ(ZTR_OK, ztr_append_fmt(&s, "%d", 42));
    ASSERT_STR_EQ("val=42", ztr_cstr(&s));
    ASSERT_EQ((size_t)6, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST append_fmt_multiple_args(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_append_fmt(&s, "%s=%d", "answer", 42));
    ASSERT_STR_EQ("answer=42", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_fmt_empty_result_is_noop(void) {
    /* Formatting an empty string (e.g. "%s" with "") produces 0 extra chars. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "keep"));
    ASSERT_EQ(ZTR_OK, ztr_append_fmt(&s, "%s", ""));
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("keep", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST append_fmt_forces_heap(void) {
    /* Start near SSO capacity, format enough to spill onto the heap. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "12345678901234")); /* len=14, SSO */
    ASSERT(!ztr_p_is_heap(&s));
    ASSERT_EQ(ZTR_OK, ztr_append_fmt(&s, "%s", "XY")); /* len becomes 16 */
    ASSERT(ztr_p_is_heap(&s));
    ASSERT_EQ((size_t)16, ztr_len(&s));
    ASSERT_STR_EQ("12345678901234XY", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

#endif /* ZTR_NO_FMT */

/* ================================================================
 * ztr_insert
 * ================================================================ */

TEST insert_at_beginning(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "world"));
    ASSERT_EQ(ZTR_OK, ztr_insert(&s, 0, "hello "));
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ASSERT_EQ((size_t)11, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST insert_at_end_equals_append(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_insert(&s, ztr_len(&s), " world"));
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST insert_in_middle(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "helloworld"));
    ASSERT_EQ(ZTR_OK, ztr_insert(&s, 5, " "));
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ASSERT_EQ((size_t)11, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST insert_null_cstr_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_insert(&s, 2, NULL));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST insert_self_referential(void) {
    /* Insert the string's own cstr at position 0 — content doubles at front. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "ab"));
    ASSERT_EQ(ZTR_OK, ztr_insert(&s, 0, ztr_cstr(&s)));
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("abab", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_insert_buf
 * ================================================================ */

TEST insert_buf_basic(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "ace"));
    ASSERT_EQ(ZTR_OK, ztr_insert_buf(&s, 1, "bXX", 1)); /* insert only 'b' */
    ASSERT_STR_EQ("abce", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST insert_buf_zero_len_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_insert_buf(&s, 2, "IGNORED", 0));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST insert_buf_pos_out_of_range(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi"));
    /* pos == 3, but len == 2 → ZTR_ERR_OUT_OF_RANGE */
    ASSERT_EQ(ZTR_ERR_OUT_OF_RANGE, ztr_insert_buf(&s, 3, "x", 1));
    /* String must be unchanged. */
    ASSERT_STR_EQ("hi", ztr_cstr(&s));
    ASSERT_EQ((size_t)2, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST insert_buf_at_pos_equal_to_len(void) {
    /* pos == len is allowed (equivalent to append). */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi"));
    ASSERT_EQ(ZTR_OK, ztr_insert_buf(&s, 2, "!", 1));
    ASSERT_STR_EQ("hi!", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_erase
 * ================================================================ */

TEST erase_from_start(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ztr_erase(&s, 0, 6);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("world", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST erase_from_middle(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ztr_erase(&s, 5, 6); /* remove " world" */
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST erase_from_end(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello!!!"));
    ztr_erase(&s, 5, 3);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST erase_pos_ge_len_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ztr_erase(&s, 5, 1); /* pos == len → no-op */
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_erase(&s, 99, 1); /* way out of range → no-op */
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST erase_count_exceeds_remainder_clamped(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    /* Erase from pos 2 with count=999 — should clamp to 3. */
    ztr_erase(&s, 2, 999);
    ASSERT_EQ((size_t)2, ztr_len(&s));
    ASSERT_STR_EQ("he", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST erase_entire_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ztr_erase(&s, 0, 5);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST erase_single_char_middle(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcde"));
    ztr_erase(&s, 2, 1); /* remove 'c' */
    ASSERT_STR_EQ("abde", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST erase_null_terminator_maintained(void) {
    /* After erase the string must remain properly null-terminated. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcde"));
    ztr_erase(&s, 1, 3);
    ASSERT_STR_EQ("ae", ztr_cstr(&s));
    ASSERT_EQ('\0', ztr_cstr(&s)[2]); /* explicit null-terminator check */
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_replace_first
 * ================================================================ */

TEST replace_first_found(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "foo bar foo"));
    ASSERT_EQ(ZTR_OK, ztr_replace_first(&s, "foo", "baz"));
    ASSERT_STR_EQ("baz bar foo", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_first_not_found(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_ERR_OUT_OF_RANGE, ztr_replace_first(&s, "xyz", "abc"));
    /* String must be unchanged. */
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_first_empty_needle_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_replace_first(&s, "", "X"));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_first_replacement_shorter(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(ZTR_OK, ztr_replace_first(&s, "world", "!"));
    ASSERT_STR_EQ("hello !", ztr_cstr(&s));
    ASSERT_EQ((size_t)7, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_first_replacement_same_length(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_replace_first(&s, "ello", "appy"));
    ASSERT_STR_EQ("happy", ztr_cstr(&s));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_first_replacement_longer(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi there"));
    ASSERT_EQ(ZTR_OK, ztr_replace_first(&s, "hi", "hello"));
    ASSERT_STR_EQ("hello there", ztr_cstr(&s));
    ASSERT_EQ((size_t)11, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_first_only_replaces_first_occurrence(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "aaa"));
    ASSERT_EQ(ZTR_OK, ztr_replace_first(&s, "a", "bb"));
    ASSERT_STR_EQ("bbaa", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_replace_all
 * ================================================================ */

TEST replace_all_multiple_occurrences(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "foo bar foo baz foo"));
    ASSERT_EQ(ZTR_OK, ztr_replace_all(&s, "foo", "qux"));
    ASSERT_STR_EQ("qux bar qux baz qux", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_all_no_occurrences(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(ZTR_OK, ztr_replace_all(&s, "xyz", "abc"));
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_all_empty_needle_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_replace_all(&s, "", "X"));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_all_replacement_longer(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "a_a_a"));
    ASSERT_EQ(ZTR_OK, ztr_replace_all(&s, "a", "abc"));
    ASSERT_STR_EQ("abc_abc_abc", ztr_cstr(&s));
    ASSERT_EQ((size_t)11, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_all_replacement_shorter(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "aXXbXXc"));
    ASSERT_EQ(ZTR_OK, ztr_replace_all(&s, "XX", "-"));
    ASSERT_STR_EQ("a-b-c", ztr_cstr(&s));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_all_single_occurrence(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(ZTR_OK, ztr_replace_all(&s, "world", "there"));
    ASSERT_STR_EQ("hello there", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_all_entire_string(void) {
    /* Needle equals the whole string. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_replace_all(&s, "hello", "bye"));
    ASSERT_STR_EQ("bye", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_all_adjacent_occurrences(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "aaaa"));
    ASSERT_EQ(ZTR_OK, ztr_replace_all(&s, "aa", "b"));
    ASSERT_STR_EQ("bb", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST replace_all_with_empty_replacement(void) {
    /* Replace all occurrences of needle with the empty string (deletion). */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "aXbXcX"));
    ASSERT_EQ(ZTR_OK, ztr_replace_all(&s, "X", ""));
    ASSERT_STR_EQ("abc", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_clear
 * ================================================================ */

TEST clear_sso_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT(!ztr_p_is_heap(&s));
    ztr_clear(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST clear_heap_string_preserves_capacity(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "this string is definitely longer than fifteen bytes"));
    ASSERT(ztr_p_is_heap(&s));
    size_t cap_before = ztr_capacity(&s);
    ztr_clear(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    /* Capacity should be preserved (clear does not free or reallocate). */
    ASSERT_EQ(cap_before, ztr_capacity(&s));
    ASSERT(ztr_p_is_heap(&s));
    ztr_free(&s);
    PASS();
}

TEST clear_already_empty(void) {
    ztr s;
    ztr_init(&s);
    ztr_clear(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_truncate
 * ================================================================ */

TEST truncate_shorter(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ztr_truncate(&s, 5);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ASSERT_EQ('\0', ztr_cstr(&s)[5]); /* null terminator in place */
    ztr_free(&s);
    PASS();
}

TEST truncate_same_length_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ztr_truncate(&s, 5);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST truncate_longer_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ztr_truncate(&s, 100);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST truncate_to_zero(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ztr_truncate(&s, 0);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST truncate_heap_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "this string is definitely longer than fifteen bytes"));
    ASSERT(ztr_p_is_heap(&s));
    size_t cap_before = ztr_capacity(&s);
    ztr_truncate(&s, 4);
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("this", ztr_cstr(&s));
    /* Capacity unchanged — truncate does not reallocate. */
    ASSERT_EQ(cap_before, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_reserve
 * ================================================================ */

TEST reserve_grow(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ASSERT_EQ(ZTR_OK, ztr_reserve(&s, 128));
    ASSERT(ztr_capacity(&s) >= (size_t)128);
    /* Content and length unchanged. */
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST reserve_already_sufficient_is_noop(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_reserve(&s, 128));
    size_t cap_after_reserve = ztr_capacity(&s);
    /* Reserving <= current capacity should leave capacity unchanged. */
    ASSERT_EQ(ZTR_OK, ztr_reserve(&s, 16));
    ASSERT_EQ(cap_after_reserve, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST reserve_sso_to_heap(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi")); /* SSO */
    ASSERT(!ztr_p_is_heap(&s));
    ASSERT_EQ(ZTR_OK, ztr_reserve(&s, 64));
    ASSERT(ztr_p_is_heap(&s));
    ASSERT(ztr_capacity(&s) >= (size_t)64);
    /* Content must survive the transition. */
    ASSERT_STR_EQ("hi", ztr_cstr(&s));
    ASSERT_EQ((size_t)2, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_shrink_to_fit
 * ================================================================ */

TEST shrink_to_fit_heap_to_sso(void) {
    /* Reserve a large capacity, then truncate content to <= SSO_CAP.
     * shrink_to_fit must move the string back into SSO. */
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_reserve(&s, 256)); /* force heap */
    ASSERT(ztr_p_is_heap(&s));
    ASSERT_EQ(ZTR_OK, ztr_assign(&s, "short")); /* 5 chars — fits in SSO */
    ASSERT(ztr_p_is_heap(&s));                  /* still on heap after assign */
    ASSERT(ztr_capacity(&s) >= (size_t)256);

    ztr_shrink_to_fit(&s);

    /* Slack was large (>= 16), content fits in SSO → transition back. */
    ASSERT(!ztr_p_is_heap(&s));
    ASSERT_STR_EQ("short", ztr_cstr(&s));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST shrink_to_fit_small_slack_is_noop(void) {
    /* Build a heap string whose cap == len (no slack).
     * shrink_to_fit should leave it untouched since slack < 16. */
    ztr s;
    /* from_buf with len > SSO_CAP puts the string on the heap with cap == len. */
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "this string is definitely longer than fifteen bytes"));
    ASSERT(ztr_p_is_heap(&s));
    size_t len = ztr_len(&s);
    size_t cap = ztr_capacity(&s);

    /* cap == len immediately after from(), so slack == 0 < 16. */
    ASSERT_EQ(len, cap);

    ztr_shrink_to_fit(&s);

    /* Nothing should change. */
    ASSERT(ztr_p_is_heap(&s));
    ASSERT_EQ(len, ztr_len(&s));
    ASSERT_EQ(cap, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST shrink_to_fit_sso_string_is_noop(void) {
    /* If the string is already in SSO, shrink_to_fit is a no-op. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi"));
    ASSERT(!ztr_p_is_heap(&s));
    ztr_shrink_to_fit(&s);
    ASSERT(!ztr_p_is_heap(&s));
    ASSERT_STR_EQ("hi", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST shrink_to_fit_large_heap_content(void) {
    /* Content stays on heap but allocation is trimmed.
     * We need len > SSO_CAP and slack >= 16. */
    ztr s;
    ztr_init(&s);
    /* Reserve 256, then put a 32-char string in — slack = 224 >= 16. */
    ASSERT_EQ(ZTR_OK, ztr_reserve(&s, 256));
    ASSERT_EQ(ZTR_OK, ztr_assign(&s, "12345678901234567890123456789012")); /* 32 chars */
    ASSERT(ztr_p_is_heap(&s));
    ASSERT(ztr_capacity(&s) >= (size_t)256);

    ztr_shrink_to_fit(&s);

    /* Still on heap (len > SSO_CAP), content preserved, cap tighter. */
    ASSERT(ztr_p_is_heap(&s));
    ASSERT_EQ((size_t)32, ztr_len(&s));
    ASSERT_STR_EQ("12345678901234567890123456789012", ztr_cstr(&s));
    ASSERT(ztr_capacity(&s) < (size_t)256);
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * Suite
 * ================================================================ */

SUITE(mutation) {
    /* ztr_append */
    RUN_TEST(append_basic);
    RUN_TEST(append_to_empty);
    RUN_TEST(append_null_cstr_is_noop);
    RUN_TEST(append_self);
    RUN_TEST(append_self_sso_boundary);
    RUN_TEST(append_sso_to_heap_transition);

    /* ztr_append_buf */
    RUN_TEST(append_buf_basic);
    RUN_TEST(append_buf_zero_len_is_noop);
    RUN_TEST(append_buf_with_embedded_null);

    /* ztr_append_ztr */
    RUN_TEST(append_ztr_basic);
    RUN_TEST(append_ztr_self);
    RUN_TEST(append_ztr_empty_other);
    RUN_TEST(append_ztr_to_empty);

    /* ztr_append_byte */
    RUN_TEST(append_byte_basic);
    RUN_TEST(append_byte_sso_boundary);
    RUN_TEST(append_byte_to_empty);

#ifndef ZTR_NO_FMT
    /* ztr_append_fmt */
    RUN_TEST(append_fmt_basic);
    RUN_TEST(append_fmt_multiple_args);
    RUN_TEST(append_fmt_empty_result_is_noop);
    RUN_TEST(append_fmt_forces_heap);
#endif

    /* ztr_insert */
    RUN_TEST(insert_at_beginning);
    RUN_TEST(insert_at_end_equals_append);
    RUN_TEST(insert_in_middle);
    RUN_TEST(insert_null_cstr_is_noop);
    RUN_TEST(insert_self_referential);

    /* ztr_insert_buf */
    RUN_TEST(insert_buf_basic);
    RUN_TEST(insert_buf_zero_len_is_noop);
    RUN_TEST(insert_buf_pos_out_of_range);
    RUN_TEST(insert_buf_at_pos_equal_to_len);

    /* ztr_erase */
    RUN_TEST(erase_from_start);
    RUN_TEST(erase_from_middle);
    RUN_TEST(erase_from_end);
    RUN_TEST(erase_pos_ge_len_is_noop);
    RUN_TEST(erase_count_exceeds_remainder_clamped);
    RUN_TEST(erase_entire_string);
    RUN_TEST(erase_single_char_middle);
    RUN_TEST(erase_null_terminator_maintained);

    /* ztr_replace_first */
    RUN_TEST(replace_first_found);
    RUN_TEST(replace_first_not_found);
    RUN_TEST(replace_first_empty_needle_is_noop);
    RUN_TEST(replace_first_replacement_shorter);
    RUN_TEST(replace_first_replacement_same_length);
    RUN_TEST(replace_first_replacement_longer);
    RUN_TEST(replace_first_only_replaces_first_occurrence);

    /* ztr_replace_all */
    RUN_TEST(replace_all_multiple_occurrences);
    RUN_TEST(replace_all_no_occurrences);
    RUN_TEST(replace_all_empty_needle_is_noop);
    RUN_TEST(replace_all_replacement_longer);
    RUN_TEST(replace_all_replacement_shorter);
    RUN_TEST(replace_all_single_occurrence);
    RUN_TEST(replace_all_entire_string);
    RUN_TEST(replace_all_adjacent_occurrences);
    RUN_TEST(replace_all_with_empty_replacement);

    /* ztr_clear */
    RUN_TEST(clear_sso_string);
    RUN_TEST(clear_heap_string_preserves_capacity);
    RUN_TEST(clear_already_empty);

    /* ztr_truncate */
    RUN_TEST(truncate_shorter);
    RUN_TEST(truncate_same_length_is_noop);
    RUN_TEST(truncate_longer_is_noop);
    RUN_TEST(truncate_to_zero);
    RUN_TEST(truncate_heap_string);

    /* ztr_reserve */
    RUN_TEST(reserve_grow);
    RUN_TEST(reserve_already_sufficient_is_noop);
    RUN_TEST(reserve_sso_to_heap);

    /* ztr_shrink_to_fit */
    RUN_TEST(shrink_to_fit_heap_to_sso);
    RUN_TEST(shrink_to_fit_small_slack_is_noop);
    RUN_TEST(shrink_to_fit_sso_string_is_noop);
    RUN_TEST(shrink_to_fit_large_heap_content);
}
