#include "greatest.h"
#include "ztr.h"

/* ztr_p_is_ascii_space matches: ' ', '\t', '\n', '\r', '\v', '\f' */

/* ================================================================
 * ztr_trim
 * ================================================================ */

TEST trim_leading_and_trailing_spaces(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "   hello world   "));
    ztr_trim(&s);
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ASSERT_EQ((size_t)11, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_leading_and_trailing_tabs(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "\t\thello\t\t"));
    ztr_trim(&s);
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_mixed_whitespace(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, " \t\r\n hello \n\t "));
    ztr_trim(&s);
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_all_whitespace_characters(void) {
    /* Every kind of ASCII whitespace used by ztr_p_is_ascii_space. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, " \t\n\r\v\f"));
    ztr_trim(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_no_whitespace_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ztr_trim(&s);
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_empty_string_is_noop(void) {
    ztr s;
    ztr_init(&s);
    ztr_trim(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_only_leading_whitespace(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "   abc"));
    ztr_trim(&s);
    ASSERT_STR_EQ("abc", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_only_trailing_whitespace(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc   "));
    ztr_trim(&s);
    ASSERT_STR_EQ("abc", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_preserves_interior_whitespace(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "  hello   world  "));
    ztr_trim(&s);
    ASSERT_STR_EQ("hello   world", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_single_space(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, " "));
    ztr_trim(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_heap_string(void) {
    /* Use a string that starts on the heap (len > ZTR_SSO_CAP). */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "   this string is longer than the sso capacity   "));
    ASSERT(ztr_p_is_heap(&s));
    ztr_trim(&s);
    ASSERT_STR_EQ("this string is longer than the sso capacity", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_trim_start
 * ================================================================ */

TEST trim_start_removes_only_leading(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "   hello   "));
    ztr_trim_start(&s);
    ASSERT_STR_EQ("hello   ", ztr_cstr(&s));
    ASSERT_EQ((size_t)8, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_start_no_leading_whitespace_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello   "));
    ztr_trim_start(&s);
    ASSERT_STR_EQ("hello   ", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_start_all_whitespace(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "   "));
    ztr_trim_start(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_start_empty_string_is_noop(void) {
    ztr s;
    ztr_init(&s);
    ztr_trim_start(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_start_mixed_whitespace_leading(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "\t \n\r abc\t"));
    ztr_trim_start(&s);
    ASSERT_STR_EQ("abc\t", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_start_does_not_touch_trailing(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "  abc  "));
    ztr_trim_start(&s);
    /* Leading stripped, trailing preserved. */
    ASSERT_STR_EQ("abc  ", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_start_single_leading_space(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, " x"));
    ztr_trim_start(&s);
    ASSERT_STR_EQ("x", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_trim_end
 * ================================================================ */

TEST trim_end_removes_only_trailing(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "   hello   "));
    ztr_trim_end(&s);
    ASSERT_STR_EQ("   hello", ztr_cstr(&s));
    ASSERT_EQ((size_t)8, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_end_no_trailing_whitespace_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "   hello"));
    ztr_trim_end(&s);
    ASSERT_STR_EQ("   hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_end_all_whitespace(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "   "));
    ztr_trim_end(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_end_empty_string_is_noop(void) {
    ztr s;
    ztr_init(&s);
    ztr_trim_end(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_end_mixed_whitespace_trailing(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc\t \n\r"));
    ztr_trim_end(&s);
    ASSERT_STR_EQ("abc", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_end_does_not_touch_leading(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "  abc  "));
    ztr_trim_end(&s);
    /* Trailing stripped, leading preserved. */
    ASSERT_STR_EQ("  abc", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST trim_end_single_trailing_space(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "x "));
    ztr_trim_end(&s);
    ASSERT_STR_EQ("x", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_to_ascii_upper
 * ================================================================ */

TEST to_upper_lowercase_to_upper(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ztr_to_ascii_upper(&s);
    ASSERT_STR_EQ("HELLO WORLD", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_upper_already_upper_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "HELLO"));
    ztr_to_ascii_upper(&s);
    ASSERT_STR_EQ("HELLO", ztr_cstr(&s));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST to_upper_mixed_case(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "HeLLo WoRLd"));
    ztr_to_ascii_upper(&s);
    ASSERT_STR_EQ("HELLO WORLD", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_upper_non_ascii_bytes_untouched(void) {
    /* Bytes outside [a-z] must not be modified.
     * Use bytes >= 128 which are neither ASCII upper nor lower. */
    ztr s;
    const char input[] = {'a', 'b', '\xC0', '\xFF', 'c', '\0'};
    const char expect[] = {'A', 'B', '\xC0', '\xFF', 'C', '\0'};
    ASSERT_EQ(ZTR_OK, ztr_from(&s, input));
    ztr_to_ascii_upper(&s);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_MEM_EQ(expect, ztr_cstr(&s), 5);
    ztr_free(&s);
    PASS();
}

TEST to_upper_digits_and_symbols_untouched(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc123!@#"));
    ztr_to_ascii_upper(&s);
    ASSERT_STR_EQ("ABC123!@#", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_upper_empty_string_is_noop(void) {
    ztr s;
    ztr_init(&s);
    ztr_to_ascii_upper(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_upper_single_char(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "a"));
    ztr_to_ascii_upper(&s);
    ASSERT_STR_EQ("A", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_upper_heap_string(void) {
    /* Verify the function works correctly on heap-allocated strings. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcdefghijklmnopqrstuvwxyz abcdefghijklmnop"));
    ASSERT(ztr_p_is_heap(&s));
    ztr_to_ascii_upper(&s);
    ASSERT_STR_EQ("ABCDEFGHIJKLMNOPQRSTUVWXYZ ABCDEFGHIJKLMNOP", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_upper_boundary_chars(void) {
    /* Test exact boundary bytes: 'a'-1, 'a', 'z', 'z'+1 */
    ztr s;
    const char input[] = {'`', 'a', 'z', '{', '\0'}; /* '`'=0x60, '{'=0x7B */
    const char expect[] = {'`', 'A', 'Z', '{', '\0'};
    ASSERT_EQ(ZTR_OK, ztr_from(&s, input));
    ztr_to_ascii_upper(&s);
    ASSERT_MEM_EQ(expect, ztr_cstr(&s), 4);
    ztr_free(&s);
    PASS();
}

/* ================================================================
 * ztr_to_ascii_lower
 * ================================================================ */

TEST to_lower_uppercase_to_lower(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "HELLO WORLD"));
    ztr_to_ascii_lower(&s);
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_lower_already_lower_is_noop(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ztr_to_ascii_lower(&s);
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST to_lower_mixed_case(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "HeLLo WoRLd"));
    ztr_to_ascii_lower(&s);
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_lower_non_ascii_bytes_untouched(void) {
    ztr s;
    const char input[] = {'A', 'B', '\xC0', '\xFF', 'C', '\0'};
    const char expect[] = {'a', 'b', '\xC0', '\xFF', 'c', '\0'};
    ASSERT_EQ(ZTR_OK, ztr_from(&s, input));
    ztr_to_ascii_lower(&s);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_MEM_EQ(expect, ztr_cstr(&s), 5);
    ztr_free(&s);
    PASS();
}

TEST to_lower_digits_and_symbols_untouched(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "ABC123!@#"));
    ztr_to_ascii_lower(&s);
    ASSERT_STR_EQ("abc123!@#", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_lower_empty_string_is_noop(void) {
    ztr s;
    ztr_init(&s);
    ztr_to_ascii_lower(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_lower_single_char(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "Z"));
    ztr_to_ascii_lower(&s);
    ASSERT_STR_EQ("z", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_lower_heap_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "ABCDEFGHIJKLMNOPQRSTUVWXYZ ABCDEFGHIJKLMNOP"));
    ASSERT(ztr_p_is_heap(&s));
    ztr_to_ascii_lower(&s);
    ASSERT_STR_EQ("abcdefghijklmnopqrstuvwxyz abcdefghijklmnop", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST to_lower_boundary_chars(void) {
    /* Test exact boundary bytes: 'A'-1, 'A', 'Z', 'Z'+1 */
    ztr s;
    const char input[] = {'@', 'A', 'Z', '[', '\0'}; /* '@'=0x40, '['=0x5B */
    const char expect[] = {'@', 'a', 'z', '[', '\0'};
    ASSERT_EQ(ZTR_OK, ztr_from(&s, input));
    ztr_to_ascii_lower(&s);
    ASSERT_MEM_EQ(expect, ztr_cstr(&s), 4);
    ztr_free(&s);
    PASS();
}

TEST to_lower_roundtrip_with_upper(void) {
    /* to_lower(to_upper(s)) == to_lower(s) for ASCII strings. */
    ztr s, t;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "Hello World 123"));
    ASSERT_EQ(ZTR_OK, ztr_clone(&t, &s));
    ztr_to_ascii_upper(&s);
    ztr_to_ascii_lower(&s);
    ztr_to_ascii_lower(&t);
    ASSERT(ztr_eq(&s, &t));
    ztr_free(&s);
    ztr_free(&t);
    PASS();
}

/* ================================================================
 * Suite
 * ================================================================ */

SUITE(transform) {
    /* ztr_trim */
    RUN_TEST(trim_leading_and_trailing_spaces);
    RUN_TEST(trim_leading_and_trailing_tabs);
    RUN_TEST(trim_mixed_whitespace);
    RUN_TEST(trim_all_whitespace_characters);
    RUN_TEST(trim_no_whitespace_is_noop);
    RUN_TEST(trim_empty_string_is_noop);
    RUN_TEST(trim_only_leading_whitespace);
    RUN_TEST(trim_only_trailing_whitespace);
    RUN_TEST(trim_preserves_interior_whitespace);
    RUN_TEST(trim_single_space);
    RUN_TEST(trim_heap_string);

    /* ztr_trim_start */
    RUN_TEST(trim_start_removes_only_leading);
    RUN_TEST(trim_start_no_leading_whitespace_is_noop);
    RUN_TEST(trim_start_all_whitespace);
    RUN_TEST(trim_start_empty_string_is_noop);
    RUN_TEST(trim_start_mixed_whitespace_leading);
    RUN_TEST(trim_start_does_not_touch_trailing);
    RUN_TEST(trim_start_single_leading_space);

    /* ztr_trim_end */
    RUN_TEST(trim_end_removes_only_trailing);
    RUN_TEST(trim_end_no_trailing_whitespace_is_noop);
    RUN_TEST(trim_end_all_whitespace);
    RUN_TEST(trim_end_empty_string_is_noop);
    RUN_TEST(trim_end_mixed_whitespace_trailing);
    RUN_TEST(trim_end_does_not_touch_leading);
    RUN_TEST(trim_end_single_trailing_space);

    /* ztr_to_ascii_upper */
    RUN_TEST(to_upper_lowercase_to_upper);
    RUN_TEST(to_upper_already_upper_is_noop);
    RUN_TEST(to_upper_mixed_case);
    RUN_TEST(to_upper_non_ascii_bytes_untouched);
    RUN_TEST(to_upper_digits_and_symbols_untouched);
    RUN_TEST(to_upper_empty_string_is_noop);
    RUN_TEST(to_upper_single_char);
    RUN_TEST(to_upper_heap_string);
    RUN_TEST(to_upper_boundary_chars);

    /* ztr_to_ascii_lower */
    RUN_TEST(to_lower_uppercase_to_lower);
    RUN_TEST(to_lower_already_lower_is_noop);
    RUN_TEST(to_lower_mixed_case);
    RUN_TEST(to_lower_non_ascii_bytes_untouched);
    RUN_TEST(to_lower_digits_and_symbols_untouched);
    RUN_TEST(to_lower_empty_string_is_noop);
    RUN_TEST(to_lower_single_char);
    RUN_TEST(to_lower_heap_string);
    RUN_TEST(to_lower_boundary_chars);
    RUN_TEST(to_lower_roundtrip_with_upper);
}
