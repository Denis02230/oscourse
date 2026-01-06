#include "itask_test.h"

void umain(int argc, char **argv) {
    (void)argc; (void)argv;

    const char *TAG = "ITASK_PRINTF_WIDTH";
    char buf[256];

    // width + zero pad
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%08.2f", 1.5);
    if (!itask_expect_str(TAG, "snprintf(\"%08.2f\", 1.5)", buf, "00001.50")) return;

    // '+' with zero pad: sign must come before zeros
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%+08.2f", 1.5);
    if (!itask_expect_str(TAG, "snprintf(\"%+08.2f\", 1.5)", buf, "+0001.50")) return;

    // ' ' with zero pad: space sign must come before zeros
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "% 08.2f", 1.5);
    if (!itask_expect_str(TAG, "snprintf(\"% 08.2f\", 1.5)", buf, " 0001.50")) return;

    // left adjust overrides zero pad
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%-8.2f", 1.5);
    if (!itask_expect_str(TAG, "snprintf(\"%-08.2f\", 1.5)", buf, "1.50    ")) return;

    cprintf("%s: OK\n", TAG);
}
