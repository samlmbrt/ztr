#include "greatest.h"

/* Suite declarations — one per test file. */
extern SUITE(lifecycle);
extern SUITE(accessors);
extern SUITE(comparison);
extern SUITE(search);
extern SUITE(mutation);
extern SUITE(transform);
extern SUITE(split_join);
extern SUITE(utf8);
extern SUITE(interop);
extern SUITE(edge_cases);

GREATEST_MAIN_DEFS();

int main(int argc, char **argv) {
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(lifecycle);
    RUN_SUITE(accessors);
    RUN_SUITE(comparison);
    RUN_SUITE(search);
    RUN_SUITE(mutation);
    RUN_SUITE(transform);
    RUN_SUITE(split_join);
    RUN_SUITE(utf8);
    RUN_SUITE(interop);
    RUN_SUITE(edge_cases);

    GREATEST_MAIN_END();
}
