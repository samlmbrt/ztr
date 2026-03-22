#include "greatest.h"
#include "ztr.h"

#include <string.h>

/* ---- Helpers ---- */

/* Build a ztr from a C string literal for convenience. Caller must ztr_free. */
static ztr make(const char *cstr) {
    ztr s;
    ztr_from(&s, cstr);
    return s;
}

/* ---- ztr_split_begin / ztr_split_next ---- */

TEST split_iter_basic(void) {
    ztr s = make("a,b,c");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, ",");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("a", part, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("b", part, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("c", part, part_len);

    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

TEST split_iter_empty_parts(void) {
    /* "a,,b" should yield "a", "", "b" */
    ztr s = make("a,,b");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, ",");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("a", part, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(0, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("b", part, part_len);

    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

TEST split_iter_single_part_no_delimiter(void) {
    /* No delimiter present — whole string is the only part. */
    ztr s = make("hello");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, ",");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(5, part_len);
    ASSERT_STRN_EQ("hello", part, part_len);

    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

TEST split_iter_empty_source(void) {
    /* Splitting an empty string on "," yields one empty part. */
    ztr s = make("");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, ",");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(0, part_len);

    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

TEST split_iter_empty_delimiter(void) {
    /* Empty delimiter — iterator should yield the whole string as one part. */
    ztr s = make("hello");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, "");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(5, part_len);
    ASSERT_STRN_EQ("hello", part, part_len);

    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

TEST split_iter_trailing_delimiter(void) {
    /* "a,b," should yield "a", "b", "" */
    ztr s = make("a,b,");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, ",");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("a", part, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("b", part, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(0, part_len);

    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

TEST split_iter_leading_delimiter(void) {
    /* ",a,b" should yield "", "a", "b" */
    ztr s = make(",a,b");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, ",");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(0, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("a", part, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("b", part, part_len);

    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

TEST split_iter_multi_char_delimiter(void) {
    /* "a::b::c" with "::" as delimiter */
    ztr s = make("a::b::c");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, "::");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("a", part, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("b", part, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(1, part_len);
    ASSERT_STRN_EQ("c", part, part_len);

    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

TEST split_iter_only_delimiter(void) {
    /* "," should yield "", "" */
    ztr s = make(",");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, ",");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(0, part_len);

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_EQ(0, part_len);

    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

TEST split_iter_repeated_calls_after_exhaustion(void) {
    /* Calling ztr_split_next after it returns false should keep returning false. */
    ztr s = make("x");
    ztr_split_iter it;
    const char *part;
    size_t part_len;

    ztr_split_begin(&it, &s, ",");

    ASSERT(ztr_split_next(&it, &part, &part_len));
    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));
    ASSERT_FALSE(ztr_split_next(&it, &part, &part_len));

    ztr_free(&s);
    PASS();
}

/* ---- ztr_split_alloc ---- */

TEST split_alloc_basic(void) {
    ztr s = make("one,two,three");
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 0));
    ASSERT_EQ(3, count);
    ASSERT(parts != NULL);

    ASSERT(ztr_eq_cstr(&parts[0], "one"));
    ASSERT(ztr_eq_cstr(&parts[1], "two"));
    ASSERT(ztr_eq_cstr(&parts[2], "three"));

    ztr_split_free(parts, count);
    ztr_free(&s);
    PASS();
}

TEST split_alloc_max_parts_zero_means_unlimited(void) {
    ztr s = make("a,b,c,d,e");
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 0));
    ASSERT_EQ(5, count);

    ztr_split_free(parts, count);
    ztr_free(&s);
    PASS();
}

TEST split_alloc_max_parts_limits_and_puts_remainder_in_last(void) {
    /* max_parts=2: first part is "one", second is the remainder "two,three" */
    ztr s = make("one,two,three");
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 2));
    ASSERT_EQ(2, count);
    ASSERT(parts != NULL);

    ASSERT(ztr_eq_cstr(&parts[0], "one"));
    ASSERT(ztr_eq_cstr(&parts[1], "two,three"));

    ztr_split_free(parts, count);
    ztr_free(&s);
    PASS();
}

TEST split_alloc_max_parts_larger_than_actual(void) {
    /* max_parts=10 on a 3-part string should just give all 3 parts */
    ztr s = make("x,y,z");
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 10));
    ASSERT_EQ(3, count);

    ASSERT(ztr_eq_cstr(&parts[0], "x"));
    ASSERT(ztr_eq_cstr(&parts[1], "y"));
    ASSERT(ztr_eq_cstr(&parts[2], "z"));

    ztr_split_free(parts, count);
    ztr_free(&s);
    PASS();
}

