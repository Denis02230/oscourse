#pragma once
#include <inc/types.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int precision;     // for %f/%e: digits after '.', for %g: significant digits
    bool alt;          // '#'
    bool plus;         // '+'
    bool space;        // ' '
    bool upper;        // E/G/F => upper-case output
} fp_fmt_t;

// Writes NUL-terminated string into out. Returns length (>=0) or negative error.
int fp_format_f(char *out, size_t outsz, double x, fp_fmt_t fmt);
int fp_format_e(char *out, size_t outsz, double x, fp_fmt_t fmt);
int fp_format_g(char *out, size_t outsz, double x, fp_fmt_t fmt);
