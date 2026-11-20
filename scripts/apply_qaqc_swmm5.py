#!/usr/bin/env python3
"""Apply the QA/QC hooks to a stock EPA SWMM 5.2.4 src/ tree (in place).

usage: apply_qaqc_swmm5.py <path-to-src>

Every inserted line carries the tag  QAQC:  so `grep -n QAQC:` lists the whole
modification. Each edit asserts that its anchor occurs exactly once, so the
script fails loudly instead of patching the wrong place.
"""
import sys, pathlib

src = pathlib.Path(sys.argv[1])
solver = src / "solver"


def edit(fname, old, new, count=1):
    p = solver / fname if (solver / fname).exists() else src / fname
    s = p.read_text(encoding="latin-1")
    n = s.count(old)
    assert n == count, f"{fname}: anchor found {n} times, expected {count}:\n{old[:120]}"
    s = s.replace(old, new)
    p.write_text(s, encoding="latin-1")


# ---------------------------------------------------------------- dwflow.c
edit("dwflow.c", '#include "headers.h"\n',
     '#include "headers.h"\n#include "qaqc.h"                                              // QAQC:\n')

edit("dwflow.c",
     "    char   isClosed = FALSE;           // TRUE if conduit closed\n",
     "    char   isClosed = FALSE;           // TRUE if conduit closed\n"
     "    double* qc = QAQC_LINK(j);         // QAQC: capture slots (NULL if off)\n"
     "    double qMom = 0.0;                 // QAQC: flow before limits\n")

edit("dwflow.c",
     "    y1 = MAX(y1, FUDGE);\n    y2 = MAX(y2, FUDGE);\n\n    // --- flow depths can't exceed full depth of conduit if slot not used\n",
     "    y1 = MAX(y1, FUDGE);\n    y2 = MAX(y2, FUDGE);\n"
     "    if ( qc ) { qc[QC_Y1RAW] = y1; qc[QC_Y2RAW] = y2; }             // QAQC:\n"
     "\n    // --- flow depths can't exceed full depth of conduit if slot not used\n")

# dry / closed branch: record the geometry, zero the momentum terms
edit("dwflow.c",
     "        Link[j].newVolume = Conduit[k].a1 * link_getLength(j) * barrels;\n"
     "        Link[j].newFlow = 0.0;\n"
     "        return;\n",
     "        Link[j].newVolume = Conduit[k].a1 * link_getLength(j) * barrels;\n"
     "        Link[j].newFlow = 0.0;\n"
     "        if ( qc )                                                  // QAQC:\n"
     "        {                                                          // QAQC:\n"
     "            int m;                                                 // QAQC:\n"
     "            for (m = QC_Y1; m < QC_NLINKSLOTS; m++)                // QAQC:\n"
     "                if ( m != QC_FASNH ) qc[m] = 0.0;                  // QAQC:\n"
     "            qc[QC_Y1] = y1;  qc[QC_Y2] = y2;                       // QAQC:\n"
     "            qc[QC_H1] = h1;  qc[QC_H2] = h2;                       // QAQC:\n"
     "            qc[QC_A1] = a1;  qc[QC_A2] = a2;  qc[QC_AMID] = aMid;  // QAQC:\n"
     "            qc[QC_AWTD] = aMid;                                    // QAQC:\n"
     "            qc[QC_R1] = r1;  qc[QC_R2] = getHydRad(xsect, y2);     // QAQC:\n"
     "            qc[QC_RMID] = rMid;  qc[QC_RWTD] = rMid;               // QAQC:\n"
     "            qc[QC_RHO] = 1.0;    qc[QC_QOLD] = qOld;               // QAQC:\n"
     "            // QC_DENOM stays 0: the momentum equation was not solved\n"
     "            qc[QC_ISFULL] = isFull;                                // QAQC:\n"
     "        }                                                          // QAQC:\n"
     "        return;\n")

edit("dwflow.c",
     "    q = (qOld - dq2 + dq3 + dq4 + dq6) / denom;\n",
     "    q = (qOld - dq2 + dq3 + dq4 + dq6) / denom;\n"
     "    qMom = q;                                                      // QAQC:\n"
     "    if ( qc ) qc[QC_QNORM] = 0.0;                                  // QAQC:\n")

