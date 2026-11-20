#!/bin/bash
# Build the SWMM6 QA/QC engine (native and WebAssembly) from a fresh checkout
# of OpenSWMM Engine. Run from the repository root.
#   scripts/build_swmm6.sh <empty work dir> [native|wasm|both]
# JOBS sets the number of parallel compile jobs (default 2; each needs about 1 GB).
# Tested against HydroCouple/openswmm.engine, branch swmm6_rel, commit
# da2b248937c85b6caf453044c4bc98b3f1de41c1. A later commit may move an anchor;
# the apply script then stops with the anchor it could not find.
set -e
W=$1; WHAT=${2:-both}; HERE=$(pwd)
COMMIT=da2b248937c85b6caf453044c4bc98b3f1de41c1
OPTS="-DCMAKE_BUILD_TYPE=Release -DOPENSWMM_WITH_GEOPACKAGE=OFF -DOPENSWMM_WITH_HDF5=OFF -DOPENSWMM_BUILD_2D=OFF -DOPENSWMM_BUILD_GPU_PLUGIN=OFF -DOPENSWMM_INSTALL=OFF"
mkdir -p "$W" && cd "$W"
[ -d openswmm.engine ] || git clone --branch swmm6_rel https://github.com/HydroCouple/openswmm.engine.git
cd openswmm.engine && git checkout -q $COMMIT
python3 "$HERE/scripts/apply_qaqc_swmm6.py" . "$HERE/engine/swmm6" "$HERE/engine/swmm5/src/solver/qaqc.c"
if [ "$WHAT" != wasm ]; then
  cmake -S . -B build-native $OPTS
  cmake --build build-native --target openswmm -j "${JOBS:-2}"
  echo "native: $W/openswmm.engine/build-native/bin/Release/openswmm"
fi
if [ "$WHAT" != native ]; then
  export EM_CACHE=${EM_CACHE:-/tmp/emcache}
  emcmake cmake -S . -B build-wasm $OPTS -DCMAKE_C_FLAGS=-fexceptions -DCMAKE_CXX_FLAGS=-fexceptions \
    "-DCMAKE_EXE_LINKER_FLAGS=-fexceptions -sMODULARIZE=1 -sEXPORT_NAME=createSwmm6QaqcModule -sENVIRONMENT=web,worker -sALLOW_MEMORY_GROWTH=1 -sINVOKE_RUN=0 -sEXIT_RUNTIME=0 -sEXPORTED_RUNTIME_METHODS=FS,callMain,ccall,cwrap,getValue,UTF8ToString,stringToUTF8 -sEXPORTED_FUNCTIONS=_main,_malloc,_free -sSTACK_SIZE=5MB"
  cmake --build build-wasm --target openswmm -j "${JOBS:-2}"
  mkdir -p "$HERE/wasm/swmm6-qaqc"
  J=$(find build-wasm -name openswmm.js | head -1)
  sed 's/openswmm\.wasm/swmm6-qaqc.wasm/g' "$J" > "$HERE/wasm/swmm6-qaqc/swmm6-qaqc.js"
  cp "${J%.js}.wasm" "$HERE/wasm/swmm6-qaqc/swmm6-qaqc.wasm"
  ls -la "$HERE/wasm/swmm6-qaqc"
fi
