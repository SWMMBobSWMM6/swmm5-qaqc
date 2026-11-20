// Generated from swmm5-qaqc/engine/swmm5/src/solver/qaqc.c - do not edit.
// Catalog of QA/QC output variables, shared by the SWMM5 and SWMM6 builds.
//-----------------------------------------------------------------------------
//  Unit kinds (for the catalog and for conversion)
//-----------------------------------------------------------------------------
enum QaqcUnits { U_NONE, U_LEN, U_AREA, U_FLOW, U_VEL, U_SEC, U_COUNT,
                 U_FLAG, U_PCT, U_DQDH, U_RAINDEPTH, U_LANDAREA, U_RATE,
                 U_DAYS, U_SPEED, U_GWFLOW };

static const char* UnitsUS[] = { "", "ft", "ft2", "flow", "ft/s", "sec",
    "count", "0/1", "%", "flow/ft", "in", "ac", "in/hr", "days", "mph",
    "cfs/ac" };
static const char* UnitsSI[] = { "", "m", "m2", "flow", "m/s", "sec",
    "count", "0/1", "%", "flow/m", "mm", "ha", "mm/hr", "days", "km/hr",
    "cms/ha" };

typedef struct
{
    const char* key;                   // short name, as in the 2017 enums.h
    int         units;                 // QaqcUnits code
    const char* def;                   // what the solver value is
    const char* src;                   // where the solver computes it
} TQaqcVar;

//-----------------------------------------------------------------------------
//  LINK variables
//-----------------------------------------------------------------------------
enum QaqcLinkVar {
    QL_FROUDE, QL_F1, QL_F2, QL_V1, QL_V2,
    QL_DQ1, QL_DQ2, QL_DQ3, QL_DQ4, QL_DQ5, QL_DQ6,
    QL_UPHLOSS, QL_DNHLOSS, QL_MIDHLOSS, QL_DQ1HW,
    QL_AMID, QL_AWTD, QL_A1, QL_A2, QL_RMID, QL_RWTD, QL_R1, QL_R2,
    QL_W1, QL_W2, QL_Y1, QL_Y2,
    QL_QNORM, QL_VENANT, QL_DQDH, QL_UP_N_AREA, QL_DN_N_AREA,
    QL_TS, QL_ITERATIONS, QL_TIMESTEP, QL_HGL, QL_AKON, QL_FASNH,
    QL_H1, QL_H2,
    QL_LENGTH, QL_MODLENGTH, QL_ROUGHNESS, QL_ROUGHFACTOR, QL_SLOPE,
    QL_QMAX, QL_BETA, QL_SETTING, QL_TARGET, QL_TIMEOPEN, QL_CURRENTDATE,
    QL_SWMM3, QL_SWMM4, QL_SWMM5,
    QL_VHUP, QL_VHMID, QL_VHDN, QL_VHHF, QL_BE_RHS, QL_BE_LHS,
    QL_RHO, QL_SIGMA,
    QL_FLOWCLASS, QL_NORMALFLOW, QL_INLETCONTROL, QL_FULLSTATE, QL_QOLD,
    QL_COUNT };

