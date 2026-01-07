#include <inc/types.h>
#include <inc/string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inc/error.h>
#include <inc/fpconv.h>
#include <inc/ryu/ryu.h>

// --- small helpers ---
static size_t u64_to_dec(char *out, size_t outsz, uint64_t v) {
    char tmp[32];
    size_t n = 0;
    do {
        tmp[n++] = '0' + (v % 10);
        v /= 10;
    } while (v && n < sizeof(tmp));
    if (n + 1 > outsz) return 0;
    // reverse
    for (size_t i = 0; i < n; i++) out[i] = tmp[n - 1 - i];
    out[n] = 0;
    return n;
}

static uint64_t pow10_u64(int p) {
    static const uint64_t t[] = {
        1ULL,
        10ULL,
        100ULL,
        1000ULL,
        10000ULL,
        100000ULL,
        1000000ULL,
        10000000ULL,
        100000000ULL,
        1000000000ULL,
        10000000000ULL,
        100000000000ULL,
        1000000000000ULL,
        10000000000000ULL,
        100000000000000ULL,
        1000000000000000ULL,
        10000000000000000ULL,
        100000000000000000ULL,
        1000000000000000000ULL
    };
    if (p < 0) return 1;
    if (p > 18) return 0; // guard (this simple impl supports up to 18)
    return t[p];
}

// Very small IEEE-754 checks without libm
static inline uint64_t dbl_bits(double x) {
    uint64_t u;
    memcpy(&u, &x, sizeof(u));
    return u;
}

static inline bool dbl_is_nan(double x) {
    uint64_t u = dbl_bits(x);
    uint64_t exp = (u >> 52) & 0x7FF;
    uint64_t mant = u & ((1ULL << 52) - 1);
    return exp == 0x7FF && mant != 0;
}

static inline bool dbl_is_inf(double x) {
    uint64_t u = dbl_bits(x);
    uint64_t exp = (u >> 52) & 0x7FF;
    uint64_t mant = u & ((1ULL << 52) - 1);
    return exp == 0x7FF && mant == 0;
}

static inline bool dbl_signbit(double x) {
    return (dbl_bits(x) >> 63) != 0;
}

// Writes sign into out (if any). Returns number of chars written (0 or 1).
static int write_sign(char *out, size_t outsz, double x, fp_fmt_t fmt) {
    if (outsz < 2) return -E_INVAL;
    if (dbl_signbit(x)) { out[0] = '-'; out[1]=0; return 1; }
    if (fmt.plus) { out[0] = '+'; out[1]=0; return 1; }
    if (fmt.space) { out[0] = ' '; out[1]=0; return 1; }
    out[0]=0;
    return 0;
}

static int append(char *out, size_t outsz, const char *s) {
    size_t a = strnlen(out, outsz);
    size_t b = strnlen(s, outsz);
    if (a + b + 1 > outsz) return -E_INVAL;
    memmove(out + a, s, b + 1);
    return 0;
}

int fp_format_f(char *out, size_t outsz, double x, fp_fmt_t fmt) {
    if (!out || outsz == 0) return -E_INVAL;
    out[0] = 0;

    // NaN/Inf
    if (dbl_is_nan(x)) {
        append(out, outsz, fmt.upper ? "NAN" : "nan");
        return (int)strnlen(out, outsz);
    }
    if (dbl_is_inf(x)) {
        char sgn[2] = {0,0};
        int sn = write_sign(sgn, sizeof(sgn), x, fmt);
        if (sn < 0) return sn;
        append(out, outsz, sgn);
        append(out, outsz, fmt.upper ? "INF" : "inf");
        return (int)strnlen(out, outsz);
    }

    int prec = fmt.precision;
    if (prec < 0) prec = 6;
    if (prec > 200) prec = 200; // safety, should still fit in tmp buffers

    // sign first (handles -0.0 too)
    char sgn[2] = {0,0};
    int sn = write_sign(sgn, sizeof(sgn), x, fmt);
    if (sn < 0) return sn;
    append(out, outsz, sgn);

    double ax = x;
    if (dbl_signbit(ax)) ax = -ax;

    // Ryu fixed conversion (no sign)
    char tmp[512];
    int n = d2fixed_buffered_n(ax, (uint32_t)prec, tmp);
    if (n < 0 || n >= (int)sizeof(tmp)) return -E_INVAL;
    tmp[n] = 0;

    // '#' with precision==0: force decimal point
    if (fmt.alt && prec == 0) {
        // tmp has no '.', append it before copying
        size_t tl = strnlen(tmp, sizeof(tmp));
        if (tl + 1 < sizeof(tmp)) {
            tmp[tl] = '.';
            tmp[tl + 1] = 0;
        }
    }

    if (append(out, outsz, tmp) < 0) return -E_INVAL;
    return (int)strnlen(out, outsz);
}


static int parse_exp10_from_ryu(const char *s) {
    // expects "...e+NN" or "...E-NN"
    const char *e = strchr(s, 'e');
    if (!e) e = strchr(s, 'E');
    if (!e) return 0;

    e++; // after e/E
    int sign = 1;
    if (*e == '+') { sign = 1; e++; }
    else if (*e == '-') { sign = -1; e++; }

    int v = 0;
    while (*e >= '0' && *e <= '9') {
        v = v * 10 + (*e - '0');
        e++;
    }
    return sign * v;
}

