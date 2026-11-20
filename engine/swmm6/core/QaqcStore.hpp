/**
 * @file QaqcStore.hpp
 * @brief Capture store for the QA/QC output variables (swmm6-qaqc).
 *
 * @details The dynamic wave solver copies the terms it has just computed
 *          (momentum-equation terms, end areas, node update terms) into this
 *          store at the end of each conduit and node evaluation. Nothing here
 *          feeds back into the solution; the store is read only when a report
 *          snapshot is built. All values are in internal units (ft, sec, cfs).
 *
 *          QA/QC variable list and definitions: R. Dickinson (2006-2017 SWMM5
 *          QA/QC builds). This port to OpenSWMM 6 was drafted with Claude
 *          (Anthropic), 2026. Modifications to OpenSWMM Engine files are
 *          marked with the tag  QAQC:
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OPENSWMM_QAQC_STORE_HPP
#define OPENSWMM_QAQC_STORE_HPP

#include <cstddef>
#include <vector>

namespace openswmm {
namespace qaqc {

/// Per-link capture slots. Same names as the SWMM5 QA/QC build (qaqc.h).
enum LinkSlot : int {
    QC_Y1 = 0, QC_Y2, QC_H1, QC_H2,
    QC_A1, QC_A2, QC_AMID, QC_AWTD,
    QC_R1, QC_RMID, QC_RWTD,
    QC_W1, QC_W2,
    QC_V, QC_RHO, QC_SIGMA,
    QC_DQ1, QC_DQ2, QC_DQ3, QC_DQ4, QC_DQ5, QC_DQ6,
    QC_DQ1FM, QC_QMOM, QC_QOLD, QC_QLAST,
    QC_FASNH, QC_SA1, QC_SA2, QC_DQDH, QC_FROUDE,
    QC_SERIAL,                 ///< routing-step serial of the last capture
    QC_NLINKSLOTS
};

/// Per-node capture slots.
enum NodeSlot : int {
    QN_AREA = 0, QN_OLDAREA, QN_SUMDQDH, QN_DENOM, QN_F, QN_CORR, QN_YCROWN,
    QN_DYDT, QN_CONVERGED, QN_SURCHARGED,
    QN_SERIAL,
    QN_NNODESLOTS
};

/// True unless switched off with setDefaultEnabled(false) (CLI --stock) or
/// the environment variable SWMM_QAQC=0.
bool defaultEnabled() noexcept;
void setDefaultEnabled(bool on) noexcept;

struct Store {
    bool enabled = defaultEnabled();
    std::vector<double> link;  ///< n_links * QC_NLINKSLOTS
    std::vector<double> node;  ///< n_nodes * QN_NNODESLOTS
    double route_step = 0.0;   ///< routing step just taken (sec)
    int trials = 0;            ///< dynamic wave trials in that step
    int non_converged = 0;     ///< nodes not converged in that step
    double serial = 0.0;       ///< incremented once per routing step

    void ensure(std::size_t n_links, std::size_t n_nodes) {
        if (!enabled)
            return;
        if (link.size() != n_links * QC_NLINKSLOTS)
            link.assign(n_links * QC_NLINKSLOTS, 0.0);
        if (node.size() != n_nodes * QN_NNODESLOTS)
            node.assign(n_nodes * QN_NNODESLOTS, 0.0);
    }
    double* linkSlot(std::size_t j) noexcept {
        return (enabled && (j + 1) * QC_NLINKSLOTS <= link.size())
                   ? link.data() + j * QC_NLINKSLOTS : nullptr;
    }
    double* nodeSlot(std::size_t i) noexcept {
        return (enabled && (i + 1) * QN_NNODESLOTS <= node.size())
                   ? node.data() + i * QN_NNODESLOTS : nullptr;
    }
    const double* linkSlot(std::size_t j) const noexcept {
        return (enabled && (j + 1) * QC_NLINKSLOTS <= link.size())
                   ? link.data() + j * QC_NLINKSLOTS : nullptr;
    }
    const double* nodeSlot(std::size_t i) const noexcept {
        return (enabled && (i + 1) * QN_NNODESLOTS <= node.size())
                   ? node.data() + i * QN_NNODESLOTS : nullptr;
    }
};

} // namespace qaqc
} // namespace openswmm

#endif // OPENSWMM_QAQC_STORE_HPP
