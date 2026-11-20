//-----------------------------------------------------------------------------
//   qaqc.h
//
//   Project:  EPA SWMM5 - QA/QC build (swmm5-qaqc)
//   Base:     EPA SWMM 5.2.4
//   Author:   R. Dickinson (variable list and definitions, 2006-2017 QA/QC
//             builds); 5.2.4 port drafted with Claude (Anthropic), 2026.
//
//   Extended QA/QC output variables computed by the solver itself.
//
//   The solver stores its own intermediate values (momentum-equation terms,
//   end areas, Newton terms at nodes, ...) at the end of every routing step.
//   When QA/QC output is on, they are appended to each object's record in
//   the binary output file, AFTER the standard variables and pollutants.
//   The standard variables are computed exactly as in EPA SWMM 5.2.4.
//
//   Every line of the engine that was added or changed for this purpose is
//   marked with the comment tag  QAQC:
//-----------------------------------------------------------------------------
#ifndef QAQC_H
#define QAQC_H

//-----------------------------------------------------------------------------
//  Capture slots - written by the solver, internal units (ft, sec, cfs).
//-----------------------------------------------------------------------------
enum QaqcLinkSlot {
    QC_Y1RAW,  QC_Y2RAW,               // end depths before capping at yFull
    QC_Y1,     QC_Y2,                  // end depths used for areas
    QC_H1,     QC_H2,                  // end heads used in dq2
    QC_A1,     QC_A2,   QC_AMID,  QC_AWTD,
    QC_R1,     QC_R2,   QC_RMID,  QC_RWTD,
    QC_V,                              // velocity from last flow estimate
    QC_RHO,    QC_SIGMA,
    QC_DQ1,    QC_DQ2,  QC_DQ3,   QC_DQ4,  QC_DQ5,  QC_DQ6,
    QC_DENOM,                          // 1 + dq1 + dq5
    QC_LOSS_UP, QC_LOSS_DN, QC_LOSS_MID, // parts of dq5
    QC_DQ1FM,                          // dq1 when it came from force main eqn.
    QC_QMOM,                           // q from momentum eqn. before limits
    QC_QNORM,                          // normal flow limit (0 if not applied)
    QC_FASNH,                          // fraction between normal & critical
    QC_QNEW,                           // final flow per barrel
    QC_QOLD,                           // previous-step flow per barrel
    QC_ISFULL,
    QC_NLINKSLOTS };

enum QaqcNodeSlot {
    QN_AREA,                           // surface area used in depth update
    QN_OLDAREA,                        // Xnode.oldSurfArea
    QN_SUMDQDH,                        // sum of link dq/dh at node
    QN_DENOM,                          // denominator of depth update
    QN_F,                              // inflow - outflow (numerator)
    QN_CORR,                           // depth change applied this iteration
    QN_YCROWN,                         // crown depth above invert
    QN_DYDT,                           // |dy/dt| over the step
    QN_CONVERGED,
    QN_SURCHARGED,
    QN_DWF,                            // dry weather inflow added this step
    QN_RDII,                           // RDII inflow added this step
    QC_NNODESLOTS };

//-----------------------------------------------------------------------------
//  Object classes for the variable catalog.
//-----------------------------------------------------------------------------
enum QaqcClass { QAQC_SUBCATCH, QAQC_NODE, QAQC_LINK, QAQC_SYS };

#define QAQC_CODE_BASE  1000           // variable codes written to .out header
#define QAQC_MAX_SYS      16           // upper bound on system QA/QC variables

//-----------------------------------------------------------------------------
//  Shared state.
//-----------------------------------------------------------------------------
extern int     QaqcEnabled;            // 1 = write QA/QC variables (default)
extern double* QaqcLink;               // Nlinks x QC_NLINKSLOTS
extern double* QaqcNode;               // Nnodes x QC_NNODESLOTS
extern double  QaqcRouteStep;          // routing step just taken (sec)
extern int     QaqcTrials;             // dynamic wave trials just taken
extern int     QaqcNonConverged;       // nodes not converged in that step

//-----------------------------------------------------------------------------
//  Capture helpers used inside the solver (NULL when QA/QC is off).
//-----------------------------------------------------------------------------
#define QAQC_LINK(j) (QaqcLink ? (QaqcLink + (size_t)(j)*QC_NLINKSLOTS) : NULL)
#define QAQC_NODE(i) (QaqcNode ? (QaqcNode + (size_t)(i)*QC_NNODESLOTS) : NULL)

//-----------------------------------------------------------------------------
//  Engine-side functions.
//-----------------------------------------------------------------------------
int   qaqc_open(void);                 // allocate capture arrays
void  qaqc_close(void);                // free them
int   qaqc_count(int objClass);        // # of QA/QC output variables
void  qaqc_getSubcatchResults(int j, float x[]);
void  qaqc_getNodeResults(int j, float x[]);
void  qaqc_getLinkResults(int j, float x[]);
void  qaqc_getSysResults(float x[]);
void  qaqc_writeCatalog(FILE* f);      // JSON description of every variable

#endif
