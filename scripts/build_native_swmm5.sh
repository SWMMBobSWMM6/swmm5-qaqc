#!/bin/bash
# Build the SWMM5 QA/QC engine natively. Run from the repository root.
#   scripts/build_native_swmm5.sh [output-binary]
set -e
S=engine/swmm5/src
OUT=${1:-build/swmm5-qaqc}
mkdir -p "$(dirname "$OUT")"
gcc -O2 -w -o "$OUT" $S/solver/*.c $S/run/main.c -I$S/solver -I$S/solver/include -lm
echo "built $OUT"
