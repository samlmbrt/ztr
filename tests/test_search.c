#include "greatest.h"
#include "ztr.h"

#include <string.h>

/* ---- ztr_find ---- */

TEST find_needle_present(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(6, ztr_find(&s, "world", 0));
    ztr_free(&s);
    PASS();
}

TEST find_needle_not_present(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(ZTR_NPOS, ztr_find(&s, "xyz", 0));
    ztr_free(&s);
    PASS();
}

TEST find_with_start_offset_skips_first(void) {
    /* "abab" — first "ab" is at 0, second at 2. Starting from 1 must find 2. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abab"));
    ASSERT_EQ(2, ztr_find(&s, "ab", 1));
    ztr_free(&s);
    PASS();
}

TEST find_start_offset_finds_first(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abab"));
    ASSERT_EQ(0, ztr_find(&s, "ab", 0));
    ztr_free(&s);
    PASS();
}

TEST find_multiple_occurrences_returns_first(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "aababcab"));
    /* "ab" appears at indices 1, 2(?), 4(?); first from start=0 is index 1 */
    size_t pos = ztr_find(&s, "ab", 0);
    ASSERT_EQ(1, pos);
    ztr_free(&s);
    PASS();
}

TEST find_at_beginning(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcdef"));
    ASSERT_EQ(0, ztr_find(&s, "abc", 0));
    ztr_free(&s);
    PASS();
}

TEST find_at_end(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcdef"));
    ASSERT_EQ(3, ztr_find(&s, "def", 0));
    ztr_free(&s);
    PASS();
}

