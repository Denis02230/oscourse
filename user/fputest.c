#include <inc/lib.h>
#include <inc/string.h>

// x87 control word rounding control bits:
// RC = bits 10..11
// 00: nearest, 01: down, 10: up, 11: toward zero
#define X87_RC_MASK   (3u << 10)
#define X87_RC_DOWN   (1u << 10)
#define X87_RC_UP     (2u << 10)

static inline uint16_t
x87_get_cw(void)
{
    uint16_t cw;
    asm volatile("fnstcw %0" : "=m"(cw));
    return cw;
}

static inline void
x87_set_cw(uint16_t cw)
{
    asm volatile("fldcw %0" : : "m"(cw));
}

// convert 1.9 using current x87 rounding mode
// with RC=down => 1, RC=up => 2
static int
x87_round_1_9_to_int(void)
{
    int out = 0;
    static const double v = 1.9;
    asm volatile(
        "fldl  %1\n"
        "fistpl %0\n"
        : "=m"(out)
        : "m"(v)
        : "st"
    );
    return out;
}

static void
set_rounding(uint16_t rc_bits)
{
    uint16_t cw = x87_get_cw();
    cw = (uint16_t)((cw & ~X87_RC_MASK) | rc_bits);
    x87_set_cw(cw);
}

static int
check_rounding_expected(int expected)
{
    int got = x87_round_1_9_to_int();
    return got == expected;
}

void
umain(int argc, char **argv)
{
    (void)argc; (void)argv;

    cprintf("ITASK_FPTEST: START\n");

    envid_t child = fork();
    if (child < 0) {
        cprintf("ITASK_FPTEST: FAIL (fork)\n");
        return;
    }

    // parent uses round-down -> expects 1
    // child uses round-up    -> expects 2
    if (child == 0) {
        set_rounding(X87_RC_UP);

        for (int i = 0; i < 2000; i++) {
            // yield to force env switches.
            sys_yield();

            if (!check_rounding_expected(2)) {
                cprintf("ITASK_FPTEST: FAIL (child rounding corrupted)\n");
                return;
            }
        }

        cprintf("ITASK_FPTEST: OK (child)\n");
        return;
    } else {
        set_rounding(X87_RC_DOWN);

        for (int i = 0; i < 2000; i++) {
            sys_yield();

            if (!check_rounding_expected(1)) {
                cprintf("ITASK_FPTEST: FAIL (parent rounding corrupted)\n");
                // wait for child so run ends cleanly
                wait(child);
                return;
            }
        }

        wait(child);
        cprintf("ITASK_FPTEST: OK\n");
        return;
    }
}