edit("dwflow.c",
     "    Link[j].newVolume = aMid * link_getLength(j) * barrels;\n"
     "    Link[j].newFlow = q * barrels;\n"
     "}\n",
     "    Link[j].newVolume = aMid * link_getLength(j) * barrels;\n"
     "    Link[j].newFlow = q * barrels;\n"
     "\n"
     "    // --- QAQC: keep the terms of this (last) evaluation for output.\n"
     "    //     Conduit[k].a1 still holds the momentum-equation aMid here;\n"
     "    //     the local aMid was just reused for the volume calculation.\n"
     "    if ( qc )                                                      // QAQC:\n"
     "    {                                                              // QAQC:\n"
     "        qc[QC_Y1] = y1;        qc[QC_Y2] = y2;                     // QAQC:\n"
     "        qc[QC_H1] = h1;        qc[QC_H2] = h2;                     // QAQC:\n"
     "        qc[QC_A1] = a1;        qc[QC_A2] = a2;                     // QAQC:\n"
     "        qc[QC_AMID] = Conduit[k].a1;                               // QAQC:\n"
     "        qc[QC_AWTD] = aWtd;                                        // QAQC:\n"
     "        qc[QC_R1] = r1;        qc[QC_R2] = getHydRad(xsect, y2);   // QAQC:\n"
     "        qc[QC_RMID] = rMid;    qc[QC_RWTD] = rWtd;                 // QAQC:\n"
     "        qc[QC_V] = v;                                              // QAQC:\n"
     "        qc[QC_RHO] = rho;      qc[QC_SIGMA] = sigma;               // QAQC:\n"
     "        qc[QC_DQ1] = dq1;      qc[QC_DQ2] = dq2;                   // QAQC:\n"
     "        qc[QC_DQ3] = dq3;      qc[QC_DQ4] = dq4;                   // QAQC:\n"
     "        qc[QC_DQ5] = dq5;      qc[QC_DQ6] = dq6;                   // QAQC:\n"
     "        qc[QC_DENOM] = denom;                                      // QAQC:\n"
     "        if ( !Conduit[k].hasLosses )                               // QAQC:\n"
     "        {                                                          // QAQC:\n"
     "            qc[QC_LOSS_UP] = 0.0;  qc[QC_LOSS_DN] = 0.0;           // QAQC:\n"
     "            qc[QC_LOSS_MID] = 0.0;                                 // QAQC:\n"
     "        }                                                          // QAQC:\n"
     "        else                                                       // QAQC:\n"
     "        {                                                          // QAQC:\n"
     "            qc[QC_LOSS_UP]  *= dt / 2.0 / length;                  // QAQC:\n"
     "            qc[QC_LOSS_DN]  *= dt / 2.0 / length;                  // QAQC:\n"
     "            qc[QC_LOSS_MID] *= dt / 2.0 / length;                  // QAQC:\n"
     "        }                                                          // QAQC:\n"
     "        qc[QC_DQ1FM] = (xsect->type == FORCE_MAIN && isFull)       // QAQC:\n"
     "                       ? dq1 : 0.0;                                // QAQC:\n"
     "        qc[QC_QMOM] = qMom;    qc[QC_QNEW] = q;                    // QAQC:\n"
     "        qc[QC_QOLD] = qOld;    qc[QC_ISFULL] = isFull;             // QAQC:\n"
     "    }                                                              // QAQC:\n"
     "}\n")

# fasnh from the flow classification (two assignment sites in findSurfArea)
edit("dwflow.c",
     "    else Link[j].flowClass = getFlowClass(j, q, *h1, *h2, *y1, *y2,\n"
     "                             &criticalDepth, &normalDepth, &fasnh);\n",
     "    else Link[j].flowClass = getFlowClass(j, q, *h1, *h2, *y1, *y2,\n"
     "                             &criticalDepth, &normalDepth, &fasnh);\n"
     "    if ( QAQC_LINK(j) ) QAQC_LINK(j)[QC_FASNH] = fasnh;            // QAQC:\n")