TEST find_empty_needle_returns_start(void) {
    /* An empty needle matches at every position; from start=0 it returns 0,
     * from start=3 it returns 3. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(0, ztr_find(&s, "", 0));
    ASSERT_EQ(3, ztr_find(&s, "", 3));
    ztr_free(&s);
    PASS();
}

TEST find_needle_longer_than_haystack(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi"));
    ASSERT_EQ(ZTR_NPOS, ztr_find(&s, "hello world", 0));
    ztr_free(&s);
    PASS();
}

TEST find_single_char_needle(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcabc"));
    ASSERT_EQ(0, ztr_find(&s, "a", 0));
    ASSERT_EQ(3, ztr_find(&s, "a", 1));
    ztr_free(&s);
    PASS();
}

TEST find_start_past_end_returns_npos(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ASSERT_EQ(ZTR_NPOS, ztr_find(&s, "a", 10));
    ztr_free(&s);
    PASS();
}

TEST find_in_empty_haystack(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_NPOS, ztr_find(&s, "x", 0));
    ztr_free(&s);
    PASS();
}

TEST find_needle_is_entire_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "exact"));
    ASSERT_EQ(0, ztr_find(&s, "exact", 0));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_rfind ---- */

TEST rfind_finds_last_occurrence(void) {
    /* "abab" — last "ab" is at index 2. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abab"));
    ASSERT_EQ(2, ztr_rfind(&s, "ab", ZTR_NPOS));
    ztr_free(&s);
    PASS();
}

TEST rfind_single_occurrence(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(6, ztr_rfind(&s, "world", ZTR_NPOS));
    ztr_free(&s);
    PASS();
}

TEST rfind_not_found(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(ZTR_NPOS, ztr_rfind(&s, "xyz", ZTR_NPOS));
    ztr_free(&s);
    PASS();
}

TEST rfind_start_limits_search_window(void) {
    /* "abab" with start=1: the search looks backwards from position 1,
     * so only "ab" at index 0 is within range. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abab"));
    ASSERT_EQ(0, ztr_rfind(&s, "ab", 1));
    ztr_free(&s);
    PASS();
}

TEST rfind_npos_means_full_search(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "xyzxyz"));
    /* ZTR_NPOS → search the whole string; last "xyz" is at index 3. */
    ASSERT_EQ(3, ztr_rfind(&s, "xyz", ZTR_NPOS));
    ztr_free(&s);
    PASS();
}

TEST rfind_empty_needle(void) {
    /* Empty needle with ZTR_NPOS should match at the end of the string. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    size_t pos = ztr_rfind(&s, "", ZTR_NPOS);
    ASSERT_EQ(ztr_len(&s), pos);
    ztr_free(&s);
    PASS();
}

TEST rfind_needle_at_start(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcdef"));
    ASSERT_EQ(0, ztr_rfind(&s, "abc", ZTR_NPOS));
    ztr_free(&s);
    PASS();
}

TEST rfind_needle_longer_than_haystack(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi"));
    ASSERT_EQ(ZTR_NPOS, ztr_rfind(&s, "hello world", ZTR_NPOS));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_contains ---- */

TEST contains_true(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT(ztr_contains(&s, "world"));
    ztr_free(&s);
    PASS();
}

TEST contains_false(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_FALSE(ztr_contains(&s, "xyz"));
    ztr_free(&s);
    PASS();
}

TEST contains_empty_needle_returns_true(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT(ztr_contains(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST contains_empty_needle_in_empty_string(void) {
    ztr s;
    ztr_init(&s);
    ASSERT(ztr_contains(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST contains_nonempty_needle_in_empty_string(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_FALSE(ztr_contains(&s, "x"));
    ztr_free(&s);
    PASS();
}

TEST contains_needle_is_entire_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "exact"));
    ASSERT(ztr_contains(&s, "exact"));
    ztr_free(&s);
    PASS();
}

TEST contains_needle_longer_than_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi"));
    ASSERT_FALSE(ztr_contains(&s, "hello world"));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_starts_with ---- */

TEST starts_with_yes(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT(ztr_starts_with(&s, "hello"));
    ztr_free(&s);
    PASS();
}

TEST starts_with_no(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_FALSE(ztr_starts_with(&s, "world"));
    ztr_free(&s);
    PASS();
}

TEST starts_with_empty_prefix_is_true(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT(ztr_starts_with(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST starts_with_empty_prefix_empty_string(void) {
    ztr s;
    ztr_init(&s);
    ASSERT(ztr_starts_with(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST starts_with_prefix_longer_than_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi"));
    ASSERT_FALSE(ztr_starts_with(&s, "hello world"));
    ztr_free(&s);
    PASS();
}

TEST starts_with_exact_match(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "exact"));
    ASSERT(ztr_starts_with(&s, "exact"));
    ztr_free(&s);
    PASS();
}

TEST starts_with_single_char_yes(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ASSERT(ztr_starts_with(&s, "a"));
    ztr_free(&s);
    PASS();
}

TEST starts_with_single_char_no(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ASSERT_FALSE(ztr_starts_with(&s, "b"));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_ends_with ---- */

TEST ends_with_yes(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT(ztr_ends_with(&s, "world"));
    ztr_free(&s);
    PASS();
}

TEST ends_with_no(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_FALSE(ztr_ends_with(&s, "hello"));
    ztr_free(&s);
    PASS();
}

TEST ends_with_empty_suffix_is_true(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT(ztr_ends_with(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST ends_with_empty_suffix_empty_string(void) {
    ztr s;
    ztr_init(&s);
    ASSERT(ztr_ends_with(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST ends_with_suffix_longer_than_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi"));
    ASSERT_FALSE(ztr_ends_with(&s, "hello world"));
    ztr_free(&s);
    PASS();
}

TEST ends_with_exact_match(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "exact"));
    ASSERT(ztr_ends_with(&s, "exact"));
    ztr_free(&s);
    PASS();
}

TEST ends_with_single_char_yes(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ASSERT(ztr_ends_with(&s, "c"));
    ztr_free(&s);
    PASS();
}

TEST ends_with_single_char_no(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abc"));
    ASSERT_FALSE(ztr_ends_with(&s, "b"));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_count ---- */

TEST count_zero_occurrences(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(0, ztr_count(&s, "xyz"));
    ztr_free(&s);
    PASS();
}

TEST count_one_occurrence(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello world"));
    ASSERT_EQ(1, ztr_count(&s, "world"));
    ztr_free(&s);
    PASS();
}

TEST count_multiple_occurrences(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "abcabcabc"));
    ASSERT_EQ(3, ztr_count(&s, "abc"));
    ztr_free(&s);
    PASS();
}

TEST count_non_overlapping_only(void) {
    /* "aaa" contains "aa" at positions 0 and 1 if overlapping, but
     * non-overlapping count advances past each match: one match at 0,
     * then the search resumes at 2 where only "a" remains -> count = 1. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "aaa"));
    ASSERT_EQ(1, ztr_count(&s, "aa"));
    ztr_free(&s);
    PASS();
}

TEST count_non_overlapping_aaaa(void) {
    /* "aaaa" has two non-overlapping "aa": positions 0 and 2. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "aaaa"));
    ASSERT_EQ(2, ztr_count(&s, "aa"));
    ztr_free(&s);
    PASS();
}

TEST count_empty_needle_returns_zero(void) {
    /* An empty needle is a degenerate case; the function returns 0. */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hello"));
    ASSERT_EQ(0, ztr_count(&s, ""));
    ztr_free(&s);
    PASS();
}

TEST count_in_empty_haystack(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(0, ztr_count(&s, "x"));
    ztr_free(&s);
    PASS();
}

TEST count_needle_longer_than_haystack(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "hi"));
    ASSERT_EQ(0, ztr_count(&s, "hello world"));
    ztr_free(&s);
    PASS();
}

TEST count_needle_is_entire_string(void) {
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "exact"));
    ASSERT_EQ(1, ztr_count(&s, "exact"));
    ztr_free(&s);
    PASS();
}

TEST count_adjacent_non_overlapping(void) {
    /* "ababab" contains three non-overlapping "ab". */
    ztr s;
    ASSERT_EQ(ZTR_OK, ztr_from(&s, "ababab"));
    ASSERT_EQ(3, ztr_count(&s, "ab"));
    ztr_free(&s);
    PASS();
}

/* ---- Suite ---- */

SUITE(search) {
    /* ztr_find */
    RUN_TEST(find_needle_present);
    RUN_TEST(find_needle_not_present);
    RUN_TEST(find_with_start_offset_skips_first);
    RUN_TEST(find_start_offset_finds_first);
    RUN_TEST(find_multiple_occurrences_returns_first);
    RUN_TEST(find_at_beginning);
    RUN_TEST(find_at_end);
    RUN_TEST(find_empty_needle_returns_start);
    RUN_TEST(find_needle_longer_than_haystack);
    RUN_TEST(find_single_char_needle);
    RUN_TEST(find_start_past_end_returns_npos);
    RUN_TEST(find_in_empty_haystack);
    RUN_TEST(find_needle_is_entire_string);

    /* ztr_rfind */
    RUN_TEST(rfind_finds_last_occurrence);
    RUN_TEST(rfind_single_occurrence);
    RUN_TEST(rfind_not_found);
    RUN_TEST(rfind_start_limits_search_window);
    RUN_TEST(rfind_npos_means_full_search);
    RUN_TEST(rfind_empty_needle);
    RUN_TEST(rfind_needle_at_start);
    RUN_TEST(rfind_needle_longer_than_haystack);

    /* ztr_contains */
    RUN_TEST(contains_true);
    RUN_TEST(contains_false);
    RUN_TEST(contains_empty_needle_returns_true);
    RUN_TEST(contains_empty_needle_in_empty_string);
    RUN_TEST(contains_nonempty_needle_in_empty_string);
    RUN_TEST(contains_needle_is_entire_string);
    RUN_TEST(contains_needle_longer_than_string);

    /* ztr_starts_with */
    RUN_TEST(starts_with_yes);
    RUN_TEST(starts_with_no);
    RUN_TEST(starts_with_empty_prefix_is_true);
    RUN_TEST(starts_with_empty_prefix_empty_string);
    RUN_TEST(starts_with_prefix_longer_than_string);
    RUN_TEST(starts_with_exact_match);
    RUN_TEST(starts_with_single_char_yes);
    RUN_TEST(starts_with_single_char_no);

    /* ztr_ends_with */
    RUN_TEST(ends_with_yes);
    RUN_TEST(ends_with_no);
    RUN_TEST(ends_with_empty_suffix_is_true);
    RUN_TEST(ends_with_empty_suffix_empty_string);
    RUN_TEST(ends_with_suffix_longer_than_string);
    RUN_TEST(ends_with_exact_match);
    RUN_TEST(ends_with_single_char_yes);
    RUN_TEST(ends_with_single_char_no);

    /* ztr_count */
    RUN_TEST(count_zero_occurrences);
    RUN_TEST(count_one_occurrence);
    RUN_TEST(count_multiple_occurrences);
    RUN_TEST(count_non_overlapping_only);
    RUN_TEST(count_non_overlapping_aaaa);
    RUN_TEST(count_empty_needle_returns_zero);
    RUN_TEST(count_in_empty_haystack);
    RUN_TEST(count_needle_longer_than_haystack);
    RUN_TEST(count_needle_is_entire_string);
    RUN_TEST(count_adjacent_non_overlapping);
}
