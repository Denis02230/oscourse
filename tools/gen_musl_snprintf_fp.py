#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
from pathlib import Path

SRC = Path("third_party/musl-libc-testsuite/snprintf.c")
OUT = Path("user/itask_musl_snprintf_fp.c")

M_PI_VALUE = "3.141592653589793238462643383279502884"

def main():
    s = SRC.read_text(encoding="utf-8", errors="replace")

    # 1) Вырезаем блок определения fp_tests[] целиком (struct + initializer + closing)
    # В оригинале это:
    # static const struct { ... } fp_tests[] = { ... { NULL, 0.0, NULL } };
    m = re.search(
        r"(static\s+const\s+struct\s*\{[^}]*\}\s*fp_tests\[\]\s*=\s*\{.*?\};)",
        s,
        flags=re.DOTALL,
    )
    if not m:
        raise SystemExit("cannot find fp_tests[] block in snprintf.c")

    fp_block = m.group(1)

    # 2) Убираем зависимость от math.h: подменяем M_PI на числовую константу
    fp_block = re.sub(r"\bM_PI\b", M_PI_VALUE, fp_block)

    # 3) Генерим JOS user program
    out = f"""\
#include <inc/lib.h>
#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/types.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

{fp_block}

static int run_one(const char *fmt, double x, const char *expect) {{
    char buf[256];
    memset(buf, 0, sizeof(buf));

    int n = snprintf(buf, sizeof(buf), fmt, x);
    int want_n = (int)strlen(expect);

    if (n != want_n) {{
        cprintf("FAIL len: fmt='%s' x=%g got_n=%d want_n=%d out='%s' expect='%s'\\n",
                fmt, x, n, want_n, buf, expect);
        return -1;
    }}
    if (strcmp(buf, expect) != 0) {{
        cprintf("FAIL str: fmt='%s' x=%g out='%s' expect='%s'\\n",
                fmt, x, buf, expect);
        return -1;
    }}
    return 0;
}}

void umain(int argc, char **argv) {{
    int fails = 0;

    for (int i = 0; fp_tests[i].fmt; i++) {{
        if (run_one(fp_tests[i].fmt, fp_tests[i].f, fp_tests[i].expect) < 0)
            fails++;
    }}

    if (fails == 0) {{
        cprintf("ITASK_MUSL_SNPRINTF_FP: OK\\n");
    }} else {{
        cprintf("ITASK_MUSL_SNPRINTF_FP: FAIL (%d)\\n", fails);
    }}
}}
"""
    OUT.write_text(out, encoding="utf-8")
    print(f"Wrote {OUT} from {SRC}")

if __name__ == "__main__":
    main()
