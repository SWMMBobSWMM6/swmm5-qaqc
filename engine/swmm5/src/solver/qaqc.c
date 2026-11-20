//-----------------------------------------------------------------------------
//   qaqc.c
//
//   Project:  EPA SWMM5 - QA/QC build (swmm5-qaqc)
//   Base:     EPA SWMM 5.2.4
//   Author:   R. Dickinson (variable list and definitions, 2006-2017 QA/QC
//             builds); 5.2.4 port drafted with Claude (Anthropic), 2026.
//
//   Extended QA/QC output variables computed by the solver.
//
//   Nothing in this file changes a computed result. It only reads values
//   the solver already produced (and the capture slots filled in dwflow.c,
//   dynwave.c and routing.c) and converts them to user units for output.
//
//   Values are those at the END of the routing step that reached the
//   reporting time. They are NOT interpolated to the reporting time the way
//   the standard variables are.
//-----------------------------------------------------------------------------
#define _CRT_SECURE_NO_DEPRECATE

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "headers.h"
#include "qaqc.h"

//-----------------------------------------------------------------------------
//  Shared state
//-----------------------------------------------------------------------------
int     QaqcEnabled      = 1;
double* QaqcLink         = NULL;
double* QaqcNode         = NULL;
double  QaqcRouteStep    = 0.0;
int     QaqcTrials       = 0;
int     QaqcNonConverged = 0;

extern TRoutingTotals FlowTotals;      // defined in massbal.c

//QAQC-CATALOG-BEGIN  (this block is shared verbatim with the SWMM6 build)
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

//QAQC-CATALOG-END

//=============================================================================

int qaqc_count(int objClass)
//
//  Input:   objClass = QaqcClass code
//  Output:  returns number of QA/QC output variables for the class
//           (0 when QA/QC output is switched off)
//
{
    if ( !QaqcEnabled ) return 0;
    switch (objClass)
    {
      case QAQC_SUBCATCH: return QS_COUNT;
      case QAQC_NODE:     return QNV_COUNT;
      case QAQC_LINK:     return QL_COUNT;
      case QAQC_SYS:      return QY_COUNT;
    }
    return 0;
}

//=============================================================================

int qaqc_open()
//
//  Purpose: allocates the capture arrays once the project is open.
//
{
    qaqc_close();
    QaqcRouteStep = 0.0;
    QaqcTrials = 0;
    QaqcNonConverged = 0;
    if ( !QaqcEnabled ) return 0;
    if ( Nobjects[LINK] > 0 )
    {
        QaqcLink = (double *) calloc((size_t)Nobjects[LINK] * QC_NLINKSLOTS,
                                     sizeof(double));
        if ( !QaqcLink ) return ERR_MEMORY;
    }
    if ( Nobjects[NODE] > 0 )
    {
        QaqcNode = (double *) calloc((size_t)Nobjects[NODE] * QC_NNODESLOTS,
                                     sizeof(double));
        if ( !QaqcNode ) return ERR_MEMORY;
    }
    return 0;
}

//=============================================================================

void qaqc_close()
{
    FREE(QaqcLink);
    FREE(QaqcNode);
    QaqcLink = NULL;
    QaqcNode = NULL;
}

//=============================================================================

static double safeDiv(double a, double b)
{
    return (fabs(b) > FUDGE) ? a / b : 0.0;
}

//=============================================================================