TEST split_alloc_empty_parts(void) {
    ztr s = make("a,,b");
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 0));
    ASSERT_EQ(3, count);

    ASSERT(ztr_eq_cstr(&parts[0], "a"));
    ASSERT(ztr_eq_cstr(&parts[1], ""));
    ASSERT(ztr_eq_cstr(&parts[2], "b"));

    ztr_split_free(parts, count);
    ztr_free(&s);
    PASS();
}

TEST split_alloc_trailing_delimiter(void) {
    ztr s = make("a,b,");
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 0));
    ASSERT_EQ(3, count);

    ASSERT(ztr_eq_cstr(&parts[0], "a"));
    ASSERT(ztr_eq_cstr(&parts[1], "b"));
    ASSERT(ztr_eq_cstr(&parts[2], ""));

    ztr_split_free(parts, count);
    ztr_free(&s);
    PASS();
}

TEST split_alloc_no_delimiter_in_source(void) {
    ztr s = make("hello");
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 0));
    ASSERT_EQ(1, count);
    ASSERT(ztr_eq_cstr(&parts[0], "hello"));

    ztr_split_free(parts, count);
    ztr_free(&s);
    PASS();
}

TEST split_alloc_empty_source(void) {
    ztr s = make("");
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 0));
    ASSERT_EQ(1, count);
    ASSERT(ztr_eq_cstr(&parts[0], ""));

    ztr_split_free(parts, count);
    ztr_free(&s);
    PASS();
}

TEST split_alloc_max_parts_one(void) {
    /* max_parts=1 should yield the whole string as a single part */
    ztr s = make("a,b,c");
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 1));
    ASSERT_EQ(1, count);
    ASSERT(ztr_eq_cstr(&parts[0], "a,b,c"));

    ztr_split_free(parts, count);
    ztr_free(&s);
    PASS();
}

/* ---- ztr_join ---- */

TEST join_basic(void) {
    ztr parts[3];
    ztr_from(&parts[0], "alpha");
    ztr_from(&parts[1], "beta");
    ztr_from(&parts[2], "gamma");

    ztr out;
    ztr_init(&out);
    ASSERT_EQ(ZTR_OK, ztr_join(&out, parts, 3, ", "));
    ASSERT(ztr_eq_cstr(&out, "alpha, beta, gamma"));

    ztr_free(&parts[0]);
    ztr_free(&parts[1]);
    ztr_free(&parts[2]);
    ztr_free(&out);
    PASS();
}

TEST join_empty_separator(void) {
    ztr parts[3];
    ztr_from(&parts[0], "foo");
    ztr_from(&parts[1], "bar");
    ztr_from(&parts[2], "baz");

    ztr out;
    ztr_init(&out);
    ASSERT_EQ(ZTR_OK, ztr_join(&out, parts, 3, ""));
    ASSERT(ztr_eq_cstr(&out, "foobarbaz"));

    ztr_free(&parts[0]);
    ztr_free(&parts[1]);
    ztr_free(&parts[2]);
    ztr_free(&out);
    PASS();
}

TEST join_single_element(void) {
    ztr parts[1];
    ztr_from(&parts[0], "only");

    ztr out;
    ztr_init(&out);
    ASSERT_EQ(ZTR_OK, ztr_join(&out, parts, 1, ","));
    ASSERT(ztr_eq_cstr(&out, "only"));

    ztr_free(&parts[0]);
    ztr_free(&out);
    PASS();
}

TEST join_empty_array(void) {
    ztr out;
    ztr_init(&out);
    ASSERT_EQ(ZTR_OK, ztr_join(&out, NULL, 0, ","));
    ASSERT(ztr_eq_cstr(&out, ""));

    ztr_free(&out);
    PASS();
}

