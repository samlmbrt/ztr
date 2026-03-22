#include "ztr.h"

#include <stdio.h>
#include <time.h>

static double elapsed_ms(struct timespec start, struct timespec end) {
    double sec = (double)(end.tv_sec - start.tv_sec);
    double nsec = (double)(end.tv_nsec - start.tv_nsec);
    return sec * 1000.0 + nsec / 1e6;
}

static void bench_append_byte_1m(void) {
    ztr s = {0};
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < 1000000; i++) {
        ztr_append_byte(&s, 'x');
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    printf("append_byte x1M: %.2f ms (len=%zu)\n", elapsed_ms(start, end),
           ztr_len(&s));
    ztr_free(&s);
}

int main(void) {
    printf("--- ztr benchmarks ---\n\n");
    bench_append_byte_1m();
    /* TODO: add more benchmarks */
    return 0;
}