void qaqc_getLinkResults(int j, float x[])
//
//  Input:   j = link index
//  Output:  x[] = QL_COUNT QA/QC values for the link, in user units
//
{
    int     i, k;
    double  ucfL = UCF(LENGTH);
    double  ucfA = ucfL * ucfL;
    double  ucfQ = UCF(FLOW);
    double  dt = QaqcRouteStep;
    double  q, a1, a2, y1, y2, v, v1, v2, len, barrels, hf, hLoc, fr, t;
    double* c = QAQC_LINK(j);
    TXsect* xsect = &Link[j].xsect;

    for (i = 0; i < QL_COUNT; i++) x[i] = 0.0f;
    if ( c == NULL ) return;

    // --- values that exist for every kind of link
    x[QL_FROUDE]       = (float)Link[j].froude;
    x[QL_DQDH]         = (float)(Link[j].dqdh * ucfQ / ucfL);
    x[QL_UP_N_AREA]    = (float)(Link[j].surfArea1 * ucfA);
    x[QL_DN_N_AREA]    = (float)(Link[j].surfArea2 * ucfA);
    x[QL_TS]           = (float)dt;
    x[QL_ITERATIONS]   = (float)QaqcTrials;
    x[QL_SETTING]      = (float)Link[j].setting;
    x[QL_TARGET]       = (float)Link[j].targetSetting;
    x[QL_TIMEOPEN]     = (float)((getDateTime(NewRoutingTime) -
                                  Link[j].timeLastSet) * SECperDAY);
    x[QL_CURRENTDATE]  = (float)(NewRoutingTime / MSECperDAY);
    x[QL_FLOWCLASS]    = (float)Link[j].flowClass;
    x[QL_NORMALFLOW]   = (float)Link[j].normalFlow;
    x[QL_INLETCONTROL] = (float)Link[j].inletControl;
    if ( Link[j].type != CONDUIT ) return;

    // --- conduit properties
    k = Link[j].subIndex;
    barrels = Conduit[k].barrels;
    len = Conduit[k].modLength;
    x[QL_LENGTH]      = (float)(Conduit[k].length * ucfL);
    x[QL_MODLENGTH]   = (float)(Conduit[k].modLength * ucfL);
    x[QL_ROUGHNESS]   = (float)Conduit[k].roughness;
    x[QL_ROUGHFACTOR] = (float)Conduit[k].roughFactor;
    x[QL_SLOPE]       = (float)Conduit[k].slope;
    x[QL_QMAX]        = (float)(Conduit[k].qMax * barrels * ucfQ);
    x[QL_BETA]        = (float)Conduit[k].beta;
    // Conduit.fullState holds the FlowClassType codes UP_FULL, DN_FULL and
    // ALL_FULL; write it as 0 neither, 1 upstream, 2 downstream, 3 both ends
    i = Conduit[k].fullState;
    x[QL_FULLSTATE]   = (float)( (i == UP_FULL) ? 1 : (i == DN_FULL) ? 2 :
                                 (i == ALL_FULL) ? 3 : 0 );

    // --- this link's Courant time (formula of getLinkStep in dynwave.c)
    q = fabs(Link[j].newFlow) / barrels;
    fr = Link[j].froude;
    if ( q > FUDGE && fr > 0.01 && Conduit[k].a1 > FUDGE )
    {
        t = Link[j].newVolume / barrels / q;
        t = t * Conduit[k].modLength / link_getLength(j);
        t = t * fr / (1.0 + fr);
        x[QL_TIMESTEP] = (float)t;
    }

    // --- the rest only exists under dynamic wave routing
    if ( RouteModel != DW ) return;
    q  = c[QC_QNEW];
    a1 = c[QC_A1];
    a2 = c[QC_A2];
    y1 = c[QC_Y1];
    y2 = c[QC_Y2];
    v  = c[QC_V];
    v1 = safeDiv(q, a1);
    v2 = safeDiv(q, a2);

    x[QL_F1]  = (float)( (a1 > FUDGE) ? link_getFroude(j, v1, y1) : 0.0 );
    x[QL_F2]  = (float)( (a2 > FUDGE) ? link_getFroude(j, v2, y2) : 0.0 );
    x[QL_V1]  = (float)(v1 * ucfL);
    x[QL_V2]  = (float)(v2 * ucfL);
    x[QL_DQ1] = (float)c[QC_DQ1];
    x[QL_DQ2] = (float)(c[QC_DQ2] * ucfQ);
    x[QL_DQ3] = (float)(c[QC_DQ3] * ucfQ);
    x[QL_DQ4] = (float)(c[QC_DQ4] * ucfQ);
    x[QL_DQ5] = (float)c[QC_DQ5];
    x[QL_DQ6] = (float)(c[QC_DQ6] * ucfQ);
    x[QL_UPHLOSS]  = (float)c[QC_LOSS_UP];
    x[QL_DNHLOSS]  = (float)c[QC_LOSS_DN];
    x[QL_MIDHLOSS] = (float)c[QC_LOSS_MID];
    x[QL_DQ1HW] = (float)c[QC_DQ1FM];
    x[QL_AMID]  = (float)(c[QC_AMID] * ucfA);
    x[QL_AWTD]  = (float)(c[QC_AWTD] * ucfA);
    x[QL_A1]    = (float)(a1 * ucfA);
    x[QL_A2]    = (float)(a2 * ucfA);
    x[QL_RMID]  = (float)(c[QC_RMID] * ucfL);
    x[QL_RWTD]  = (float)(c[QC_RWTD] * ucfL);
    x[QL_R1]    = (float)(c[QC_R1] * ucfL);
    x[QL_R2]    = (float)(c[QC_R2] * ucfL);
    x[QL_W1]    = (float)(xsect_getWofY(xsect, MIN(y1, xsect->yFull)) * ucfL);
    x[QL_W2]    = (float)(xsect_getWofY(xsect, MIN(y2, xsect->yFull)) * ucfL);
    x[QL_Y1]    = (float)(y1 * ucfL);
    x[QL_Y2]    = (float)(y2 * ucfL);
    x[QL_QNORM]  = (float)(c[QC_QNORM] * barrels * ucfQ);
    x[QL_VENANT] = (float)(c[QC_QMOM] * barrels * ucfQ);
    x[QL_HGL]    = (float)((c[QC_H2] - c[QC_H1]) * ucfL);
    x[QL_AKON]   = (float)c[QC_DENOM];
    x[QL_FASNH]  = (float)c[QC_FASNH];
    x[QL_H1]     = (float)safeDiv(c[QC_Y1RAW], xsect->yFull);
    x[QL_H2]     = (float)safeDiv(c[QC_Y2RAW], xsect->yFull);
    x[QL_SWMM3]  = (float)(0.5 * (a1 + a2) * ucfA);
    x[QL_SWMM4]  = (float)(c[QC_AMID] * ucfA);
    x[QL_SWMM5]  = (float)(c[QC_AWTD] * ucfA);
    x[QL_VHUP]   = (float)(v1 * v1 / 2.0 / GRAVITY * ucfL);
    x[QL_VHMID]  = (float)(v * v / 2.0 / GRAVITY * ucfL);
    x[QL_VHDN]   = (float)(v2 * v2 / 2.0 / GRAVITY * ucfL);
    hf = 0.0;
    hLoc = 0.0;
    if ( dt > 0.0 )
    {
        hf   = c[QC_DQ1] * fabs(v) * len / GRAVITY / dt;
        hLoc = c[QC_DQ5] * fabs(v) * len / GRAVITY / dt;
    }
    x[QL_VHHF]   = (float)(hf * ucfL);
    x[QL_BE_LHS] = (float)((c[QC_H1] + v1 * v1 / 2.0 / GRAVITY) * ucfL);
    x[QL_BE_RHS] = (float)((c[QC_H2] + v2 * v2 / 2.0 / GRAVITY + hf + hLoc)
                           * ucfL);
    x[QL_RHO]    = (float)c[QC_RHO];
    x[QL_SIGMA]  = (float)c[QC_SIGMA];
    x[QL_QOLD]   = (float)(c[QC_QOLD] * barrels * ucfQ);
}

