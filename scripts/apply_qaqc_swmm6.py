#!/usr/bin/env python3
"""Apply the QA/QC output hooks to an OpenSWMM Engine checkout (swmm6_rel).

usage: apply_qaqc_swmm6.py <openswmm.engine checkout> <dir with core/Qaqc*.{hpp,cpp}> <swmm5 qaqc.c>

Tested against HydroCouple/openswmm.engine swmm6_rel at commit
da2b248937c85b6caf453044c4bc98b3f1de41c1 (2026-11-19). Every inserted line
carries the tag  QAQC:  and each edit asserts that its anchor occurs exactly
once, so a moved anchor fails loudly instead of patching the wrong place.
"""
import pathlib
import shutil
import sys

repo = pathlib.Path(sys.argv[1])
newsrc = pathlib.Path(sys.argv[2])
qaqc_c = pathlib.Path(sys.argv[3])
eng = repo / "src" / "engine"


def edit(rel, old, new, count=1):
    p = repo / rel
    s = p.read_text(encoding="utf-8")
    n = s.count(old)
    assert n == count, f"{rel}: anchor found {n} times, expected {count}:\n{old[:160]}"
    p.write_text(s.replace(old, new), encoding="utf-8")


# --- new files -------------------------------------------------------------
for f in ("QaqcStore.hpp", "Qaqc.hpp", "Qaqc.cpp"):  # <dir>/core/
    shutil.copy(newsrc / "core" / f, eng / "core" / f)

# catalog block shared verbatim with the SWMM5 build
s = qaqc_c.read_text(encoding="latin-1")
a = s.index("//QAQC-CATALOG-BEGIN")
b = s.index("//QAQC-CATALOG-END")
block = s[s.index("\n", a) + 1:b]
(eng / "core" / "QaqcCatalog.inc").write_text(
    "// Generated from swmm5-qaqc/engine/swmm5/src/solver/qaqc.c - do not edit.\n"
    "// Catalog of QA/QC output variables, shared by the SWMM5 and SWMM6 builds.\n"
    + block, encoding="utf-8")

# --- SimulationContext: the capture store ----------------------------------
edit("src/engine/core/SimulationContext.hpp",
     '#include "../hydrology/FrozenGround.hpp"\n',
     '#include "../hydrology/FrozenGround.hpp"\n'
     '#include "QaqcStore.hpp" // QAQC:\n')
edit("src/engine/core/SimulationContext.hpp",
     "    /**\n     * @brief Per-virtual-junction momentum-residual accumulators.\n",
     "    /// QAQC: capture store for the QA/QC output variables. Written by\n"
     "    /// DWSolver, read when a report snapshot is built; never read by the\n"
     "    /// solver itself, hence mutable.\n"
     "    mutable qaqc::Store qaqc; // QAQC:\n"
     "\n"
     "    /**\n     * @brief Per-virtual-junction momentum-residual accumulators.\n")

# --- SimulationSnapshot: the QA/QC block -----------------------------------
edit("include/openswmm/plugin_sdk/SimulationSnapshot.hpp",
     "    std::vector<double> subcatch_quality;\n",
     "    std::vector<double> subcatch_quality;\n"
     "\n"
     "    // QAQC: solver QA/QC variables in display units (empty when off).\n"
     "    // Layout: [object_index * qaqc_*_vars + variable_index].\n"
     "    int qaqc_link_vars = 0;          // QAQC:\n"
     "    int qaqc_node_vars = 0;          // QAQC:\n"
     "    int qaqc_sys_vars = 0;           // QAQC:\n"
     "    std::vector<float> qaqc_link;    // QAQC:\n"
     "    std::vector<float> qaqc_node;    // QAQC:\n"
     "    std::vector<float> qaqc_sys;     // QAQC:\n")

# --- DWSolver: capture -----------------------------------------------------
DW = "src/engine/hydraulics/DynamicWave.cpp"

