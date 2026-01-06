#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/string.h>

static void fail(const char *case_desc, const char *got, const char *exp) {
    cprintf("ITASK_PRINTF: FAIL\n");
    cprintf("  case: %s\n", case_desc);
    cprintf("  got: \"%s\"\n", got);
    cprintf("  expected: \"%s\"\n", exp);
}

void
umain(int argc, char **argv)
{
    (void)argc; (void)argv;

    cprintf("ITASK_PRINTF: START\n");

    char buf[128];

    // Phase 0: sprintf path alive
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "X=%d", 7);
    if (strcmp(buf, "X=7") != 0) {
        fail("sprintf(\"X=%d\", 7)", buf, "X=7");
        return;
    }

    // Phase 1: minimal %f
    memset(buf, 0, sizeof(buf));
    snprintf(buf, sizeof(buf), "%.2f", 1.5);
    if (strcmp(buf, "1.50") != 0) {
        fail("snprintf(\"%.2f\", 1.5)", buf, "1.50");
        return;
    }

    cprintf("ITASK_PRINTF: OK\n");
}
