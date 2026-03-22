#include "greatest.h"
#include "ztr.h"

TEST zero_init_is_valid_empty_string(void) {
    ztr s = {0};
    ASSERT_EQ(0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ASSERT_STR_EQ("", ztr_cstr(&s));
    ztr_free(&s);
    PASS();
}

TEST init_produces_empty_string(void) {
    ztr s;
    ztr_init(&s);
    ASSERT_EQ(0, ztr_len(&s));
    ASSERT(ztr_is_empty(&s));
    ztr_free(&s);
    PASS();
}

/* TODO: add tests for from, from_buf, with_cap, fmt, clone, assign,
   assign_buf, move, free (double-free safety, NULL safety) */

SUITE(lifecycle) {
    RUN_TEST(zero_init_is_valid_empty_string);
    RUN_TEST(init_produces_empty_string);
}
