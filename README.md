# swmm5-qaqc

SWMM engines that report what the solver computed, not only its answers.

This is Bob Dickinson's QA/QC build of SWMM5 brought forward to EPA SWMM 5.2.4,
compiled natively and to WebAssembly, with a second version built on OpenSWMM 6.
Beside the standard results, each link, node and subcatchment record in the
binary output file carries the solver's own intermediate values: the six terms
of the momentum equation, the end areas and hydraulic radii, the weighting and
damping factors, the Froude numbers, the node update terms, the routing step
and the trial count. They are taken from `dwflow.c` and `dynwave.c` at the
routing step, as the solver leaves them.

Nothing about the solution changes. With the QA/QC output switched on, the
report file and every standard result are bit-identical to the unmodified
engine. The evidence is below.

| | SWMM5 version | SWMM6 version |
|---|---|---|
| Base | EPA SWMM 5.2.4 (tag `v5.2.4`, commit `7952ca83`) | OpenSWMM Engine 6.0.0-alpha.4 (`swmm6_rel`, commit `da2b2489`) |
| Link variables | 67 | 64 of the same 67 |
| Node variables | 20 | 18 of the same 20 |
| Subcatchment variables | 37 | none yet |
| System variables | 6 | 5 of the same 6 |
| Engine change | 7 files changed (157 lines added, 7 changed) and 2 added (798 lines) | 7 files changed (182 lines added, 3 changed) and 4 added (709 lines) |
| WebAssembly | `wasm/swmm5-qaqc/` (476 KB) | `wasm/swmm6-qaqc/` (4.8 MB) |

The full list, with what each value is and where the solver computes it, is in
[docs/VARIABLES.md](docs/VARIABLES.md). It is generated from
[`qaqc_variables.json`](qaqc_variables.json), which the engine writes itself.

## Why this exists

The Swmm5-UI-Engine app shows about 230 variables in its Time Series Graph.
Its engine writes 5 per link. The others are rebuilt in the browser from the
reporting-step results, some with formulas that are not the solver's and three
as constants:

```ts
      const beta = 1.0;
      ext.targetSetting = 1.0;
      ext.timeOpen = 0;
```

Bob's instruction was that the solver should do it. His original solver that
did is the `SWMM5.013_QAQC` build of 2016-2017 (with parts from CDM in 2006).
This repository ports that build's variables to the current EPA engine and to
SWMM6. [docs/APP_CATALOG_VS_SOLVER.md](docs/APP_CATALOG_VS_SOLVER.md) lists the
names whose meaning in the app differs from what the solver computes, and
[docs/DIFFERENCES_FROM_2017.md](docs/DIFFERENCES_FROM_2017.md) lists where this
port departs from the 2017 code and why.

## Using it

### Native

```bash
scripts/build_native_swmm5.sh                       # gcc, no dependencies
build/swmm5-qaqc model.inp model.rpt model.out      # QA/QC variables on
build/swmm5-qaqc --stock model.inp model.rpt model.out   # stock 5.2.4 output
build/swmm5-qaqc --catalog qaqc_variables.json      # write the variable list

python3 tools/swmm_qaqc_out.py model.out qaqc_variables.json            # summary
python3 tools/swmm_qaqc_out.py model.out qaqc_variables.json link 1600  # CSV, all variables of one link
```

The SWMM6 version takes the same arguments. `scripts/build_swmm6.sh <workdir>`
clones OpenSWMM Engine at the tested commit, applies the changes and builds it.

### In a browser

```js
const w = new Worker('wasm/swmm-qaqc-worker.js');
w.postMessage({ engine: 'swmm5', inp: inpText });          // or engine: 'swmm6'
w.onmessage = e => {
  const out = SwmmQaqcOut.parse(e.data.out, catalog);       // wasm/swmm-qaqc-out.js
  const dq2 = out.series('link', '1600', 'LINK_DQ2');       // { t, x }
};
```