edit("dwflow.c",
     "    if ( a1 > FUDGE ) losses += Link[j].cLossInlet  * (q/a1);\n"
     "    if ( a2 > FUDGE ) losses += Link[j].cLossOutlet * (q/a2);\n"
     "    if ( aMid  > FUDGE ) losses += Link[j].cLossAvg * (q/aMid);\n",
     "    if ( a1 > FUDGE ) losses += Link[j].cLossInlet  * (q/a1);\n"
     "    if ( a2 > FUDGE ) losses += Link[j].cLossOutlet * (q/a2);\n"
     "    if ( aMid  > FUDGE ) losses += Link[j].cLossAvg * (q/aMid);\n"
     "    if ( QAQC_LINK(j) )                                            // QAQC:\n"
     "    {                                                              // QAQC:\n"
     "        double* qc = QAQC_LINK(j);                                 // QAQC:\n"
     "        qc[QC_LOSS_UP]  = (a1 > FUDGE) ? Link[j].cLossInlet*(q/a1) : 0.0;   // QAQC:\n"
     "        qc[QC_LOSS_DN]  = (a2 > FUDGE) ? Link[j].cLossOutlet*(q/a2) : 0.0;  // QAQC:\n"
     "        qc[QC_LOSS_MID] = (aMid > FUDGE) ? Link[j].cLossAvg*(q/aMid) : 0.0; // QAQC:\n"
     "    }                                                              // QAQC:\n")

edit("dwflow.c",
     "            Link[j].normalFlow = TRUE;\n            return qNorm;\n",
     "            Link[j].normalFlow = TRUE;\n"
     "            if ( QAQC_LINK(j) ) QAQC_LINK(j)[QC_QNORM] = qNorm;    // QAQC:\n"
     "            return qNorm;\n")

# ---------------------------------------------------------------- dynwave.c
edit("dynwave.c", '#include "headers.h"\n',
     '#include "headers.h"\n#include "qaqc.h"                                              // QAQC:\n')

edit("dynwave.c",
     "    if ( !converged ) updateConvergenceStats();\n",
     "    if ( !converged ) updateConvergenceStats();\n"
     "    QaqcTrials = Steps;                                            // QAQC:\n"
     "    if ( QaqcNode )                                                // QAQC:\n"
     "    {                                                              // QAQC:\n"
     "        int qi;                                                    // QAQC:\n"
     "        QaqcNonConverged = 0;                                      // QAQC:\n"
     "        for (qi = 0; qi < Nobjects[NODE]; qi++)                    // QAQC:\n"
     "        {                                                          // QAQC:\n"
     "            if ( Node[qi].type == OUTFALL ) continue;              // QAQC:\n"
     "            QAQC_NODE(qi)[QN_CONVERGED] = Xnode[qi].converged;     // QAQC:\n"
     "            QAQC_NODE(qi)[QN_SUMDQDH] = Xnode[qi].sumdqdh;         // QAQC:\n"
     "            if ( !Xnode[qi].converged ) QaqcNonConverged++;        // QAQC:\n"
     "        }                                                          // QAQC:\n"
     "    }                                                              // QAQC:\n")

edit("dynwave.c",
     "    double  f;                         // relative surcharge depth\n",
     "    double  f;                         // relative surcharge depth\n"
     "    double* qn = QAQC_NODE(i);         // QAQC: capture slots (NULL if off)\n")

edit("dynwave.c",
     "    Xnode[i].dYdT = fabs(yNew - yOld) / dt;\n\n    // --- save new depth for node\n    Node[i].newDepth = yNew;\n",
     "    Xnode[i].dYdT = fabs(yNew - yOld) / dt;\n\n    // --- save new depth for node\n    Node[i].newDepth = yNew;\n"
     "    if ( qn )                                                      // QAQC:\n"
     "    {                                                              // QAQC:\n"
     "        qn[QN_AREA] = surfArea;                                    // QAQC:\n"
     "        qn[QN_OLDAREA] = Xnode[i].oldSurfArea;                     // QAQC:\n"
     "        qn[QN_DENOM] = isSurcharged ? denom : surfArea / dt;       // QAQC:\n"
     "        qn[QN_F] = dQ;                                             // QAQC:\n"
     "        qn[QN_CORR] = yNew - yLast;                                // QAQC:\n"
     "        qn[QN_YCROWN] = yCrown;                                    // QAQC:\n"
     "        qn[QN_DYDT] = Xnode[i].dYdT;                               // QAQC:\n"
     "        qn[QN_SURCHARGED] = isSurcharged;                          // QAQC:\n"
     "    }                                                              // QAQC:\n")