//=============================================================================

void qaqc_getNodeResults(int j, float x[])
//
//  Input:   j = node index
//  Output:  x[] = QNV_COUNT QA/QC values for the node, in user units
//
{
    int     i, k;
    double  ucfL = UCF(LENGTH);
    double  ucfA = ucfL * ucfL;
    double  ucfQ = UCF(FLOW);
    double  dt = QaqcRouteStep;
    double  r, big;
    double* c = QAQC_NODE(j);

    for (i = 0; i < QNV_COUNT; i++) x[i] = 0.0f;
    if ( c == NULL ) return;

    x[QNV_TS]         = (float)dt;
    x[QNV_IIFLOW]     = (float)(c[QN_RDII] * ucfQ);
    x[QNV_DWFFLOW]    = (float)(c[QN_DWF] * ucfQ);
    x[QNV_OUTFLOW]    = (float)(Node[j].outflow * ucfQ);
    x[QNV_ITERATIONS] = (float)QaqcTrials;
    x[QNV_DEGREE]     = (float)Node[j].degree;
    x[QNV_YCROWN]     = (float)((Node[j].crownElev - Node[j].invertElev)
                                * ucfL);

    // --- node continuity residual over the step (trapezoidal net inflow
    //     less overflow less rate of change of stored volume)
    if ( dt > 0.0 && Node[j].type != OUTFALL )
    {
        r = 0.5 * (Node[j].oldNetInflow + Node[j].inflow - Node[j].outflow)
            - Node[j].overflow
            - (Node[j].newVolume - Node[j].oldVolume) / dt;
        big = MAX(fabs(Node[j].inflow), fabs(Node[j].outflow));
        if ( big > FLOW_TOL ) x[QNV_CE] = (float)(100.0 * r / big);
    }

    // --- storage unit losses over the step
    if ( Node[j].type == STORAGE && dt > 0.0 )
    {
        k = Node[j].subIndex;
        x[QNV_INFIL] = (float)(Storage[k].exfilLoss / dt * ucfQ);
        x[QNV_EVAP]  = (float)(Storage[k].evapLoss / dt * ucfQ);
    }

    // --- the rest only exists under dynamic wave routing
    if ( RouteModel != DW ) return;
    x[QNV_AREA]        = (float)(c[QN_AREA] * ucfA);
    x[QNV_DQDH]        = (float)(c[QN_SUMDQDH] * ucfQ / ucfL);
    x[QNV_DENOM]       = (float)(c[QN_DENOM] * ucfQ / ucfL);
    x[QNV_F]           = (float)(c[QN_F] * ucfQ);
    x[QNV_OLDAREA]     = (float)(c[QN_OLDAREA] * ucfA);
    x[QNV_CORRECTION]  = (float)(c[QN_CORR] * ucfL);
    x[QNV_CONVERGENCE] = (float)c[QN_CONVERGED];
    x[QNV_SURCHARGED]  = (float)c[QN_SURCHARGED];
    if ( dt > 0.0 )
        x[QNV_OLDAREABYDT] = (float)(c[QN_OLDAREA] / dt * ucfQ / ucfL);
    if ( c[QN_DYDT] > FUDGE && c[QN_YCROWN] * 0.25 >= FUDGE )
        x[QNV_TIMESTEP] = (float)(0.25 * c[QN_YCROWN] / c[QN_DYDT]);
}

