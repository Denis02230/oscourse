#!/usr/bin/env bash
set -e

OUT=oscourse_debug.zip

rm -f "$OUT"

zip -r "$OUT" \
    kern \
    fs \
    lib \
    llvm/asan \
    inc \
    conf \
    GNUmakefile \
    jos.out \
    jos.out.date \
    qemu.log 2>/dev/null || true

echo "Created $OUT"
