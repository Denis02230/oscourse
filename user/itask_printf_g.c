#include "itask_test.h"

void umain(int argc, char **argv) {
    (void)argc; (void)argv;

    const char *TAG = "ITASK_PRINTF_G";
    char buf[256];

    // default precision=6, trims trailing zeros (no '#')
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%g", 1.0);
    if (!itask_expect_str(TAG, "snprintf(\"%g\", 1.0)", buf, "1")) return;

    // with '#': do not trim, keep zeros to precision-1 significant digits
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%#.6g", 1.0);
    if (!itask_expect_str(TAG, "snprintf(\"%#.6g\", 1.0)", buf, "1.00000")) return;

    // choose exponential when exp >= precision
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%g", 1000000.0);
    if (!itask_expect_str(TAG, "snprintf(\"%g\", 1000000.0)", buf, "1e+06")) return;

    // uppercase G
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%G", 1000000.0);
    if (!itask_expect_str(TAG, "snprintf(\"%G\", 1000000.0)", buf, "1E+06")) return;

    cprintf("%s: OK\n", TAG);
}
