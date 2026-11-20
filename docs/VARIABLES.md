# QA/QC variables

Generated from `qaqc_variables.json`, which the engine writes itself (`swmm5-qaqc --catalog qaqc_variables.json`). Do not edit by hand.

Each record in the binary output file holds the standard variables, then the pollutants, then the variables below in index order. In the file header each QA/QC variable has the code `1000 + index`.

`flow` means the model's FLOW_UNITS. Units are shown as US / SI.

The *SWMM6* column says whether the SWMM6 build fills the variable. The SWMM6 build appends no subcatchment variables at all.

## Link variables (67)

| # | Variable | Units | What the solver value is | Computed in | SWMM6 |
|--:|---|---|---|---|:-:|
| 0 | `LINK_FROUDE` | - | Froude number at mid depth, Link.froude | `dwflow_findConduitFlow` | yes |
| 1 | `LINK_F1` | - | Froude number at upstream end, link_getFroude(q/a1, y1) | `qaqc_getLinkResults` | yes |
| 2 | `LINK_F2` | - | Froude number at downstream end, link_getFroude(q/a2, y2) | `qaqc_getLinkResults` | no |
| 3 | `LINK_V1` | ft/s / m/s | velocity at upstream end, q/a1 | `qaqc_getLinkResults` | yes |
| 4 | `LINK_V2` | ft/s / m/s | velocity at downstream end, q/a2 | `qaqc_getLinkResults` | yes |
| 5 | `LINK_DQ1` | - | dq1, friction term: dt*roughFactor/rWtd^1.33333*\|v\| (force main: dt*forcemain_getFricSlope) | `dwflow_findConduitFlow` | yes |
| 6 | `LINK_DQ2` | flow | dq2, energy (head) slope term: dt*g*aWtd*(h2-h1)/length, per barrel | `dwflow_findConduitFlow` | yes |
| 7 | `LINK_DQ3` | flow | dq3, local inertia term: 2*v*(aMid-aOld)*sigma, per barrel | `dwflow_findConduitFlow` | yes |
| 8 | `LINK_DQ4` | flow | dq4, convective inertia term: dt*v*v*(a2-a1)/length*sigma, per barrel | `dwflow_findConduitFlow` | yes |
| 9 | `LINK_DQ5` | - | dq5, local losses term: findLocalLosses/2/length*dt | `dwflow_findConduitFlow` | yes |
| 10 | `LINK_DQ6` | flow | dq6, evaporation and seepage loss term, per barrel | `dwflow_findConduitFlow` | yes |
| 11 | `LINK_UPHLoss` | - | inlet-loss part of dq5: cLossInlet*\|q\|/a1/2/length*dt | `findLocalLosses` | yes |
| 12 | `LINK_DNHLoss` | - | outlet-loss part of dq5: cLossOutlet*\|q\|/a2/2/length*dt | `findLocalLosses` | yes |
| 13 | `LINK_MIDHLoss` | - | average-loss part of dq5: cLossAvg*\|q\|/aMid/2/length*dt | `findLocalLosses` | yes |
| 14 | `LINK_DQ1HW` | - | dq1 when it came from the force main (Hazen-Williams or Darcy-Weisbach) equation, else 0 | `dwflow_findConduitFlow` | yes |
| 15 | `LINK_AMID` | ft2 / m2 | aMid, flow area at the mean of the end depths, as used in the momentum equation | `dwflow_findConduitFlow` | yes |
| 16 | `LINK_AWTD` | ft2 / m2 | aWtd, upstream-weighted area: a1 + (aMid-a1)*rho | `dwflow_findConduitFlow` | yes |
| 17 | `LINK_A1` | ft2 / m2 | a1, flow area at upstream end | `dwflow_findConduitFlow` | yes |
| 18 | `LINK_A2` | ft2 / m2 | a2, flow area at downstream end | `dwflow_findConduitFlow` | yes |
| 19 | `LINK_RMID` | ft / m | rMid, hydraulic radius at the mean of the end depths | `dwflow_findConduitFlow` | yes |
| 20 | `LINK_RWTD` | ft / m | rWtd, upstream-weighted hydraulic radius: r1 + (rMid-r1)*rho | `dwflow_findConduitFlow` | yes |
| 21 | `LINK_R1` | ft / m | r1, hydraulic radius at upstream end | `dwflow_findConduitFlow` | yes |
| 22 | `LINK_R2` | ft / m | r2, hydraulic radius at downstream end | `dwflow_findConduitFlow (QAQC)` | no |
| 23 | `LINK_W1` | ft / m | top width at upstream end, xsect_getWofY(y1) | `qaqc_getLinkResults` | yes |
| 24 | `LINK_W2` | ft / m | top width at downstream end, xsect_getWofY(y2) | `qaqc_getLinkResults` | no |
| 25 | `LINK_Y1` | ft / m | y1, flow depth at upstream end after findSurfArea | `dwflow_findConduitFlow` | yes |
| 26 | `LINK_Y2` | ft / m | y2, flow depth at downstream end after findSurfArea | `dwflow_findConduitFlow` | yes |
| 27 | `LINK_QNORM` | flow | normal flow that replaced the dynamic wave flow, beta*a1*r1^(2/3) x barrels; 0 when the limit did not apply | `checkNormalFlow` | yes |
| 28 | `LINK_VENANT` | flow | flow from the momentum equation before any limit or under-relaxation, (qOld-dq2+dq3+dq4+dq6)/denom x barrels | `dwflow_findConduitFlow` | yes |
| 29 | `LINK_DQDH` | flow/ft / flow/m | Link.dqdh, derivative of link flow with respect to head | `dwflow_findConduitFlow` | yes |
| 30 | `LINK_UP_N_AREA` | ft2 / m2 | Link.surfArea1, link surface area assigned to its upstream node | `findSurfArea` | yes |
| 31 | `LINK_DN_N_AREA` | ft2 / m2 | Link.surfArea2, link surface area assigned to its downstream node | `findSurfArea` | yes |
| 32 | `LINK_TS` | sec | routing time step just taken | `routing_execute` | yes |
| 33 | `LINK_ITERATIONS` | count | dynamic wave trials used in that step | `dynwave_execute` | yes |
| 34 | `LINK_TIMESTEP` | sec | this link's Courant time: (volume/q)*(modLength/length)*Fr/(1+Fr), not scaled by the variable step factor | `qaqc_getLinkResults (formula of getLinkStep)` | yes |
| 35 | `LINK_HGL` | ft / m | h2 - h1, head difference used in dq2 (as in the 2017 build) | `dwflow_findConduitFlow` | yes |
| 36 | `LINK_AKON` | - | denominator of the flow update, 1 + dq1 + dq5; 0 when the conduit was dry or closed and the momentum equation was not solved in that step | `dwflow_findConduitFlow` | yes |
| 37 | `LINK_FASNH` | - | fasnh, fraction between normal and critical depth used to scale downstream surface area | `getFlowClass` | yes |
| 38 | `LINK_H1` | - | upstream end depth before capping, divided by yFull (as in the 2017 build) | `dwflow_findConduitFlow` | yes |
| 39 | `LINK_H2` | - | downstream end depth before capping, divided by yFull (as in the 2017 build) | `dwflow_findConduitFlow` | yes |
| 40 | `LINK_length` | ft / m | Conduit.length | `input` | yes |
| 41 | `LINK_modLength` | ft / m | Conduit.modLength, length after LENGTHENING_STEP | `dynwave_init` | yes |
| 42 | `LINK_roughness` | - | Conduit.roughness | `input` | yes |
| 43 | `LINK_roughFactor` | - | Conduit.roughFactor, g*(n/1.486)^2 adjusted for lengthening | `conduit_validate` | yes |
| 44 | `LINK_slope` | - | Conduit.slope | `conduit_validate` | yes |
| 45 | `LINK_qMax` | flow | Conduit.qMax x barrels, maximum Manning flow | `conduit_validate` | yes |
| 46 | `LINK_beta` | - | Conduit.beta, Manning discharge factor 1.486*sqrt(slope)/n | `conduit_validate` | yes |
| 47 | `LINK_SETTING` | - | Link.setting, current control setting | `controls / link_setSetting` | yes |
| 48 | `LINK_TARGET` | - | Link.targetSetting | `controls` | yes |
| 49 | `LINK_TIMEOPEN` | sec | time since Link.timeLastSet | `qaqc_getLinkResults` | yes |
| 50 | `LINK_CURRENTDATE` | days | elapsed simulation time at the end of the routing step | `qaqc_getLinkResults` | yes |
| 51 | `LINK_SWMM3` | ft2 / m2 | area by the SWMM 3 weighting, (a1+a2)/2 | `qaqc_getLinkResults` | yes |
| 52 | `LINK_SWMM4` | ft2 / m2 | area by the SWMM 4 weighting, aMid with no upstream weighting | `qaqc_getLinkResults` | yes |
| 53 | `LINK_SWMM5` | ft2 / m2 | area by the SWMM 5 weighting, aWtd | `qaqc_getLinkResults` | yes |
| 54 | `LINK_VHUP` | ft / m | velocity head at upstream end, V1^2/2g | `qaqc_getLinkResults` | yes |
| 55 | `LINK_VHMID` | ft / m | velocity head at mid depth, v^2/2g | `qaqc_getLinkResults` | yes |
| 56 | `LINK_VHDN` | ft / m | velocity head at downstream end, V2^2/2g | `qaqc_getLinkResults` | yes |
| 57 | `LINK_VHHF` | ft / m | friction head loss implied by dq1: dq1*\|v\|*length/(g*dt) | `qaqc_getLinkResults` | yes |
| 58 | `LINK_BE_RHS` | ft / m | Bernoulli right side: h2 + V2^2/2g + friction loss + local losses | `qaqc_getLinkResults` | yes |
| 59 | `LINK_BE_LHS` | ft / m | Bernoulli left side: h1 + V1^2/2g | `qaqc_getLinkResults` | yes |
| 60 | `LINK_RHO` | - | rho, upstream weighting factor | `dwflow_findConduitFlow` | yes |
| 61 | `LINK_SIGMA` | - | sigma, inertial damping factor actually applied | `dwflow_findConduitFlow` | yes |
| 62 | `LINK_FLOWCLASS` | count | Link.flowClass: 0 dry, 1 up dry, 2 dn dry, 3 subcritical, 4 supercritical, 5 up critical, 6 dn critical | `findSurfArea` | yes |
| 63 | `LINK_NORMALFLOW` | 0/1 | Link.normalFlow, 1 when flow was limited to normal flow | `checkNormalFlow` | yes |
| 64 | `LINK_INLETCONTROL` | 0/1 | Link.inletControl, 1 when culvert inlet control applied | `culvert_getInflow` | yes |
| 65 | `LINK_FULLSTATE` | count | from Conduit.fullState: 0 neither end full, 1 upstream end full, 2 downstream end full, 3 both ends full | `link_getFullState` | yes |
| 66 | `LINK_QOLD` | flow | qOld, flow at the previous routing step x barrels (with dq1..dq6 it closes the momentum equation) | `dwflow_findConduitFlow` | yes |

