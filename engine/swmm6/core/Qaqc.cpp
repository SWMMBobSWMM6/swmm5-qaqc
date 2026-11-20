/**
 * @file Qaqc.cpp
 * @brief QA/QC output variables computed by the solver (swmm6-qaqc).
 *
 * @details Nothing in this file changes a computed result. It reads the
 *          capture store filled by DWSolver and the simulation context, and
 *          converts to display units. Values are those at the END of the
 *          routing step that reached the reporting time; they are not
 *          interpolated to the reporting time.
 *
 *          Variable list and definitions: R. Dickinson (2006-2017 SWMM5 QA/QC
 *          builds). Port to OpenSWMM 6 drafted with Claude (Anthropic), 2026.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "Qaqc.hpp"
#include "QaqcStore.hpp"
#include "SimulationContext.hpp"
#include "UnitConversion.hpp"
#include "../../../include/openswmm/plugin_sdk/SimulationSnapshot.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace openswmm {
namespace qaqc {

namespace {

// The catalog block below is generated from the SWMM5 build's qaqc.c
// (scripts/make_catalog_inc.py) so the two engines cannot drift apart.
#include "QaqcCatalog.inc"

constexpr double kGravity = 32.2;
constexpr double kFudge = 0.0001;
constexpr double kFlowTol = 0.00001;
constexpr double kSecPerDay = 86400.0;

bool g_default_enabled = true;
bool g_default_set = false;

inline double safeDiv(double a, double b) { return (std::fabs(b) > kFudge) ? a / b : 0.0; }

// Froude number at one end of a conduit from area and top width.
inline double endFroude(double v, double a, double w, double y, double yFull) {
    if (y <= kFudge || a <= kFudge || w <= kFudge)
        return 0.0;
    if (yFull > 0.0 && y >= yFull * (1.0 - 1.0e-6))
        return 0.0;
    return std::fabs(v) / std::sqrt(kGravity * a / w);
}

void linkResults(const SimulationContext& ctx, int j, float* x) {
    const auto uj = static_cast<std::size_t>(j);
    const auto& links = ctx.links;
    const auto& st = ctx.qaqc;
    const double ucfL = ucf::UCF(ucf::LENGTH, ctx.options);
    const double ucfA = ucfL * ucfL;
    const double ucfQ = ucf::UCF(ucf::FLOW, ctx.options);
    const double dt = st.route_step;
    const double* c = st.linkSlot(uj);

    for (int i = 0; i < QL_COUNT; ++i)
        x[i] = 0.0f;
    if (c == nullptr)
        return;

    // values that exist for every kind of link
    x[QL_FROUDE] = static_cast<float>(uj < links.froude.size() ? links.froude[uj] : 0.0);
    x[QL_DQDH] = static_cast<float>((uj < links.dqdh.size() ? links.dqdh[uj] : 0.0) * ucfQ / ucfL);
    x[QL_TS] = static_cast<float>(dt);
    x[QL_ITERATIONS] = static_cast<float>(st.trials);
    x[QL_SETTING] = static_cast<float>(links.setting[uj]);
    x[QL_TARGET] = static_cast<float>(links.target_setting[uj]);
    if (uj < links.time_last_set.size())
        x[QL_TIMEOPEN] =
            static_cast<float>((ctx.current_date - links.time_last_set[uj]) * kSecPerDay);
    x[QL_CURRENTDATE] = static_cast<float>(ctx.current_date - ctx.options.start_date);
    x[QL_FLOWCLASS] = static_cast<float>(static_cast<int>(links.flow_class[uj]));
    if (links.type[uj] != LinkType::CONDUIT)
        return;

    const int cr = ctx.link_subtypes.conduit_row(j);
    if (cr < 0)
        return;
    const auto ucr = static_cast<std::size_t>(cr);
    const auto& CD = ctx.link_subtypes.conduits;
    const double barrels = static_cast<double>(CD.barrels[ucr]);
    const double len = CD.mod_length[ucr];
    x[QL_LENGTH] = static_cast<float>(CD.length[ucr] * ucfL);
    x[QL_MODLENGTH] = static_cast<float>(CD.mod_length[ucr] * ucfL);
    x[QL_ROUGHNESS] = static_cast<float>(CD.roughness[ucr]);
    x[QL_ROUGHFACTOR] = static_cast<float>(CD.rough_factor[ucr]);
    x[QL_SLOPE] = static_cast<float>(CD.slope[ucr]);
    x[QL_QMAX] = static_cast<float>(CD.q_max[ucr] * barrels * ucfQ);
    x[QL_BETA] = static_cast<float>(CD.beta[ucr]);
    // full_state bits: 1 upstream end full, 2 downstream end full (bit 4,
    // the mid-area flag, is dropped so the value matches the SWMM5 build)
    x[QL_FULLSTATE] = static_cast<float>(CD.full_state[ucr] & 3);
    x[QL_NORMALFLOW] = static_cast<float>(CD.normal_flow_limited[ucr]);
    x[QL_INLETCONTROL] = static_cast<float>(CD.inlet_control[ucr]);

    if (ctx.options.routing_model != RoutingModel::DYNWAVE)
        return;

    // geometry and surface areas are refreshed for every conduit each step.
    // The SWMM6 momentum kernel works with conveyance areas: the stored area
    // limited to the full area, so a Preissmann slot is excluded (upstream
    // conveyArea(), issue #144). Those are the areas reported here.
    const double aFull = links.xsect_a_full[uj];
    auto conv = [aFull](double a) { return (aFull > 0.0 && a > aFull) ? aFull : a; };
    const double a1 = conv(c[QC_A1]);
    const double a2 = conv(c[QC_A2]);
    const double aMid = conv(c[QC_AMID]);
    const double y1 = c[QC_Y1];
    const double y2 = c[QC_Y2];
    const double yFull = links.xsect_y_full[uj];
    x[QL_UP_N_AREA] = static_cast<float>(c[QC_SA1] * ucfA);
    x[QL_DN_N_AREA] = static_cast<float>(c[QC_SA2] * ucfA);
    x[QL_FASNH] = static_cast<float>(c[QC_FASNH]);

    // raw end depths (before capping at full depth) relative to full depth
    {
        const auto n1 = static_cast<std::size_t>(links.node1[uj]);
        const auto n2 = static_cast<std::size_t>(links.node2[uj]);
        const double z1 = ctx.nodes.invert_elev[n1] + links.offset1[uj];
        const double z2 = ctx.nodes.invert_elev[n2] + links.offset2[uj];
        const double hh1 = std::max(ctx.nodes.depth[n1] + ctx.nodes.invert_elev[n1], z1);
        const double hh2 = std::max(ctx.nodes.depth[n2] + ctx.nodes.invert_elev[n2], z2);
        x[QL_H1] = static_cast<float>(safeDiv(std::max(hh1 - z1, kFudge), yFull));
        x[QL_H2] = static_cast<float>(safeDiv(std::max(hh2 - z2, kFudge), yFull));
    }

    // the momentum terms exist only if the kernel evaluated this conduit in
    // the routing step just taken (dry and closed conduits return early)
    const bool fresh = (c[QC_SERIAL] == st.serial);
    if (!fresh)
        return;

    const double q = links.flow[uj] / barrels;
    const double v = c[QC_V];

    // Froude number at mid depth as the kernel computed it, and this link's
    // Courant time (formula of legacy getLinkStep)
    x[QL_FROUDE] = static_cast<float>(c[QC_FROUDE]);
    {
        const double qa = std::fabs(q);
        const double fr = c[QC_FROUDE];
        if (qa > kFudge && fr > 0.01 && CD.length[ucr] > 0.0) {
            double t = links.volume[uj] / barrels / qa;
            t = t * CD.mod_length[ucr] / CD.length[ucr];
            t = t * fr / (1.0 + fr);
            x[QL_TIMESTEP] = static_cast<float>(t);
        }
    }
    const double v1 = safeDiv(q, a1);
    const double v2 = safeDiv(q, a2);

    x[QL_F1] = static_cast<float>(endFroude(v1, a1, c[QC_W1], y1, yFull));
    // QL_F2 and QL_W2 are not filled: the kernel's downstream width array is
    // not the plain top width at y2 in every flow class (it matched the SWMM5
    // build on only 63% of otherwise identical records), so no value is
    // reported rather than a doubtful one.
    x[QL_V1] = static_cast<float>(v1 * ucfL);
    x[QL_V2] = static_cast<float>(v2 * ucfL);
    x[QL_DQ1] = static_cast<float>(c[QC_DQ1]);
    x[QL_DQ2] = static_cast<float>(c[QC_DQ2] * ucfQ);
    x[QL_DQ3] = static_cast<float>(c[QC_DQ3] * ucfQ);
    x[QL_DQ4] = static_cast<float>(c[QC_DQ4] * ucfQ);
    x[QL_DQ5] = static_cast<float>(c[QC_DQ5]);
    x[QL_DQ6] = static_cast<float>(c[QC_DQ6] * ucfQ);
    if (c[QC_DQ5] != 0.0 && len > 0.0) {
        const double ql = std::fabs(c[QC_QLAST]);
        const double f = dt / 2.0 / len;
        if (a1 > kFudge)
            x[QL_UPHLOSS] = static_cast<float>(CD.loss_inlet[ucr] * (ql / a1) * f);
        if (a2 > kFudge)
            x[QL_DNHLOSS] = static_cast<float>(CD.loss_outlet[ucr] * (ql / a2) * f);
        if (aMid > kFudge)
            x[QL_MIDHLOSS] = static_cast<float>(CD.loss_avg[ucr] * (ql / aMid) * f);
    }
    x[QL_DQ1HW] = static_cast<float>(c[QC_DQ1FM]);
    x[QL_AMID] = static_cast<float>(aMid * ucfA);
    x[QL_AWTD] = static_cast<float>(c[QC_AWTD] * ucfA);
    x[QL_A1] = static_cast<float>(a1 * ucfA);
    x[QL_A2] = static_cast<float>(a2 * ucfA);
    x[QL_RMID] = static_cast<float>(c[QC_RMID] * ucfL);
    x[QL_RWTD] = static_cast<float>(c[QC_RWTD] * ucfL);
    x[QL_R1] = static_cast<float>(c[QC_R1] * ucfL);
    // QL_R2: the SWMM6 kernel does not compute a downstream hydraulic radius
    x[QL_W1] = static_cast<float>(c[QC_W1] * ucfL);
    x[QL_Y1] = static_cast<float>(y1 * ucfL);
    x[QL_Y2] = static_cast<float>(y2 * ucfL);
    if (CD.normal_flow_limited[ucr])
        x[QL_QNORM] = static_cast<float>(CD.beta[ucr] * a1 * std::pow(c[QC_R1], 2.0 / 3.0) *
                                         barrels * ucfQ);
    x[QL_VENANT] = static_cast<float>(c[QC_QMOM] * barrels * ucfQ);
    x[QL_HGL] = static_cast<float>((c[QC_H2] - c[QC_H1]) * ucfL);
    x[QL_AKON] = static_cast<float>(1.0 + c[QC_DQ1] + c[QC_DQ5]);
    x[QL_SWMM3] = static_cast<float>(0.5 * (a1 + a2) * ucfA);
    x[QL_SWMM4] = static_cast<float>(aMid * ucfA);
    x[QL_SWMM5] = static_cast<float>(c[QC_AWTD] * ucfA);
    x[QL_VHUP] = static_cast<float>(v1 * v1 / 2.0 / kGravity * ucfL);
    x[QL_VHMID] = static_cast<float>(v * v / 2.0 / kGravity * ucfL);
    x[QL_VHDN] = static_cast<float>(v2 * v2 / 2.0 / kGravity * ucfL);
    double hf = 0.0;
    double hLoc = 0.0;
    if (dt > 0.0) {
        hf = c[QC_DQ1] * std::fabs(v) * len / kGravity / dt;
        hLoc = c[QC_DQ5] * std::fabs(v) * len / kGravity / dt;
    }
    x[QL_VHHF] = static_cast<float>(hf * ucfL);
    x[QL_BE_LHS] = static_cast<float>((c[QC_H1] + v1 * v1 / 2.0 / kGravity) * ucfL);
    x[QL_BE_RHS] = static_cast<float>((c[QC_H2] + v2 * v2 / 2.0 / kGravity + hf + hLoc) * ucfL);
    x[QL_RHO] = static_cast<float>(c[QC_RHO]);
    x[QL_SIGMA] = static_cast<float>(c[QC_SIGMA]);
    x[QL_QOLD] = static_cast<float>(c[QC_QOLD] * barrels * ucfQ);
}

void nodeResults(const SimulationContext& ctx, int i, float* x) {
    const auto ui = static_cast<std::size_t>(i);
    const auto& nodes = ctx.nodes;
    const auto& st = ctx.qaqc;
    const double ucfL = ucf::UCF(ucf::LENGTH, ctx.options);
    const double ucfA = ucfL * ucfL;
    const double ucfQ = ucf::UCF(ucf::FLOW, ctx.options);
    const double dt = st.route_step;
    const double* c = st.nodeSlot(ui);

    for (int k = 0; k < QNV_COUNT; ++k)
        x[k] = 0.0f;
    if (c == nullptr)
        return;

    x[QNV_TS] = static_cast<float>(dt);
    if (ui < nodes.rdii_inflow.size())
        x[QNV_IIFLOW] = static_cast<float>(nodes.rdii_inflow[ui] * ucfQ);
    if (ui < nodes.dwf_inflow.size())
        x[QNV_DWFFLOW] = static_cast<float>(nodes.dwf_inflow[ui] * ucfQ);
    x[QNV_OUTFLOW] = static_cast<float>(nodes.outflow[ui] * ucfQ);
    x[QNV_ITERATIONS] = static_cast<float>(st.trials);
    x[QNV_DEGREE] = static_cast<float>(nodes.degree[ui]);
    x[QNV_YCROWN] = static_cast<float>((nodes.crown_elev[ui] - nodes.invert_elev[ui]) * ucfL);

    // node continuity residual over the step
    if (dt > 0.0 && nodes.type[ui] != NodeType::OUTFALL && ui < nodes.old_net_inflow.size() &&
        ui < nodes.old_volume.size()) {
        const double r = 0.5 * (nodes.old_net_inflow[ui] + nodes.inflow[ui] - nodes.outflow[ui]) -
                         nodes.overflow[ui] - (nodes.volume[ui] - nodes.old_volume[ui]) / dt;
        const double big = std::max(std::fabs(nodes.inflow[ui]), std::fabs(nodes.outflow[ui]));
        if (big > kFlowTol)
            x[QNV_CE] = static_cast<float>(100.0 * r / big);
    }
    // QNV_INFIL, QNV_EVAP: storage loss rates are not exposed per step here

    if (ctx.options.routing_model != RoutingModel::DYNWAVE || c[QN_SERIAL] != st.serial)
        return;
    x[QNV_AREA] = static_cast<float>(c[QN_AREA] * ucfA);
    x[QNV_DQDH] = static_cast<float>(c[QN_SUMDQDH] * ucfQ / ucfL);
    x[QNV_DENOM] = static_cast<float>(c[QN_DENOM] * ucfQ / ucfL);
    x[QNV_F] = static_cast<float>(c[QN_F] * ucfQ);
    x[QNV_OLDAREA] = static_cast<float>(c[QN_OLDAREA] * ucfA);
    x[QNV_CORRECTION] = static_cast<float>(c[QN_CORR] * ucfL);
    x[QNV_CONVERGENCE] = static_cast<float>(c[QN_CONVERGED]);
    x[QNV_SURCHARGED] = static_cast<float>(c[QN_SURCHARGED]);
    if (dt > 0.0)
        x[QNV_OLDAREABYDT] = static_cast<float>(c[QN_OLDAREA] / dt * ucfQ / ucfL);
    if (c[QN_DYDT] > kFudge && c[QN_YCROWN] * 0.25 >= kFudge)
        x[QNV_TIMESTEP] = static_cast<float>(0.25 * c[QN_YCROWN] / c[QN_DYDT]);
}

void sysResults(const SimulationContext& ctx, float* x) {
    const auto& st = ctx.qaqc;
    for (int k = 0; k < QY_COUNT; ++k)
        x[k] = 0.0f;
    x[QY_STEPFLOWERROR] = static_cast<float>(100.0 * ctx.mass_balance.last_step_flow_error);
    // QY_CE: running flow continuity error is not ported to the SWMM6 build
    x[QY_ITERATIONS] = static_cast<float>(st.trials);
    x[QY_TS] = static_cast<float>(st.route_step);
    x[QY_NONCONVERGED] = static_cast<float>(st.non_converged);
    x[QY_WINDSPEED] = static_cast<float>(ctx.climate_state.wind_speed *
                                         ucf::UCF(ucf::WINDSPEED, ctx.options));
}

void writeClass(std::FILE* f, const char* name, const TQaqcVar* v, int n, bool last) {
    std::fprintf(f, "  \"%s\": [\n", name);
    for (int i = 0; i < n; ++i) {
        std::fprintf(f,
                     "    {\"index\": %d, \"code\": %d, \"key\": \"%s\", "
                     "\"unitsUS\": \"%s\", \"unitsSI\": \"%s\", "
                     "\"definition\": \"%s\", \"source\": \"%s\"}%s\n",
                     i, CODE_BASE + i, v[i].key, UnitsUS[v[i].units], UnitsSI[v[i].units],
                     v[i].def, v[i].src, (i < n - 1) ? "," : "");
    }
    std::fprintf(f, "  ]%s\n", last ? "" : ",");
}

} // namespace

bool defaultEnabled() noexcept {
    if (!g_default_set) {
        const char* e = std::getenv("SWMM_QAQC");
        if (e != nullptr && std::strcmp(e, "0") == 0)
            g_default_enabled = false;
        g_default_set = true;
    }
    return g_default_enabled;
}

void setDefaultEnabled(bool on) noexcept {
    g_default_enabled = on;
    g_default_set = true;
}

int linkCount() noexcept { return QL_COUNT; }
int nodeCount() noexcept { return QNV_COUNT; }
int sysCount() noexcept { return QY_COUNT; }

void fillSnapshot(const SimulationContext& ctx, SimulationSnapshot& snap) {
    snap.qaqc_link_vars = 0;
    snap.qaqc_node_vars = 0;
    snap.qaqc_sys_vars = 0;
    if (!ctx.qaqc.enabled)
        return;
    const int nL = ctx.n_links();
    const int nN = ctx.n_nodes();
    snap.qaqc_link_vars = QL_COUNT;
    snap.qaqc_node_vars = QNV_COUNT;
    snap.qaqc_sys_vars = QY_COUNT;
    snap.qaqc_link.assign(static_cast<std::size_t>(nL) * QL_COUNT, 0.0f);
    snap.qaqc_node.assign(static_cast<std::size_t>(nN) * QNV_COUNT, 0.0f);
    snap.qaqc_sys.assign(QY_COUNT, 0.0f);
    for (int j = 0; j < nL; ++j)
        linkResults(ctx, j, snap.qaqc_link.data() + static_cast<std::size_t>(j) * QL_COUNT);
    for (int i = 0; i < nN; ++i)
        nodeResults(ctx, i, snap.qaqc_node.data() + static_cast<std::size_t>(i) * QNV_COUNT);
    sysResults(ctx, snap.qaqc_sys.data());
}

void writeCatalog(std::FILE* f) {
    std::fprintf(f, "{\n");
    std::fprintf(f, "  \"engine\": \"swmm6-qaqc\",\n");
    std::fprintf(f, "  \"base\": \"OpenSWMM Engine 6 (swmm6_rel)\",\n");
    std::fprintf(f, "  \"layout\": \"each record = standard variables, then pollutants, then "
                    "the QA/QC variables below; the code in the .out header is 1000 + index\",\n");
    std::fprintf(f, "  \"flowUnits\": \"'flow' means the model's FLOW_UNITS\",\n");
    writeClass(f, "subcatchment", SubVars, QS_COUNT, false);
    writeClass(f, "node", NodeVars, QNV_COUNT, false);
    writeClass(f, "link", LinkVars, QL_COUNT, false);
    writeClass(f, "system", SysVars, QY_COUNT, true);
    std::fprintf(f, "}\n");
}

} // namespace qaqc
} // namespace openswmm
