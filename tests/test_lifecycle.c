#include "greatest.h"
#include "ztr.h"

#include <string.h>

#include "test_helpers.h"

/* ---- ztr_init ---- */

TEST init_produces_empty_string(void) {
    ztr s;
    /* Fill with garbage so we know init zeroes things out. */
    memset(&s, 0xFF, sizeof(s));
    ztr_init(&s);

    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    /* After init the string must be safe to free. */
    ztr_free(&s);
    PASS();
}

/* ---- Zero-initialised struct is a valid empty string ---- */

TEST zero_init_is_valid_empty_string(void) {
    ztr s = {0};

    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_from ---- */

TEST from_null_produces_empty_string(void) {
    ztr s;
    ztr_err err = ztr_from(&s, NULL);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST from_empty_cstr(void) {
    ztr s;
    ztr_err err = ztr_from(&s, "");

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST from_short_string_uses_sso(void) {
    /* "hello" — length 5, well within ZTR_SSO_CAP. */
    ztr s;
    ztr_err err = ztr_from(&s, "hello");

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    /* Must be null-terminated. */
    ASSERT_EQ('\0', ztr_cstr(&s)[5]);
    /* SSO strings must not be heap-allocated. */
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST from_boundary_sso_14(void) {
    /* 14 chars — one below the SSO cap. */
    const char *str = "12345678901234";
    ztr s;
    ztr_err err = ztr_from(&s, str);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)14, ztr_len(&s));
    ASSERT_STR_EQ(str, ztr_cstr(&s));
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST from_boundary_sso_15(void) {
    /* Exactly ZTR_SSO_CAP — still SSO. */
    ztr s;
    ztr_err err = ztr_from(&s, SSO_STR);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)15, ztr_len(&s));
    ASSERT_STR_EQ(SSO_STR, ztr_cstr(&s));
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST from_boundary_heap_16(void) {
    /* 16 chars — one past SSO cap, must go to heap. */
    ztr s;
    ztr_err err = ztr_from(&s, HEAP_STR);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)16, ztr_len(&s));
    ASSERT_STR_EQ(HEAP_STR, ztr_cstr(&s));
    /* Heap capacity must be at least the string length. */
    ASSERT(ztr_capacity(&s) >= 16);
    ztr_free(&s);
    PASS();
}

TEST from_long_string_uses_heap(void) {
    ztr s;
    ztr_err err = ztr_from(&s, LONG_STR);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(strlen(LONG_STR), ztr_len(&s));
    ASSERT_STR_EQ(LONG_STR, ztr_cstr(&s));
    ASSERT(ztr_capacity(&s) >= strlen(LONG_STR));
    ztr_free(&s);
    PASS();
}

TEST from_very_long_string(void) {
    /* 256-character string. */
    char buf[257];
    memset(buf, 'x', 256);
    buf[256] = '\0';

    ztr s;
    ztr_err err = ztr_from(&s, buf);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)256, ztr_len(&s));
    ASSERT_STR_EQ(buf, ztr_cstr(&s));
    ASSERT(ztr_capacity(&s) >= 256);
    ztr_free(&s);
    PASS();
}

TEST from_result_is_null_terminated(void) {
    /* Verify the byte immediately after the content is '\0'. */
    ztr sso, heap;
    ztr_from(&sso, "hi");
    ztr_from(&heap, HEAP_STR);

    ASSERT_EQ('\0', ztr_cstr(&sso)[ztr_len(&sso)]);
    ASSERT_EQ('\0', ztr_cstr(&heap)[ztr_len(&heap)]);

    ztr_free(&sso);
    ztr_free(&heap);
    PASS();
}

/* ---- ztr_from_buf ---- */

TEST from_buf_with_explicit_length(void) {
    const char *src = "hello world";
    ztr s;
    ztr_err err = ztr_from_buf(&s, src, 5); /* Only "hello". */

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("hello", ztr_cstr(&s));
    ASSERT_EQ('\0', ztr_cstr(&s)[5]);
    ztr_free(&s);
    PASS();
}