//=============================================================================

void qaqc_getSubcatchResults(int j, float x[])
//
//  Input:   j = subcatchment index
//  Output:  x[] = QS_COUNT QA/QC values for the subcatchment, in user units
//
{
    int     i, m;
    double  s[6];
    double  nonLidArea = Subcatch[j].area - Subcatch[j].lidArea;
    double  ucfQ = UCF(FLOW);
    double  ucfD = UCF(RAINDEPTH);
    double  ucfL = UCF(LENGTH);
    double  ucfLA = UCF(LANDAREA);
    TGroundwater* gw = Subcatch[j].groundwater;
    TSnowpack* sp = Subcatch[j].snowpack;

    for (i = 0; i < QS_COUNT; i++) x[i] = 0.0f;

    // --- runoff and ponded depth on each of the three subareas
    for (i = IMPERV0; i <= PERV; i++)
    {
        x[QS_RUNOFF_IMPERV0 + i] = (float)(Subcatch[j].subArea[i].runoff *
            Subcatch[j].subArea[i].fArea * nonLidArea * ucfQ);
        x[QS_DEPTH_IMPERV0 + i] = (float)(Subcatch[j].subArea[i].depth * ucfD);
    }
    x[QS_DEPTH] = (float)(subcatch_getDepth(j) * ucfD);
    x[QS_RUNON] = (float)(Subcatch[j].runon * UCF(RAINFALL));

    // --- areas
    x[QS_AREA]          = (float)(Subcatch[j].area * ucfLA);
    x[QS_IMP_AREA_DS]   = (float)(Subcatch[j].subArea[IMPERV1].fArea *
                                  nonLidArea * ucfLA);
    x[QS_IMP_AREA_NODS] = (float)(Subcatch[j].subArea[IMPERV0].fArea *
                                  nonLidArea * ucfLA);
    x[QS_PERV_AREA]     = (float)(Subcatch[j].subArea[PERV].fArea *
                                  nonLidArea * ucfLA);
    x[QS_NONLIDAREA]    = (float)(nonLidArea * ucfLA);
    x[QS_LID_AREA]      = (float)(Subcatch[j].lidArea * ucfLA);

    // --- groundwater
    if ( gw )
    {
        x[QS_GW_THETA]         = (float)gw->theta;
        x[QS_GW_LOWERDEPTH]    = (float)(gw->lowerDepth * ucfL);
        x[QS_AQ_WATERTABLELEV] = (float)((gw->bottomElev + gw->lowerDepth)
                                         * ucfL);
        x[QS_GW_NODEFLOW]      = (float)(gw->newFlow * UCF(GWFLOW));
        x[QS_GW_EVAPLOSS]      = (float)(gw->evapLoss * UCF(EVAPRATE) / 24.0);
        x[QS_GW_MAXINFILVOL]   = (float)(gw->maxInfilVol * ucfD);
        x[QS_AQ_BOTTOMELEV]    = (float)(gw->bottomElev * ucfL);
        x[QS_AQ_POROSITY]      = (float)Aquifer[gw->aquifer].porosity;
    }

    // --- snow pack
    if ( sp )
    {
        s[0] = 0.0;
        s[1] = 0.0;
        for (i = 0; i < 3; i++)
        {
            s[0] += sp->fw[i] * sp->fArea[i];
            s[1] += sp->coldc[i] * sp->fArea[i];
        }
        x[QS_FREEWATER] = (float)(s[0] * ucfD);
        x[QS_COLD]      = (float)(s[1] * ucfD);
    }

    // --- infiltration state for whichever method the subcatchment uses
    if ( Subcatch[j].infil == j )
    {
        for (i = 0; i < 6; i++) s[i] = 0.0;
        infil_getState(j, s);
        m = Subcatch[j].infilModel;
        if ( m == HORTON || m == MOD_HORTON )
        {
            x[QS_HORTON_TP] = (float)s[0];
            x[QS_HORTON_FE] = (float)(s[1] * ucfD);
        }
        else if ( m == GREEN_AMPT || m == MOD_GREEN_AMPT )
        {
            x[QS_GA_IMD] = (float)s[0];
            x[QS_GA_F]   = (float)(s[1] * ucfD);
            x[QS_GA_FU]  = (float)(s[2] * ucfD);
            x[QS_GA_SAT] = (float)s[3];
            x[QS_GA_T]   = (float)s[4];
        }
        else if ( m == CURVE_NUMBER )
        {
            x[QS_CN_S]  = (float)(s[0] * ucfD);
            x[QS_CN_P]  = (float)(s[1] * ucfD);
            x[QS_CN_F]  = (float)(s[2] * ucfD);
            x[QS_CN_T]  = (float)s[3];
            x[QS_CN_SE] = (float)(s[4] * ucfD);
            x[QS_CN_f]  = (float)(s[5] * UCF(RAINFALL));
        }
    }
}

