#include "greatest.h"
#include "ztr.h"

#include <string.h>

/* ---- ztr_view_from_cstr ---- */

TEST view_from_cstr_basic(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_EQ((size_t)5, ztr_view_len(v));
    ASSERT_STRN_EQ("hello", ztr_view_data(v), 5);
    PASS();
}

TEST view_from_cstr_empty(void) {
    ztr_view v = ztr_view_from_cstr("");
    ASSERT_EQ((size_t)0, ztr_view_len(v));
    ASSERT(ztr_view_is_empty(v));
    PASS();
}

TEST view_from_cstr_null(void) {
    ztr_view v = ztr_view_from_cstr(NULL);
    ASSERT_EQ((size_t)0, ztr_view_len(v));
    ASSERT(ztr_view_is_empty(v));
    ASSERT(ztr_view_data(v) != NULL);
    PASS();
}

/* ---- ztr_view_from_buf ---- */

TEST view_from_buf_basic(void) {
    ztr_view v = ztr_view_from_buf("hello world", 5);
    ASSERT_EQ((size_t)5, ztr_view_len(v));
    ASSERT_STRN_EQ("hello", ztr_view_data(v), 5);
    PASS();
}

TEST view_from_buf_null_zero(void) {
    ztr_view v = ztr_view_from_buf(NULL, 0);
    ASSERT_EQ((size_t)0, ztr_view_len(v));
    ASSERT(ztr_view_data(v) != NULL);
    PASS();
}

TEST view_from_buf_null_nonzero(void) {
    ztr_view v = ztr_view_from_buf(NULL, 42);
    ASSERT_EQ((size_t)0, ztr_view_len(v));
    PASS();
}

TEST view_from_buf_embedded_null(void) {
    ztr_view v = ztr_view_from_buf("he\0lo", 5);
    ASSERT_EQ((size_t)5, ztr_view_len(v));
    ASSERT_EQ('l', ztr_view_at(v, 3));
    PASS();
}

/* ---- ztr_view_from_ztr ---- */

TEST view_from_ztr_sso(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ztr_view v = ztr_view_from_ztr(&s);
    ASSERT_EQ(ztr_len(&s), ztr_view_len(v));
    ASSERT_EQ(ztr_cstr(&s), ztr_view_data(v));
    ztr_free(&s);
    PASS();
}

TEST view_from_ztr_heap(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "this string is definitely on the heap"));
    ztr_view v = ztr_view_from_ztr(&s);
    ASSERT_EQ(ztr_len(&s), ztr_view_len(v));
    ASSERT_EQ(ztr_cstr(&s), ztr_view_data(v));
    ztr_free(&s);
    PASS();
}

TEST view_from_ztr_empty(void) {
    ztr s;
    ztr_init(&s);
    ztr_view v = ztr_view_from_ztr(&s);
    ASSERT_EQ((size_t)0, ztr_view_len(v));
    ztr_free(&s);
    PASS();
}

TEST view_from_ztr_null(void) {
    ztr_view v = ztr_view_from_ztr(NULL);
    ASSERT_EQ((size_t)0, ztr_view_len(v));
    ASSERT(ztr_view_data(v) != NULL);
    PASS();
}

/* ---- ztr_view_substr ---- */

TEST view_substr_basic(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ztr_view sub = ztr_view_substr(v, 6, 5);
    ASSERT_EQ((size_t)5, ztr_view_len(sub));
    ASSERT_STRN_EQ("world", ztr_view_data(sub), 5);
    PASS();
}

TEST view_substr_pos_past_end(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ztr_view sub = ztr_view_substr(v, 10, 5);
    ASSERT_EQ((size_t)0, ztr_view_len(sub));
    PASS();
}

TEST view_substr_count_clamped(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ztr_view sub = ztr_view_substr(v, 3, 100);
    ASSERT_EQ((size_t)2, ztr_view_len(sub));
    ASSERT_STRN_EQ("lo", ztr_view_data(sub), 2);
    PASS();
}

TEST view_substr_zero_count(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ztr_view sub = ztr_view_substr(v, 2, 0);
    ASSERT_EQ((size_t)0, ztr_view_len(sub));
    PASS();
}

TEST view_substr_full_string(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ztr_view sub = ztr_view_substr(v, 0, 5);
    ASSERT_EQ((size_t)5, ztr_view_len(sub));
    ASSERT_STRN_EQ("hello", ztr_view_data(sub), 5);
    PASS();
}

TEST view_substr_pos_equals_len(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ztr_view sub = ztr_view_substr(v, 3, 5);
    ASSERT_EQ((size_t)0, ztr_view_len(sub));
    PASS();
}

TEST view_substr_count_npos(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ztr_view sub = ztr_view_substr(v, 2, ZTR_NPOS);
    ASSERT_EQ((size_t)3, ztr_view_len(sub));
    ASSERT_STRN_EQ("llo", ztr_view_data(sub), 3);
    PASS();
}

/* ---- ztr_view_at ---- */

TEST view_at_valid_index(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ASSERT_EQ('a', ztr_view_at(v, 0));
    ASSERT_EQ('b', ztr_view_at(v, 1));
    ASSERT_EQ('c', ztr_view_at(v, 2));
    PASS();
}

