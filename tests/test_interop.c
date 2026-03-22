#include "greatest.h"
#include "ztr.h"

#include <string.h>

/* ---------------------------------------------------------------------------
 * ztr_data_mut
 * ------------------------------------------------------------------------- */

/* ztr_data_mut on an SSO string returns a pointer into the inline buffer.
 * Writing through it then calling ztr_set_len should produce the right
 * content and length. */
TEST data_mut_sso_write_then_set_len(void) {
    ztr s;
    ztr_init(&s);

    /* Reserve space without promoting to heap -- a 10-byte request stays SSO
     * on 64-bit (ZTR_SSO_CAP == 15). */
    ztr_err err = ztr_reserve(&s, 10);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&s)); /* should still be SSO */

    char *buf = ztr_data_mut(&s);
    ASSERT(buf != NULL);

    /* Write bytes directly, bypassing the normal API. */
    memcpy(buf, "hello", 5);

    err = ztr_set_len(&s, 5);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    /* Null terminator must be placed at position 5. */
    ASSERT_EQ('\0', buf[5]);

    ztr_free(&s);
    PASS();
}

/* ztr_data_mut on a heap string returns the heap buffer pointer.
 * Writing through it then calling ztr_set_len should work identically. */
TEST data_mut_heap_write_then_set_len(void) {
    ztr s;
    /* Force heap allocation by requesting a capacity beyond ZTR_SSO_CAP. */
    ztr_err err = ztr_with_cap(&s, ZTR_SSO_CAP + 1);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&s));

    char *buf = ztr_data_mut(&s);
    ASSERT(buf != NULL);
    /* The pointer must equal what the public accessor returns. */
    ASSERT_EQ(buf, ztr_data_mut(&s));

    memcpy(buf, "world", 5);

    err = ztr_set_len(&s, 5);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("world", ztr_cstr(&s));
    ASSERT_EQ('\0', buf[5]);

    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * ztr_set_len
 * ------------------------------------------------------------------------- */

/* Setting a valid shorter length truncates logically but keeps bytes intact
 * up to the new null terminator. */
TEST set_len_valid_shorter(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "abcdefgh");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)8, ztr_len(&s));

    err = ztr_set_len(&s, 3);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)3, ztr_len(&s));
    ASSERT_STR_EQ("abc", ztr_cstr(&s));
    /* Null terminator at new position. */
    ASSERT_EQ('\0', ztr_cstr(&s)[3]);

    ztr_free(&s);
    PASS();
}

/* Setting length to zero is valid and produces an empty string. */
TEST set_len_to_zero(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "nonempty");
    ASSERT_EQ(ZTR_OK, err);

    err = ztr_set_len(&s, 0);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* Setting length equal to the current length is a valid no-op. */
TEST set_len_same_length(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "same");
    ASSERT_EQ(ZTR_OK, err);

    err = ztr_set_len(&s, 4);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("same", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* Setting length equal to capacity is valid (uses every allocated byte). */
TEST set_len_equal_to_capacity(void) {
    ztr s;
    /* with_cap gives us exactly cap bytes of usable space. */
    ztr_err err = ztr_with_cap(&s, 8);
    ASSERT_EQ(ZTR_OK, err);

    size_t cap = ztr_capacity(&s);
    char *buf = ztr_data_mut(&s);
    memset(buf, 'x', cap);

    err = ztr_set_len(&s, cap);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(cap, ztr_len(&s));
    ASSERT_EQ('\0', ztr_cstr(&s)[cap]);

    ztr_free(&s);
    PASS();
}

/* A length larger than the current capacity must return ZTR_ERR_OUT_OF_RANGE. */
TEST set_len_exceeds_capacity_returns_error(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "hi");
    ASSERT_EQ(ZTR_OK, err);

    size_t cap = ztr_capacity(&s);
    err = ztr_set_len(&s, cap + 1);
    ASSERT_EQ(ZTR_ERR_OUT_OF_RANGE, err);

    /* Original string must be unmodified. */
    ASSERT_EQ((size_t)2, ztr_len(&s));
    ASSERT_STR_EQ("hi", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* A length larger than ZTR_MAX_LEN must return ZTR_ERR_OVERFLOW. */
TEST set_len_exceeds_max_len_returns_overflow(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "hi");
    ASSERT_EQ(ZTR_OK, err);

    err = ztr_set_len(&s, ZTR_MAX_LEN + 1);
    ASSERT_EQ(ZTR_ERR_OVERFLOW, err);

    /* Original string must be unmodified. */
    ASSERT_EQ((size_t)2, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

/* set_len null-terminates: byte at new_len must be '\0' after the call. */
TEST set_len_null_terminates(void) {
    ztr s;
    /* Build "abcde" then shrink to 2 -- verify null at index 2. */
    ztr_err err = ztr_from(&s, "abcde");
    ASSERT_EQ(ZTR_OK, err);

    err = ztr_set_len(&s, 2);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ('\0', ztr_cstr(&s)[2]);
    ASSERT_EQ('a', ztr_cstr(&s)[0]);
    ASSERT_EQ('b', ztr_cstr(&s)[1]);

    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * ztr_detach
 * ------------------------------------------------------------------------- */

/* Detaching a heap string transfers ownership of the heap pointer.
 * After detach the ztr must be reset to an empty, valid state. */
TEST detach_heap_string(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "a heap-allocated string that is long enough");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&s));

    const char *original_data = ztr_data_mut(&s); /* pointer before detach */

    char *out = NULL;
    err = ztr_detach(&s, &out);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(out != NULL);

    /* The transferred pointer must point to the same memory. */
    ASSERT_EQ(original_data, out);
    ASSERT_STR_EQ("a heap-allocated string that is long enough", out);

    /* The ztr must now be empty and usable. */
    ASSERT(ztr_is_empty(&s));
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));

    /* We own the pointer now; free it directly. */
    free(out);

    ztr_free(&s);
    PASS();
}

