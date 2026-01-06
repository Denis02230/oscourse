#pragma once
#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/types.h>

static inline void itask_fail(const char *tag, const char *desc,
                              const char *got, const char *exp) {
    cprintf("%s: FAIL\n", tag);
    cprintf("  case: %s\n", desc);
    cprintf("  got: \"%s\"\n", got);
    cprintf("  expected: \"%s\"\n", exp);
}

static inline int itask_expect_str(const char *tag, const char *desc,
                                   const char *got, const char *exp) {
    if (strcmp(got, exp) != 0) {
        itask_fail(tag, desc, got, exp);
        return 0;
    }
    return 1;
}

static inline double itask_from_u64(uint64_t bits) {
    double x;
    memcpy(&x, &bits, sizeof(x));
    return x;
}

static inline double itask_pos_inf(void) { return itask_from_u64(0x7ff0000000000000ULL); }
static inline double itask_neg_inf(void) { return itask_from_u64(0xfff0000000000000ULL); }
static inline double itask_qnan(void)    { return itask_from_u64(0x7ff8000000000001ULL); }
static inline double itask_neg_zero(void){ return itask_from_u64(0x8000000000000000ULL); }