One run per worker; terminate it afterwards. `{ qaqc: false }` gives stock
output. The SWMM5 module also exports the usual `swmm_*` API plus
`swmm_setQaqc(0|1)`, `swmm_getQaqcCount(class)` and
`swmm_writeQaqcCatalog(path)`.

### The output file

The layout is the standard SWMM binary output file. Each record holds the
standard variables, then the pollutants, then the QA/QC variables. In the file
header a QA/QC variable has the code `1000 + index`, so a reader can tell them
from pollutants and one catalog serves both engines. A reader that takes the
variable counts from the header, as the format intends, reads the file
unchanged and sees the standard results where they always were. OpenSWMM 6
already extends subcatchment records the same way for its snow and frozen
ground variables.

QA/QC values are the solver's values at the end of the routing step that
reached the reporting time. They are not interpolated to the reporting time,
as the standard variables are.

## One link, both engines

EXTRAN Example 1, link 1600, at 01:45 (the period of peak flow), CFS units:

| Variable | SWMM5 QA/QC | SWMM6 QA/QC |
|---|--:|--:|
| `FLOW` (standard) | 71.9851 | 71.9851 |
| `LINK_DQ1` friction | 0.201133 | 0.201133 |
| `LINK_DQ2` energy slope | -14.5871 | -14.5871 |
| `LINK_DQ3` local inertia | -0.000266007 | -0.000266007 |
| `LINK_DQ4` convective inertia | -0.1077 | -0.1077 |
| `LINK_AKON` = 1 + dq1 + dq5 | 1.20113 | 1.20113 |
| `LINK_QOLD` | 71.9845 | 71.9845 |
| `LINK_VENANT` | 71.9851 | 71.9851 |
| `LINK_A1` / `LINK_AMID` / `LINK_A2` | 13.9085 / 13.8586 / 13.8087 | same |
| `LINK_RHO` / `LINK_SIGMA` | 0.795592 / 1 | same |
| `LINK_FROUDE` | 0.602204 | 0.602204 |
| `LINK_BE_LHS` | 106.178 | 106.178 |
| `LINK_BE_RHS` | 106.178 | 106.178 |

The momentum equation closes from the reported terms:
(71.9845 + 14.5871 - 0.000266 - 0.1077) / 1.20113 = 71.985. And at this
steady peak the two sides of the Bernoulli equation agree to the six digits
printed.

## Evidence

All of it can be rerun with the scripts in `tests/`. The corpus is the 113
sample models of BatchSWMMRunner (`public/samples`); 10 of them stop with an
input error in stock 5.2.4 and are compared on their report file only. Result
files are in `tests/results/`.

### SWMM5 version, native, against EPA 5.2.4 built from the `v5.2.4` tag

| Check | Result |
|---|---|
| Run with `--stock`, the binary output file is byte-identical to stock | 113 of 113 |
| With QA/QC on, the report file is identical apart from the run-time lines | 113 of 113 |
| With QA/QC on, every standard value of every object in every period is bit-identical | 103 of 103 models, 160,591,429 values |
| `AKON = 1 + DQ1 + DQ5` | 10,085,911 of 10,085,911 link-periods |
| `DQ5 = UPHLoss + DNHLoss + MIDHLoss` | 10,085,911 of 10,085,911 |
| `AWTD = A1 + (AMID - A1) x RHO` | 10,085,911 of 10,085,911 |
| Momentum closure, `VENANT x AKON = QOLD - DQ2 + DQ3 + DQ4 + DQ6` | 10,085,911 of 10,085,911 |
| Last `SYS_CE` equals the flow routing continuity error in the report, where the last record is the end of the run | 84 of 84 |

The identities are checked from the float32 values in the output file, to a
relative 5e-4, on every link-period where the momentum equation was solved.

### SWMM5 version, WebAssembly

The control here is stock EPA 5.2.4 compiled to WebAssembly with the same
Emscripten and the same flags, because a WebAssembly build and a native build
of the *unmodified* engine already differ in the last digits on some models:
the stock engine's binary output is byte-identical between the two on
73 of 113 models and its report on 92 of 113.

