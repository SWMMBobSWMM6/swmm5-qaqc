# What changed from the 2017 QA/QC build

The source of this work is Bob Dickinson's own QA/QC build of SWMM5, kept in his
Google Drive folder `SWMM5.013_QAQC`. Its changes are marked
`RED Advanced QA/QC 2017` and `Innovyze RED - 2016`, with older ones marked
`cdm october 2006` and `cdm december 2006`. The files are based on EPA SWMM
5.1.012 and 5.1.013.

## What was read from the original

Four files were read through the Google Drive connector on 2026-11-19:

| File | What it gave |
|---|---|
| `enums.h` | the full list of QA/QC variables for subcatchments, nodes, links and the system |
| `dwflow.c` | where each link term is captured inside `dwflow_findConduitFlow`, `findLocalLosses` and `checkNormalFlow` |
| `link.c` | `link_getResults`: which captured value goes to which output variable, and with which unit conversion |
| `node.c` | `node_getResults`: the node variables that are written |

The original stores values in `Link[j].LinkResults[]` and `Node[j].NodeResults[]`
and writes them into the standard result arrays by enlarging
`MAX_LINK_RESULTS`, `MAX_NODE_RESULTS` and `MAX_SUBCATCH_RESULTS` to 100.

The folder does not look like a complete build. The only `objects` header found
in it is `objects (3).h`, and that file has no `LinkResults` member, so it is
not the header the modified `dwflow.c` was compiled against. A search of the
folder found no `output.c` other than `output (3).c`, which was not read.
`dynwave.c` is in the folder but was not opened, and neither was the
subcatchment code. So this repository is a port of the original's variables to
EPA SWMM 5.2.4, not a copy of its source, and the original source is not
included here.

## Where this port uses the original's definition

`LINK_F1`, `LINK_F2`, `LINK_V1`, `LINK_V2`, `LINK_DQ1`, `LINK_DQ2`, `LINK_DQ4`,
`LINK_DQ5`, `LINK_DQ6`, `LINK_AWTD`, `LINK_RMID`, `LINK_RWTD`, `LINK_A1`,
`LINK_A2`, `LINK_R1`, `LINK_R2`, `LINK_Y1`, `LINK_Y2`, `LINK_W1`, `LINK_W2`,
`LINK_RHO`, `LINK_SIGMA`, `LINK_QNORM`, `LINK_VENANT`, `LINK_HGL` (stored as
`h2 - h1`), `LINK_H1` and `LINK_H2` (end depth before capping, divided by
`yFull`), the conduit properties (`LINK_length` to `LINK_qMax`),
`LINK_SETTING`, `LINK_TARGET`, `LINK_FROUDE`, `NODE_DEGREE`.

## Where it differs, and why

### 1. `LINK_DQ3` was overwritten in the original

The last capture line of `dwflow_findConduitFlow` in the 2017 file reuses the
`LINK_DQ3` slot for the numerator of the flow update:

```c
    Link[j].LinkResults[LINK_DQ3] = dq3;
    ...
    Link[j].LinkResults[LINK_HGL] = (h2 - h1);
    Link[j].LinkResults[LINK_DQ3] = (qOld - dq2 + dq3 + dq4 - dq6);
```

So the original's `LINK_DQ3` series is the numerator, not the dq3 term. Here
`LINK_DQ3` is dq3. The numerator is recoverable as `LINK_VENANT x LINK_AKON`,
and `LINK_QOLD` was added so the whole equation can be checked from the output.

### 2. The local-loss parts were rescaled on every call

The original fills `LINK_UPHLoss`, `LINK_DNHLoss` and `LINK_MIDHLoss` inside
`findLocalLosses`, which only runs when the conduit has loss coefficients, and
then rescales them unconditionally:

```c
    Link[j].LinkResults[LINK_UPHLoss] = Link[j].LinkResults[LINK_UPHLoss] / 2.0 / length * dt;
```

For a conduit with losses that is right. For a conduit without losses the slot
is never refilled, so the stale value is multiplied by `dt/2/length` again on
every call. Here the three parts are set to zero when the conduit has no loss
coefficients, and they sum to `LINK_DQ5` otherwise.

### 3. Units of the momentum terms

`link_getResults` in the original multiplies all six terms and the three loss
parts by `UCF(FLOW)`. dq1 and dq5 are pure numbers (they sit in the denominator
`1 + dq1 + dq5`), so a flow conversion does not apply to them. Here `LINK_DQ1`,
`LINK_DQ5` and the three loss parts are written unconverted, and `LINK_DQ2`,
`LINK_DQ3`, `LINK_DQ4`, `LINK_DQ6` are converted to the model's flow units. In
CFS models the numbers are the same either way.

