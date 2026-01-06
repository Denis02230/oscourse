#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/string.h>

void
umain(int argc, char **argv)
{
    (void)argc; (void)argv;

    cprintf("ITASK_PRINTF: START\n");

    char buf[128];
    memset(buf, 0, sizeof(buf));

    snprintf(buf, sizeof(buf), "%.2f", 1.5);

    const char *expect = "1.50";
    if (strcmp(buf, expect) != 0) {
        cprintf("ITASK_PRINTF: FAIL\n");
        cprintf("  case: snprintf(\"%%.2f\", 1.5)\n");
        cprintf("  got: \"%s\"\n", buf);
        cprintf("  expected: \"%s\"\n", expect);
        return;
    }

    cprintf("ITASK_PRINTF: OK\n");
}