TEST view_at_out_of_bounds(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ASSERT_EQ('\0', ztr_view_at(v, 3));
    ASSERT_EQ('\0', ztr_view_at(v, 100));
    PASS();
}

TEST view_at_empty_view(void) {
    ztr_view v = ZTR_VIEW_EMPTY;
    ASSERT_EQ('\0', ztr_view_at(v, 0));
    PASS();
}

/* ---- ZTR_VIEW_LIT ---- */

TEST view_lit_basic(void) {
    ztr_view v = ZTR_VIEW_LIT("hello");
    ASSERT_EQ((size_t)5, ztr_view_len(v));
    ASSERT_STRN_EQ("hello", ztr_view_data(v), 5);
    PASS();
}

TEST view_lit_empty(void) {
    ztr_view v = ZTR_VIEW_LIT("");
    ASSERT_EQ((size_t)0, ztr_view_len(v));
    PASS();
}

/* ---- ZTR_VIEW_EMPTY ---- */

TEST view_empty_is_valid(void) {
    ztr_view v = ZTR_VIEW_EMPTY;
    ASSERT_EQ((size_t)0, ztr_view_len(v));
    ASSERT(ztr_view_is_empty(v));
    ASSERT(ztr_view_data(v) != NULL);
    PASS();
}

/* ---- ztr_view_eq ---- */

TEST view_eq_same(void) {
    ztr_view a = ztr_view_from_cstr("hello");
    ztr_view b = ztr_view_from_cstr("hello");
    ASSERT(ztr_view_eq(a, b));
    PASS();
}

TEST view_eq_different(void) {
    ztr_view a = ztr_view_from_cstr("hello");
    ztr_view b = ztr_view_from_cstr("world");
    ASSERT_FALSE(ztr_view_eq(a, b));
    PASS();
}

TEST view_eq_different_lengths(void) {
    ztr_view a = ztr_view_from_cstr("abc");
    ztr_view b = ztr_view_from_cstr("abcd");
    ASSERT_FALSE(ztr_view_eq(a, b));
    PASS();
}

TEST view_eq_both_empty(void) {
    ztr_view a = ZTR_VIEW_EMPTY;
    ztr_view b = ZTR_VIEW_EMPTY;
    ASSERT(ztr_view_eq(a, b));
    PASS();
}

/* ---- ztr_view_eq_cstr ---- */

TEST view_eq_cstr_match(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT(ztr_view_eq_cstr(v, "hello"));
    PASS();
}

TEST view_eq_cstr_no_match(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_FALSE(ztr_view_eq_cstr(v, "world"));
    PASS();
}

TEST view_eq_cstr_null(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_FALSE(ztr_view_eq_cstr(v, NULL));
    PASS();
}

TEST view_eq_cstr_empty_vs_null(void) {
    ztr_view v = ZTR_VIEW_EMPTY;
    ASSERT(ztr_view_eq_cstr(v, NULL));
    ASSERT(ztr_view_eq_cstr(v, ""));
    PASS();
}

/* ---- ztr_view_cmp ---- */

TEST view_cmp_equal(void) {
    ztr_view a = ztr_view_from_cstr("abc");
    ztr_view b = ztr_view_from_cstr("abc");
    ASSERT_EQ(0, ztr_view_cmp(a, b));
    PASS();
}

TEST view_cmp_less_than(void) {
    ztr_view a = ztr_view_from_cstr("abc");
    ztr_view b = ztr_view_from_cstr("abd");
    ASSERT_LT(ztr_view_cmp(a, b), 0);
    PASS();
}

TEST view_cmp_greater_than(void) {
    ztr_view a = ztr_view_from_cstr("abd");
    ztr_view b = ztr_view_from_cstr("abc");
    ASSERT_GT(ztr_view_cmp(a, b), 0);
    PASS();
}

TEST view_cmp_prefix_is_less(void) {
    ztr_view a = ztr_view_from_cstr("abc");
    ztr_view b = ztr_view_from_cstr("abcd");
    ASSERT_LT(ztr_view_cmp(a, b), 0);
    PASS();
}

TEST view_cmp_both_empty(void) {
    ztr_view a = ZTR_VIEW_EMPTY;
    ztr_view b = ZTR_VIEW_EMPTY;
    ASSERT_EQ(0, ztr_view_cmp(a, b));
    PASS();
}

/* ---- ztr_view_cmp_cstr ---- */

TEST view_cmp_cstr_equal(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ASSERT_EQ(0, ztr_view_cmp_cstr(v, "abc"));
    PASS();
}

TEST view_cmp_cstr_less_than(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ASSERT_LT(ztr_view_cmp_cstr(v, "abd"), 0);
    PASS();
}

TEST view_cmp_cstr_greater_than(void) {
    ztr_view v = ztr_view_from_cstr("abd");
    ASSERT_GT(ztr_view_cmp_cstr(v, "abc"), 0);
    PASS();
}

TEST view_cmp_cstr_null(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_GT(ztr_view_cmp_cstr(v, NULL), 0);
    PASS();
}

TEST view_cmp_cstr_empty_vs_null(void) {
    ztr_view v = ZTR_VIEW_EMPTY;
    ASSERT_EQ(0, ztr_view_cmp_cstr(v, NULL));
    PASS();
}