edit(DW,
     "    // Per-timestep constant\n    dt_gravity_ = dt * GRAVITY;\n",
     "    // QAQC: size the capture store and open a new capture serial\n"
     "    ctx.qaqc.ensure(static_cast<std::size_t>(n_links_),           // QAQC:\n"
     "                    static_cast<std::size_t>(n_nodes_));          // QAQC:\n"
     "    ctx.qaqc.serial += 1.0;                                       // QAQC:\n"
     "    ctx.qaqc.route_step = dt;                                     // QAQC:\n"
     "\n"
     "    // Per-timestep constant\n    dt_gravity_ = dt * GRAVITY;\n")

edit(DW,
     "    steps = steps_final;\n    converged = converged_final;\n",
     "    steps = steps_final;\n    converged = converged_final;\n"
     "    // QAQC: step totals, plus the geometry of the conduits whose momentum\n"
     "    // kernel did not run in this step (dry or closed)\n"
     "    if (ctx.qaqc.enabled) {                                       // QAQC:\n"
     "        ctx.qaqc.trials = steps;                                  // QAQC:\n"
     "        ctx.qaqc.non_converged = 0;                               // QAQC:\n"
     "        for (int qi = 0; qi < n_nodes_; ++qi) {                   // QAQC:\n"
     "            const auto uq = static_cast<std::size_t>(qi);         // QAQC:\n"
     "            double* qn = ctx.qaqc.nodeSlot(uq);                   // QAQC:\n"
     "            if (qn == nullptr || ctx.nodes.type[uq] == NodeType::OUTFALL) // QAQC:\n"
     "                continue;                                         // QAQC:\n"
     "            qn[qaqc::QN_CONVERGED] = xnode_.converged[uq] ? 1.0 : 0.0;   // QAQC:\n"
     "            qn[qaqc::QN_SUMDQDH] = xnode_.sumdqdh[uq];            // QAQC:\n"
     "            if (!xnode_.converged[uq])                            // QAQC:\n"
     "                ++ctx.qaqc.non_converged;                         // QAQC:\n"
     "        }                                                         // QAQC:\n"
     "        for (int qj = 0; qj < n_links_; ++qj) {                   // QAQC:\n"
     "            const auto uq = static_cast<std::size_t>(qj);         // QAQC:\n"
     "            double* qc = ctx.qaqc.linkSlot(uq);                   // QAQC:\n"
     "            if (qc == nullptr || uq >= area1_.size())             // QAQC:\n"
     "                continue;                                         // QAQC:\n"
     "            if (qc[qaqc::QC_SERIAL] == ctx.qaqc.serial)           // QAQC: kernel ran;\n"
     "                continue;                                         // QAQC: it captured these\n"
     "            qc[qaqc::QC_Y1] = depth1_[uq];                        // QAQC:\n"
     "            qc[qaqc::QC_Y2] = depth2_[uq];                        // QAQC:\n"
     "            qc[qaqc::QC_H1] = h1_[uq];                            // QAQC:\n"
     "            qc[qaqc::QC_H2] = h2_[uq];                            // QAQC:\n"
     "            qc[qaqc::QC_A1] = area1_[uq];                         // QAQC:\n"
     "            qc[qaqc::QC_A2] = area2_[uq];                         // QAQC:\n"
     "            qc[qaqc::QC_W1] = width1_[uq];                        // QAQC:\n"
     "            qc[qaqc::QC_W2] = width2_[uq];                        // QAQC:\n"
     "            qc[qaqc::QC_SA1] = surf_area1_[uq];                   // QAQC:\n"
     "            qc[qaqc::QC_SA2] = surf_area2_[uq];                   // QAQC:\n"
     "            qc[qaqc::QC_FASNH] = fasnh_[uq];                      // QAQC:\n"
     "        }                                                         // QAQC:\n"
     "    }                                                             // QAQC:\n")

