#!/bin/bash
# Build the SWMM5 QA/QC engine to WebAssembly. Run from the repository root
# with Emscripten (tested: 3.1.51) on PATH.
set -e
S=engine/swmm5/src
O=wasm/swmm5-qaqc
export EM_CACHE=${EM_CACHE:-/tmp/emcache}
mkdir -p $O
emcc $S/solver/*.c -I$S/solver -I$S/solver/include -O2 -w \
  -s MODULARIZE=1 -s EXPORT_NAME=createSwmm5QaqcModule \
  -s ENVIRONMENT=web,worker -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS='["_swmm_run","_swmm_open","_swmm_start","_swmm_step","_swmm_end","_swmm_report","_swmm_close","_swmm_getError","_swmm_getWarnings","_swmm_getVersion","_swmm_getMassBalErr","_swmm_setQaqc","_swmm_getQaqcCount","_swmm_writeQaqcCatalog","_malloc","_free"]' \
  -s EXPORTED_RUNTIME_METHODS='["FS","ccall","cwrap","getValue","UTF8ToString","stringToUTF8"]' \
  -o $O/swmm5-qaqc.js
ls -la $O
