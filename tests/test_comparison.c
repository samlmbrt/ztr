#include "greatest.h"
#include "ztr.h"

/* ------------------------------------------------------------------ */
/* ztr_eq                                                               */
/* ------------------------------------------------------------------ */

TEST eq_identical_sso_strings(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "hello"));
    ASSERT(ztr_eq(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST eq_different_sso_strings(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "hello"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "world"));
    ASSERT_FALSE(ztr_eq(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST eq_different_lengths(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "abc"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "abcd"));
    ASSERT_FALSE(ztr_eq(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST eq_both_empty(void) {
    ztr a, b;
    ztr_init(&a);
    ztr_init(&b);
    ASSERT(ztr_eq(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST eq_one_empty_one_nonempty(void) {
    ztr a, b;
    ztr_init(&a);
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "x"));
    ASSERT_FALSE(ztr_eq(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST eq_both_heap_same_content(void) {
    /* Both strings are heap-allocated (longer than ZTR_SSO_CAP).
     * Verify that two distinct heap strings with identical content
     * compare equal. */
    const char *long_str = "this string is definitely longer than the sso buffer";
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, long_str));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, long_str));
    /* Both on heap; confirm equal content. */
    ASSERT(ztr_p_is_heap(&a));
    ASSERT(ztr_p_is_heap(&b));
    ASSERT(ztr_eq(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST eq_short_vs_long_same_prefix(void) {
    /* "abcde" != "abcdef" */
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "abcde"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "abcdef"));
    ASSERT_FALSE(ztr_eq(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST eq_self_reference(void) {
    ztr a;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "same"));
    ASSERT(ztr_eq(&a, &a));
    ztr_free(&a);
    PASS();
}

/* ------------------------------------------------------------------ */
/* ztr_eq_cstr                                                          */
/* ------------------------------------------------------------------ */

TEST eq_cstr_match(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT(ztr_eq_cstr(&s, "hello"));
    ztr_free(&s);
    PASS();
}

TEST eq_cstr_no_match(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_FALSE(ztr_eq_cstr(&s, "world"));
    ztr_free(&s);
    PASS();
}

TEST eq_cstr_null_cstr(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    /* NULL cstr must not match a non-empty string. */
    ASSERT_FALSE(ztr_eq_cstr(&s, NULL));
    ztr_free(&s);
    PASS();
}

TEST eq_cstr_empty_ztr_vs_empty_cstr(void) {
    ztr s;
    ztr_init(&s);
    ASSERT(ztr_eq_cstr(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST eq_cstr_empty_ztr_vs_nonempty_cstr(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_FALSE(ztr_eq_cstr(&s, "a"));
    ztr_free(&s);
    PASS();
}

TEST eq_cstr_nonempty_ztr_vs_empty_cstr(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "a"));
    ASSERT_FALSE(ztr_eq_cstr(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST eq_cstr_prefix_not_equal(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ASSERT_FALSE(ztr_eq_cstr(&s, "ab"));
    ztr_free(&s);
    PASS();
}

/* ------------------------------------------------------------------ */
/* ztr_cmp                                                              */
/* ------------------------------------------------------------------ */

TEST cmp_equal_strings(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "abc"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "abc"));
    ASSERT_EQ(0, ztr_cmp(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST cmp_less_than(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "abc"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "abd"));
    ASSERT_LT(ztr_cmp(&a, &b), 0);
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST cmp_greater_than(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "abd"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "abc"));
    ASSERT_GT(ztr_cmp(&a, &b), 0);
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST cmp_prefix_is_less(void) {
    /* "abc" < "abcd" */
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "abc"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "abcd"));
    ASSERT_LT(ztr_cmp(&a, &b), 0);
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST cmp_longer_is_greater(void) {
    /* "abcd" > "abc" */
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "abcd"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "abc"));
    ASSERT_GT(ztr_cmp(&a, &b), 0);
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST cmp_both_empty(void) {
    ztr a, b;
    ztr_init(&a);
    ztr_init(&b);
    ASSERT_EQ(0, ztr_cmp(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST cmp_empty_vs_nonempty(void) {
    ztr a, b;
    ztr_init(&a);
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "a"));
    ASSERT_LT(ztr_cmp(&a, &b), 0);
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST cmp_nonempty_vs_empty(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "a"));
    ztr_init(&b);
    ASSERT_GT(ztr_cmp(&a, &b), 0);
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST cmp_antisymmetric(void) {
    /* cmp(a, b) and cmp(b, a) must have opposite signs. */
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "cat"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "dog"));
    int ab = ztr_cmp(&a, &b);
    int ba = ztr_cmp(&b, &a);
    ASSERT_LT(ab, 0);
    ASSERT_GT(ba, 0);
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

/* ------------------------------------------------------------------ */
/* ztr_cmp_cstr                                                         */
/* ------------------------------------------------------------------ */

TEST cmp_cstr_equal(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ASSERT_EQ(0, ztr_cmp_cstr(&s, "abc"));
    ztr_free(&s);
    PASS();
}

TEST cmp_cstr_less_than(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ASSERT_LT(ztr_cmp_cstr(&s, "abd"), 0);
    ztr_free(&s);
    PASS();
}

TEST cmp_cstr_greater_than(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abd"));
    ASSERT_GT(ztr_cmp_cstr(&s, "abc"), 0);
    ztr_free(&s);
    PASS();
}

TEST cmp_cstr_prefix(void) {
    /* "abc" < "abcd" */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ASSERT_LT(ztr_cmp_cstr(&s, "abcd"), 0);
    ztr_free(&s);
    PASS();
}

TEST cmp_cstr_longer_than_cstr(void) {
    /* "abcd" > "abc" */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcd"));
    ASSERT_GT(ztr_cmp_cstr(&s, "abc"), 0);
    ztr_free(&s);
    PASS();
}

TEST cmp_cstr_null(void) {
    /* NULL cstr: a non-empty string must compare as greater than nothing. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_GT(ztr_cmp_cstr(&s, NULL), 0);
    ztr_free(&s);
    PASS();
}

TEST cmp_cstr_empty_ztr_vs_empty_cstr(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(0, ztr_cmp_cstr(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST cmp_cstr_empty_ztr_vs_nonempty_cstr(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_LT(ztr_cmp_cstr(&s, "a"), 0);
    ztr_free(&s);
    PASS();
}

/* ------------------------------------------------------------------ */
/* ztr_eq_ascii_nocase                                                  */
/* ------------------------------------------------------------------ */

TEST nocase_same_case(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "Hello"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "Hello"));
    ASSERT(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST nocase_upper_vs_lower(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "HELLO"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "hello"));
    ASSERT(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST nocase_mixed_case(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "HeLLo WoRLd"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "hello world"));
    ASSERT(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST nocase_different_strings(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "HELLO"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "WORLD"));
    ASSERT_FALSE(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST nocase_different_lengths(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "ABC"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "ABCD"));
    ASSERT_FALSE(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST nocase_both_empty(void) {
    ztr a, b;
    ztr_init(&a);
    ztr_init(&b);
    ASSERT(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST nocase_non_ascii_bytes_unchanged(void) {
    /* Bytes > 127 are not ASCII letters; the function must treat them as
     * opaque and compare them byte-for-byte (or at least consistently).
     * Two strings that differ only in a non-ASCII byte must NOT compare equal. */
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from_buf(&a, "caf\xc3\xa9", 5)); /* "café" in UTF-8 */
    ASSERT_EQ(ZTR_OK, ztr_from_buf(&b, "caf\xc3\xa8", 5)); /* "cafè" in UTF-8 */
    ASSERT_FALSE(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST nocase_non_ascii_bytes_same(void) {
    /* Same non-ASCII content must compare equal. */
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from_buf(&a, "caf\xc3\xa9", 5));
    ASSERT_EQ(ZTR_OK, ztr_from_buf(&b, "caf\xc3\xa9", 5));
    ASSERT(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST nocase_digits_and_symbols_unchanged(void) {
    /* Digits and punctuation have no case; identical strings must be equal. */
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "abc123!@#"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "ABC123!@#"));
    ASSERT(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

TEST nocase_all_uppercase(void) {
    ztr a, b;
    ASSERT_EQ(ZTR_OK, ztr_from(&a, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
    ASSERT_EQ(ZTR_OK, ztr_from(&b, "abcdefghijklmnopqrstuvwxyz"));
    ASSERT(ztr_eq_ascii_nocase(&a, &b));
    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

/* ------------------------------------------------------------------ */
/* Suite                                                                */
/* ------------------------------------------------------------------ */

SUITE(comparison) {
    /* ztr_eq */
    RUN_TEST(eq_identical_sso_strings);
    RUN_TEST(eq_different_sso_strings);
    RUN_TEST(eq_different_lengths);
    RUN_TEST(eq_both_empty);
    RUN_TEST(eq_one_empty_one_nonempty);
    RUN_TEST(eq_both_heap_same_content);
    RUN_TEST(eq_short_vs_long_same_prefix);
    RUN_TEST(eq_self_reference);

    /* ztr_eq_cstr */
    RUN_TEST(eq_cstr_match);
    RUN_TEST(eq_cstr_no_match);
    RUN_TEST(eq_cstr_null_cstr);
    RUN_TEST(eq_cstr_empty_ztr_vs_empty_cstr);
    RUN_TEST(eq_cstr_empty_ztr_vs_nonempty_cstr);
    RUN_TEST(eq_cstr_nonempty_ztr_vs_empty_cstr);
    RUN_TEST(eq_cstr_prefix_not_equal);

    /* ztr_cmp */
    RUN_TEST(cmp_equal_strings);
    RUN_TEST(cmp_less_than);
    RUN_TEST(cmp_greater_than);
    RUN_TEST(cmp_prefix_is_less);
    RUN_TEST(cmp_longer_is_greater);
    RUN_TEST(cmp_both_empty);
    RUN_TEST(cmp_empty_vs_nonempty);
    RUN_TEST(cmp_nonempty_vs_empty);
    RUN_TEST(cmp_antisymmetric);

    /* ztr_cmp_cstr */
    RUN_TEST(cmp_cstr_equal);
    RUN_TEST(cmp_cstr_less_than);
    RUN_TEST(cmp_cstr_greater_than);
    RUN_TEST(cmp_cstr_prefix);
    RUN_TEST(cmp_cstr_longer_than_cstr);
    RUN_TEST(cmp_cstr_null);
    RUN_TEST(cmp_cstr_empty_ztr_vs_empty_cstr);
    RUN_TEST(cmp_cstr_empty_ztr_vs_nonempty_cstr);

    /* ztr_eq_ascii_nocase */
    RUN_TEST(nocase_same_case);
    RUN_TEST(nocase_upper_vs_lower);
    RUN_TEST(nocase_mixed_case);
    RUN_TEST(nocase_different_strings);
    RUN_TEST(nocase_different_lengths);
    RUN_TEST(nocase_both_empty);
    RUN_TEST(nocase_non_ascii_bytes_unchanged);
    RUN_TEST(nocase_non_ascii_bytes_same);
    RUN_TEST(nocase_digits_and_symbols_unchanged);
    RUN_TEST(nocase_all_uppercase);
}