# every conduit that reaches a momentum kernel passes through traceLinkTerms
edit(DW,
     "    const auto& links = ctx.links;\n    {\n        static FILE* lf = nullptr;\n",
     "    const auto& links = ctx.links;\n"
     "    if (double* qc = ctx.qaqc.linkSlot(uj)) {                     // QAQC:\n"
     "        qc[qaqc::QC_Y1] = depth1_[uj];                            // QAQC:\n"
     "        qc[qaqc::QC_Y2] = depth2_[uj];                            // QAQC:\n"
     "        qc[qaqc::QC_H1] = h1_[uj];                                // QAQC:\n"
     "        qc[qaqc::QC_H2] = h2_[uj];                                // QAQC:\n"
     "        qc[qaqc::QC_A1] = area1_[uj];                             // QAQC:\n"
     "        qc[qaqc::QC_A2] = area2_[uj];                             // QAQC:\n"
     "        qc[qaqc::QC_W1] = width1_[uj];                            // QAQC:\n"
     "        qc[qaqc::QC_W2] = width2_[uj];                            // QAQC:\n"
     "        qc[qaqc::QC_SA1] = surf_area1_[uj];                       // QAQC:\n"
     "        qc[qaqc::QC_SA2] = surf_area2_[uj];                       // QAQC:\n"
     "        qc[qaqc::QC_FASNH] = fasnh_[uj];                          // QAQC:\n"
     "        qc[qaqc::QC_AMID] = area_mid_[uj];                        // QAQC:\n"
     "        qc[qaqc::QC_AWTD] = aWtd;                                 // QAQC:\n"
     "        qc[qaqc::QC_R1] = hrad1_[uj];                             // QAQC:\n"
     "        qc[qaqc::QC_RMID] = hrad_mid_[uj];                        // QAQC:\n"
     "        qc[qaqc::QC_RWTD] = rWtd;                                 // QAQC:\n"
     "        qc[qaqc::QC_V] = v;                                       // QAQC:\n"
     "        qc[qaqc::QC_RHO] = rho;                                   // QAQC:\n"
     "        qc[qaqc::QC_SIGMA] = sig;                                 // QAQC:\n"
     "        qc[qaqc::QC_DQ1] = dq1;                                   // QAQC:\n"
     "        qc[qaqc::QC_DQ2] = dq2;                                   // QAQC:\n"
     "        qc[qaqc::QC_DQ3] = dq3;                                   // QAQC:\n"
     "        qc[qaqc::QC_DQ4] = dq4;                                   // QAQC:\n"
     "        qc[qaqc::QC_DQ5] = dq5;                                   // QAQC:\n"
     "        qc[qaqc::QC_DQ6] = dq6;                                   // QAQC:\n"
     "        qc[qaqc::QC_DQ1FM] = 0.0;                                 // QAQC:\n"
     "        qc[qaqc::QC_QMOM] = q;                                    // QAQC:\n"
     "        qc[qaqc::QC_QOLD] = qOld;                                 // QAQC:\n"
     "        qc[qaqc::QC_QLAST] = qLast;                               // QAQC:\n"
     "        qc[qaqc::QC_DQDH] = dqdh_[uj];                            // QAQC:\n"
     "        qc[qaqc::QC_FROUDE] = froude_[uj];                        // QAQC:\n"
     "        qc[qaqc::QC_SERIAL] = ctx.qaqc.serial;                    // QAQC:\n"
     "    }                                                             // QAQC:\n"
     "    {\n        static FILE* lf = nullptr;\n")

# force-main kernel: remember that dq1 came from the force main equation
edit(DW,
     "    traceLinkTerms(ctx, uj, qLast, v, 0.0, 1.0, aWtd, rWtd, dq1, dq2, 0.0, 0.0, dq5, dq6, qOld, q,\n"
     "                   aMidConv);\n",
     "    traceLinkTerms(ctx, uj, qLast, v, 0.0, 1.0, aWtd, rWtd, dq1, dq2, 0.0, 0.0, dq5, dq6, qOld, q,\n"
     "                   aMidConv);\n"
     "    if (double* qc = ctx.qaqc.linkSlot(uj))                       // QAQC:\n"
     "        qc[qaqc::QC_DQ1FM] = dq1;                                 // QAQC:\n")