### 4. `LINK_AMID`

The original stores `aMid` after the solver has reused that variable for the
volume calculation, so its `LINK_AMID` is `(a1 + a2)/2` capped at the full
area. Here `LINK_AMID` is the area at the mean of the end depths, which is the
`aMid` the momentum equation uses (it is what `LINK_AWTD` is built from). The
average of the end areas is written as `LINK_SWMM3`.

### 5. `LINK_VENANT` and `LINK_QNORM` are multiplied by the number of barrels

So they compare directly with the standard `FLOW` variable. The dq terms are
left per barrel, as the solver computes them.

### 6. `LINK_TIMEOPEN`

The original writes `Link[j].timeLastSet`, a date. Here it is the time in
seconds since the setting last changed.

### 7. `LINK_TS` and `LINK_TIMESTEP`

The original writes its `LINK_TIMESTEP` slot to `LINK_TS` and never writes
`LINK_TIMESTEP`. Here `LINK_TS` is the routing step just taken and
`LINK_TIMESTEP` is the link's own Courant time, by the formula of
`getLinkStep`.

## Variables whose definition is this port's, not the original's

The original assignments for these were not in the files that were read, so
they are defined here from the 5.2.4 solver. Each definition is in
[VARIABLES.md](VARIABLES.md). Bob should confirm them.

| Variable | Definition used here |
|---|---|
| `LINK_AKON` | `1 + dq1 + dq5`, the denominator of the flow update; 0 when the momentum equation was not solved |
| `LINK_FASNH` | `fasnh` from `getFlowClass` |
| `LINK_UP_N_AREA`, `LINK_DN_N_AREA` | `Link.surfArea1`, `Link.surfArea2` |
| `LINK_SWMM3`, `LINK_SWMM4`, `LINK_SWMM5` | `(a1+a2)/2`, `aMid`, `aWtd`, after the COMPATIBILITY option of SWMM 5.0 |
| `LINK_VHUP`, `LINK_VHMID`, `LINK_VHDN` | `V1^2/2g`, `v^2/2g`, `V2^2/2g` |
| `LINK_VHHF` | `dq1 * |v| * length / (g * dt)` |
| `LINK_BE_LHS`, `LINK_BE_RHS` | `h1 + V1^2/2g` and `h2 + V2^2/2g + friction loss + local losses` |
| `LINK_DQ1HW` | dq1 when the force main equation supplied it |
| `LINK_beta` | `Conduit.beta` |
| `LINK_DQDH`, `LINK_ITERATIONS`, `LINK_CURRENTDATE` | `Link.dqdh`, trials in the step, elapsed days |
| all node variables except `NODE_DEGREE` | see VARIABLES.md |
| all subcatchment and system variables | see VARIABLES.md |

## Variables in the 2017 list that are not ported

| Variable | Reason |
|---|---|
| `NODE_UH1`, `NODE_UH2`, `NODE_UH3` | 5.2.4 keeps only the total RDII per node in the RDII interface file; the per-hydrograph split needs a change to `rdii.c` |
| `SUBCATCH_GW_Flow_A1/A2/A3`, `GW_Percolation`, `GW_HSTAR`, `GW_HSW`, `GW_MAXGWQ`, `GW_NEGMAXGWQ` | computed in local variables of `gwater.c`; need capture hooks there |
| `SUBCATCH_SNOWMELT`, `IMELT`, `RAINMELT`, `SNOWAREA` and the `SYS_` snow variables | need capture hooks in `snow.c` |
| the twelve `LID_` variables, `LIDTotalInflow`, `SUBCATCH_LID_FROMIMP_AREA`, `IMPtoLID`, `LIDCount` | need capture hooks in `lidproc.c` |
| `SUBCATCH_UL`, `FTOT`, `FU`, `FUMAX`, `IMD`, `IMDbyEvent`, `SAT`, `INFIL_TIME`, `LOSS_ONLY`, `WLMAX`, `CN_Smax`, `CN_F1`, `CN_Regen`, `CN_CN` | the method-specific states `HORTON_*`, `GA_*`, `CN_*` that 5.2.4 exposes through `infil_getState` are written instead |
| `SYS_SNOWFALL` | not available as a system total in 5.2.4 |
| the `NODE_X..`, `LINK_X..`, `SUBCATCH_X..` placeholders | never defined |

## Variables added here that the 2017 list does not have

`LINK_FLOWCLASS`, `LINK_NORMALFLOW`, `LINK_INLETCONTROL`, `LINK_FULLSTATE`,
`LINK_QOLD`, `NODE_SURCHARGED`, `SYS_NONCONVERGED`.