## Node variables (20)

| # | Variable | Units | What the solver value is | Computed in | SWMM6 |
|--:|---|---|---|---|:-:|
| 0 | `NODE_AREA` | ft2 / m2 | surface area used in the depth update, max(Xnode.newSurfArea, MinSurfArea) | `setNodeDepth` | yes |
| 1 | `NODE_TS` | sec | routing time step just taken | `routing_execute` | yes |
| 2 | `NODE_IIFLOW` | flow | RDII inflow added to the node in that step | `addRdiiInflows` | yes |
| 3 | `NODE_DWFFLOW` | flow | dry weather inflow added to the node in that step | `addDryWeatherInflows` | yes |
| 4 | `NODE_OUTFLOW` | flow | Node.outflow, total outflow | `updateNodeFlows` | yes |
| 5 | `NODE_CE` | % | node continuity residual as % of the larger of inflow and outflow: 0.5*(oldNetInflow+inflow-outflow) - overflow - dV/dt | `qaqc_getNodeResults` | yes |
| 6 | `NODE_DQDH` | flow/ft / flow/m | Xnode.sumdqdh, sum of dq/dh of connected links | `findLinkFlows / updateNodeFlows` | yes |
| 7 | `NODE_DENOM` | flow/ft / flow/m | denominator of the depth update: surface area/dt when free surface, sumdqdh (plus the near-crown term) when surcharged | `setNodeDepth` | yes |
| 8 | `NODE_F` | flow | dQ, inflow minus outflow at the last trial | `setNodeDepth` | yes |
| 9 | `NODE_YCROWN` | ft / m | yCrown, crown elevation minus invert elevation | `setNodeDepth` | yes |
| 10 | `NODE_OLDAREA` | ft2 / m2 | Xnode.oldSurfArea | `setNodeDepth` | yes |
| 11 | `NODE_CORRECTION` | ft / m | depth change applied in the last trial, new depth minus previous trial depth | `setNodeDepth` | yes |
| 12 | `NODE_ITERATIONS` | count | dynamic wave trials used in that step | `dynwave_execute` | yes |
| 13 | `NODE_TIMESTEP` | sec | this node's time step limit: 0.25*yCrown/\|dy/dt\| (0 when not limiting), formula of getNodeStep | `qaqc_getNodeResults` | yes |
| 14 | `NODE_CONVERGENCE` | 0/1 | Xnode.converged, 1 when the node met HEAD_TOLERANCE | `findNodeDepths` | yes |
| 15 | `NODE_Infil` | flow | storage exfiltration rate over the step, Storage.exfilLoss/dt | `storage_getLosses` | no |
| 16 | `NODE_Evap` | flow | storage evaporation rate over the step, Storage.evapLoss/dt | `storage_getLosses` | no |
| 17 | `NODE_DEGREE` | count | Node.degree, number of outflow links (negative for a terminal node) | `dynwave / toposort` | yes |
| 18 | `NODE_OLDAREABYDT` | flow/ft / flow/m | Xnode.oldSurfArea / dt | `setNodeDepth` | yes |
| 19 | `NODE_SURCHARGED` | 0/1 | 1 when the surcharge (EXTRAN) branch of the depth update was used | `setNodeDepth` | yes |