| Check, against stock 5.2.4 compiled to WebAssembly | Result |
|---|---|
| QA/QC switched off, binary output byte-identical | 113 of 113 |
| QA/QC on, report file identical | 113 of 113 |
| QA/QC on, standard values bit-identical | 103 of 103 models, 160,591,429 values |
| The four identities | 0 failures on 10,086,018 link-periods |
| Last `SYS_CE` equals the report's continuity error | 84 of 84 |

The WebAssembly QA/QC engine's whole output file, QA/QC variables included, is
byte-identical to the native QA/QC engine's on 54 of 113 models.

### SWMM6 version

Native, against OpenSWMM Engine built from the same commit without the
changes:

| Check | Result |
|---|---|
| Run with `--stock`, the binary output file is byte-identical to stock | 113 of 113 |
| With QA/QC on, the report file is identical apart from the run-time lines | 113 of 113 |
| With QA/QC on, every standard value is bit-identical | 103 of 103 models, 160,599,317 values |
| The four identities, momentum closure included | 0 failures on 10,151,456 link-periods |

WebAssembly: there is no unmodified WebAssembly build of OpenSWMM 6 to compare
with here, so the control is weaker: the same WebAssembly engine run with
`--stock`. Against that, the report is identical on 113 of 113 models, the
standard values are bit-identical on 103 of 103 (160,599,317 values) and
the four identities hold on all 10,162,934 link-periods. Against the native
build, the WebAssembly engine's whole output file is byte-identical on 57 of
113 models with QA/QC on and 77 of 113 with it off.

Do the two engines report the same terms? `tests/compare_swmm5_swmm6.py`
compares the SWMM5 and SWMM6 QA/QC variables on the link-periods where the two
engines' standard results for the link are bit-identical and the momentum
kernel ran in both: 5,576,520 link-periods in 64 models. 50 of the 67
link variables agree on at least 99.9% of them (relative 1e-4). The others:

| Variable | Agree | Why |
|---|--:|---|
| `LINK_TS` | 98.28% | the routing step at the moment of capture differs on these records; not investigated |
| `LINK_DQ1`, `LINK_DQ2`, `LINK_DQ4`, `LINK_DQDH`, `LINK_AKON` | 98.26% to 99.29% | each is proportional to the routing step |
| `LINK_ITERATIONS` | 98.62% | trial counts differ |
| `LINK_VENANT`, `LINK_F1` | 99.83%, 99.81% | follow from the above; `LINK_F1` also uses the SWMM6 width array |
| `LINK_H1`, `LINK_H2` | 98.57%, 98.49% | SWMM6 takes them from the node depths at the end of the step, SWMM5 from the last trial |
| `LINK_W1` | 94.66% | SWMM6 reports the kernel's width, which applies the crown cutoff and slot rule; SWMM5 reports the plain top width |
| `LINK_TIMEOPEN`, `LINK_CURRENTDATE` | 99.56%, 99.63% | float32 rounding of long times |
| `LINK_R2`, `LINK_W2`, `LINK_F2` | not filled in SWMM6 | the SWMM6 kernel computes no downstream hydraulic radius, and its downstream width array matched SWMM5 on only 63% of these records, so no value is written |

In the SWMM6 version `LINK_A1`, `LINK_A2` and `LINK_AMID` are conveyance
areas: the area limited to the full area, which is what its momentum kernel
uses (a Preissmann slot is excluded there). `NODE_Infil`, `NODE_Evap` and
`SYS_CE` are not filled, and no subcatchment variables are written.

## What this does not show

- That the 5.2.4 solver is right. The QA/QC variables report what it computed.
- That every definition is the one Bob's 2017 build used. Four of its files
  were read; the rest of the definitions were written from the 5.2.4 code and
  are listed in DIFFERENCES_FROM_2017.md for him to confirm. `LINK_AKON`,
  `LINK_UP_N_AREA`, `LINK_DN_N_AREA` and the three `LINK_SWMM3/4/5` areas are
  the ones most open to a different reading.