# denom is only assigned in the surcharge branch; give it a defined value
edit("dynwave.c",
     "    double  denom;                     // denominator term\n",
     "    double  denom = 0.0;               // denominator term (QAQC: initialized)\n")

# ---------------------------------------------------------------- routing.c
edit("routing.c", '#include "headers.h"\n',
     '#include "headers.h"\n#include "qaqc.h"                                              // QAQC:\n')

edit("routing.c",
     "        if ( fabs(q) < FLOW_TOL ) q = 0.0;\n\n        // --- add flow inflow to node's lateral inflow\n        Node[j].newLatFlow += q;\n        massbal_addInflowFlow(DRY_WEATHER_INFLOW, q);\n",
     "        if ( fabs(q) < FLOW_TOL ) q = 0.0;\n\n        // --- add flow inflow to node's lateral inflow\n        Node[j].newLatFlow += q;\n        massbal_addInflowFlow(DRY_WEATHER_INFLOW, q);\n"
     "        if ( QAQC_NODE(j) ) QAQC_NODE(j)[QN_DWF] = q;                  // QAQC:\n")

edit("routing.c",
     "        Node[j].newLatFlow += q;\n        massbal_addInflowFlow(RDII_INFLOW, q);\n",
     "        Node[j].newLatFlow += q;\n        massbal_addInflowFlow(RDII_INFLOW, q);\n"
     "        if ( QAQC_NODE(j) ) QAQC_NODE(j)[QN_RDII] = q;                 // QAQC:\n")

edit("routing.c",
     "    if ( ErrorCode ) return;\n\n    // --- update mass balance totals over previous half time step\n",
     "    if ( ErrorCode ) return;\n"
     "    QaqcRouteStep = routingStep;                                   // QAQC:\n"
     "    QaqcTrials = 0;                                                // QAQC:\n"
     "    if ( QaqcNode )                                                // QAQC:\n"
     "    {                                                              // QAQC:\n"
     "        int qj;                                                    // QAQC:\n"
     "        for (qj = 0; qj < Nobjects[NODE]; qj++)                    // QAQC:\n"
     "        {                                                          // QAQC:\n"
     "            QAQC_NODE(qj)[QN_DWF] = 0.0;                           // QAQC:\n"
     "            QAQC_NODE(qj)[QN_RDII] = 0.0;                          // QAQC:\n"
     "        }                                                          // QAQC:\n"
     "    }                                                              // QAQC:\n"
     "\n    // --- update mass balance totals over previous half time step\n")

# ---------------------------------------------------------------- output.c
edit("output.c", '#include "headers.h"\n',
     '#include "headers.h"\n#include "qaqc.h"                                              // QAQC:\n')

edit("output.c",
     "static REAL4     SysResults[MAX_SYS_RESULTS];    // values of system output vars.\n",
     "static REAL4     SysResults[MAX_SYS_RESULTS + QAQC_MAX_SYS]; // values of system output vars. (QAQC: room for extras)\n"
     "static INT4      NumSysVars;           // QAQC: number of system output variables\n"
     "static INT4      QaqcSubStart;         // QAQC: index of first QA/QC value in a subcatchment record\n"
     "static INT4      QaqcNodeStart;        // QAQC: ... in a node record\n"
     "static INT4      QaqcLinkStart;        // QAQC: ... in a link record\n")

edit("output.c",
     "    NumSubcatchVars = MAX_SUBCATCH_RESULTS - 1 + NumPolluts;\n",
     "    NumSubcatchVars = MAX_SUBCATCH_RESULTS - 1 + NumPolluts;\n"
     "    QaqcSubStart = NumSubcatchVars;                                // QAQC:\n"
     "    NumSubcatchVars += qaqc_count(QAQC_SUBCATCH);                  // QAQC:\n")