TEST from_buf_with_embedded_nulls(void) {
    /* The buffer contains a null byte in the middle. */
    const char buf[] = {'a', '\0', 'b', '\0', 'c'};
    ztr s;
    ztr_err err = ztr_from_buf(&s, buf, sizeof(buf));

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(sizeof(buf), ztr_len(&s));
    /* Content must be preserved byte-for-byte. */
    ASSERT_EQ(0, memcmp(ztr_cstr(&s), buf, sizeof(buf)));
    /* The buffer must still be null-terminated after its content. */
    ASSERT_EQ('\0', ztr_cstr(&s)[sizeof(buf)]);
    ztr_free(&s);
    PASS();
}

TEST from_buf_empty(void) {
    const char src[] = "ignored";
    ztr s;
    ztr_err err = ztr_from_buf(&s, src, 0);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST from_buf_sso_boundary(void) {
    /* 15 bytes — exactly ZTR_SSO_CAP. */
    char buf[15];
    memset(buf, 'z', 15);

    ztr s;
    ztr_err err = ztr_from_buf(&s, buf, 15);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)15, ztr_len(&s));
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ASSERT_EQ(0, memcmp(ztr_cstr(&s), buf, 15));
    ztr_free(&s);
    PASS();
}

TEST from_buf_heap_boundary(void) {
    /* 16 bytes — one past SSO cap. */
    char buf[16];
    memset(buf, 'z', 16);

    ztr s;
    ztr_err err = ztr_from_buf(&s, buf, 16);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)16, ztr_len(&s));
    ASSERT(ztr_capacity(&s) >= 16);
    ASSERT_EQ(0, memcmp(ztr_cstr(&s), buf, 16));
    ztr_free(&s);
    PASS();
}

TEST from_buf_embedded_nulls_heap(void) {
    /* A 32-byte buffer with embedded nulls, forcing heap allocation. */
    char buf[32];
    for (int i = 0; i < 32; i++) {
        buf[i] = (char)(i % 8); /* Includes many '\0' bytes. */
    }

    ztr s;
    ztr_err err = ztr_from_buf(&s, buf, 32);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)32, ztr_len(&s));
    ASSERT_EQ(0, memcmp(ztr_cstr(&s), buf, 32));
    ztr_free(&s);
    PASS();
}

/* ---- ztr_with_cap ---- */

TEST with_cap_zero(void) {
    ztr s;
    ztr_err err = ztr_with_cap(&s, 0);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    /* Cap 0 stays SSO. */
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ztr_free(&s);
    PASS();
}

TEST with_cap_small_stays_sso(void) {
    /* Requesting less than or equal to ZTR_SSO_CAP should not heap-allocate. */
    ztr s;
    ztr_err err = ztr_with_cap(&s, ZTR_SSO_CAP);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST with_cap_large_uses_heap(void) {
    ztr s;
    ztr_err err = ztr_with_cap(&s, 256);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT(ztr_capacity(&s) >= 256);
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST with_cap_string_is_null_terminated(void) {
    ztr s;
    ztr_with_cap(&s, 128);
    /* Even though length is 0, cstr must be null-terminated. */
    ASSERT_EQ('\0', ztr_cstr(&s)[0]);
    ztr_free(&s);
    PASS();
}

/* ---- ztr_clone ---- */

TEST clone_sso_string(void) {
    ztr src, dst;
    ztr_from(&src, "hello");
    ztr_err err = ztr_clone(&dst, &src);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(ztr_len(&src), ztr_len(&dst));
    ASSERT_STR_EQ(ztr_cstr(&src), ztr_cstr(&dst));
    /* Must be independent — mutate src and dst should be unchanged. */
    ztr_assign(&src, "world");
    ASSERT_STR_EQ("hello", ztr_cstr(&dst));

    ztr_free(&src);
    ztr_free(&dst);
    PASS();
}

TEST clone_heap_string(void) {
    ztr src, dst;
    ztr_from(&src, HEAP_STR);
    ztr_err err = ztr_clone(&dst, &src);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(ztr_len(&src), ztr_len(&dst));
    ASSERT_STR_EQ(ztr_cstr(&src), ztr_cstr(&dst));

    ztr_free(&src);
    ztr_free(&dst);
    PASS();
}

TEST clone_empty_string(void) {
    ztr src, dst;
    ztr_init(&src);
    ztr_err err = ztr_clone(&dst, &src);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&dst));
    ASSERT_STR_EQ("", ztr_cstr(&dst));

    ztr_free(&src);
    ztr_free(&dst);
    PASS();
}

