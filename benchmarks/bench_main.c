/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Samuel Lambert
 *
 * ztr micro-benchmarks.
 *
 * Each benchmark runs the operation in a tight loop, reports wall-clock time
 * and throughput. Results are printed in a machine-parseable format:
 *
 *   bench_name  iterations  total_ms  ops_per_sec
 *
 * Build and run:  make bench
 */

#include "ztr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---- Harness ---- */

typedef struct {
    const char *name;
    void (*fn)(void);
} bench_entry;

static double elapsed_ms(struct timespec start, struct timespec end) {
    double sec = (double)(end.tv_sec - start.tv_sec);
    double nsec = (double)(end.tv_nsec - start.tv_nsec);
    return sec * 1000.0 + nsec / 1e6;
}

static void report(const char *name, int iterations, double ms) {
    double ops_per_sec = (double)iterations / (ms / 1000.0);
    printf("  %-40s %10d ops  %8.2f ms  %12.0f ops/s\n", name, iterations, ms, ops_per_sec);
}

/* Prevent the compiler from optimizing away a value. */
static volatile int bench_sink;
static void do_not_optimize(const void *p) { bench_sink = *(const volatile char *)p; }

/* ---- Lifecycle benchmarks ---- */

static void bench_from_free_sso(void) {
    enum { N = 1000000 };
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        ztr s;
        ztr_from(&s, "hello");
        do_not_optimize(&s);
        ztr_free(&s);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    report("from+free (SSO, 5 bytes)", N, elapsed_ms(t0, t1));
}

static void bench_from_free_heap(void) {
    enum { N = 1000000 };
    const char *str = "this string is longer than fifteen bytes and forces heap";
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        ztr s;
        ztr_from(&s, str);
        do_not_optimize(&s);
        ztr_free(&s);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    report("from+free (heap, 56 bytes)", N, elapsed_ms(t0, t1));
}

static void bench_clone_sso(void) {
    enum { N = 1000000 };
    ztr src;
    ztr_from(&src, "hello world!!!");
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        ztr dst;
        ztr_clone(&dst, &src);
        do_not_optimize(&dst);
        ztr_free(&dst);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    ztr_free(&src);
    report("clone+free (SSO)", N, elapsed_ms(t0, t1));
}

/* ---- Append benchmarks ---- */

static void bench_append_byte_1m(void) {
    enum { N = 1000000 };
    ztr s = {0};
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        ztr_append_byte(&s, 'x');
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    do_not_optimize(ztr_cstr(&s));
    report("append_byte x1M", N, elapsed_ms(t0, t1));
    ztr_free(&s);
}

static void bench_append_short(void) {
    enum { N = 100000 };
    ztr s = {0};
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        ztr_append(&s, "hello ");
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    do_not_optimize(ztr_cstr(&s));
    report("append \"hello \" x100K", N, elapsed_ms(t0, t1));
    ztr_free(&s);
}

static void bench_append_prealloc(void) {
    enum { N = 1000000 };
    ztr s = {0};
    ztr_reserve(&s, (size_t)N);
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        ztr_append_byte(&s, 'x');
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    do_not_optimize(ztr_cstr(&s));
    report("append_byte x1M (pre-reserved)", N, elapsed_ms(t0, t1));
    ztr_free(&s);
}

/* ---- Accessor benchmarks ---- */

static void bench_len_mixed(void) {
    enum { N = 10000000, COUNT = 8 };
    /* Mix of SSO and heap strings to stress branch prediction. */
    ztr strings[COUNT];
    ztr_from(&strings[0], "short");
    ztr_from(&strings[1], "this forces a heap allocation!!");
    ztr_from(&strings[2], "hi");
    ztr_from(&strings[3], "another heap string, longer than fifteen");
    ztr_from(&strings[4], "sso");
    ztr_from(&strings[5], "yet another long heap-allocated string here");
    ztr_from(&strings[6], "tiny");
    ztr_from(&strings[7], "and one more long one for good measure!!");

    struct timespec t0, t1;
    size_t total = 0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        total += ztr_len(&strings[i % COUNT]);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    do_not_optimize(&total);
    report("ztr_len (mixed SSO/heap) x10M", N, elapsed_ms(t0, t1));
    for (int i = 0; i < COUNT; i++) {
        ztr_free(&strings[i]);
    }
}

/* ---- Search benchmarks ---- */

static void bench_find_short_needle(void) {
    enum { N = 100000 };
    /* Build a 10KB haystack. */
    ztr s = {0};
    ztr_reserve(&s, 10240);
    for (int i = 0; i < 1024; i++) {
        ztr_append(&s, "abcdefghij");
    }
    /* Plant the needle near the end. */
    ztr_append(&s, "NEEDLE");

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        size_t pos = ztr_find(&s, "NEEDLE", 0);
        do_not_optimize(&pos);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    report("find 6-byte needle in 10KB x100K", N, elapsed_ms(t0, t1));
    ztr_free(&s);
}