edit("output.c",
     "    NumNodeVars = MAX_NODE_RESULTS - 1 + NumPolluts;\n",
     "    NumNodeVars = MAX_NODE_RESULTS - 1 + NumPolluts;\n"
     "    QaqcNodeStart = NumNodeVars;                                   // QAQC:\n"
     "    NumNodeVars += qaqc_count(QAQC_NODE);                          // QAQC:\n")
edit("output.c",
     "    NumLinkVars = MAX_LINK_RESULTS - 1 + NumPolluts;\n",
     "    NumLinkVars = MAX_LINK_RESULTS - 1 + NumPolluts;\n"
     "    QaqcLinkStart = NumLinkVars;                                   // QAQC:\n"
     "    NumLinkVars += qaqc_count(QAQC_LINK);                          // QAQC:\n"
     "    NumSysVars = MAX_SYS_RESULTS + qaqc_count(QAQC_SYS);           // QAQC:\n")

edit("output.c",
     "        + ((F_OFF)NumLinks * (F_OFF)NumLinkVars) + MAX_SYS_RESULTS;\n",
     "        + ((F_OFF)NumLinks * (F_OFF)NumLinkVars) + NumSysVars;     // QAQC: was MAX_SYS_RESULTS\n")

# header: QA/QC variable codes follow the pollutant codes of each class
edit("output.c",
     "        k = SUBCATCH_WASHOFF + j;\n        fwrite(&k, sizeof(INT4), 1, Fout.file);\n    }\n",
     "        k = SUBCATCH_WASHOFF + j;\n        fwrite(&k, sizeof(INT4), 1, Fout.file);\n    }\n"
     "    for (j=0; j<qaqc_count(QAQC_SUBCATCH); j++)                    // QAQC:\n"
     "    {                                                              // QAQC:\n"
     "        k = QAQC_CODE_BASE + j;                                    // QAQC:\n"
     "        fwrite(&k, sizeof(INT4), 1, Fout.file);                    // QAQC:\n"
     "    }                                                              // QAQC:\n")
edit("output.c",
     "        k = NODE_QUAL + j;\n        fwrite(&k, sizeof(INT4), 1, Fout.file);\n    }\n",
     "        k = NODE_QUAL + j;\n        fwrite(&k, sizeof(INT4), 1, Fout.file);\n    }\n"
     "    for (j=0; j<qaqc_count(QAQC_NODE); j++)                        // QAQC:\n"
     "    {                                                              // QAQC:\n"
     "        k = QAQC_CODE_BASE + j;                                    // QAQC:\n"
     "        fwrite(&k, sizeof(INT4), 1, Fout.file);                    // QAQC:\n"
     "    }                                                              // QAQC:\n")
edit("output.c",
     "        k = LINK_QUAL + j;\n        fwrite(&k, sizeof(INT4), 1, Fout.file);\n    }\n",
     "        k = LINK_QUAL + j;\n        fwrite(&k, sizeof(INT4), 1, Fout.file);\n    }\n"
     "    for (j=0; j<qaqc_count(QAQC_LINK); j++)                        // QAQC:\n"
     "    {                                                              // QAQC:\n"
     "        k = QAQC_CODE_BASE + j;                                    // QAQC:\n"
     "        fwrite(&k, sizeof(INT4), 1, Fout.file);                    // QAQC:\n"
     "    }                                                              // QAQC:\n")
edit("output.c",
     "    k = MAX_SYS_RESULTS;\n    fwrite(&k, sizeof(INT4), 1, Fout.file);\n"
     "    for (k=0; k<MAX_SYS_RESULTS; k++) fwrite(&k, sizeof(INT4), 1, Fout.file);\n",
     "    k = NumSysVars;                                                // QAQC: was MAX_SYS_RESULTS\n"
     "    fwrite(&k, sizeof(INT4), 1, Fout.file);\n"
     "    for (k=0; k<MAX_SYS_RESULTS; k++) fwrite(&k, sizeof(INT4), 1, Fout.file);\n"
     "    for (j=0; j<qaqc_count(QAQC_SYS); j++)                         // QAQC:\n"
     "    {                                                              // QAQC:\n"
     "        k = QAQC_CODE_BASE + j;                                    // QAQC:\n"
     "        fwrite(&k, sizeof(INT4), 1, Fout.file);                    // QAQC:\n"
     "    }                                                              // QAQC:\n")

