#include "greatest.h"
#include "ztr.h"

/* TODO: add tests for NULL args, overflow, empty needles,
   self-referential aliasing, embedded nulls, SSO boundary conditions */

TEST placeholder(void) {
    PASS();
}

SUITE(edge_cases) {
    RUN_TEST(placeholder);
}
