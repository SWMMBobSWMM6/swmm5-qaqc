/**
 * @file Qaqc.hpp
 * @brief QA/QC output variables computed by the solver (swmm6-qaqc).
 *
 * @details Builds the QA/QC block of a report snapshot from the capture store
 *          (QaqcStore.hpp) and the simulation context. The variable catalog
 *          (QaqcCatalog.inc) is shared verbatim with the SWMM5 QA/QC build so
 *          both engines write the same variables under the same codes.
 *
 *          Variable list and definitions: R. Dickinson (2006-2017 SWMM5 QA/QC
 *          builds). Port to OpenSWMM 6 drafted with Claude (Anthropic), 2026.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef OPENSWMM_QAQC_HPP
#define OPENSWMM_QAQC_HPP

#include <cstdio>

namespace openswmm {
struct SimulationContext;
struct SimulationSnapshot;

namespace qaqc {

constexpr int CODE_BASE = 1000; ///< variable codes written to the .out header

int linkCount() noexcept;   ///< number of link QA/QC variables in the catalog
int nodeCount() noexcept;   ///< number of node QA/QC variables
int sysCount() noexcept;    ///< number of system QA/QC variables

/// Fill snap.qaqc_link / qaqc_node / qaqc_sys (display units) from ctx.
void fillSnapshot(const SimulationContext& ctx, SimulationSnapshot& snap);

/// Write the JSON catalog of every QA/QC variable.
void writeCatalog(std::FILE* f);

} // namespace qaqc
} // namespace openswmm

#endif // OPENSWMM_QAQC_HPP
