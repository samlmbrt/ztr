#include "greatest.h"
#include "ztr.h"

#include <string.h>

#include "test_helpers.h"

/* ---- ztr_len ---- */

TEST len_of_empty_string_is_zero(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST len_of_zero_init_is_zero(void) {
    ztr s = {0};
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST len_of_sso_string(void) {
    ztr s;
    ztr_from(&s, "hello");
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST len_of_sso_boundary_15(void) {
    ztr s;
    ztr_from(&s, SSO_STR);
    ASSERT_EQ((size_t)15, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST len_of_heap_boundary_16(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    ASSERT_EQ((size_t)16, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST len_of_heap_string(void) {
    ztr s;
    ztr_from(&s, LONG_STR);
    ASSERT_EQ(strlen(LONG_STR), ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST len_after_append_mutation(void) {
    /* After appending content, ztr_len must reflect the new length. */
    ztr s;
    ztr_from(&s, "abc");
    ztr_append(&s, "def");
    ASSERT_EQ((size_t)6, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST len_after_truncate_mutation(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    ztr_truncate(&s, 4);
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST len_after_clear_mutation(void) {
    ztr s;
    ztr_from(&s, LONG_STR);
    ztr_clear(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST len_matches_strlen_for_ascii(void) {
    /* For a string without embedded nulls, len must equal strlen(cstr). */
    ztr s;
    ztr_from(&s, LONG_STR);
    ASSERT_EQ(strlen(ztr_cstr(&s)), ztr_len(&s));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_cstr ---- */

TEST cstr_of_empty_string_is_empty_literal(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_STR_EQ("", ztr_cstr(&s));
    /* Pointer must be non-NULL. */
    ASSERT(ztr_cstr(&s) != NULL);
    ztr_free(&s);
    PASS();
}

TEST cstr_of_zero_init_is_empty_literal(void) {
    ztr s = {0};
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ASSERT(ztr_cstr(&s) != NULL);
    ztr_free(&s);
    PASS();
}

TEST cstr_of_sso_string(void) {
    ztr s;
    ztr_from(&s, "hello");
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST cstr_of_heap_string(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    ASSERT_STR_EQ(HEAP_STR, ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST cstr_is_null_terminated_sso(void) {
    ztr s;
    ztr_from(&s, SSO_STR); /* fills all 15 SSO bytes */
    const char *p = ztr_cstr(&s);
    /* The byte at index 15 must be '\0'. */
    ASSERT_EQ('\0', p[15]);
    ztr_free(&s);
    PASS();
}

TEST cstr_is_null_terminated_heap(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    const char *p = ztr_cstr(&s);
    ASSERT_EQ('\0', p[16]);
    ztr_free(&s);
    PASS();
}

TEST cstr_content_matches_source(void) {
    const char *inputs[] = {
        "", "a", "hello", SSO_STR, HEAP_STR, LONG_STR,
    };
    for (size_t i = 0; i < sizeof(inputs) / sizeof(inputs[0]); i++) {
        ztr s;
        ztr_from(&s, inputs[i]);
        ASSERT_STR_EQ(inputs[i], ztr_cstr(&s));
        ztr_free(&s);
    }
    PASS();
}

TEST cstr_stable_after_sso_to_heap_growth(void) {
    /* After a mutation that forces SSO → heap, ztr_cstr must still work. */
    ztr s;
    ztr_from(&s, "hello");
    /* Append enough to exceed SSO cap. */
    ztr_append(&s, " world - some more text here");
    ASSERT(ztr_len(&s) > ZTR_SSO_CAP);
    /* Content must still be correct. */
    ASSERT_STR_EQ("hello world - some more text here", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_is_empty ---- */

TEST is_empty_true_for_init(void) {
    ztr s;
    ztr_init(&s);
    ASSERT(ztr_is_empty(&s));
    ztr_free(&s);
    PASS();
}

TEST is_empty_true_for_zero_init(void) {
    ztr s = {0};
    ASSERT(ztr_is_empty(&s));
    ztr_free(&s);
    PASS();
}

TEST is_empty_true_for_from_empty_cstr(void) {
    ztr s;
    ztr_from(&s, "");
    ASSERT(ztr_is_empty(&s));
    ztr_free(&s);
    PASS();
}

TEST is_empty_false_for_sso_string(void) {
    ztr s;
    ztr_from(&s, "x");
    ASSERT(!ztr_is_empty(&s));
    ztr_free(&s);
    PASS();
}

TEST is_empty_false_for_heap_string(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    ASSERT(!ztr_is_empty(&s));
    ztr_free(&s);
    PASS();
}

TEST is_empty_true_after_clear(void) {
    ztr s;
    ztr_from(&s, LONG_STR);
    ztr_clear(&s);
    ASSERT(ztr_is_empty(&s));
    ztr_free(&s);
    PASS();
}

TEST is_empty_false_after_append(void) {
    ztr s;
    ztr_init(&s);
    ztr_append(&s, "a");
    ASSERT(!ztr_is_empty(&s));
    ztr_free(&s);
    PASS();
}

TEST is_empty_consistent_with_len(void) {
    /* is_empty must always agree with len == 0. */
    ztr s;

    ztr_init(&s);
    ASSERT_EQ(ztr_is_empty(&s), ztr_len(&s) == 0);

    ztr_from(&s, "hi");
    ASSERT_EQ(ztr_is_empty(&s), ztr_len(&s) == 0);

    ztr_free(&s);
    PASS();
}

/* ---- ztr_capacity ---- */

TEST capacity_of_init_is_sso_cap(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST capacity_of_zero_init_is_sso_cap(void) {
    ztr s = {0};
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST capacity_of_sso_string_is_sso_cap(void) {
    ztr s;
    /* Any string that fits in SSO must report ZTR_SSO_CAP. */
    ztr_from(&s, "hello");
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST capacity_of_sso_boundary_is_sso_cap(void) {
    ztr s;
    ztr_from(&s, SSO_STR); /* exactly 15 chars */
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST capacity_of_heap_string_at_least_len(void) {
    ztr s;
    ztr_from(&s, HEAP_STR); /* 16 chars, goes to heap */
    ASSERT(ztr_capacity(&s) >= ztr_len(&s));
    ztr_free(&s);
    PASS();
}

TEST capacity_of_heap_string_exceeds_sso_cap(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    ASSERT(ztr_capacity(&s) > ZTR_SSO_CAP);
    ztr_free(&s);
    PASS();
}

TEST capacity_from_with_cap_small(void) {
    /* ztr_with_cap(ZTR_SSO_CAP) must keep SSO mode. */
    ztr s;
    ztr_with_cap(&s, ZTR_SSO_CAP);
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST capacity_from_with_cap_large(void) {
    ztr s;
    ztr_with_cap(&s, 256);
    ASSERT(ztr_capacity(&s) >= 256);
    ztr_free(&s);
    PASS();
}

TEST capacity_does_not_decrease_after_assign_shorter(void) {
    /* Assigning a shorter string must not shrink the capacity. */
    ztr s;
    ztr_from(&s, LONG_STR);
    size_t cap_before = ztr_capacity(&s);
    ztr_assign(&s, "x");
    ASSERT(ztr_capacity(&s) == cap_before);
    ztr_free(&s);
    PASS();
}

/* ---- ztr_at ---- */

TEST at_first_char_sso(void) {
    ztr s;
    ztr_from(&s, "hello");
    ASSERT_EQ('h', ztr_at(&s, 0));
    ztr_free(&s);
    PASS();
}

TEST at_last_char_sso(void) {
    ztr s;
    ztr_from(&s, "hello");
    ASSERT_EQ('o', ztr_at(&s, 4));
    ztr_free(&s);
    PASS();
}

TEST at_middle_char_sso(void) {
    ztr s;
    ztr_from(&s, "hello");
    ASSERT_EQ('l', ztr_at(&s, 2));
    ztr_free(&s);
    PASS();
}

TEST at_first_char_heap(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    ASSERT_EQ('1', ztr_at(&s, 0));
    ztr_free(&s);
    PASS();
}

TEST at_last_char_heap(void) {
    ztr s;
    ztr_from(&s, HEAP_STR); /* "1234567890123456" — last char is '6' */
    ASSERT_EQ('6', ztr_at(&s, 15));
    ztr_free(&s);
    PASS();
}

TEST at_oob_returns_null_char(void) {
    /* Any index >= len must return '\0'. */
    ztr s;
    ztr_from(&s, "hello");
    ASSERT_EQ('\0', ztr_at(&s, 5));   /* exactly len */
    ASSERT_EQ('\0', ztr_at(&s, 100)); /* way past */
    ztr_free(&s);
    PASS();
}

TEST at_oob_empty_string(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ('\0', ztr_at(&s, 0));
    ASSERT_EQ('\0', ztr_at(&s, 1));
    ztr_free(&s);
    PASS();
}

TEST at_oob_zero_init(void) {
    ztr s = {0};
    ASSERT_EQ('\0', ztr_at(&s, 0));
    ztr_free(&s);
    PASS();
}

TEST at_all_chars_match_cstr(void) {
    /* ztr_at(s, i) must equal ztr_cstr(s)[i] for all valid indices. */
    ztr s;
    ztr_from(&s, LONG_STR);
    const char *p = ztr_cstr(&s);
    size_t len = ztr_len(&s);
    for (size_t i = 0; i < len; i++) {
        ASSERT_EQ(p[i], ztr_at(&s, i));
    }
    ztr_free(&s);
    PASS();
}

TEST at_size_t_max_oob(void) {
    /* SIZE_MAX as index must return '\0' without undefined behaviour. */
    ztr s;
    ztr_from(&s, "hello");
    ASSERT_EQ('\0', ztr_at(&s, (size_t)-1));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_err_str ---- */

TEST err_str_ok_is_non_null(void) {
    ASSERT(ztr_err_str(ZTR_OK) != NULL);
    PASS();
}

TEST err_str_alloc_is_non_null(void) {
    ASSERT(ztr_err_str(ZTR_ERR_ALLOC) != NULL);
    PASS();
}

TEST err_str_null_arg_is_non_null(void) {
    ASSERT(ztr_err_str(ZTR_ERR_NULL_ARG) != NULL);
    PASS();
}

TEST err_str_out_of_range_is_non_null(void) {
    ASSERT(ztr_err_str(ZTR_ERR_OUT_OF_RANGE) != NULL);
    PASS();
}

TEST err_str_invalid_utf8_is_non_null(void) {
    ASSERT(ztr_err_str(ZTR_ERR_INVALID_UTF8) != NULL);
    PASS();
}

TEST err_str_overflow_is_non_null(void) {
    ASSERT(ztr_err_str(ZTR_ERR_OVERFLOW) != NULL);
    PASS();
}

TEST err_str_all_codes_are_non_empty(void) {
    /* Every error string must be a non-empty C string. */
    ztr_err codes[] = {
        ZTR_OK,
        ZTR_ERR_ALLOC,
        ZTR_ERR_NULL_ARG,
        ZTR_ERR_OUT_OF_RANGE,
        ZTR_ERR_INVALID_UTF8,
        ZTR_ERR_OVERFLOW,
    };
    for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        const char *str = ztr_err_str(codes[i]);
        ASSERT(str != NULL);
        ASSERT(str[0] != '\0');
    }
    PASS();
}

TEST err_str_codes_are_distinct(void) {
    /* Each code must map to a distinct string (pointer OR content differs). */
    const char *ok = ztr_err_str(ZTR_OK);
    const char *al = ztr_err_str(ZTR_ERR_ALLOC);
    const char *nu = ztr_err_str(ZTR_ERR_NULL_ARG);
    const char *oor = ztr_err_str(ZTR_ERR_OUT_OF_RANGE);
    const char *utf = ztr_err_str(ZTR_ERR_INVALID_UTF8);
    const char *ov = ztr_err_str(ZTR_ERR_OVERFLOW);

    ASSERT(strcmp(ok, al) != 0);
    ASSERT(strcmp(ok, nu) != 0);
    ASSERT(strcmp(ok, oor) != 0);
    ASSERT(strcmp(ok, utf) != 0);
    ASSERT(strcmp(ok, ov) != 0);
    ASSERT(strcmp(al, nu) != 0);
    ASSERT(strcmp(al, oor) != 0);
    ASSERT(strcmp(al, utf) != 0);
    ASSERT(strcmp(al, ov) != 0);
    ASSERT(strcmp(nu, oor) != 0);
    ASSERT(strcmp(nu, utf) != 0);
    ASSERT(strcmp(nu, ov) != 0);
    ASSERT(strcmp(oor, utf) != 0);
    ASSERT(strcmp(oor, ov) != 0);
    ASSERT(strcmp(utf, ov) != 0);

    PASS();
}

/* ---- Suite registration ---- */

SUITE(accessors) {
    /* ztr_len */
    RUN_TEST(len_of_empty_string_is_zero);
    RUN_TEST(len_of_zero_init_is_zero);
    RUN_TEST(len_of_sso_string);
    RUN_TEST(len_of_sso_boundary_15);
    RUN_TEST(len_of_heap_boundary_16);
    RUN_TEST(len_of_heap_string);
    RUN_TEST(len_after_append_mutation);
    RUN_TEST(len_after_truncate_mutation);
    RUN_TEST(len_after_clear_mutation);
    RUN_TEST(len_matches_strlen_for_ascii);

    /* ztr_cstr */
    RUN_TEST(cstr_of_empty_string_is_empty_literal);
    RUN_TEST(cstr_of_zero_init_is_empty_literal);
    RUN_TEST(cstr_of_sso_string);
    RUN_TEST(cstr_of_heap_string);
    RUN_TEST(cstr_is_null_terminated_sso);
    RUN_TEST(cstr_is_null_terminated_heap);
    RUN_TEST(cstr_content_matches_source);
    RUN_TEST(cstr_stable_after_sso_to_heap_growth);

    /* ztr_is_empty */
    RUN_TEST(is_empty_true_for_init);
    RUN_TEST(is_empty_true_for_zero_init);
    RUN_TEST(is_empty_true_for_from_empty_cstr);
    RUN_TEST(is_empty_false_for_sso_string);
    RUN_TEST(is_empty_false_for_heap_string);
    RUN_TEST(is_empty_true_after_clear);
    RUN_TEST(is_empty_false_after_append);
    RUN_TEST(is_empty_consistent_with_len);

    /* ztr_capacity */
    RUN_TEST(capacity_of_init_is_sso_cap);
    RUN_TEST(capacity_of_zero_init_is_sso_cap);
    RUN_TEST(capacity_of_sso_string_is_sso_cap);
    RUN_TEST(capacity_of_sso_boundary_is_sso_cap);
    RUN_TEST(capacity_of_heap_string_at_least_len);
    RUN_TEST(capacity_of_heap_string_exceeds_sso_cap);
    RUN_TEST(capacity_from_with_cap_small);
    RUN_TEST(capacity_from_with_cap_large);
    RUN_TEST(capacity_does_not_decrease_after_assign_shorter);

    /* ztr_at */
    RUN_TEST(at_first_char_sso);
    RUN_TEST(at_last_char_sso);
    RUN_TEST(at_middle_char_sso);
    RUN_TEST(at_first_char_heap);
    RUN_TEST(at_last_char_heap);
    RUN_TEST(at_oob_returns_null_char);
    RUN_TEST(at_oob_empty_string);
    RUN_TEST(at_oob_zero_init);
    RUN_TEST(at_all_chars_match_cstr);
    RUN_TEST(at_size_t_max_oob);

    /* ztr_err_str */
    RUN_TEST(err_str_ok_is_non_null);
    RUN_TEST(err_str_alloc_is_non_null);
    RUN_TEST(err_str_null_arg_is_non_null);
    RUN_TEST(err_str_out_of_range_is_non_null);
    RUN_TEST(err_str_invalid_utf8_is_non_null);
    RUN_TEST(err_str_overflow_is_non_null);
    RUN_TEST(err_str_all_codes_are_non_empty);
    RUN_TEST(err_str_codes_are_distinct);
}