//=============================================================================

void qaqc_getSysResults(float x[])
//
//  Output:  x[] = QY_COUNT system-wide QA/QC values
//
{
    int    i;
    double totalInflow, totalOutflow, storage, pctError;

    for (i = 0; i < QY_COUNT; i++) x[i] = 0.0f;
    x[QY_ITERATIONS]   = (float)QaqcTrials;
    x[QY_TS]           = (float)QaqcRouteStep;
    x[QY_NONCONVERGED] = (float)QaqcNonConverged;
    x[QY_WINDSPEED]    = (float)(Wind.ws * UCF(WINDSPEED));
    if ( IgnoreRouting || Nobjects[NODE] == 0 ) return;
    x[QY_STEPFLOWERROR] = (float)(100.0 * massbal_getStepFlowError());

    // --- flow routing continuity error to date: same bookkeeping as
    //     massbal_getFlowError() but with the storage held right now and
    //     without touching FlowTotals
    storage = 0.0;
    for (i = 0; i < Nobjects[NODE]; i++) storage += Node[i].newVolume;
    if ( RouteModel != SF )
        for (i = 0; i < Nobjects[LINK]; i++) storage += Link[i].newVolume;
    totalInflow = FlowTotals.initStorage + FlowTotals.wwInflow +
                  FlowTotals.iiInflow;
    totalOutflow = storage + FlowTotals.flooding + FlowTotals.evapLoss +
                   FlowTotals.seepLoss + FlowTotals.reacted;
    if ( FlowTotals.dwInflow >= 0.0 ) totalInflow += FlowTotals.dwInflow;
    else                              totalOutflow -= FlowTotals.dwInflow;
    if ( FlowTotals.gwInflow >= 0.0 ) totalInflow += FlowTotals.gwInflow;
    else                              totalOutflow -= FlowTotals.gwInflow;
    if ( FlowTotals.exInflow >= 0.0 ) totalInflow += FlowTotals.exInflow;
    else                              totalOutflow -= FlowTotals.exInflow;
    if ( FlowTotals.outflow >= 0.0 )  totalOutflow += FlowTotals.outflow;
    else                              totalInflow -= FlowTotals.outflow;
    pctError = 0.0;
    if ( fabs(totalInflow - totalOutflow) < 1.0 ) pctError = 0.0;
    else if ( fabs(totalInflow) > 0.0 )
        pctError = 100.0 * (1.0 - totalOutflow / totalInflow);
    else if ( fabs(totalOutflow) > 0.0 )
        pctError = 100.0 * (totalInflow / totalOutflow - 1.0);
    x[QY_CE] = (float)pctError;
}