static const TQaqcVar LinkVars[QL_COUNT] = {
 {"LINK_FROUDE",     U_NONE, "Froude number at mid depth, Link.froude", "dwflow_findConduitFlow"},
 {"LINK_F1",         U_NONE, "Froude number at upstream end, link_getFroude(q/a1, y1)", "qaqc_getLinkResults"},
 {"LINK_F2",         U_NONE, "Froude number at downstream end, link_getFroude(q/a2, y2)", "qaqc_getLinkResults"},
 {"LINK_V1",         U_VEL,  "velocity at upstream end, q/a1", "qaqc_getLinkResults"},
 {"LINK_V2",         U_VEL,  "velocity at downstream end, q/a2", "qaqc_getLinkResults"},
 {"LINK_DQ1",        U_NONE, "dq1, friction term: dt*roughFactor/rWtd^1.33333*|v| (force main: dt*forcemain_getFricSlope)", "dwflow_findConduitFlow"},
 {"LINK_DQ2",        U_FLOW, "dq2, energy (head) slope term: dt*g*aWtd*(h2-h1)/length, per barrel", "dwflow_findConduitFlow"},
 {"LINK_DQ3",        U_FLOW, "dq3, local inertia term: 2*v*(aMid-aOld)*sigma, per barrel", "dwflow_findConduitFlow"},
 {"LINK_DQ4",        U_FLOW, "dq4, convective inertia term: dt*v*v*(a2-a1)/length*sigma, per barrel", "dwflow_findConduitFlow"},
 {"LINK_DQ5",        U_NONE, "dq5, local losses term: findLocalLosses/2/length*dt", "dwflow_findConduitFlow"},
 {"LINK_DQ6",        U_FLOW, "dq6, evaporation and seepage loss term, per barrel", "dwflow_findConduitFlow"},
 {"LINK_UPHLoss",    U_NONE, "inlet-loss part of dq5: cLossInlet*|q|/a1/2/length*dt", "findLocalLosses"},
 {"LINK_DNHLoss",    U_NONE, "outlet-loss part of dq5: cLossOutlet*|q|/a2/2/length*dt", "findLocalLosses"},
 {"LINK_MIDHLoss",   U_NONE, "average-loss part of dq5: cLossAvg*|q|/aMid/2/length*dt", "findLocalLosses"},
 {"LINK_DQ1HW",      U_NONE, "dq1 when it came from the force main (Hazen-Williams or Darcy-Weisbach) equation, else 0", "dwflow_findConduitFlow"},
 {"LINK_AMID",       U_AREA, "aMid, flow area at the mean of the end depths, as used in the momentum equation", "dwflow_findConduitFlow"},
 {"LINK_AWTD",       U_AREA, "aWtd, upstream-weighted area: a1 + (aMid-a1)*rho", "dwflow_findConduitFlow"},
 {"LINK_A1",         U_AREA, "a1, flow area at upstream end", "dwflow_findConduitFlow"},
 {"LINK_A2",         U_AREA, "a2, flow area at downstream end", "dwflow_findConduitFlow"},
 {"LINK_RMID",       U_LEN,  "rMid, hydraulic radius at the mean of the end depths", "dwflow_findConduitFlow"},
 {"LINK_RWTD",       U_LEN,  "rWtd, upstream-weighted hydraulic radius: r1 + (rMid-r1)*rho", "dwflow_findConduitFlow"},
 {"LINK_R1",         U_LEN,  "r1, hydraulic radius at upstream end", "dwflow_findConduitFlow"},
 {"LINK_R2",         U_LEN,  "r2, hydraulic radius at downstream end", "dwflow_findConduitFlow (QAQC)"},
 {"LINK_W1",         U_LEN,  "top width at upstream end, xsect_getWofY(y1)", "qaqc_getLinkResults"},
 {"LINK_W2",         U_LEN,  "top width at downstream end, xsect_getWofY(y2)", "qaqc_getLinkResults"},
 {"LINK_Y1",         U_LEN,  "y1, flow depth at upstream end after findSurfArea", "dwflow_findConduitFlow"},
 {"LINK_Y2",         U_LEN,  "y2, flow depth at downstream end after findSurfArea", "dwflow_findConduitFlow"},
 {"LINK_QNORM",      U_FLOW, "normal flow that replaced the dynamic wave flow, beta*a1*r1^(2/3) x barrels; 0 when the limit did not apply", "checkNormalFlow"},
 {"LINK_VENANT",     U_FLOW, "flow from the momentum equation before any limit or under-relaxation, (qOld-dq2+dq3+dq4+dq6)/denom x barrels", "dwflow_findConduitFlow"},
 {"LINK_DQDH",       U_DQDH, "Link.dqdh, derivative of link flow with respect to head", "dwflow_findConduitFlow"},
 {"LINK_UP_N_AREA",  U_AREA, "Link.surfArea1, link surface area assigned to its upstream node", "findSurfArea"},
 {"LINK_DN_N_AREA",  U_AREA, "Link.surfArea2, link surface area assigned to its downstream node", "findSurfArea"},
 {"LINK_TS",         U_SEC,  "routing time step just taken", "routing_execute"},
 {"LINK_ITERATIONS", U_COUNT,"dynamic wave trials used in that step", "dynwave_execute"},
 {"LINK_TIMESTEP",   U_SEC,  "this link's Courant time: (volume/q)*(modLength/length)*Fr/(1+Fr), not scaled by the variable step factor", "qaqc_getLinkResults (formula of getLinkStep)"},
 {"LINK_HGL",        U_LEN,  "h2 - h1, head difference used in dq2 (as in the 2017 build)", "dwflow_findConduitFlow"},
 {"LINK_AKON",       U_NONE, "denominator of the flow update, 1 + dq1 + dq5; 0 when the conduit was dry or closed and the momentum equation was not solved in that step", "dwflow_findConduitFlow"},
 {"LINK_FASNH",      U_NONE, "fasnh, fraction between normal and critical depth used to scale downstream surface area", "getFlowClass"},
 {"LINK_H1",         U_NONE, "upstream end depth before capping, divided by yFull (as in the 2017 build)", "dwflow_findConduitFlow"},
 {"LINK_H2",         U_NONE, "downstream end depth before capping, divided by yFull (as in the 2017 build)", "dwflow_findConduitFlow"},
 {"LINK_length",     U_LEN,  "Conduit.length", "input"},
 {"LINK_modLength",  U_LEN,  "Conduit.modLength, length after LENGTHENING_STEP", "dynwave_init"},
 {"LINK_roughness",  U_NONE, "Conduit.roughness", "input"},
 {"LINK_roughFactor",U_NONE, "Conduit.roughFactor, g*(n/1.486)^2 adjusted for lengthening", "conduit_validate"},
 {"LINK_slope",      U_NONE, "Conduit.slope", "conduit_validate"},
 {"LINK_qMax",       U_FLOW, "Conduit.qMax x barrels, maximum Manning flow", "conduit_validate"},
 {"LINK_beta",       U_NONE, "Conduit.beta, Manning discharge factor 1.486*sqrt(slope)/n", "conduit_validate"},
 {"LINK_SETTING",    U_NONE, "Link.setting, current control setting", "controls / link_setSetting"},
 {"LINK_TARGET",     U_NONE, "Link.targetSetting", "controls"},
 {"LINK_TIMEOPEN",   U_SEC,  "time since Link.timeLastSet", "qaqc_getLinkResults"},
 {"LINK_CURRENTDATE",U_DAYS, "elapsed simulation time at the end of the routing step", "qaqc_getLinkResults"},
 {"LINK_SWMM3",      U_AREA, "area by the SWMM 3 weighting, (a1+a2)/2", "qaqc_getLinkResults"},
 {"LINK_SWMM4",      U_AREA, "area by the SWMM 4 weighting, aMid with no upstream weighting", "qaqc_getLinkResults"},
 {"LINK_SWMM5",      U_AREA, "area by the SWMM 5 weighting, aWtd", "qaqc_getLinkResults"},
 {"LINK_VHUP",       U_LEN,  "velocity head at upstream end, V1^2/2g", "qaqc_getLinkResults"},
 {"LINK_VHMID",      U_LEN,  "velocity head at mid depth, v^2/2g", "qaqc_getLinkResults"},
 {"LINK_VHDN",       U_LEN,  "velocity head at downstream end, V2^2/2g", "qaqc_getLinkResults"},
 {"LINK_VHHF",       U_LEN,  "friction head loss implied by dq1: dq1*|v|*length/(g*dt)", "qaqc_getLinkResults"},
 {"LINK_BE_RHS",     U_LEN,  "Bernoulli right side: h2 + V2^2/2g + friction loss + local losses", "qaqc_getLinkResults"},
 {"LINK_BE_LHS",     U_LEN,  "Bernoulli left side: h1 + V1^2/2g", "qaqc_getLinkResults"},
 {"LINK_RHO",        U_NONE, "rho, upstream weighting factor", "dwflow_findConduitFlow"},
 {"LINK_SIGMA",      U_NONE, "sigma, inertial damping factor actually applied", "dwflow_findConduitFlow"},
 {"LINK_FLOWCLASS",  U_COUNT,"Link.flowClass: 0 dry, 1 up dry, 2 dn dry, 3 subcritical, 4 supercritical, 5 up critical, 6 dn critical", "findSurfArea"},
 {"LINK_NORMALFLOW", U_FLAG, "Link.normalFlow, 1 when flow was limited to normal flow", "checkNormalFlow"},
 {"LINK_INLETCONTROL",U_FLAG,"Link.inletControl, 1 when culvert inlet control applied", "culvert_getInflow"},
 {"LINK_FULLSTATE",  U_COUNT,"from Conduit.fullState: 0 neither end full, 1 upstream end full, 2 downstream end full, 3 both ends full", "link_getFullState"},
 {"LINK_QOLD",       U_FLOW, "qOld, flow at the previous routing step x barrels (with dq1..dq6 it closes the momentum equation)", "dwflow_findConduitFlow"}
};

