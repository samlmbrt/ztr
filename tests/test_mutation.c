#include "greatest.h"
#include "ztr.h"

/* TODO: add tests for append, append_buf, append_ztr, append_byte, append_fmt,
   insert, insert_buf, erase, replace_first, replace_all, clear, truncate,
   reserve, shrink_to_fit */

TEST placeholder(void) {
    PASS();
}

SUITE(mutation) {
    RUN_TEST(placeholder);
}