TEST join_two_elements(void) {
    ztr parts[2];
    ztr_from(&parts[0], "hello");
    ztr_from(&parts[1], "world");

    ztr out;
    ztr_init(&out);
    ASSERT_EQ(ZTR_OK, ztr_join(&out, parts, 2, " "));
    ASSERT(ztr_eq_cstr(&out, "hello world"));

    ztr_free(&parts[0]);
    ztr_free(&parts[1]);
    ztr_free(&out);
    PASS();
}

TEST join_with_empty_parts(void) {
    /* Joining strings where some are empty */
    ztr parts[3];
    ztr_from(&parts[0], "a");
    ztr_from(&parts[1], "");
    ztr_from(&parts[2], "b");

    ztr out;
    ztr_init(&out);
    ASSERT_EQ(ZTR_OK, ztr_join(&out, parts, 3, "-"));
    ASSERT(ztr_eq_cstr(&out, "a--b"));

    ztr_free(&parts[0]);
    ztr_free(&parts[1]);
    ztr_free(&parts[2]);
    ztr_free(&out);
    PASS();
}

TEST join_all_empty_parts(void) {
    ztr parts[3];
    ztr_from(&parts[0], "");
    ztr_from(&parts[1], "");
    ztr_from(&parts[2], "");

    ztr out;
    ztr_init(&out);
    ASSERT_EQ(ZTR_OK, ztr_join(&out, parts, 3, ","));
    ASSERT(ztr_eq_cstr(&out, ",,"));

    ztr_free(&parts[0]);
    ztr_free(&parts[1]);
    ztr_free(&parts[2]);
    ztr_free(&out);
    PASS();
}

TEST join_multi_char_separator(void) {
    ztr parts[3];
    ztr_from(&parts[0], "one");
    ztr_from(&parts[1], "two");
    ztr_from(&parts[2], "three");

    ztr out;
    ztr_init(&out);
    ASSERT_EQ(ZTR_OK, ztr_join(&out, parts, 3, " | "));
    ASSERT(ztr_eq_cstr(&out, "one | two | three"));

    ztr_free(&parts[0]);
    ztr_free(&parts[1]);
    ztr_free(&parts[2]);
    ztr_free(&out);
    PASS();
}

/* ---- ztr_join_cstr ---- */

TEST join_cstr_basic(void) {
    const char *parts[] = {"alpha", "beta", "gamma"};
    ztr out;
    ztr_init(&out);

    ASSERT_EQ(ZTR_OK, ztr_join_cstr(&out, parts, 3, ", "));
    ASSERT(ztr_eq_cstr(&out, "alpha, beta, gamma"));

    ztr_free(&out);
    PASS();
}

TEST join_cstr_empty_separator(void) {
    const char *parts[] = {"foo", "bar", "baz"};
    ztr out;
    ztr_init(&out);

    ASSERT_EQ(ZTR_OK, ztr_join_cstr(&out, parts, 3, ""));
    ASSERT(ztr_eq_cstr(&out, "foobarbaz"));

    ztr_free(&out);
    PASS();
}

TEST join_cstr_single_element(void) {
    const char *parts[] = {"only"};
    ztr out;
    ztr_init(&out);

    ASSERT_EQ(ZTR_OK, ztr_join_cstr(&out, parts, 1, ","));
    ASSERT(ztr_eq_cstr(&out, "only"));

    ztr_free(&out);
    PASS();
}

TEST join_cstr_empty_array(void) {
    ztr out;
    ztr_init(&out);

    ASSERT_EQ(ZTR_OK, ztr_join_cstr(&out, NULL, 0, ","));
    ASSERT(ztr_eq_cstr(&out, ""));

    ztr_free(&out);
    PASS();
}

TEST join_cstr_two_elements(void) {
    const char *parts[] = {"hello", "world"};
    ztr out;
    ztr_init(&out);

    ASSERT_EQ(ZTR_OK, ztr_join_cstr(&out, parts, 2, " "));
    ASSERT(ztr_eq_cstr(&out, "hello world"));

    ztr_free(&out);
    PASS();
}