//=============================================================================

static void writeClass(FILE* f, const char* name, const TQaqcVar* v, int n,
                       int firstIndexNote, int last)
{
    int i;
    fprintf(f, "  \"%s\": [\n", name);
    for (i = 0; i < n; i++)
    {
        fprintf(f, "    {\"index\": %d, \"code\": %d, \"key\": \"%s\", "
                   "\"unitsUS\": \"%s\", \"unitsSI\": \"%s\", "
                   "\"definition\": \"%s\", \"source\": \"%s\"}%s\n",
                i, QAQC_CODE_BASE + i, v[i].key,
                UnitsUS[v[i].units], UnitsSI[v[i].units],
                v[i].def, v[i].src, (i < n-1) ? "," : "");
    }
    fprintf(f, "  ]%s\n", last ? "" : ",");
    (void)firstIndexNote;
}

void qaqc_writeCatalog(FILE* f)
//
//  Purpose: writes a JSON description of every QA/QC variable, in the order
//           they are appended to each record of the binary output file.
//
{
    fprintf(f, "{\n");
    fprintf(f, "  \"engine\": \"swmm5-qaqc\",\n");
    fprintf(f, "  \"base\": \"EPA SWMM 5.2.4\",\n");
    fprintf(f, "  \"layout\": \"each record = standard variables, then "
               "pollutants, then the QA/QC variables below in index order\",\n");
    fprintf(f, "  \"flowUnits\": \"'flow' means the model's FLOW_UNITS\",\n");
    writeClass(f, "subcatchment", SubVars, QS_COUNT, 0, 0);
    writeClass(f, "node", NodeVars, QNV_COUNT, 0, 0);
    writeClass(f, "link", LinkVars, QL_COUNT, 0, 0);
    writeClass(f, "system", SysVars, QY_COUNT, 0, 1);
    fprintf(f, "}\n");
}
