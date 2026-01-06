#include "itask_test.h"

void umain(int argc, char **argv) {
    (void)argc; (void)argv;

    const char *TAG = "ITASK_PRINTF_E";
    char buf[256];

    // default precision = 6
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%e", 1.0);
    if (!itask_expect_str(TAG, "snprintf(\"%e\", 1.0)", buf, "1.000000e+00")) return;

    // explicit precision
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%.3e", 1.0);
    if (!itask_expect_str(TAG, "snprintf(\"%.3e\", 1.0)", buf, "1.000e+00")) return;

    // uppercase
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%E", 1.0);
    if (!itask_expect_str(TAG, "snprintf(\"%E\", 1.0)", buf, "1.000000E+00")) return;

    cprintf("%s: OK\n", TAG);
}