/* ---- ztr_view_eq_ascii_nocase ---- */

TEST view_nocase_upper_vs_lower(void) {
    ztr_view a = ztr_view_from_cstr("HELLO");
    ztr_view b = ztr_view_from_cstr("hello");
    ASSERT(ztr_view_eq_ascii_nocase(a, b));
    PASS();
}

TEST view_nocase_different(void) {
    ztr_view a = ztr_view_from_cstr("HELLO");
    ztr_view b = ztr_view_from_cstr("WORLD");
    ASSERT_FALSE(ztr_view_eq_ascii_nocase(a, b));
    PASS();
}

TEST view_nocase_different_lengths(void) {
    ztr_view a = ztr_view_from_cstr("ABC");
    ztr_view b = ztr_view_from_cstr("ABCD");
    ASSERT_FALSE(ztr_view_eq_ascii_nocase(a, b));
    PASS();
}

TEST view_nocase_both_empty(void) {
    ASSERT(ztr_view_eq_ascii_nocase(ZTR_VIEW_EMPTY, ZTR_VIEW_EMPTY));
    PASS();
}

/* ---- ztr_view_eq_ascii_nocase_cstr ---- */

TEST view_nocase_cstr_match(void) {
    ztr_view v = ztr_view_from_cstr("Content-Type");
    ASSERT(ztr_view_eq_ascii_nocase_cstr(v, "content-type"));
    PASS();
}

TEST view_nocase_cstr_null(void) {
    ztr_view v = ZTR_VIEW_EMPTY;
    ASSERT(ztr_view_eq_ascii_nocase_cstr(v, NULL));
    PASS();
}

/* ---- ztr_view_find ---- */

TEST view_find_present(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT_EQ(6, ztr_view_find(v, ZTR_VIEW_LIT("world"), 0));
    PASS();
}

TEST view_find_not_present(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT_EQ(ZTR_NPOS, ztr_view_find(v, ZTR_VIEW_LIT("xyz"), 0));
    PASS();
}

TEST view_find_with_offset(void) {
    ztr_view v = ztr_view_from_cstr("abab");
    ASSERT_EQ(2, ztr_view_find(v, ZTR_VIEW_LIT("ab"), 1));
    PASS();
}

TEST view_find_empty_needle(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_EQ(0, ztr_view_find(v, ZTR_VIEW_EMPTY, 0));
    ASSERT_EQ(3, ztr_view_find(v, ZTR_VIEW_EMPTY, 3));
    PASS();
}

TEST view_find_needle_longer_than_view(void) {
    ztr_view v = ztr_view_from_cstr("hi");
    ASSERT_EQ(ZTR_NPOS, ztr_view_find(v, ZTR_VIEW_LIT("hello world"), 0));
    PASS();
}

TEST view_find_start_past_end(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ASSERT_EQ(ZTR_NPOS, ztr_view_find(v, ZTR_VIEW_LIT("a"), 10));
    PASS();
}

TEST view_find_in_empty_view(void) {
    ASSERT_EQ(ZTR_NPOS, ztr_view_find(ZTR_VIEW_EMPTY, ZTR_VIEW_LIT("x"), 0));
    PASS();
}

/* ---- ztr_view_rfind ---- */

TEST view_rfind_last_occurrence(void) {
    ztr_view v = ztr_view_from_cstr("abab");
    ASSERT_EQ(2, ztr_view_rfind(v, ZTR_VIEW_LIT("ab"), ZTR_NPOS));
    PASS();
}

TEST view_rfind_not_found(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_EQ(ZTR_NPOS, ztr_view_rfind(v, ZTR_VIEW_LIT("xyz"), ZTR_NPOS));
    PASS();
}

TEST view_rfind_start_limits_window(void) {
    ztr_view v = ztr_view_from_cstr("abab");
    ASSERT_EQ(0, ztr_view_rfind(v, ZTR_VIEW_LIT("ab"), 1));
    PASS();
}

TEST view_rfind_empty_needle(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_EQ(ztr_view_len(v), ztr_view_rfind(v, ZTR_VIEW_EMPTY, ZTR_NPOS));
    PASS();
}

TEST view_rfind_needle_longer_than_view(void) {
    ztr_view v = ztr_view_from_cstr("hi");
    ASSERT_EQ(ZTR_NPOS, ztr_view_rfind(v, ZTR_VIEW_LIT("hello world"), ZTR_NPOS));
    PASS();
}

/* ---- ztr_view_contains / starts_with / ends_with ---- */

TEST view_contains_true(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT(ztr_view_contains(v, ZTR_VIEW_LIT("world")));
    PASS();
}

TEST view_contains_false(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT_FALSE(ztr_view_contains(v, ZTR_VIEW_LIT("xyz")));
    PASS();
}

TEST view_starts_with_yes(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT(ztr_view_starts_with(v, ZTR_VIEW_LIT("hello")));
    PASS();
}

TEST view_starts_with_no(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT_FALSE(ztr_view_starts_with(v, ZTR_VIEW_LIT("world")));
    PASS();
}