//-----------------------------------------------------------------------------
//  NODE variables
//-----------------------------------------------------------------------------
enum QaqcNodeVar {
    QNV_AREA, QNV_TS, QNV_IIFLOW, QNV_DWFFLOW, QNV_OUTFLOW, QNV_CE,
    QNV_DQDH, QNV_DENOM, QNV_F, QNV_YCROWN, QNV_OLDAREA, QNV_CORRECTION,
    QNV_ITERATIONS, QNV_TIMESTEP, QNV_CONVERGENCE, QNV_INFIL, QNV_EVAP,
    QNV_DEGREE, QNV_OLDAREABYDT, QNV_SURCHARGED,
    QNV_COUNT };

static const TQaqcVar NodeVars[QNV_COUNT] = {
 {"NODE_AREA",        U_AREA, "surface area used in the depth update, max(Xnode.newSurfArea, MinSurfArea)", "setNodeDepth"},
 {"NODE_TS",          U_SEC,  "routing time step just taken", "routing_execute"},
 {"NODE_IIFLOW",      U_FLOW, "RDII inflow added to the node in that step", "addRdiiInflows"},
 {"NODE_DWFFLOW",     U_FLOW, "dry weather inflow added to the node in that step", "addDryWeatherInflows"},
 {"NODE_OUTFLOW",     U_FLOW, "Node.outflow, total outflow", "updateNodeFlows"},
 {"NODE_CE",          U_PCT,  "node continuity residual as % of the larger of inflow and outflow: 0.5*(oldNetInflow+inflow-outflow) - overflow - dV/dt", "qaqc_getNodeResults"},
 {"NODE_DQDH",        U_DQDH, "Xnode.sumdqdh, sum of dq/dh of connected links", "findLinkFlows / updateNodeFlows"},
 {"NODE_DENOM",       U_DQDH, "denominator of the depth update: surface area/dt when free surface, sumdqdh (plus the near-crown term) when surcharged", "setNodeDepth"},
 {"NODE_F",           U_FLOW, "dQ, inflow minus outflow at the last trial", "setNodeDepth"},
 {"NODE_YCROWN",      U_LEN,  "yCrown, crown elevation minus invert elevation", "setNodeDepth"},
 {"NODE_OLDAREA",     U_AREA, "Xnode.oldSurfArea", "setNodeDepth"},
 {"NODE_CORRECTION",  U_LEN,  "depth change applied in the last trial, new depth minus previous trial depth", "setNodeDepth"},
 {"NODE_ITERATIONS",  U_COUNT,"dynamic wave trials used in that step", "dynwave_execute"},
 {"NODE_TIMESTEP",    U_SEC,  "this node's time step limit: 0.25*yCrown/|dy/dt| (0 when not limiting), formula of getNodeStep", "qaqc_getNodeResults"},
 {"NODE_CONVERGENCE", U_FLAG, "Xnode.converged, 1 when the node met HEAD_TOLERANCE", "findNodeDepths"},
 {"NODE_Infil",       U_FLOW, "storage exfiltration rate over the step, Storage.exfilLoss/dt", "storage_getLosses"},
 {"NODE_Evap",        U_FLOW, "storage evaporation rate over the step, Storage.evapLoss/dt", "storage_getLosses"},
 {"NODE_DEGREE",      U_COUNT,"Node.degree, number of outflow links (negative for a terminal node)", "dynwave / toposort"},
 {"NODE_OLDAREABYDT", U_DQDH, "Xnode.oldSurfArea / dt", "setNodeDepth"},
 {"NODE_SURCHARGED",  U_FLAG, "1 when the surcharge (EXTRAN) branch of the depth update was used", "setNodeDepth"}
};