# node depth update: the terms of the last trial
edit(DW,
     "    // A3 parity term tracing for one node (SWMM_TRACE_NODE=<index>, first 64\n",
     "    if (double* qn = ctx.qaqc.nodeSlot(ui)) {                     // QAQC:\n"
     "        double qd = surf_area / dt;                               // QAQC:\n"
     "        if (node_continuity == NodeContinuity::SEMI_IMPLICIT)     // QAQC:\n"
     "            qd = (surf_area + 0.5 * dt * xnode_.sumdqdh[ui]) / dt; // QAQC:\n"
     "        else if (use_surcharge_dqdh &&                            // QAQC:\n"
     "                 !(is_storage && xnode_.sumdqdh[ui] == 0.0)) {    // QAQC:\n"
     "            qd = xnode_.sumdqdh[ui];                              // QAQC:\n"
     "            if (y_last < 1.25 * yCrown && yCrown > 0.0)           // QAQC:\n"
     "                qd += (xnode_.old_surf_area[ui] / dt - xnode_.sumdqdh[ui]) * // QAQC:\n"
     "                      std::exp(-15.0 * (y_last - yCrown) / yCrown); // QAQC:\n"
     "        }                                                         // QAQC:\n"
     "        qn[qaqc::QN_AREA] = surf_area;                            // QAQC:\n"
     "        qn[qaqc::QN_OLDAREA] = xnode_.old_surf_area[ui];          // QAQC:\n"
     "        qn[qaqc::QN_DENOM] = qd;                                  // QAQC:\n"
     "        qn[qaqc::QN_F] = dQ;                                      // QAQC:\n"
     "        qn[qaqc::QN_CORR] = y_new - y_last;                       // QAQC:\n"
     "        qn[qaqc::QN_YCROWN] = yCrown;                             // QAQC:\n"
     "        qn[qaqc::QN_DYDT] = std::fabs(y_new - y_old) / dt;        // QAQC:\n"
     "        qn[qaqc::QN_SURCHARGED] = is_surcharged ? 1.0 : 0.0;      // QAQC:\n"
     "        qn[qaqc::QN_SERIAL] = ctx.qaqc.serial;                    // QAQC:\n"
     "    }                                                             // QAQC:\n"
     "\n"
     "    // A3 parity term tracing for one node (SWMM_TRACE_NODE=<index>, first 64\n")

# --- SWMMEngine: fill the snapshot's QA/QC block ---------------------------
edit("src/engine/core/SWMMEngine.cpp",
     '#include "SWMMEngine.hpp"\n',
     '#include "SWMMEngine.hpp"\n#include "Qaqc.hpp" // QAQC:\n')
edit("src/engine/core/SWMMEngine.cpp",
     "            snap.pollut_names = &ctx_.reported_species_names;\n",
     "            snap.pollut_names = &ctx_.reported_species_names;\n"
     "            qaqc::fillSnapshot(ctx_, snap); // QAQC:\n")

# --- DefaultOutputPlugin: append the QA/QC variables -----------------------
OP = "src/engine/plugins/DefaultOutputPlugin.cpp"
edit("src/engine/plugins/DefaultOutputPlugin.hpp",
     "    int n_node_vars_ = 0;\n    int n_link_vars_ = 0;\n",
     "    int n_node_vars_ = 0;\n    int n_link_vars_ = 0;\n"
     "    int n_qaqc_node_ = 0; // QAQC: QA/QC variables appended to each node record\n"
     "    int n_qaqc_link_ = 0; // QAQC: ... to each link record\n"
     "    int n_qaqc_sys_ = 0;  // QAQC: ... to the system record\n")