## System variables (6)

| # | Variable | Units | What the solver value is | Computed in | SWMM6 |
|--:|---|---|---|---|:-:|
| 0 | `SYS_StepFlowError` | % | massbal_getStepFlowError x 100, flow imbalance of the routing step | `massbal_getStepFlowError` | yes |
| 1 | `SYS_CE` | % | flow routing continuity error to date, same formula as massbal_getFlowError using current storage | `qaqc_getSysResults` | no |
| 2 | `SYS_ITERATIONS` | count | dynamic wave trials used in the routing step | `dynwave_execute` | yes |
| 3 | `SYS_TS` | sec | routing time step just taken | `routing_execute` | yes |
| 4 | `SYS_NONCONVERGED` | count | number of nodes that did not converge in the routing step | `dynwave_execute` | yes |
| 5 | `SYS_WINDSPEED` | mph / km/hr | Wind.ws, current wind speed | `climate_setState` | yes |

## Subcatchment variables (SWMM5 build only) (37)

| # | Variable | Units | What the solver value is | Computed in | SWMM6 |
|--:|---|---|---|---|:-:|
| 0 | `SUBCATCH_RUNOFF_IMPERV0` | flow | runoff from the impervious subarea without depression storage: subArea[IMPERV0].runoff x its area | `subcatch_getRunoff` | no |
| 1 | `SUBCATCH_RUNOFF_IMPERV1` | flow | runoff from the impervious subarea with depression storage | `subcatch_getRunoff` | no |
| 2 | `SUBCATCH_RUNOFF_PERV` | flow | runoff from the pervious subarea | `subcatch_getRunoff` | no |
| 3 | `SUBCATCH_DEPTH_IMPERV0` | in / mm | ponded depth on the impervious subarea without depression storage | `getSubareaRunoff` | no |
| 4 | `SUBCATCH_DEPTH_IMPERV1` | in / mm | ponded depth on the impervious subarea with depression storage | `getSubareaRunoff` | no |
| 5 | `SUBCATCH_DEPTH_PERV` | in / mm | ponded depth on the pervious subarea | `getSubareaRunoff` | no |
| 6 | `SUBCATCH_DEPTH` | in / mm | area-weighted ponded depth, subcatch_getDepth | `subcatch_getDepth` | no |
| 7 | `SUBCATCH_RUNON` | in/hr / mm/hr | Subcatch.runon, runon from other subcatchments | `subcatch_getRunon` | no |
| 8 | `SUBCATCH_AREA` | ac / ha | Subcatch.area | `input` | no |
| 9 | `SUBCATCH_IMP_AREA_DS` | ac / ha | impervious area with depression storage, non-LID area x subArea[IMPERV1].fArea | `input` | no |
| 10 | `SUBCATCH_IMP_AREA_NoDS` | ac / ha | impervious area without depression storage, non-LID area x subArea[IMPERV0].fArea | `input` | no |
| 11 | `SUBCATCH_PERV_AREA` | ac / ha | pervious area, non-LID area x subArea[PERV].fArea | `input` | no |
| 12 | `SUBCATCH_nonLidArea` | ac / ha | Subcatch.area - Subcatch.lidArea | `input` | no |
| 13 | `SUBCATCH_LID_AREA` | ac / ha | Subcatch.lidArea | `lid_validate` | no |
| 14 | `SUBCATCH_MOISTURE` | - | groundwater upper zone moisture content, theta | `gwater_getGroundwater` | no |
| 15 | `SUBCATCH_GW_LOWERDEPTH` | ft / m | depth of the saturated zone, lowerDepth | `gwater_getGroundwater` | no |
| 16 | `SUBCATCH_AQ_WATERTABLELEV` | ft / m | water table elevation, bottomElev + lowerDepth | `gwater_getGroundwater` | no |
| 17 | `SUBCATCH_GW_NODEFLOW` | cfs/ac / cms/ha | groundwater flow to the node per unit area, newFlow | `gwater_getGroundwater` | no |
| 18 | `SUBCATCH_GW_EVAPLOSS` | in/hr / mm/hr | groundwater evaporation loss rate, evapLoss | `gwater_getGroundwater` | no |
| 19 | `SUBCATCH_GW_MAXINFILVOL` | in / mm | maximum infiltration the upper zone can accept, maxInfilVol | `gwater_getGroundwater` | no |
| 20 | `SUBCATCH_AQ_BOTTOMELEV` | ft / m | bottom elevation of the lower groundwater zone | `input` | no |
| 21 | `SUBCATCH_AQ_POROSITY` | - | aquifer porosity | `input` | no |
| 22 | `SUBCATCH_FREEWATER` | in / mm | free water in the snow pack, sum of fw x fArea over the three snow surfaces | `snow_getSnowMelt` | no |
| 23 | `SUBCATCH_COLD` | in / mm | cold content of the snow pack, sum of coldc x fArea | `snow_getSnowMelt` | no |
| 24 | `HORTON_TP` | sec | Horton: present time on the infiltration curve, tp | `horton_getInfil` | no |
| 25 | `HORTON_Fe` | in / mm | Horton: cumulative infiltration, Fe | `horton_getInfil` | no |
| 26 | `GA_IMD` | - | Green-Ampt: current initial moisture deficit, IMD | `grnampt_getInfil` | no |
| 27 | `GA_F` | in / mm | Green-Ampt: cumulative infiltration, F | `grnampt_getInfil` | no |
| 28 | `GA_FU` | in / mm | Green-Ampt: upper zone infiltration volume, Fu | `grnampt_getInfil` | no |
| 29 | `GA_Sat` | 0/1 | Green-Ampt: saturation flag, Sat | `grnampt_getInfil` | no |
| 30 | `GA_T` | sec | Green-Ampt: time until the next event, T | `grnampt_getInfil` | no |
| 31 | `CN_S` | in / mm | Curve Number: current infiltration capacity, S | `curvenum_getInfil` | no |
| 32 | `CN_P` | in / mm | Curve Number: cumulative precipitation, P | `curvenum_getInfil` | no |
| 33 | `CN_F` | in / mm | Curve Number: cumulative infiltration, F | `curvenum_getInfil` | no |
| 34 | `CN_T` | sec | Curve Number: inter-event time, T | `curvenum_getInfil` | no |
| 35 | `CN_Se` | in / mm | Curve Number: event infiltration capacity, Se | `curvenum_getInfil` | no |
| 36 | `CN_f` | in/hr / mm/hr | Curve Number: previous infiltration rate, f | `curvenum_getInfil` | no |