TEST clone_dst_equals_src_is_noop(void) {
    /* dst == src: must return OK and leave s unchanged. */
    ztr s;
    ztr_from(&s, "aliased");
    size_t orig_len = ztr_len(&s);

    ztr_err err = ztr_clone(&s, &s);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(orig_len, ztr_len(&s));
    ASSERT_STR_EQ("aliased", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST clone_long_string(void) {
    ztr src, dst;
    ztr_from(&src, LONG_STR);
    ztr_err err = ztr_clone(&dst, &src);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(ztr_len(&src), ztr_len(&dst));
    ASSERT_STR_EQ(ztr_cstr(&src), ztr_cstr(&dst));

    ztr_free(&src);
    ztr_free(&dst);
    PASS();
}

/* ---- ztr_assign ---- */

TEST assign_sso_over_sso(void) {
    ztr s;
    ztr_from(&s, "hello");
    ztr_err err = ztr_assign(&s, "world");

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("world", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST assign_heap_with_shorter_string(void) {
    /* Start with a heap string, then assign something shorter — the heap
       buffer should be reused (cap does not shrink). */
    ztr s;
    ztr_from(&s, LONG_STR);
    size_t old_cap = ztr_capacity(&s);

    ztr_err err = ztr_assign(&s, "short");

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("short", ztr_cstr(&s));
    /* Capacity should not decrease (buffer was reused). */
    ASSERT(ztr_capacity(&s) == old_cap);

    ztr_free(&s);
    PASS();
}

TEST assign_sso_with_longer_heap_string(void) {
    /* Grow from SSO into heap via assign. */
    ztr s;
    ztr_from(&s, "hi");
    ztr_err err = ztr_assign(&s, HEAP_STR);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(strlen(HEAP_STR), ztr_len(&s));
    ASSERT_STR_EQ(HEAP_STR, ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST assign_null_clears_string(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    ztr_err err = ztr_assign(&s, NULL);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));

    ztr_free(&s);
    PASS();
}

TEST assign_self_referential_cstr(void) {
    /* Assigning from a pointer that lives inside s's own buffer. */
    ztr s;
    ztr_from(&s, "hello world");
    /* Point cstr at the middle of s's buffer — the "world" substring. */
    const char *inner = ztr_cstr(&s) + 6;
    ztr_err err = ztr_assign(&s, inner);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)5, ztr_len(&s));
    ASSERT_STR_EQ("world", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST assign_empty_string(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    ztr_err err = ztr_assign(&s, "");

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

/* ---- ztr_assign_buf ---- */

TEST assign_buf_basic(void) {
    ztr s;
    ztr_from(&s, "old content");
    ztr_err err = ztr_assign_buf(&s, "newdata", 4); /* Only "newd". */

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("newd", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST assign_buf_empty(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    ztr_err err = ztr_assign_buf(&s, "anything", 0);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST assign_buf_with_embedded_nulls(void) {
    const char buf[] = {'x', '\0', 'y'};
    ztr s;
    ztr_init(&s);
    ztr_err err = ztr_assign_buf(&s, buf, sizeof(buf));

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(sizeof(buf), ztr_len(&s));
    ASSERT_EQ(0, memcmp(ztr_cstr(&s), buf, sizeof(buf)));

    ztr_free(&s);
    PASS();
}

TEST assign_buf_self_referential(void) {
    /* Assign a slice of s's own buffer back to s. */
    ztr s;
    ztr_from(&s, "abcdefghijklmnopqrstuvwxyz"); /* heap string */
    const char *inner = ztr_cstr(&s) + 3;       /* points to "defg..." */
    ztr_err err = ztr_assign_buf(&s, inner, 4); /* assign "defg" */

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)4, ztr_len(&s));
    ASSERT_STR_EQ("defg", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST assign_buf_grows_from_sso_to_heap(void) {
    /* Build a 32-byte payload and assign into an SSO string. */
    char payload[32];
    memset(payload, 'Q', sizeof(payload));

    ztr s;
    ztr_from(&s, "tiny");
    ztr_err err = ztr_assign_buf(&s, payload, sizeof(payload));

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ(sizeof(payload), ztr_len(&s));
    ASSERT_EQ(0, memcmp(ztr_cstr(&s), payload, sizeof(payload)));
    ASSERT(ztr_capacity(&s) >= sizeof(payload));

    ztr_free(&s);
    PASS();
}

/* ---- ztr_move ---- */

TEST move_sso_string(void) {
    ztr src, dst;
    ztr_from(&src, "hello");
    ztr_init(&dst);
    ztr_move(&dst, &src);

    /* dst now owns the content. */
    ASSERT_STR_EQ("hello", ztr_cstr(&dst));
    ASSERT_EQ((size_t)5, ztr_len(&dst));
    /* src must be reset to empty. */
    ASSERT_EQ((size_t)0, ztr_len(&src));
    ASSERT(ztr_is_empty(&src));

    ztr_free(&dst);
    ztr_free(&src);
    PASS();
}

TEST move_heap_string(void) {
    ztr src, dst;
    ztr_from(&src, HEAP_STR);
    ztr_init(&dst);
    ztr_move(&dst, &src);

    ASSERT_STR_EQ(HEAP_STR, ztr_cstr(&dst));
    ASSERT_EQ(strlen(HEAP_STR), ztr_len(&dst));
    ASSERT(ztr_is_empty(&src));

    ztr_free(&dst);
    ztr_free(&src);
    PASS();
}

TEST move_dst_equals_src_is_noop(void) {
    ztr s;
    ztr_from(&s, "same");
    ztr_move(&s, &s);

    /* Value must be intact. */
    ASSERT_STR_EQ("same", ztr_cstr(&s));
    ASSERT_EQ((size_t)4, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

TEST move_overwrites_existing_dst(void) {
    /* dst already holds a heap string that must be freed during move. */
    ztr src, dst;
    ztr_from(&src, LONG_STR);
    ztr_from(&dst, HEAP_STR); /* dst also on heap */
    ztr_move(&dst, &src);

    ASSERT_STR_EQ(LONG_STR, ztr_cstr(&dst));
    ASSERT(ztr_is_empty(&src));

    ztr_free(&dst);
    ztr_free(&src);
    PASS();
}

/* ---- ztr_free ---- */

TEST free_heap_string(void) {
    ztr s;
    ztr_from(&s, HEAP_STR);
    /* Must not crash or report a leak when freed. */
    ztr_free(&s);
    /* After free the struct must be a valid empty string. */
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    PASS();
}

TEST free_sso_string(void) {
    ztr s;
    ztr_from(&s, "hello");
    ztr_free(&s);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    PASS();
}

TEST free_null_pointer_is_safe(void) {
    /* ztr_free(NULL) must not crash. */
    ztr_free(NULL);
    PASS();
}

TEST free_double_free_is_safe(void) {
    /* ztr_free resets the struct to all-zeros, so a second call is safe. */
    ztr s;
    ztr_from(&s, HEAP_STR);
    ztr_free(&s);
    ztr_free(&s); /* Must not crash or corrupt memory. */
    PASS();
}

TEST free_sso_double_free_is_safe(void) {
    ztr s;
    ztr_from(&s, "hi");
    ztr_free(&s);
    ztr_free(&s);
    PASS();
}

/* ---- ztr_fmt ---- */

#ifndef ZTR_NO_FMT

TEST fmt_basic_integer(void) {
    ztr s;
    ztr_err err = ztr_fmt(&s, "%d", 42);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_STR_EQ("42", ztr_cstr(&s));
    ASSERT_EQ((size_t)2, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

TEST fmt_empty_format_string(void) {
    ztr s;
    ztr_err err = ztr_fmt(&s, "");

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)0, ztr_len(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST fmt_literal_only(void) {
    ztr s;
    ztr_err err = ztr_fmt(&s, "hello world");

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_STR_EQ("hello world", ztr_cstr(&s));
    ASSERT_EQ((size_t)11, ztr_len(&s));

    ztr_free(&s);
    PASS();
}

TEST fmt_multiple_args(void) {
    ztr s;
    ztr_err err = ztr_fmt(&s, "%s=%d", "answer", 42);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_STR_EQ("answer=42", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

TEST fmt_result_short_stays_sso(void) {
    /* "x" — length 1, within SSO cap. */
    ztr s;
    ztr_err err = ztr_fmt(&s, "%c", 'x');

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)1, ztr_len(&s));
    ASSERT_EQ(ZTR_SSO_CAP, ztr_capacity(&s));

    ztr_free(&s);
    PASS();
}

TEST fmt_large_result_uses_heap(void) {
    /* Format a string longer than ZTR_SSO_CAP. */
    ztr s;
    /* "%032d" produces "00000000000000000000000000000001" — 32 chars. */
    ztr_err err = ztr_fmt(&s, "%032d", 1);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_EQ((size_t)32, ztr_len(&s));
    ASSERT(ztr_capacity(&s) >= 32);
    ASSERT_EQ('\0', ztr_cstr(&s)[32]);

    ztr_free(&s);
    PASS();
}

TEST fmt_overwrites_previous_content(void) {
    /* ztr_fmt initialises s; any previous content is irrelevant, but the
       caller must ensure s is not already initialised (or has been freed).
       Here we simply confirm the result is correct from a fresh struct. */
    ztr s;
    ztr_err err = ztr_fmt(&s, "pi=%.4f", 3.14159);

    ASSERT_EQ(ZTR_OK, err);
    ASSERT_STR_EQ("pi=3.1416", ztr_cstr(&s));

    ztr_free(&s);
    PASS();
}

#endif /* ZTR_NO_FMT */

/* ---- Suite registration ---- */

SUITE(lifecycle) {
    /* Zero / init */
    RUN_TEST(zero_init_is_valid_empty_string);
    RUN_TEST(init_produces_empty_string);

    /* ztr_from */
    RUN_TEST(from_null_produces_empty_string);
    RUN_TEST(from_empty_cstr);
    RUN_TEST(from_short_string_uses_sso);
    RUN_TEST(from_boundary_sso_14);
    RUN_TEST(from_boundary_sso_15);
    RUN_TEST(from_boundary_heap_16);
    RUN_TEST(from_long_string_uses_heap);
    RUN_TEST(from_very_long_string);
    RUN_TEST(from_result_is_null_terminated);

    /* ztr_from_buf */
    RUN_TEST(from_buf_with_explicit_length);
    RUN_TEST(from_buf_with_embedded_nulls);
    RUN_TEST(from_buf_empty);
    RUN_TEST(from_buf_sso_boundary);
    RUN_TEST(from_buf_heap_boundary);
    RUN_TEST(from_buf_embedded_nulls_heap);

    /* ztr_with_cap */
    RUN_TEST(with_cap_zero);
    RUN_TEST(with_cap_small_stays_sso);
    RUN_TEST(with_cap_large_uses_heap);
    RUN_TEST(with_cap_string_is_null_terminated);

    /* ztr_clone */
    RUN_TEST(clone_sso_string);
    RUN_TEST(clone_heap_string);
    RUN_TEST(clone_empty_string);
    RUN_TEST(clone_dst_equals_src_is_noop);
    RUN_TEST(clone_long_string);

    /* ztr_assign */
    RUN_TEST(assign_sso_over_sso);
    RUN_TEST(assign_heap_with_shorter_string);
    RUN_TEST(assign_sso_with_longer_heap_string);
    RUN_TEST(assign_null_clears_string);
    RUN_TEST(assign_self_referential_cstr);
    RUN_TEST(assign_empty_string);

    /* ztr_assign_buf */
    RUN_TEST(assign_buf_basic);
    RUN_TEST(assign_buf_empty);
    RUN_TEST(assign_buf_with_embedded_nulls);
    RUN_TEST(assign_buf_self_referential);
    RUN_TEST(assign_buf_grows_from_sso_to_heap);

    /* ztr_move */
    RUN_TEST(move_sso_string);
    RUN_TEST(move_heap_string);
    RUN_TEST(move_dst_equals_src_is_noop);
    RUN_TEST(move_overwrites_existing_dst);

    /* ztr_free */
    RUN_TEST(free_heap_string);
    RUN_TEST(free_sso_string);
    RUN_TEST(free_null_pointer_is_safe);
    RUN_TEST(free_double_free_is_safe);
    RUN_TEST(free_sso_double_free_is_safe);

#ifndef ZTR_NO_FMT
    /* ztr_fmt */
    RUN_TEST(fmt_basic_integer);
    RUN_TEST(fmt_empty_format_string);
    RUN_TEST(fmt_literal_only);
    RUN_TEST(fmt_multiple_args);
    RUN_TEST(fmt_result_short_stays_sso);
    RUN_TEST(fmt_large_result_uses_heap);
    RUN_TEST(fmt_overwrites_previous_content);
#endif
}