edit("output.c",
     "    fwrite(SysResults, sizeof(REAL4), MAX_SYS_RESULTS, Fout.file);\n\n    // --- save outfall flows",
     "    if ( qaqc_count(QAQC_SYS) > 0 )                                // QAQC:\n"
     "        qaqc_getSysResults(SysResults + MAX_SYS_RESULTS);          // QAQC:\n"
     "    fwrite(SysResults, sizeof(REAL4), NumSysVars, Fout.file);      // QAQC: was MAX_SYS_RESULTS\n\n    // --- save outfall flows")

edit("output.c",
     "        subcatch_getResults(j, f, SubcatchResults);\n        if ( Subcatch[j].rptFlag )\n",
     "        subcatch_getResults(j, f, SubcatchResults);\n"
     "        if ( Subcatch[j].rptFlag && qaqc_count(QAQC_SUBCATCH) > 0 )            // QAQC:\n"
     "            qaqc_getSubcatchResults(j, SubcatchResults + QaqcSubStart);        // QAQC:\n"
     "        if ( Subcatch[j].rptFlag )\n")

# node_getResults / link_getResults are called from three places each (point
# results, running averages); extend every one
edit("output.c",
     "        node_getResults(j, f, NodeResults);\n",
     "        node_getResults(j, f, NodeResults);\n"
     "        if ( Node[j].rptFlag && qaqc_count(QAQC_NODE) > 0 )                    // QAQC:\n"
     "            qaqc_getNodeResults(j, NodeResults + QaqcNodeStart);               // QAQC:\n")
edit("output.c",
     "            link_getResults(j, f, LinkResults);\n",
     "            link_getResults(j, f, LinkResults);\n"
     "            if ( qaqc_count(QAQC_LINK) > 0 )                                   // QAQC:\n"
     "                qaqc_getLinkResults(j, LinkResults + QaqcLinkStart);           // QAQC:\n")
edit("output.c",
     "        node_getResults(i, 1.0, NodeResults);\n",
     "        node_getResults(i, 1.0, NodeResults);\n"
     "        if ( qaqc_count(QAQC_NODE) > 0 )                                       // QAQC:\n"
     "            qaqc_getNodeResults(i, NodeResults + QaqcNodeStart);               // QAQC:\n")
edit("output.c",
     "        link_getResults(i, 1.0, LinkResults);\n",
     "        link_getResults(i, 1.0, LinkResults);\n"
     "        if ( qaqc_count(QAQC_LINK) > 0 )                                       // QAQC:\n"
     "            qaqc_getLinkResults(i, LinkResults + QaqcLinkStart);               // QAQC:\n")

edit("output.c",
     "    fread(SysResults, sizeof(REAL4), MAX_SYS_RESULTS, Fout.file);\n",
     "    fread(SysResults, sizeof(REAL4), NumSysVars, Fout.file);       // QAQC: was MAX_SYS_RESULTS\n")

# ---------------------------------------------------------------- swmm5.c
edit("swmm5.c", '#include "swmm5.h"                     // declaration of SWMM\'s API functions\n',
     '#include "swmm5.h"                     // declaration of SWMM\'s API functions\n#include "qaqc.h"                      // QAQC:\n')

edit("swmm5.c",
     "        output_open();\n",
     "        if ( qaqc_open() ) report_writeErrorMsg(ERR_MEMORY, \"\");   // QAQC:\n"
     "        output_open();\n")

edit("swmm5.c",
     "    if ( Fout.file ) output_close();\n",
     "    if ( Fout.file ) output_close();\n"
     "    qaqc_close();                                                  // QAQC:\n")