- Anything about the QA/QC values beyond the four identities and the
  continuity check. The node, subcatchment and most link values have no
  independent check yet; a wrong unit conversion in one of them would pass
  every test here.
- The SWMM5 version under OpenMP. Its builds here are single-threaded. The
  capture writes one slot per link or node, so a threaded build should be
  safe, but that was not run. (The native SWMM6 build does use OpenMP, on a
  two-core machine.)
- A line-by-line review by a person. No one has reviewed the C or C++ changes.
- The SWMM6 version on any commit but `da2b2489`. The change script also
  applied cleanly to `5b2b673e`, which landed on `swmm6_rel` the same day, but
  that was not built or tested.
- The SWMM6 version with its newer options (dynamic slot, semi-implicit node
  continuity, virtual junctions, unsteady friction, finite-volume routing).
  The corpus uses none of them. With unsteady friction on, the closure
  identity above will not hold as written, because that option adds a term.
- The 2D, GeoPackage, HDF5 and GPU parts of OpenSWMM 6. They were switched
  off in every build here.

## What is in the repository

```
qaqc_variables.json        the variable catalog, written by the engine
engine/swmm5/src/          EPA SWMM 5.2.4 with the QA/QC changes applied (builds as is)
engine/swmm5/patches/      the same changes as one patch against the v5.2.4 tag
engine/swmm6/core/         the four files added to OpenSWMM Engine
engine/swmm6/swmm6-qaqc.patch   the changes to its seven existing files
wasm/                      both engines compiled, the output reader and a worker
tools/                     Python reader for the output file; VARIABLES.md generator
tests/                     the checks, three sample models, and the result files
scripts/                   build and rebuild scripts
docs/                      VARIABLES, DIFFERENCES_FROM_2017, APP_CATALOG_VS_SOLVER
licenses/                  Apache-2.0 text and the OpenSWMM NOTICE
```

Every line added to or changed in an engine file carries the tag `QAQC:`, so
`grep -rn "QAQC:" engine/swmm5/src` lists the whole SWMM5 change. One of the
seven changed lines is not about QA/QC: on Linux, stock 5.2.4 aborts with
"buffer overflow detected" because of this line in `swmm5.c`,

```c
            realpath(fname, absPath);
```

where `absPath` holds 260 bytes and `realpath` may write `PATH_MAX`. The build
here lets `realpath` allocate its own buffer and copies the result.

## AI use

This repository was produced with Claude (Anthropic, model Claude Fable 5.2)
on 19 November 2026 at the request of Robert (Bob) Dickinson. Bob asked for
his SWMM5 QA/QC version to be extracted, built for WebAssembly, put on GitHub
and given a SWMM6 version, said that the solver and not the app must compute
the variables, and pointed to his original 2017 source. Claude read four files
of that source, designed and wrote the capture and output code for both
engines, built them, wrote the readers, the tests and these documents, and ran
every check reported here. The variable list and the definitions taken from
the 2017 files are Bob's. The definitions listed in DIFFERENCES_FROM_2017.md as
this port's own are Claude's and have not been confirmed by him. No person has
reviewed the code changes or rerun the checks on another machine.

## License and credit

EPA SWMM is a work of the U.S. Environmental Protection Agency and is in the
public domain; version 5.2.4's engine is Lewis Rossman's design. The USEPA does
not endorse this product.

The SWMM6 version is built on OpenSWMM Engine, Copyright 2026 HydroCouple
Developers, licensed under the Apache License, Version 2.0. See
[NOTICE](NOTICE) and [licenses/Apache-2.0-OpenSWMM.txt](licenses/Apache-2.0-OpenSWMM.txt).
Source: https://github.com/HydroCouple/openswmm.engine (branch `swmm6_rel`,
commit `da2b248937c85b6caf453044c4bc98b3f1de41c1`). The seven engine files that
were changed carry a modification notice in their headers. This is not a
release of OpenSWMM Engine or of HydroCouple.

The files written for this repository are under the MIT license, Copyright
2026 Robert Dickinson. See [LICENSE](LICENSE).
