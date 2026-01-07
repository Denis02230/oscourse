#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/types.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static const struct {
	const char *fmt;
	double f;
	const char *expect;
} fp_tests[] = {
	/* basic form, handling of exponent/precision for 0 */
	{ "%e", 0.0, "0.000000e+00" },
	{ "%f", 0.0, "0.000000" },
	{ "%g", 0.0, "0" },
	{ "%#g", 0.0, "0.00000" },

	/* rounding */
	{ "%f", 1.1, "1.100000" },
	{ "%f", 1.2, "1.200000" },
	{ "%f", 1.3, "1.300000" },
	{ "%f", 1.4, "1.400000" },
	{ "%f", 1.5, "1.500000" },
	{ "%.4f", 1.06125, "1.0612" },
	{ "%.2f", 1.375, "1.38" },
	{ "%.1f", 1.375, "1.4" },
	{ "%.15f", 1.1, "1.100000000000000" },
	{ "%.16f", 1.1, "1.1000000000000001" },
	{ "%.17f", 1.1, "1.10000000000000009" },
	{ "%.2e", 1500001.0, "1.50e+06" },
	{ "%.2e", 1505000.0, "1.50e+06" },
	{ "%.2e", 1505000.00000095367431640625, "1.51e+06" },
	{ "%.2e", 1505001.0, "1.51e+06" },
	{ "%.2e", 1506000.0, "1.51e+06" },
	
	/* correctness in DBL_DIG places */
	{ "%.15g", 1.23456789012345, "1.23456789012345" },

	/* correct choice of notation for %g */
	{ "%g", 0.0001, "0.0001" },
	{ "%g", 0.00001, "1e-05" },
	{ "%g", 123456, "123456" },
	{ "%g", 1234567, "1.23457e+06" },
	{ "%.7g", 1234567, "1234567" },
	{ "%.7g", 12345678, "1.234568e+07" },
	{ "%.8g", 0.1, "0.1" },
	{ "%.9g", 0.1, "0.1" },
	{ "%.10g", 0.1, "0.1" },
	{ "%.11g", 0.1, "0.1" },

	/* pi in double precision, printed to a few extra places */
	{ "%.15f", 3.141592653589793238462643383279502884, "3.141592653589793" },
	{ "%.18f", 3.141592653589793238462643383279502884, "3.141592653589793116" },

	/* exact conversion of large integers */
	{ "%.0f", 340282366920938463463374607431768211456.0,
	         "340282366920938463463374607431768211456" },

	{ NULL, 0.0, NULL }
};

static int run_one(const char *fmt, double x, const char *expect) {
    char buf[256];
    memset(buf, 0, sizeof(buf));

    int n = snprintf(buf, sizeof(buf), fmt, x);
    int want_n = (int)strlen(expect);

    if (n != want_n) {
        cprintf("FAIL len: fmt='%s' x=%g got_n=%d want_n=%d out='%s' expect='%s'\n",
                fmt, x, n, want_n, buf, expect);
        return -1;
    }
    if (strcmp(buf, expect) != 0) {
        cprintf("FAIL str: fmt='%s' x=%g out='%s' expect='%s'\n",
                fmt, x, buf, expect);
        return -1;
    }
    return 0;
}

void umain(int argc, char **argv) {
    int fails = 0;

    for (int i = 0; fp_tests[i].fmt; i++) {
        if (run_one(fp_tests[i].fmt, fp_tests[i].f, fp_tests[i].expect) < 0)
            fails++;
    }

    if (fails == 0) {
        cprintf("ITASK_MUSL_SNPRINTF_FP: OK\n");
    } else {
        cprintf("ITASK_MUSL_SNPRINTF_FP: FAIL (%d)\n", fails);
    }
}