//-----------------------------------------------------------------------------
//  SUBCATCHMENT variables
//-----------------------------------------------------------------------------
enum QaqcSubVar {
    QS_RUNOFF_IMPERV0, QS_RUNOFF_IMPERV1, QS_RUNOFF_PERV,
    QS_DEPTH_IMPERV0, QS_DEPTH_IMPERV1, QS_DEPTH_PERV, QS_DEPTH, QS_RUNON,
    QS_AREA, QS_IMP_AREA_DS, QS_IMP_AREA_NODS, QS_PERV_AREA,
    QS_NONLIDAREA, QS_LID_AREA,
    QS_GW_THETA, QS_GW_LOWERDEPTH, QS_AQ_WATERTABLELEV, QS_GW_NODEFLOW,
    QS_GW_EVAPLOSS, QS_GW_MAXINFILVOL, QS_AQ_BOTTOMELEV, QS_AQ_POROSITY,
    QS_FREEWATER, QS_COLD,
    QS_HORTON_TP, QS_HORTON_FE,
    QS_GA_IMD, QS_GA_F, QS_GA_FU, QS_GA_SAT, QS_GA_T,
    QS_CN_S, QS_CN_P, QS_CN_F, QS_CN_T, QS_CN_SE, QS_CN_f,
    QS_COUNT };