TEST join_cstr_with_empty_parts(void) {
    const char *parts[] = {"a", "", "b"};
    ztr out;
    ztr_init(&out);

    ASSERT_EQ(ZTR_OK, ztr_join_cstr(&out, parts, 3, "-"));
    ASSERT(ztr_eq_cstr(&out, "a--b"));

    ztr_free(&out);
    PASS();
}

TEST join_cstr_all_empty_parts(void) {
    const char *parts[] = {"", "", ""};
    ztr out;
    ztr_init(&out);

    ASSERT_EQ(ZTR_OK, ztr_join_cstr(&out, parts, 3, ","));
    ASSERT(ztr_eq_cstr(&out, ",,"));

    ztr_free(&out);
    PASS();
}

TEST join_cstr_multi_char_separator(void) {
    const char *parts[] = {"one", "two", "three"};
    ztr out;
    ztr_init(&out);

    ASSERT_EQ(ZTR_OK, ztr_join_cstr(&out, parts, 3, " | "));
    ASSERT(ztr_eq_cstr(&out, "one | two | three"));

    ztr_free(&out);
    PASS();
}

/* ---- Round-trip: split then join ---- */

TEST split_alloc_then_join_roundtrip(void) {
    const char *original = "red,green,blue";
    ztr s = make(original);
    ztr *parts = NULL;
    size_t count = 0;

    ASSERT_EQ(ZTR_OK, ztr_split_alloc(&s, ",", &parts, &count, 0));
    ASSERT_EQ(3, count);

    ztr out;
    ztr_init(&out);
    ASSERT_EQ(ZTR_OK, ztr_join(&out, parts, count, ","));
    ASSERT(ztr_eq_cstr(&out, original));

    ztr_split_free(parts, count);
    ztr_free(&out);
    ztr_free(&s);
    PASS();
}

/* ---- Suite ---- */

SUITE(split_join) {
    /* ztr_split_begin / ztr_split_next */
    RUN_TEST(split_iter_basic);
    RUN_TEST(split_iter_empty_parts);
    RUN_TEST(split_iter_single_part_no_delimiter);
    RUN_TEST(split_iter_empty_source);
    RUN_TEST(split_iter_empty_delimiter);
    RUN_TEST(split_iter_trailing_delimiter);
    RUN_TEST(split_iter_leading_delimiter);
    RUN_TEST(split_iter_multi_char_delimiter);
    RUN_TEST(split_iter_only_delimiter);
    RUN_TEST(split_iter_repeated_calls_after_exhaustion);

    /* ztr_split_alloc / ztr_split_free */
    RUN_TEST(split_alloc_basic);
    RUN_TEST(split_alloc_max_parts_zero_means_unlimited);
    RUN_TEST(split_alloc_max_parts_limits_and_puts_remainder_in_last);
    RUN_TEST(split_alloc_max_parts_larger_than_actual);
    RUN_TEST(split_alloc_empty_parts);
    RUN_TEST(split_alloc_trailing_delimiter);
    RUN_TEST(split_alloc_no_delimiter_in_source);
    RUN_TEST(split_alloc_empty_source);
    RUN_TEST(split_alloc_max_parts_one);

    /* ztr_join */
    RUN_TEST(join_basic);
    RUN_TEST(join_empty_separator);
    RUN_TEST(join_single_element);
    RUN_TEST(join_empty_array);
    RUN_TEST(join_two_elements);
    RUN_TEST(join_with_empty_parts);
    RUN_TEST(join_all_empty_parts);
    RUN_TEST(join_multi_char_separator);

    /* ztr_join_cstr */
    RUN_TEST(join_cstr_basic);
    RUN_TEST(join_cstr_empty_separator);
    RUN_TEST(join_cstr_single_element);
    RUN_TEST(join_cstr_empty_array);
    RUN_TEST(join_cstr_two_elements);
    RUN_TEST(join_cstr_with_empty_parts);
    RUN_TEST(join_cstr_all_empty_parts);
    RUN_TEST(join_cstr_multi_char_separator);

    /* Round-trip */
    RUN_TEST(split_alloc_then_join_roundtrip);
}