TEST view_starts_with_empty_prefix(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT(ztr_view_starts_with(v, ZTR_VIEW_EMPTY));
    PASS();
}

TEST view_ends_with_yes(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT(ztr_view_ends_with(v, ZTR_VIEW_LIT("world")));
    PASS();
}

TEST view_ends_with_no(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT_FALSE(ztr_view_ends_with(v, ZTR_VIEW_LIT("hello")));
    PASS();
}

TEST view_ends_with_empty_suffix(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT(ztr_view_ends_with(v, ZTR_VIEW_EMPTY));
    PASS();
}

TEST view_contains_empty_view(void) {
    ASSERT(ztr_view_contains(ZTR_VIEW_EMPTY, ZTR_VIEW_EMPTY));
    PASS();
}

TEST view_starts_with_empty_view(void) {
    ASSERT(ztr_view_starts_with(ZTR_VIEW_EMPTY, ZTR_VIEW_EMPTY));
    PASS();
}

TEST view_ends_with_empty_view(void) {
    ASSERT(ztr_view_ends_with(ZTR_VIEW_EMPTY, ZTR_VIEW_EMPTY));
    PASS();
}

/* ---- ztr_view_count ---- */

TEST view_count_multiple(void) {
    ztr_view v = ztr_view_from_cstr("abcabcabc");
    ASSERT_EQ(3, ztr_view_count(v, ZTR_VIEW_LIT("abc")));
    PASS();
}

TEST view_count_none(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_EQ(0, ztr_view_count(v, ZTR_VIEW_LIT("xyz")));
    PASS();
}

TEST view_count_empty_needle(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_EQ(0, ztr_view_count(v, ZTR_VIEW_EMPTY));
    PASS();
}

TEST view_count_non_overlapping(void) {
    ztr_view v = ztr_view_from_cstr("aaaa");
    ASSERT_EQ(2, ztr_view_count(v, ZTR_VIEW_LIT("aa")));
    PASS();
}

/* ---- ztr_view_find_cstr / contains_cstr / starts_with_cstr / ends_with_cstr ---- */

TEST view_find_cstr_basic(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT_EQ(6, ztr_view_find_cstr(v, "world", 0));
    PASS();
}

TEST view_rfind_cstr_basic(void) {
    ztr_view v = ztr_view_from_cstr("abab");
    ASSERT_EQ(2, ztr_view_rfind_cstr(v, "ab", ZTR_NPOS));
    PASS();
}

TEST view_contains_cstr_true(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT(ztr_view_contains_cstr(v, "world"));
    PASS();
}

TEST view_starts_with_cstr_yes(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT(ztr_view_starts_with_cstr(v, "hello"));
    PASS();
}

TEST view_ends_with_cstr_yes(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ASSERT(ztr_view_ends_with_cstr(v, "world"));
    PASS();
}

TEST view_count_cstr_basic(void) {
    ztr_view v = ztr_view_from_cstr("abcabcabc");
    ASSERT_EQ(3, ztr_view_count_cstr(v, "abc"));
    PASS();
}

TEST view_cstr_null_needle(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    /* NULL needle treated as "" — empty needle returns start for find. */
    ASSERT_EQ(0, ztr_view_find_cstr(v, NULL, 0));
    ASSERT_EQ(ztr_view_len(v), ztr_view_rfind_cstr(v, NULL, ZTR_NPOS));
    ASSERT(ztr_view_contains_cstr(v, NULL));
    ASSERT(ztr_view_starts_with_cstr(v, NULL));
    ASSERT(ztr_view_ends_with_cstr(v, NULL));
    ASSERT_EQ(0, ztr_view_count_cstr(v, NULL));
    PASS();
}

/* ---- ztr_view_find_char / rfind_char / contains_char ---- */

TEST view_find_char_present(void) {
    ztr_view v = ztr_view_from_cstr("abcabc");
    ASSERT_EQ(0, ztr_view_find_char(v, 'a', 0));
    ASSERT_EQ(3, ztr_view_find_char(v, 'a', 1));
    PASS();
}

TEST view_find_char_not_present(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ASSERT_EQ(ZTR_NPOS, ztr_view_find_char(v, 'x', 0));
    PASS();
}

TEST view_find_char_start_past_end(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ASSERT_EQ(ZTR_NPOS, ztr_view_find_char(v, 'a', 10));
    PASS();
}

TEST view_find_char_empty_view(void) {
    ASSERT_EQ(ZTR_NPOS, ztr_view_find_char(ZTR_VIEW_EMPTY, 'a', 0));
    PASS();
}

TEST view_rfind_char_last(void) {
    ztr_view v = ztr_view_from_cstr("abcabc");
    ASSERT_EQ(3, ztr_view_rfind_char(v, 'a', ZTR_NPOS));
    PASS();
}

TEST view_rfind_char_with_start(void) {
    ztr_view v = ztr_view_from_cstr("abcabc");
    ASSERT_EQ(0, ztr_view_rfind_char(v, 'a', 2));
    PASS();
}

TEST view_rfind_char_not_found(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ASSERT_EQ(ZTR_NPOS, ztr_view_rfind_char(v, 'x', ZTR_NPOS));
    PASS();
}

TEST view_contains_char_true(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT(ztr_view_contains_char(v, 'e'));
    PASS();
}