static void bench_contains_miss(void) {
    enum { N = 100000 };
    ztr s = {0};
    ztr_reserve(&s, 10240);
    for (int i = 0; i < 1024; i++) {
        ztr_append(&s, "abcdefghij");
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        bool found = ztr_contains(&s, "ZZZZZ");
        do_not_optimize(&found);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    report("contains (miss) in 10KB x100K", N, elapsed_ms(t0, t1));
    ztr_free(&s);
}

static void bench_rfind(void) {
    enum { N = 100000 };
    ztr s = {0};
    ztr_reserve(&s, 10240);
    for (int i = 0; i < 1024; i++) {
        ztr_append(&s, "abcdefghij");
    }
    /* Plant the needle near the start. */
    ztr_insert(&s, 10, "NEEDLE");

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        size_t pos = ztr_rfind(&s, "NEEDLE", ZTR_NPOS);
        do_not_optimize(&pos);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    report("rfind 6-byte needle in 10KB x100K", N, elapsed_ms(t0, t1));
    ztr_free(&s);
}

/* ---- Replace benchmarks ---- */

static void bench_replace_all(void) {
    enum { N = 1000 };
    /* Build a string with many occurrences. */
    ztr template_str = {0};
    for (int i = 0; i < 1000; i++) {
        ztr_append(&template_str, "the quick brown fox ");
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        ztr s;
        ztr_clone(&s, &template_str);
        ztr_replace_all(&s, "fox", "cat");
        do_not_optimize(ztr_cstr(&s));
        ztr_free(&s);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    report("replace_all (1K occ, 20KB) x1K", N, elapsed_ms(t0, t1));
    ztr_free(&template_str);
}

/* ---- Split benchmarks ---- */

static void bench_split_iter(void) {
    enum { N = 10000 };
    /* Build a CSV-like string with 1000 fields. */
    ztr s = {0};
    for (int i = 0; i < 1000; i++) {
        if (i > 0) {
            ztr_append_byte(&s, ',');
        }
        ztr_append(&s, "field");
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        ztr_split_iter it;
        ztr_split_begin(&it, &s, ",");
        const char *part;
        size_t part_len;
        size_t count = 0;
        while (ztr_split_next(&it, &part, &part_len)) {
            count++;
        }
        do_not_optimize(&count);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    report("split_iter (1K fields) x10K", N, elapsed_ms(t0, t1));
    ztr_free(&s);
}

/* ---- UTF-8 benchmarks ---- */

static void bench_utf8_validate(void) {
    enum { N = 10000 };
    /* Build a mixed ASCII + multibyte string (~10KB). */
    ztr s = {0};
    for (int i = 0; i < 500; i++) {
        ztr_append(&s, "hello world ");
        ztr_utf8_append(&s, 0x00E9);  /* e-acute, 2 bytes */
        ztr_utf8_append(&s, 0x20AC);  /* euro sign, 3 bytes */
        ztr_utf8_append(&s, 0x1F389); /* party popper, 4 bytes */
    }

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        bool valid = ztr_is_valid_utf8(&s);
        do_not_optimize(&valid);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    report("is_valid_utf8 (~10KB mixed) x10K", N, elapsed_ms(t0, t1));
    ztr_free(&s);
}

/* ---- Comparison benchmarks ---- */

static void bench_eq_same(void) {
    enum { N = 1000000 };
    ztr a, b;
    ztr_from(&a, "a]moderately long string for comparison benchmarks!!");
    ztr_clone(&b, &a);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++) {
        bool eq = ztr_eq(&a, &b);
        do_not_optimize(&eq);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    report("eq (equal, 52 bytes) x1M", N, elapsed_ms(t0, t1));
    ztr_free(&a);
    ztr_free(&b);
}

/* ---- Main ---- */

static const bench_entry benchmarks[] = {
    {"Lifecycle", NULL},
    {"  from+free (SSO)", bench_from_free_sso},
    {"  from+free (heap)", bench_from_free_heap},
    {"  clone+free (SSO)", bench_clone_sso},

    {"Append", NULL},
    {"  append_byte x1M", bench_append_byte_1m},
    {"  append short string x100K", bench_append_short},
    {"  append_byte x1M (reserved)", bench_append_prealloc},

    {"Accessors", NULL},
    {"  ztr_len (mixed) x10M", bench_len_mixed},

    {"Search", NULL},
    {"  find (hit, 10KB) x100K", bench_find_short_needle},
    {"  contains (miss, 10KB) x100K", bench_contains_miss},
    {"  rfind (hit, 10KB) x100K", bench_rfind},

    {"Replace", NULL},
    {"  replace_all (1K occ) x1K", bench_replace_all},

    {"Split", NULL},
    {"  split_iter (1K fields) x10K", bench_split_iter},

    {"UTF-8", NULL},
    {"  is_valid_utf8 (~10KB) x10K", bench_utf8_validate},

    {"Comparison", NULL},
    {"  eq (equal, 52B) x1M", bench_eq_same},
};

int main(void) {
    printf("\n");
    printf("ztr micro-benchmarks\n");
    printf("====================\n\n");
    printf("  %-40s %10s  %8s  %12s\n", "benchmark", "iters", "time", "throughput");
    printf("  %-40s %10s  %8s  %12s\n", "----------------------------------------", "----------",
           "--------", "------------");

    size_t n = sizeof(benchmarks) / sizeof(benchmarks[0]);
    for (size_t i = 0; i < n; i++) {
        if (!benchmarks[i].fn) {
            printf("\n  [%s]\n", benchmarks[i].name);
        } else {
            benchmarks[i].fn();
        }
    }

    printf("\n");
    return 0;
}
