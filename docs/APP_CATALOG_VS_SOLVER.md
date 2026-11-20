# The app's variable catalog against the solver

The Swmm5-UI-Engine app (github.com/dickinsonre/Swmm5-UI-Engine, as pushed on
2026-09-19) offers about 230 variables in its Time Series Graph. Its engine
writes only the standard ones: 5 per link, 6 per node, 8 per subcatchment. The
rest are made in the browser by `computeExtendedVariables()` in
`client/src/lib/swmm-engine.ts`, from the standard results at each reporting
step. The app's own `docs/engine-graph-variables.md` says so, and labels the
categories `reconstructed`, `derived` and `estimated`.

Three of them are constants in that file:

```ts
      const beta = 1.0;
      ...
      ext.beta = beta;
      ext.targetSetting = 1.0;
      ext.timeOpen = 0;
```

The momentum terms are rebuilt from reporting-step values with formulas that
are not the solver's:

```ts
      const DQ1 = sigma * qOld;
      const DQ2 = sigma * dt * g * aMid * (h1 - h2) / len;
      const DQ4 = entryHL + exitHL + avgHL;
      const DQ5 = len > 0 ? 0.5 * dt * (latUS + latDS) * vMid / len : 0;
      const DQ6 = len > 0 ? sigma * dt * (convA2 - convA1) / len : 0;
      const DQ3 = dt * g * aMid * sf * Math.sign(flowPerBarrel);
```

So the names are the 2017 names, but several carry a different quantity. With
this engine the same names come from `dwflow.c` at the routing step. The table
lists every name whose meaning in the app differs from the solver's. The app
descriptions are paraphrased from the variable catalog kept in that repository
at `attached_assets/swmm5-inner-workings_(2)_1775222293717.jsx`.

| Variable | The app's description | What the solver computes |
|---|---|---|
| `LINK_DQ1` | inertial term, sigma x Q_old | friction term, `dt*roughFactor/rWtd^1.33333*|v|` (a pure number) |
| `LINK_DQ2` | pressure gradient, with sigma and `aMid` | energy slope term, `dt*g*aWtd*(h2-h1)/length`; no sigma, uses `aWtd` |
| `LINK_DQ3` | friction loss | local inertia, `2*v*(aMid-aOld)*sigma` |
| `LINK_DQ4` | entry, exit and average losses | convective inertia, `dt*v*v*(a2-a1)/length*sigma` |
| `LINK_DQ5` | lateral inflow momentum | local losses term |
| `LINK_DQ6` | convective acceleration | evaporation and seepage loss term |
| `LINK_F1`, `LINK_F2` | cross-section area functions | Froude numbers at the two ends |
| `LINK_RHO` | a density-related factor | the upstream weighting factor `rho` |
| `LINK_beta` | Boussinesq momentum coefficient, about 1.0 | `Conduit.beta`, the Manning discharge factor `1.486*sqrt(slope)/n` |
| `LINK_AKON` | a Manning conveyance, `(k/n) A R^(2/3)` | `1 + dq1 + dq5` (this port's definition; see DIFFERENCES_FROM_2017.md) |
| `LINK_FASNH` | a compound friction factor | `fasnh`, the fraction between normal and critical depth |
| `LINK_UP_N_AREA`, `LINK_DN_N_AREA` | normal-flow areas | link surface area assigned to each end node (this port's reading of the name) |
| `LINK_HGL` | `(H1+H2)/2` | `h2 - h1`, as the 2017 code stores it |
| `LINK_H1`, `LINK_H2` | node heads | end depth before capping, divided by full depth, as the 2017 code stores it |
| `LINK_UPHLoss`, `LINK_DNHLoss`, `LINK_MIDHLoss` | head losses in ft | the three parts of dq5 (pure numbers) |
| `LINK_AMID` | `(A1+A2)/2` | area at the mean of the end depths; `(A1+A2)/2` is `LINK_SWMM3` |
| `LINK_TARGET`, `LINK_TIMEOPEN` | constants 1.0 and 0 | `Link.targetSetting`, seconds since the setting changed |
| `NODE_DENOM` | `dQ/dH + A/dt` | `A/dt` when the node has a free surface, `sumdqdh` (plus the near-crown term) when surcharged |
| `NODE_F` | Newton residual including the storage term | `dQ`, inflow minus outflow at the last trial |

To use the engine in the app: read the binary output with
`wasm/swmm-qaqc-out.js` and `qaqc_variables.json`, map each key above to the
app's variable key, and drop the reconstruction for every variable the engine
supplies. The app's descriptions for the rows above would need to change to the
right-hand column.