TEST view_contains_char_false(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT_FALSE(ztr_view_contains_char(v, 'x'));
    PASS();
}

TEST view_contains_char_empty_view(void) {
    ASSERT_FALSE(ztr_view_contains_char(ZTR_VIEW_EMPTY, 'a'));
    PASS();
}

/* ---- ztr_view_trim ---- */

TEST view_trim_basic(void) {
    ztr_view v = ztr_view_from_cstr("  hello  ");
    ztr_view t = ztr_view_trim(v);
    ASSERT_EQ((size_t)5, ztr_view_len(t));
    ASSERT_STRN_EQ("hello", ztr_view_data(t), 5);
    PASS();
}

TEST view_trim_start_only(void) {
    ztr_view v = ztr_view_from_cstr("  hello");
    ztr_view t = ztr_view_trim_start(v);
    ASSERT_EQ((size_t)5, ztr_view_len(t));
    ASSERT_STRN_EQ("hello", ztr_view_data(t), 5);
    PASS();
}

TEST view_trim_end_only(void) {
    ztr_view v = ztr_view_from_cstr("hello  ");
    ztr_view t = ztr_view_trim_end(v);
    ASSERT_EQ((size_t)5, ztr_view_len(t));
    ASSERT_STRN_EQ("hello", ztr_view_data(t), 5);
    PASS();
}

TEST view_trim_no_whitespace(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ztr_view t = ztr_view_trim(v);
    ASSERT_EQ((size_t)5, ztr_view_len(t));
    ASSERT_STRN_EQ("hello", ztr_view_data(t), 5);
    PASS();
}

TEST view_trim_all_whitespace(void) {
    ztr_view v = ztr_view_from_cstr("   ");
    ztr_view t = ztr_view_trim(v);
    ASSERT_EQ((size_t)0, ztr_view_len(t));
    PASS();
}

TEST view_trim_empty_view(void) {
    ztr_view t = ztr_view_trim(ZTR_VIEW_EMPTY);
    ASSERT_EQ((size_t)0, ztr_view_len(t));
    PASS();
}

TEST view_trim_mixed_whitespace(void) {
    ztr_view v = ztr_view_from_cstr(" \t\r\n hello \n\t ");
    ztr_view t = ztr_view_trim(v);
    ASSERT_EQ((size_t)5, ztr_view_len(t));
    ASSERT_STRN_EQ("hello", ztr_view_data(t), 5);
    PASS();
}

TEST view_trim_start_all_whitespace(void) {
    ztr_view v = ztr_view_from_cstr("   ");
    ztr_view t = ztr_view_trim_start(v);
    ASSERT_EQ((size_t)0, ztr_view_len(t));
    PASS();
}

TEST view_trim_start_empty(void) {
    ztr_view t = ztr_view_trim_start(ZTR_VIEW_EMPTY);
    ASSERT_EQ((size_t)0, ztr_view_len(t));
    PASS();
}

TEST view_trim_end_all_whitespace(void) {
    ztr_view v = ztr_view_from_cstr("   ");
    ztr_view t = ztr_view_trim_end(v);
    ASSERT_EQ((size_t)0, ztr_view_len(t));
    PASS();
}

TEST view_trim_end_empty(void) {
    ztr_view t = ztr_view_trim_end(ZTR_VIEW_EMPTY);
    ASSERT_EQ((size_t)0, ztr_view_len(t));
    PASS();
}

/* ---- ztr_view_remove_prefix / remove_suffix ---- */

TEST view_remove_prefix_basic(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ztr_view r = ztr_view_remove_prefix(v, 6);
    ASSERT_EQ((size_t)5, ztr_view_len(r));
    ASSERT_STRN_EQ("world", ztr_view_data(r), 5);
    PASS();
}

TEST view_remove_prefix_too_large(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ztr_view r = ztr_view_remove_prefix(v, 10);
    ASSERT_EQ((size_t)0, ztr_view_len(r));
    PASS();
}

TEST view_remove_prefix_zero(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ztr_view r = ztr_view_remove_prefix(v, 0);
    ASSERT_EQ((size_t)5, ztr_view_len(r));
    PASS();
}

TEST view_remove_suffix_basic(void) {
    ztr_view v = ztr_view_from_cstr("hello world");
    ztr_view r = ztr_view_remove_suffix(v, 6);
    ASSERT_EQ((size_t)5, ztr_view_len(r));
    ASSERT_STRN_EQ("hello", ztr_view_data(r), 5);
    PASS();
}

TEST view_remove_suffix_too_large(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ztr_view r = ztr_view_remove_suffix(v, 10);
    ASSERT_EQ((size_t)0, ztr_view_len(r));
    PASS();
}

TEST view_remove_suffix_zero(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ztr_view r = ztr_view_remove_suffix(v, 0);
    ASSERT_EQ((size_t)5, ztr_view_len(r));
    PASS();
}

TEST view_remove_prefix_exact(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ztr_view r = ztr_view_remove_prefix(v, 3);
    ASSERT_EQ((size_t)0, ztr_view_len(r));
    PASS();
}

