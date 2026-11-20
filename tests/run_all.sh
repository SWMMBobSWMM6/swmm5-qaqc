#!/bin/bash
# Rerun the evidence in the README. Run from the repository root.
#
#   tests/run_all.sh <EPA v5.2.4 checkout> <folder of .inp models> <work dir>
#
# Needs gcc, python3 and, for the WebAssembly part, node and Emscripten (emcc).
# The corpus used for the README is BatchSWMMRunner's public/samples (113
# models): https://github.com/dickinsonre/BatchSWMMRunner
#
# The SWMM6 part is separate because it needs a 350,000-line C++ build:
#   scripts/build_swmm6.sh <work dir>             (QA/QC build)
#   and a stock build of the same commit, then the same three run_corpus.sh
#   passes and  verify_corpus.py ... --skip-c5
set -e
EPA=$(readlink -f "$1"); CORPUS=$(readlink -f "$2"); W=$(readlink -f "$3")
HERE=$(pwd); mkdir -p "$W"

# 1. the unmodified engine, straight from EPA's tag (FORTIFY off: stock 5.2.4
#    trips glibc's realpath() buffer check on Linux)
gcc -O2 -w -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=0 -o "$W/swmm5_stock" \
    "$EPA"/src/solver/*.c "$EPA"/src/run/main.c -I"$EPA"/src/solver -I"$EPA"/src/solver/include -lm

# 2. the QA/QC engine, from this repository
scripts/build_native_swmm5.sh "$W/swmm5_qaqc"
"$W/swmm5_qaqc" --catalog "$W/qaqc_variables.json"
cmp "$W/qaqc_variables.json" qaqc_variables.json && echo "catalog in the repository matches the engine"

# 3. three passes over the corpus
scripts/run_corpus.sh "$W/swmm5_stock" "$CORPUS" "$W/stock"
scripts/run_corpus.sh "$W/swmm5_qaqc"  "$CORPUS" "$W/qaqc"
scripts/run_corpus.sh "$W/swmm5_qaqc"  "$CORPUS" "$W/qaqc_stockmode" --stock

# 4. the checks
python3 tests/verify_corpus.py "$CORPUS" "$W/stock" "$W/qaqc" "$W/qaqc_stockmode" \
    qaqc_variables.json "$W/verify_swmm5.json"

# 5. WebAssembly: stock 5.2.4 compiled the same way is the control
if command -v emcc >/dev/null && command -v node >/dev/null; then
  export EM_CACHE=${EM_CACHE:-/tmp/emcache}
  mkdir -p "$W/wasm5_stock_build"
  emcc "$EPA"/src/solver/*.c -I"$EPA"/src/solver -I"$EPA"/src/solver/include -O2 -w \
    -s MODULARIZE=1 -s EXPORT_NAME=createSwmm5StockModule -s ENVIRONMENT=web,worker \
    -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_FUNCTIONS='["_swmm_run","_swmm_getVersion","_malloc","_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["FS","ccall","cwrap","getValue","UTF8ToString","stringToUTF8"]' \
    -o "$W/wasm5_stock_build/swmm5-stock.js"
  T=tests/wasm_corpus.cjs
  node $T "$W/wasm5_stock_build/swmm5-stock.js" createSwmm5StockModule swmm5 "$CORPUS" "$W/stock" "$W/wasm5_stock" 1
  node $T wasm/swmm5-qaqc/swmm5-qaqc.js createSwmm5QaqcModule swmm5 "$CORPUS" "$W/qaqc" "$W/wasm5_qaqc" 1
  node $T wasm/swmm5-qaqc/swmm5-qaqc.js createSwmm5QaqcModule swmm5 "$CORPUS" "$W/stock" "$W/wasm5_qaqc_stockmode" 1 stock
  python3 tests/verify_corpus.py "$CORPUS" "$W/wasm5_stock" "$W/wasm5_qaqc" "$W/wasm5_qaqc_stockmode" \
      qaqc_variables.json "$W/verify_wasm5.json"
fi