static void trim_g_trailing_zeros(char *s, size_t cap) {
    if (!s || cap == 0) return;

    // Separate exponent part (if any) so trimming doesn't cut it off.
    char *e = strchr(s, 'e');
    if (!e) e = strchr(s, 'E');

    char expbuf[32];
    expbuf[0] = 0;

    if (e) {
        // Copy exponent substring to expbuf, then terminate mantissa at 'e/E'
        size_t explen = strnlen(e, sizeof(expbuf) - 1);
        memcpy(expbuf, e, explen);
        expbuf[explen] = 0;
        *e = 0;
    }

    // Trim mantissa trailing zeros after '.'
    char *dot = strchr(s, '.');
    if (dot) {
        size_t len = strnlen(s, cap);
        if (len > 0) {
            char *end = s + len - 1;
            while (end > dot && *end == '0') {
                *end-- = 0;
            }
            if (end == dot) {
                *end = 0; // remove '.'
            }
        }
    }

    // Re-attach exponent if present
    if (expbuf[0]) {
        size_t len = strnlen(s, cap);
        size_t explen = strnlen(expbuf, sizeof(expbuf));
        if (len + explen + 1 <= cap) {
            memcpy(s + len, expbuf, explen + 1);
        }
    }
}

int fp_format_e(char *out, size_t outsz, double x, fp_fmt_t fmt) {
    if (!out || outsz == 0) return -E_INVAL;
    out[0] = 0;

    // NaN/Inf handled here to match "nan/inf" style (not "Infinity")
    if (dbl_is_nan(x)) {
        return append(out, outsz, fmt.upper ? "NAN" : "nan"), (int)strnlen(out, outsz);
    }
    if (dbl_is_inf(x)) {
        char sgn[2] = {0,0};
        int sn = write_sign(sgn, sizeof(sgn), x, fmt);
        if (sn < 0) return sn;
        append(out, outsz, sgn);
        append(out, outsz, fmt.upper ? "INF" : "inf");
        return (int)strnlen(out, outsz);
    }

    int prec = fmt.precision;
    if (prec < 0) prec = 6;
    if (prec > 64) prec = 64; // safety

    // sign first
    char sgn[2] = {0,0};
    int sn = write_sign(sgn, sizeof(sgn), x, fmt);
    if (sn < 0) return sn;
    append(out, outsz, sgn);

    double ax = x;
    if (dbl_signbit(ax)) ax = -ax;

    char tmp[512];
    int n = d2exp_buffered_n(ax, (uint32_t)prec, tmp);
    if (n < 0 || n >= (int)sizeof(tmp)) return -E_INVAL;
    tmp[n] = 0;

    // if '#' and precision==0: must keep decimal point before exponent
    if (fmt.alt && prec == 0) {
        char *e = strchr(tmp, 'e');
        if (e) {
            // shift right by 1 to insert '.'
            size_t tail = strnlen(e, sizeof(tmp) - (e - tmp));
            if ((size_t)n + 1 < sizeof(tmp)) {
                memmove(e + 1, e, tail + 1);
                *e = '.';
            }
        }
    }

    if (fmt.upper) {
        char *e = strchr(tmp, 'e');
        if (e) *e = 'E';
    }

    if (append(out, outsz, tmp) < 0) return -E_INVAL;
    return (int)strnlen(out, outsz);
}

int fp_format_g(char *out, size_t outsz, double x, fp_fmt_t fmt) {
    if (!out || outsz == 0) return -E_INVAL;
    out[0] = 0;

    if (dbl_is_nan(x)) {
        return append(out, outsz, fmt.upper ? "NAN" : "nan"), (int)strnlen(out, outsz);
    }
    if (dbl_is_inf(x)) {
        char sgn[2] = {0,0};
        int sn = write_sign(sgn, sizeof(sgn), x, fmt);
        if (sn < 0) return sn;
        append(out, outsz, sgn);
        append(out, outsz, fmt.upper ? "INF" : "inf");
        return (int)strnlen(out, outsz);
    }

    int p = fmt.precision;
    if (p < 0) p = 6;
    if (p == 0) p = 1;
    if (p > 64) p = 64;

    double ax = x;
    if (dbl_signbit(ax)) ax = -ax;

    // Get base-10 exponent via ryu exp with precision 0: like "1e+06"
    char expbuf[64];
    int en = d2exp_buffered_n(ax, 0, expbuf);
    if (en < 0 || en >= (int)sizeof(expbuf)) return -E_INVAL;
    expbuf[en] = 0;
    int exp10 = parse_exp10_from_ryu(expbuf);

    // %g rule: use exp if exp10 < -4 or exp10 >= p
    bool use_exp = (exp10 < -4) || (exp10 >= p);

    fp_fmt_t sub = fmt;

    char tmp[512];
    int n = -E_INVAL;

    if (use_exp) {
        sub.precision = p - 1; // significant digits p => after dot p-1
        n = fp_format_e(tmp, sizeof(tmp), x, sub);
    } else {
        int frac_prec = p - (exp10 + 1);
        if (frac_prec < 0) frac_prec = 0;
        sub.precision = frac_prec;
        n = fp_format_f(tmp, sizeof(tmp), x, sub);
    }

    if (n < 0) return n;
    tmp[sizeof(tmp)-1] = 0;

    // trim trailing zeros and dot if no '#'
    if (!fmt.alt) {
        trim_g_trailing_zeros(tmp, sizeof(tmp));
    }

    // uppercase for %G: fp_format_e already uppercases 'E'; fp_format_f has no 'e'
    if (fmt.upper) {
        char *e = strchr(tmp, 'e');
        if (e) *e = 'E';
    }

    if (append(out, outsz, tmp) < 0) return -E_INVAL;
    return (int)strnlen(out, outsz);
}