edit(OP,
     '#include "../core/UnitConversion.hpp"\n',
     '#include "../core/UnitConversion.hpp"\n#include "../core/Qaqc.hpp" // QAQC:\n')
edit(OP,
     "    n_link_vars_ = 5 + n_polluts_;          // flow..capacity + pollutants\n",
     "    n_link_vars_ = 5 + n_polluts_;          // flow..capacity + pollutants\n"
     "    n_qaqc_node_ = ctx.qaqc.enabled ? qaqc::nodeCount() : 0; // QAQC:\n"
     "    n_qaqc_link_ = ctx.qaqc.enabled ? qaqc::linkCount() : 0; // QAQC:\n"
     "    n_qaqc_sys_ = ctx.qaqc.enabled ? qaqc::sysCount() : 0;   // QAQC:\n"
     "    n_node_vars_ += n_qaqc_node_;                            // QAQC:\n"
     "    n_link_vars_ += n_qaqc_link_;                            // QAQC:\n")
edit(OP,
     "    writeInt4(n_node_vars_);\n    for (int v = 0; v < n_node_vars_; ++v)\n        writeInt4(v);\n",
     "    writeInt4(n_node_vars_);\n"
     "    for (int v = 0; v < n_node_vars_ - n_qaqc_node_; ++v) // QAQC: standard codes\n"
     "        writeInt4(v);\n"
     "    for (int v = 0; v < n_qaqc_node_; ++v)                // QAQC:\n"
     "        writeInt4(qaqc::CODE_BASE + v);                   // QAQC:\n")
edit(OP,
     "    writeInt4(n_link_vars_);\n    for (int v = 0; v < n_link_vars_; ++v)\n        writeInt4(v);\n",
     "    writeInt4(n_link_vars_);\n"
     "    for (int v = 0; v < n_link_vars_ - n_qaqc_link_; ++v) // QAQC: standard codes\n"
     "        writeInt4(v);\n"
     "    for (int v = 0; v < n_qaqc_link_; ++v)                // QAQC:\n"
     "        writeInt4(qaqc::CODE_BASE + v);                   // QAQC:\n")
edit(OP,
     "    writeInt4(MAX_SYS_RESULTS);\n    for (int v = 0; v < MAX_SYS_RESULTS; ++v)\n        writeInt4(v);\n",
     "    writeInt4(MAX_SYS_RESULTS + n_qaqc_sys_);             // QAQC:\n"
     "    for (int v = 0; v < MAX_SYS_RESULTS; ++v)\n        writeInt4(v);\n"
     "    for (int v = 0; v < n_qaqc_sys_; ++v)                 // QAQC:\n"
     "        writeInt4(qaqc::CODE_BASE + v);                   // QAQC:\n")
edit(OP,
     "                qi < snapshot.node_quality.size() ? snapshot.node_quality[qi] : 0.0));\n        }\n",
     "                qi < snapshot.node_quality.size() ? snapshot.node_quality[qi] : 0.0));\n        }\n"
     "        for (int v = 0; v < n_qaqc_node_; ++v) {                               // QAQC:\n"
     "            auto qi = uj * static_cast<std::size_t>(n_qaqc_node_) +             // QAQC:\n"
     "                      static_cast<std::size_t>(v);                             // QAQC:\n"
     "            writeReal4(qi < snapshot.qaqc_node.size() ? snapshot.qaqc_node[qi] // QAQC:\n"
     "                                                      : 0.0f);                 // QAQC:\n"
     "        }                                                                      // QAQC:\n")
edit(OP,
     "                qi < snapshot.link_quality.size() ? snapshot.link_quality[qi] : 0.0));\n        }\n",
     "                qi < snapshot.link_quality.size() ? snapshot.link_quality[qi] : 0.0));\n        }\n"
     "        for (int v = 0; v < n_qaqc_link_; ++v) {                               // QAQC:\n"
     "            auto qi = uj * static_cast<std::size_t>(n_qaqc_link_) +             // QAQC:\n"
     "                      static_cast<std::size_t>(v);                             // QAQC:\n"
     "            writeReal4(qi < snapshot.qaqc_link.size() ? snapshot.qaqc_link[qi] // QAQC:\n"
     "                                                      : 0.0f);                 // QAQC:\n"
     "        }                                                                      // QAQC:\n")
