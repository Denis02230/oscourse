#include <inc/types.h>
#include <inc/string.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inc/error.h>
#include <inc/fpconv.h>

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

// TODO temporary, will be replaced with a proper algorithm
int fp_format_f(char *out, size_t outsz, double x, fp_fmt_t fmt) {
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

    int prec = fmt.precision;
    if (prec < 0) prec = 6;
    if (prec > 18) prec = 18; // limited in this simple version

    // sign
    char sgn[2] = {0,0};
    int sn = write_sign(sgn, sizeof(sgn), x, fmt);
    if (sn < 0) return sn;
    append(out, outsz, sgn);

    double ax = x;
    if (dbl_signbit(ax)) ax = -ax;

    // integer part (works reliably for |x| < 2^63-ish)
    uint64_t ip = (uint64_t)ax;
    double frac = ax - (double)ip;

    // scaled fractional with rounding (simple)
    uint64_t scale = pow10_u64(prec);
    uint64_t fp = 0;
    if (prec > 0) {
        double y = frac * (double)scale;
        uint64_t fl = (uint64_t)y;
        double r = y - (double)fl;
        if (r > 0.5) fl++;
        else if (r == 0.5 && (fl & 1)) fl++; // ties-to-even (best effort)
        fp = fl;

        if (fp >= scale) { fp = 0; ip++; } // carry
    }

    // write integer part
    char intbuf[64];
    if (!u64_to_dec(intbuf, sizeof(intbuf), ip)) return -E_INVAL;
    append(out, outsz, intbuf);

    // dot / frac
    if (prec > 0 || fmt.alt) {
        append(out, outsz, ".");
    }
    if (prec > 0) {
        // pad leading zeros for fractional digits
        char fracbuf[32];
        char decbuf[32];
        size_t dn = u64_to_dec(decbuf, sizeof(decbuf), fp);
        if (dn == 0) return -E_INVAL;

        size_t need0 = (size_t)prec > dn ? (size_t)prec - dn : 0;
        size_t pos = 0;
        if (need0 + dn + 1 > sizeof(fracbuf)) return -E_INVAL;
        for (size_t i = 0; i < need0; i++) fracbuf[pos++] = '0';
        memcpy(fracbuf + pos, decbuf, dn + 1);
        append(out, outsz, fracbuf);
    }

    return (int)strnlen(out, outsz);
}

// stubs for now
int fp_format_e(char *out, size_t outsz, double x, fp_fmt_t fmt) {
    (void)x; (void)fmt;
    if (!out || outsz == 0) return -E_INVAL;
    // TODO: implement
    strncpy(out, fmt.upper ? "E_TODO" : "e_todo", outsz);
    out[outsz-1] = 0;
    return (int)strnlen(out, outsz);
}

int fp_format_g(char *out, size_t outsz, double x, fp_fmt_t fmt) {
    (void)x; (void)fmt;
    if (!out || outsz == 0) return -E_INVAL;
    // TODO: implement
    strncpy(out, fmt.upper ? "G_TODO" : "g_todo", outsz);
    out[outsz-1] = 0;
    return (int)strnlen(out, outsz);
}