/* Detaching an SSO string must allocate a fresh heap copy and hand it back.
 * The returned pointer is null-terminated and contains the original content. */
TEST detach_sso_string(void) {
    ztr s;
    /* "sso" is well within ZTR_SSO_CAP (15 bytes). */
    ztr_err err = ztr_from(&s, "sso");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&s));

    char *out = NULL;
    err = ztr_detach(&s, &out);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(out != NULL);
    ASSERT_STR_EQ("sso", out);

    /* The pointer must not be pointing into the ztr's own sso buffer since
     * the ztr is stack-allocated and the caller needs an independent copy. */
    ASSERT(out < (char *)&s || out > (char *)&s + sizeof(s));

    /* The ztr must be empty after detach. */
    ASSERT(ztr_is_empty(&s));

    free(out);
    ztr_free(&s);
    PASS();
}

/* Detaching an empty string must succeed and return a valid empty C string. */
TEST detach_empty_string(void) {
    ztr s;
    ztr_init(&s);

    char *out = NULL;
    ztr_err err = ztr_detach(&s, &out);
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(out != NULL);
    ASSERT_STR_EQ("", out);

    ASSERT(ztr_is_empty(&s));

    free(out);
    ztr_free(&s);
    PASS();
}

/* ---------------------------------------------------------------------------
 * ztr_swap
 * ------------------------------------------------------------------------- */

/* Swapping two different strings exchanges both content and length. */
TEST swap_two_different_strings(void) {
    ztr a, b;
    ztr_err err;

    err = ztr_from(&a, "alpha");
    ASSERT_EQ(ZTR_OK, err);
    err = ztr_from(&b, "beta");
    ASSERT_EQ(ZTR_OK, err);

    ztr_swap(&a, &b);

    ASSERT_STR_EQ("beta", ztr_cstr(&a));
    ASSERT_EQ((size_t)4, ztr_len(&a));
    ASSERT_STR_EQ("alpha", ztr_cstr(&b));
    ASSERT_EQ((size_t)5, ztr_len(&b));

    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

/* Swapping a string with itself is a no-op; content is preserved. */
TEST swap_same_string_is_noop(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "self");
    ASSERT_EQ(ZTR_OK, err);

    ztr_swap(&s, &s);

    ASSERT_STR_EQ("self", ztr_cstr(&s));
    ASSERT_EQ((size_t)4, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

/* Swapping an SSO string with a heap string produces the expected exchange. */
TEST swap_sso_with_heap(void) {
    ztr sso, heap;
    ztr_err err;

    /* "tiny" fits in SSO; the long string forces heap allocation. */
    err = ztr_from(&sso, "tiny");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT_FALSE(ztr_p_is_heap(&sso));

    err = ztr_from(&heap, "this string is definitely longer than fifteen bytes");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&heap));

    ztr_swap(&sso, &heap);

    /* After swap: sso holds what was in heap, and heap holds what was in sso. */
    ASSERT_STR_EQ("this string is definitely longer than fifteen bytes", ztr_cstr(&sso));
    ASSERT_STR_EQ("tiny", ztr_cstr(&heap));

    ztr_free(&sso);
    ztr_free(&heap);
    PASS();
}

/* Swapping two empty strings leaves both empty. */
TEST swap_two_empty_strings(void) {
    ztr a, b;
    ztr_init(&a);
    ztr_init(&b);

    ztr_swap(&a, &b);

    ASSERT(ztr_is_empty(&a));
    ASSERT(ztr_is_empty(&b));

    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

/* Swapping two heap strings exchanges heap pointers and capacities. */
TEST swap_two_heap_strings(void) {
    ztr a, b;
    ztr_err err;

    err = ztr_from(&a, "a long string that must live on the heap, first");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&a));

    err = ztr_from(&b, "another long string that must also live on the heap");
    ASSERT_EQ(ZTR_OK, err);
    ASSERT(ztr_p_is_heap(&b));

    ztr_swap(&a, &b);

    ASSERT_STR_EQ("another long string that must also live on the heap", ztr_cstr(&a));
    ASSERT_STR_EQ("a long string that must live on the heap, first", ztr_cstr(&b));

    ztr_free(&a);
    ztr_free(&b);
    PASS();
}

/* ---------------------------------------------------------------------------
 * Suite
 * ------------------------------------------------------------------------- */

SUITE(interop) {
    /* ztr_data_mut */
    RUN_TEST(data_mut_sso_write_then_set_len);
    RUN_TEST(data_mut_heap_write_then_set_len);

    /* ztr_set_len */
    RUN_TEST(set_len_valid_shorter);
    RUN_TEST(set_len_to_zero);
    RUN_TEST(set_len_same_length);
    RUN_TEST(set_len_equal_to_capacity);
    RUN_TEST(set_len_exceeds_capacity_returns_error);
    RUN_TEST(set_len_exceeds_max_len_returns_overflow);
    RUN_TEST(set_len_null_terminates);

    /* ztr_detach */
    RUN_TEST(detach_heap_string);
    RUN_TEST(detach_sso_string);
    RUN_TEST(detach_empty_string);

    /* ztr_swap */
    RUN_TEST(swap_two_different_strings);
    RUN_TEST(swap_same_string_is_noop);
    RUN_TEST(swap_sso_with_heap);
    RUN_TEST(swap_two_empty_strings);
    RUN_TEST(swap_two_heap_strings);
}