edit(OP,
     "    std::fwrite(sys, sizeof(float), MAX_SYS_RESULTS, out_file_);\n",
     "    std::fwrite(sys, sizeof(float), MAX_SYS_RESULTS, out_file_);\n"
     "    for (int v = 0; v < n_qaqc_sys_; ++v)                                      // QAQC:\n"
     "        writeReal4(static_cast<std::size_t>(v) < snapshot.qaqc_sys.size()      // QAQC:\n"
     "                       ? snapshot.qaqc_sys[static_cast<std::size_t>(v)] : 0.0f); // QAQC:\n")

# --- CLI: --stock and --catalog --------------------------------------------
edit("src/cli/main.cpp",
     '#include "version.h"\n',
     '#include "version.h"\n'
     '#include "../engine/core/Qaqc.hpp"      // QAQC:\n'
     '#include "../engine/core/QaqcStore.hpp" // QAQC:\n')
edit("src/cli/main.cpp",
     "int main(int argc, char* argv[]) {\n\n    if (argc < 2) {\n",
     "int main(int argc, char* argv[]) {\n"
     "\n"
     "    // QAQC: optional flags, removed from argv before normal parsing\n"
     "    //   --stock           write stock output (no QA/QC variables)\n"
     "    //   --catalog <file>  write the QA/QC variable catalog (JSON) and exit\n"
     "    {                                                             // QAQC:\n"
     "        int n = 1;                                                // QAQC:\n"
     "        for (int i = 1; i < argc; ++i) {                          // QAQC:\n"
     "            if (std::strcmp(argv[i], \"--stock\") == 0) {           // QAQC:\n"
     "                openswmm::qaqc::setDefaultEnabled(false);         // QAQC:\n"
     "            } else if (std::strcmp(argv[i], \"--catalog\") == 0 && i + 1 < argc) { // QAQC:\n"
     "                std::FILE* cf = std::fopen(argv[i + 1], \"wt\");    // QAQC:\n"
     "                if (cf == nullptr)                                // QAQC:\n"
     "                    return 1;                                     // QAQC:\n"
     "                openswmm::qaqc::writeCatalog(cf);                 // QAQC:\n"
     "                std::fclose(cf);                                  // QAQC:\n"
     "                return 0;                                         // QAQC:\n"
     "            } else {                                              // QAQC:\n"
     "                argv[n++] = argv[i];                              // QAQC:\n"
     "            }                                                     // QAQC:\n"
     "        }                                                         // QAQC:\n"
     "        argc = n;                                                 // QAQC:\n"
     "    }                                                             // QAQC:\n"
     "\n    if (argc < 2) {\n")

print("QA/QC hooks applied to", repo)

# --- Apache-2.0 section 4(b): mark every upstream file that was changed -----
NOTICE_LINE = ("// Modified 2026-11-19 by Robert Dickinson (swmm6-qaqc): solver QA/QC output\n"
               "// variables. Every changed or added line is tagged  QAQC:\n")
for rel in ("src/engine/core/SimulationContext.hpp",
            "include/openswmm/plugin_sdk/SimulationSnapshot.hpp",
            "src/engine/hydraulics/DynamicWave.cpp",
            "src/engine/core/SWMMEngine.cpp",
            "src/engine/plugins/DefaultOutputPlugin.cpp",
            "src/engine/plugins/DefaultOutputPlugin.hpp",
            "src/cli/main.cpp"):
    p = repo / rel
    s = p.read_text(encoding="utf-8")
    if not s.startswith("// Modified 2026-11-19 by Robert Dickinson"):
        p.write_text(NOTICE_LINE + s, encoding="utf-8")
print("modification notices added")
