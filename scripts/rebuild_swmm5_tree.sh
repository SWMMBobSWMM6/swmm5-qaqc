#!/bin/bash
# Recreate engine/swmm5/src from EPA's v5.2.4 tag plus the QA/QC changes.
#   scripts/rebuild_swmm5_tree.sh <checkout of USEPA/Stormwater-Management-Model at v5.2.4>
set -e
EPA=$1
T=engine/swmm5/src
cp $T/solver/qaqc.c $T/solver/qaqc.h /tmp/
rm -rf $T && mkdir -p $T && cp -r "$EPA"/src/. $T/
cp /tmp/qaqc.c /tmp/qaqc.h $T/solver/
python3 scripts/apply_qaqc_swmm5.py $T
