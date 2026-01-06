#include "itask_test.h"

void umain(int argc, char **argv) {
    (void)argc; (void)argv;

    const char *TAG = "ITASK_PRINTF_F";
    char buf[256];

    // default precision = 6
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%f", 1.0);
    if (!itask_expect_str(TAG, "snprintf(\"%f\", 1.0)", buf, "1.000000")) return;

    // precision
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%.2f", 1.5);
    if (!itask_expect_str(TAG, "snprintf(\"%.2f\", 1.5)", buf, "1.50")) return;

    // rounding with exact .5
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%.0f", 1.5);
    if (!itask_expect_str(TAG, "snprintf(\"%.0f\", 1.5)", buf, "2")) return;

    // '#' keeps decimal point when precision=0
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%#.0f", 1.0);
    if (!itask_expect_str(TAG, "snprintf(\"%#.0f\", 1.0)", buf, "1.")) return;

    // -0.0 must keep sign
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%f", itask_neg_zero());
    if (!itask_expect_str(TAG, "snprintf(\"%f\", -0.0)", buf, "-0.000000")) return;

    // inf / nan (lowercase for %f)
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%f", itask_pos_inf());
    if (!itask_expect_str(TAG, "snprintf(\"%f\", +inf)", buf, "inf")) return;

    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%f", itask_neg_inf());
    if (!itask_expect_str(TAG, "snprintf(\"%f\", -inf)", buf, "-inf")) return;

    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%f", itask_qnan());
    if (!itask_expect_str(TAG, "snprintf(\"%f\", nan)", buf, "nan")) return;

    cprintf("%s: OK\n", TAG);
}