TEST view_remove_suffix_exact(void) {
    ztr_view v = ztr_view_from_cstr("abc");
    ztr_view r = ztr_view_remove_suffix(v, 3);
    ASSERT_EQ((size_t)0, ztr_view_len(r));
    PASS();
}

/* ---- ztr_view_split ---- */

TEST view_split_basic(void) {
    ztr_view s = ztr_view_from_cstr("a,b,c");
    ztr_view_split_iter it;
    ztr_view token;

    ztr_view_split_begin(&it, s, ZTR_VIEW_LIT(","));

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_EQ((size_t)1, ztr_view_len(token));
    ASSERT_STRN_EQ("a", ztr_view_data(token), 1);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_EQ((size_t)1, ztr_view_len(token));
    ASSERT_STRN_EQ("b", ztr_view_data(token), 1);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_EQ((size_t)1, ztr_view_len(token));
    ASSERT_STRN_EQ("c", ztr_view_data(token), 1);

    ASSERT_FALSE(ztr_view_split_next(&it, &token));
    PASS();
}

TEST view_split_empty_parts(void) {
    ztr_view s = ztr_view_from_cstr("a,,b");
    ztr_view_split_iter it;
    ztr_view token;

    ztr_view_split_begin(&it, s, ZTR_VIEW_LIT(","));

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("a", ztr_view_data(token), 1);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_EQ((size_t)0, ztr_view_len(token));

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("b", ztr_view_data(token), 1);

    ASSERT_FALSE(ztr_view_split_next(&it, &token));
    PASS();
}

TEST view_split_trailing_delimiter(void) {
    ztr_view s = ztr_view_from_cstr("a,b,");
    ztr_view_split_iter it;
    ztr_view token;

    ztr_view_split_begin(&it, s, ZTR_VIEW_LIT(","));

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("a", ztr_view_data(token), 1);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("b", ztr_view_data(token), 1);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_EQ((size_t)0, ztr_view_len(token));

    ASSERT_FALSE(ztr_view_split_next(&it, &token));
    PASS();
}

TEST view_split_no_delimiter(void) {
    ztr_view s = ztr_view_from_cstr("hello");
    ztr_view_split_iter it;
    ztr_view token;

    ztr_view_split_begin(&it, s, ZTR_VIEW_LIT(","));

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_EQ((size_t)5, ztr_view_len(token));
    ASSERT_STRN_EQ("hello", ztr_view_data(token), 5);

    ASSERT_FALSE(ztr_view_split_next(&it, &token));
    PASS();
}

TEST view_split_empty_source(void) {
    ztr_view_split_iter it;
    ztr_view token;

    ztr_view_split_begin(&it, ZTR_VIEW_EMPTY, ZTR_VIEW_LIT(","));

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_EQ((size_t)0, ztr_view_len(token));

    ASSERT_FALSE(ztr_view_split_next(&it, &token));
    PASS();
}

TEST view_split_empty_delimiter(void) {
    ztr_view s = ztr_view_from_cstr("hello");
    ztr_view_split_iter it;
    ztr_view token;

    ztr_view_split_begin(&it, s, ZTR_VIEW_EMPTY);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_EQ((size_t)5, ztr_view_len(token));

    ASSERT_FALSE(ztr_view_split_next(&it, &token));
    PASS();
}

TEST view_split_multi_char_delimiter(void) {
    ztr_view s = ztr_view_from_cstr("a::b::c");
    ztr_view_split_iter it;
    ztr_view token;

    ztr_view_split_begin(&it, s, ZTR_VIEW_LIT("::"));

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("a", ztr_view_data(token), 1);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("b", ztr_view_data(token), 1);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("c", ztr_view_data(token), 1);

    ASSERT_FALSE(ztr_view_split_next(&it, &token));
    PASS();
}

TEST view_split_begin_cstr_basic(void) {
    ztr_view s = ztr_view_from_cstr("x-y-z");
    ztr_view_split_iter it;
    ztr_view token;

    ztr_view_split_begin_cstr(&it, s, "-");

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("x", ztr_view_data(token), 1);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("y", ztr_view_data(token), 1);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_STRN_EQ("z", ztr_view_data(token), 1);

    ASSERT_FALSE(ztr_view_split_next(&it, &token));
    PASS();
}

TEST view_split_begin_cstr_null_delimiter(void) {
    ztr_view s = ztr_view_from_cstr("hello");
    ztr_view_split_iter it;
    ztr_view token;

    ztr_view_split_begin_cstr(&it, s, NULL);

    ASSERT(ztr_view_split_next(&it, &token));
    ASSERT_EQ((size_t)5, ztr_view_len(token));

    ASSERT_FALSE(ztr_view_split_next(&it, &token));
    PASS();
}

/* ---- ztr_view_is_ascii ---- */

TEST view_is_ascii_true(void) {
    ztr_view v = ztr_view_from_cstr("hello world 123 !@#");
    ASSERT(ztr_view_is_ascii(v));
    PASS();
}

TEST view_is_ascii_false(void) {
    ztr_view v = ztr_view_from_buf("caf\xc3\xa9", 5);
    ASSERT_FALSE(ztr_view_is_ascii(v));
    PASS();
}

TEST view_is_ascii_empty(void) {
    ASSERT(ztr_view_is_ascii(ZTR_VIEW_EMPTY));
    PASS();
}