static const TQaqcVar SubVars[QS_COUNT] = {
 {"SUBCATCH_RUNOFF_IMPERV0", U_FLOW, "runoff from the impervious subarea without depression storage: subArea[IMPERV0].runoff x its area", "subcatch_getRunoff"},
 {"SUBCATCH_RUNOFF_IMPERV1", U_FLOW, "runoff from the impervious subarea with depression storage", "subcatch_getRunoff"},
 {"SUBCATCH_RUNOFF_PERV",    U_FLOW, "runoff from the pervious subarea", "subcatch_getRunoff"},
 {"SUBCATCH_DEPTH_IMPERV0",  U_RAINDEPTH, "ponded depth on the impervious subarea without depression storage", "getSubareaRunoff"},
 {"SUBCATCH_DEPTH_IMPERV1",  U_RAINDEPTH, "ponded depth on the impervious subarea with depression storage", "getSubareaRunoff"},
 {"SUBCATCH_DEPTH_PERV",     U_RAINDEPTH, "ponded depth on the pervious subarea", "getSubareaRunoff"},
 {"SUBCATCH_DEPTH",          U_RAINDEPTH, "area-weighted ponded depth, subcatch_getDepth", "subcatch_getDepth"},
 {"SUBCATCH_RUNON",          U_RATE, "Subcatch.runon, runon from other subcatchments", "subcatch_getRunon"},
 {"SUBCATCH_AREA",           U_LANDAREA, "Subcatch.area", "input"},
 {"SUBCATCH_IMP_AREA_DS",    U_LANDAREA, "impervious area with depression storage, non-LID area x subArea[IMPERV1].fArea", "input"},
 {"SUBCATCH_IMP_AREA_NoDS",  U_LANDAREA, "impervious area without depression storage, non-LID area x subArea[IMPERV0].fArea", "input"},
 {"SUBCATCH_PERV_AREA",      U_LANDAREA, "pervious area, non-LID area x subArea[PERV].fArea", "input"},
 {"SUBCATCH_nonLidArea",     U_LANDAREA, "Subcatch.area - Subcatch.lidArea", "input"},
 {"SUBCATCH_LID_AREA",       U_LANDAREA, "Subcatch.lidArea", "lid_validate"},
 {"SUBCATCH_MOISTURE",       U_NONE, "groundwater upper zone moisture content, theta", "gwater_getGroundwater"},
 {"SUBCATCH_GW_LOWERDEPTH",  U_LEN,  "depth of the saturated zone, lowerDepth", "gwater_getGroundwater"},
 {"SUBCATCH_AQ_WATERTABLELEV",U_LEN, "water table elevation, bottomElev + lowerDepth", "gwater_getGroundwater"},
 {"SUBCATCH_GW_NODEFLOW",    U_GWFLOW, "groundwater flow to the node per unit area, newFlow", "gwater_getGroundwater"},
 {"SUBCATCH_GW_EVAPLOSS",    U_RATE, "groundwater evaporation loss rate, evapLoss", "gwater_getGroundwater"},
 {"SUBCATCH_GW_MAXINFILVOL", U_RAINDEPTH, "maximum infiltration the upper zone can accept, maxInfilVol", "gwater_getGroundwater"},
 {"SUBCATCH_AQ_BOTTOMELEV",  U_LEN,  "bottom elevation of the lower groundwater zone", "input"},
 {"SUBCATCH_AQ_POROSITY",    U_NONE, "aquifer porosity", "input"},
 {"SUBCATCH_FREEWATER",      U_RAINDEPTH, "free water in the snow pack, sum of fw x fArea over the three snow surfaces", "snow_getSnowMelt"},
 {"SUBCATCH_COLD",           U_RAINDEPTH, "cold content of the snow pack, sum of coldc x fArea", "snow_getSnowMelt"},
 {"HORTON_TP",               U_SEC,  "Horton: present time on the infiltration curve, tp", "horton_getInfil"},
 {"HORTON_Fe",               U_RAINDEPTH, "Horton: cumulative infiltration, Fe", "horton_getInfil"},
 {"GA_IMD",                  U_NONE, "Green-Ampt: current initial moisture deficit, IMD", "grnampt_getInfil"},
 {"GA_F",                    U_RAINDEPTH, "Green-Ampt: cumulative infiltration, F", "grnampt_getInfil"},
 {"GA_FU",                   U_RAINDEPTH, "Green-Ampt: upper zone infiltration volume, Fu", "grnampt_getInfil"},
 {"GA_Sat",                  U_FLAG, "Green-Ampt: saturation flag, Sat", "grnampt_getInfil"},
 {"GA_T",                    U_SEC,  "Green-Ampt: time until the next event, T", "grnampt_getInfil"},
 {"CN_S",                    U_RAINDEPTH, "Curve Number: current infiltration capacity, S", "curvenum_getInfil"},
 {"CN_P",                    U_RAINDEPTH, "Curve Number: cumulative precipitation, P", "curvenum_getInfil"},
 {"CN_F",                    U_RAINDEPTH, "Curve Number: cumulative infiltration, F", "curvenum_getInfil"},
 {"CN_T",                    U_SEC,  "Curve Number: inter-event time, T", "curvenum_getInfil"},
 {"CN_Se",                   U_RAINDEPTH, "Curve Number: event infiltration capacity, Se", "curvenum_getInfil"},
 {"CN_f",                    U_RATE, "Curve Number: previous infiltration rate, f", "curvenum_getInfil"}
};

//-----------------------------------------------------------------------------
//  SYSTEM variables
//-----------------------------------------------------------------------------
enum QaqcSysVar {
    QY_STEPFLOWERROR, QY_CE, QY_ITERATIONS, QY_TS, QY_NONCONVERGED,
    QY_WINDSPEED,
    QY_COUNT };

static const TQaqcVar SysVars[QY_COUNT] = {
 {"SYS_StepFlowError", U_PCT,  "massbal_getStepFlowError x 100, flow imbalance of the routing step", "massbal_getStepFlowError"},
 {"SYS_CE",            U_PCT,  "flow routing continuity error to date, same formula as massbal_getFlowError using current storage", "qaqc_getSysResults"},
 {"SYS_ITERATIONS",    U_COUNT,"dynamic wave trials used in the routing step", "dynwave_execute"},
 {"SYS_TS",            U_SEC,  "routing time step just taken", "routing_execute"},
 {"SYS_NONCONVERGED",  U_COUNT,"number of nodes that did not converge in the routing step", "dynwave_execute"},
 {"SYS_WINDSPEED",     U_SPEED,"Wind.ws, current wind speed", "climate_setState"}
};