# new API entry points, placed just ahead of swmm_getVersion
edit("swmm5.c",
     "int  DLLEXPORT swmm_getVersion()\n",
     "//// ---- QAQC: API additions ---- ////\n"
     "\n"
     "int DLLEXPORT swmm_setQaqc(int enabled)\n"
     "//\n"
     "//  Input:   enabled = 1 to append QA/QC variables to the binary output\n"
     "//                    file (the default), 0 for stock 5.2.4 output\n"
     "//  Output:  returns the new setting\n"
     "//  Purpose: switches the extended QA/QC output on or off. Call it before\n"
     "//           swmm_start (or swmm_run).\n"
     "//\n"
     "{\n"
     "    QaqcEnabled = (enabled != 0);\n"
     "    return QaqcEnabled;\n"
     "}\n"
     "\n"
     "int DLLEXPORT swmm_getQaqcCount(int objClass)\n"
     "//\n"
     "//  Input:   objClass = 0 subcatchment, 1 node, 2 link, 3 system\n"
     "//  Output:  returns number of QA/QC variables appended for that class\n"
     "//\n"
     "{\n"
     "    return qaqc_count(objClass);\n"
     "}\n"
     "\n"
     "int DLLEXPORT swmm_writeQaqcCatalog(const char* fname)\n"
     "//\n"
     "//  Input:   fname = name of file to write\n"
     "//  Output:  returns 0 if successful\n"
     "//  Purpose: writes a JSON description of every QA/QC variable.\n"
     "//\n"
     "{\n"
     "    FILE* f = fopen(fname, \"wt\");\n"
     "    if ( f == NULL ) return 1;\n"
     "    qaqc_writeCatalog(f);\n"
     "    fclose(f);\n"
     "    return 0;\n"
     "}\n"
     "\n"
     "//// ---- QAQC: end of API additions ---- ////\n"
     "\n"
     "int  DLLEXPORT swmm_getVersion()\n")

# Linux: realpath() writes up to PATH_MAX bytes into a 260-byte buffer
edit("swmm5.c",
     "            realpath(fname, absPath);\n",
     "            {                                                      // QAQC: Linux fix -\n"
     "                char* rp = realpath(fname, NULL);                  // realpath() can write PATH_MAX\n"
     "                if ( rp )                                          // bytes; absPath holds MAXFNAME+1\n"
     "                {\n"
     "                    sstrncpy(absPath, rp, size - 1);\n"
     "                    free(rp);\n"
     "                }\n"
     "                else sstrncpy(absPath, fname, size - 1);\n"
     "            }\n")

# ---------------------------------------------------------------- swmm5.h
p = solver / "include" / "swmm5.h"
s = p.read_text(encoding="latin-1")
anchor = "int    DLLEXPORT swmm_getVersion(void);"
assert s.count(anchor) == 1, "swmm5.h anchor"
s = s.replace(anchor, anchor + "\n"
    "int    DLLEXPORT swmm_setQaqc(int enabled);                 // QAQC:\n"
    "int    DLLEXPORT swmm_getQaqcCount(int objClass);           // QAQC:\n"
    "int    DLLEXPORT swmm_writeQaqcCatalog(const char* fname);  // QAQC:")
p.write_text(s, encoding="latin-1")


# ---------------------------------------------------------------- run/main.c
p = src / "run" / "main.c"
s = p.read_text(encoding="latin-1")
old = "    if (argc == 1)\n"
assert s.count(old) == 1, "main.c anchor"
s = s.replace(old,
    "    // --- QAQC: optional flags, removed from argv before normal parsing\n"
    "    //     --stock           write stock 5.2.4 output (no QA/QC variables)\n"
    "    //     --catalog <file>  write the QA/QC variable catalog (JSON) and exit\n"
    "    {                                                              // QAQC:\n"
    "        int i, n = 1;                                              // QAQC:\n"
    "        for (i = 1; i < argc; i++)                                 // QAQC:\n"
    "        {                                                          // QAQC:\n"
    "            if ( strcmp(argv[i], \"--stock\") == 0 ) swmm_setQaqc(0); // QAQC:\n"
    "            else if ( strcmp(argv[i], \"--catalog\") == 0 && i+1 < argc ) // QAQC:\n"
    "                return swmm_writeQaqcCatalog(argv[i+1]);           // QAQC:\n"
    "            else argv[n++] = argv[i];                              // QAQC:\n"
    "        }                                                          // QAQC:\n"
    "        argc = n;                                                  // QAQC:\n"
    "    }                                                              // QAQC:\n"
    + old)
p.write_text(s, encoding="latin-1")
print("main.c flags added")
print("QA/QC hooks applied to", src)
