#!/bin/bash
# usage: run_corpus.sh <engine-binary> <inp-dir> <out-dir> [engine flag]
# Runs every .inp in <inp-dir>; writes NAME.rpt/.out and NAME.log to <out-dir>.
BIN=$1; IN=$2; OUT=$3; FLAG=$4
mkdir -p "$OUT"; rm -f "$OUT/_rc.txt"; cd "$IN" || exit 1
for f in *.inp; do
  b="${f%.inp}"
  timeout 600 "$BIN" $FLAG "$f" "$OUT/$b.rpt" "$OUT/$b.out" > "$OUT/$b.log" 2>&1
  echo "$? $b" >> "$OUT/_rc.txt"
done