/* ---- ztr_view_is_valid_utf8 ---- */

TEST view_utf8_valid_ascii(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ASSERT(ztr_view_is_valid_utf8(v));
    PASS();
}

TEST view_utf8_valid_multibyte(void) {
    ztr_view v = ztr_view_from_buf("caf\xc3\xa9", 5);
    ASSERT(ztr_view_is_valid_utf8(v));
    PASS();
}

TEST view_utf8_invalid_truncated(void) {
    ztr_view v = ztr_view_from_buf("caf\xc3", 4);
    ASSERT_FALSE(ztr_view_is_valid_utf8(v));
    PASS();
}

TEST view_utf8_invalid_overlong(void) {
    /* Overlong encoding of '/' (U+002F): 0xC0 0xAF */
    ztr_view v = ztr_view_from_buf("\xc0\xaf", 2);
    ASSERT_FALSE(ztr_view_is_valid_utf8(v));
    PASS();
}

TEST view_utf8_empty(void) {
    ASSERT(ztr_view_is_valid_utf8(ZTR_VIEW_EMPTY));
    PASS();
}

/* ---- ztr_from_view ---- */

TEST from_view_basic(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_from_view(&s, v));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST from_view_empty(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_OK, ztr_from_view(&s, ZTR_VIEW_EMPTY));
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST from_view_overwrites_existing(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "original"));
    ztr_view v = ztr_view_from_cstr("replacement");
    ASSERT_EQ(ZTR_OK, ztr_from_view(&s, v));
    ASSERT_STR_EQ("replacement", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST from_view_self_referential(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "  hello world  "));
    ztr_view v = ztr_view_from_ztr(&s);
    ztr_view trimmed = ztr_view_trim(v);
    ASSERT_EQ(ZTR_OK, ztr_from_view(&s, trimmed));
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ASSERT_EQ((size_t)11, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST from_view_substr_of_self(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ztr_view v = ztr_view_substr(ztr_view_from_ztr(&s), 6, 5);
    ASSERT_EQ(ZTR_OK, ztr_from_view(&s, v));
    ASSERT_STR_EQ("world", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ---- ZTR_FMT / ZTR_ARG / ZTR_VIEW_FMT / ZTR_VIEW_ARG ---- */

TEST view_fmt_macros(void) {
    ztr_view v = ztr_view_from_cstr("hello");
    char buf[32];
    snprintf(buf, sizeof(buf), ZTR_VIEW_FMT, ZTR_VIEW_ARG(v));
    ASSERT_STR_EQ("hello", buf);
    PASS();
}

TEST ztr_fmt_macros(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "world"));
    char buf[32];
    snprintf(buf, sizeof(buf), ZTR_FMT, ZTR_ARG(&s));
    ASSERT_STR_EQ("world", buf);
    ztr_free(&s);
    PASS();
}

/* ---- Suite ---- */

SUITE(view) {
    /* Construction */
    RUN_TEST(view_from_cstr_basic);
    RUN_TEST(view_from_cstr_empty);
    RUN_TEST(view_from_cstr_null);
    RUN_TEST(view_from_buf_basic);
    RUN_TEST(view_from_buf_null_zero);
    RUN_TEST(view_from_buf_null_nonzero);
    RUN_TEST(view_from_buf_embedded_null);
    RUN_TEST(view_from_ztr_sso);
    RUN_TEST(view_from_ztr_heap);
    RUN_TEST(view_from_ztr_empty);
    RUN_TEST(view_from_ztr_null);
    RUN_TEST(view_substr_basic);
    RUN_TEST(view_substr_pos_past_end);
    RUN_TEST(view_substr_count_clamped);
    RUN_TEST(view_substr_zero_count);
    RUN_TEST(view_substr_full_string);
    RUN_TEST(view_substr_pos_equals_len);
    RUN_TEST(view_substr_count_npos);

    /* Accessors */
    RUN_TEST(view_at_valid_index);
    RUN_TEST(view_at_out_of_bounds);
    RUN_TEST(view_at_empty_view);
    RUN_TEST(view_lit_basic);
    RUN_TEST(view_lit_empty);
    RUN_TEST(view_empty_is_valid);

    /* Comparison */
    RUN_TEST(view_eq_same);
    RUN_TEST(view_eq_different);
    RUN_TEST(view_eq_different_lengths);
    RUN_TEST(view_eq_both_empty);
    RUN_TEST(view_eq_cstr_match);
    RUN_TEST(view_eq_cstr_no_match);
    RUN_TEST(view_eq_cstr_null);
    RUN_TEST(view_eq_cstr_empty_vs_null);
    RUN_TEST(view_cmp_equal);
    RUN_TEST(view_cmp_less_than);
    RUN_TEST(view_cmp_greater_than);
    RUN_TEST(view_cmp_prefix_is_less);
    RUN_TEST(view_cmp_both_empty);
    RUN_TEST(view_cmp_cstr_equal);
    RUN_TEST(view_cmp_cstr_less_than);
    RUN_TEST(view_cmp_cstr_greater_than);
    RUN_TEST(view_cmp_cstr_null);
    RUN_TEST(view_cmp_cstr_empty_vs_null);
    RUN_TEST(view_nocase_upper_vs_lower);
    RUN_TEST(view_nocase_different);
    RUN_TEST(view_nocase_different_lengths);
    RUN_TEST(view_nocase_both_empty);
    RUN_TEST(view_nocase_cstr_match);
    RUN_TEST(view_nocase_cstr_null);

    /* Search (view needle) */
    RUN_TEST(view_find_present);
    RUN_TEST(view_find_not_present);
    RUN_TEST(view_find_with_offset);
    RUN_TEST(view_find_empty_needle);
    RUN_TEST(view_find_needle_longer_than_view);
    RUN_TEST(view_find_start_past_end);
    RUN_TEST(view_find_in_empty_view);
    RUN_TEST(view_rfind_last_occurrence);
    RUN_TEST(view_rfind_not_found);
    RUN_TEST(view_rfind_start_limits_window);
    RUN_TEST(view_rfind_empty_needle);
    RUN_TEST(view_rfind_needle_longer_than_view);
    RUN_TEST(view_contains_true);
    RUN_TEST(view_contains_false);
    RUN_TEST(view_starts_with_yes);
    RUN_TEST(view_starts_with_no);
    RUN_TEST(view_starts_with_empty_prefix);
    RUN_TEST(view_ends_with_yes);
    RUN_TEST(view_ends_with_no);
    RUN_TEST(view_ends_with_empty_suffix);
    RUN_TEST(view_contains_empty_view);
    RUN_TEST(view_starts_with_empty_view);
    RUN_TEST(view_ends_with_empty_view);
    RUN_TEST(view_count_multiple);
    RUN_TEST(view_count_none);
    RUN_TEST(view_count_empty_needle);
    RUN_TEST(view_count_non_overlapping);

    /* Search (cstr needle) */
    RUN_TEST(view_find_cstr_basic);
    RUN_TEST(view_rfind_cstr_basic);
    RUN_TEST(view_contains_cstr_true);
    RUN_TEST(view_starts_with_cstr_yes);
    RUN_TEST(view_ends_with_cstr_yes);
    RUN_TEST(view_count_cstr_basic);
    RUN_TEST(view_cstr_null_needle);

    /* Search (char) */
    RUN_TEST(view_find_char_present);
    RUN_TEST(view_find_char_not_present);
    RUN_TEST(view_find_char_start_past_end);
    RUN_TEST(view_find_char_empty_view);
    RUN_TEST(view_rfind_char_last);
    RUN_TEST(view_rfind_char_with_start);
    RUN_TEST(view_rfind_char_not_found);
    RUN_TEST(view_contains_char_true);
    RUN_TEST(view_contains_char_false);
    RUN_TEST(view_contains_char_empty_view);

    /* Trimming */
    RUN_TEST(view_trim_basic);
    RUN_TEST(view_trim_start_only);
    RUN_TEST(view_trim_end_only);
    RUN_TEST(view_trim_no_whitespace);
    RUN_TEST(view_trim_all_whitespace);
    RUN_TEST(view_trim_empty_view);
    RUN_TEST(view_trim_mixed_whitespace);
    RUN_TEST(view_trim_start_all_whitespace);
    RUN_TEST(view_trim_start_empty);
    RUN_TEST(view_trim_end_all_whitespace);
    RUN_TEST(view_trim_end_empty);

    /* Slice manipulation */
    RUN_TEST(view_remove_prefix_basic);
    RUN_TEST(view_remove_prefix_too_large);
    RUN_TEST(view_remove_prefix_zero);
    RUN_TEST(view_remove_suffix_basic);
    RUN_TEST(view_remove_suffix_too_large);
    RUN_TEST(view_remove_suffix_zero);
    RUN_TEST(view_remove_prefix_exact);
    RUN_TEST(view_remove_suffix_exact);

    /* Split */
    RUN_TEST(view_split_basic);
    RUN_TEST(view_split_empty_parts);
    RUN_TEST(view_split_trailing_delimiter);
    RUN_TEST(view_split_no_delimiter);
    RUN_TEST(view_split_empty_source);
    RUN_TEST(view_split_empty_delimiter);
    RUN_TEST(view_split_multi_char_delimiter);
    RUN_TEST(view_split_begin_cstr_basic);
    RUN_TEST(view_split_begin_cstr_null_delimiter);

    /* Utility */
    RUN_TEST(view_is_ascii_true);
    RUN_TEST(view_is_ascii_false);
    RUN_TEST(view_is_ascii_empty);
    RUN_TEST(view_utf8_valid_ascii);
    RUN_TEST(view_utf8_valid_multibyte);
    RUN_TEST(view_utf8_invalid_truncated);
    RUN_TEST(view_utf8_invalid_overlong);
    RUN_TEST(view_utf8_empty);

    /* Interop */
    RUN_TEST(from_view_basic);
    RUN_TEST(from_view_empty);
    RUN_TEST(from_view_overwrites_existing);
    RUN_TEST(from_view_self_referential);
    RUN_TEST(from_view_substr_of_self);

    /* Printf macros */
    RUN_TEST(view_fmt_macros);
    RUN_TEST(ztr_fmt_macros);
}
