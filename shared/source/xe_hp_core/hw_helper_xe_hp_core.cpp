/*
 * Copyright (C) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/xe_hp_core/aub_mapper.h"
#include "shared/source/xe_hp_core/hw_cmds.h"

using Family = NEO::XeHpFamily;

#include "shared/source/helpers/api_specific_config.h"
#include "shared/source/helpers/constants.h"
#include "shared/source/helpers/extra_allocation_data_xehp_and_later.inl"
#include "shared/source/helpers/flat_batch_buffer_helper_hw.inl"
#include "shared/source/helpers/hw_helper_base.inl"
#include "shared/source/helpers/hw_helper_tgllp_and_later.inl"
#include "shared/source/helpers/hw_helper_xehp_and_later.inl"
#include "shared/source/os_interface/hw_info_config.h"

namespace NEO {
template <>
const AuxTranslationMode HwHelperHw<Family>::defaultAuxTranslationMode = AuxTranslationMode::Blit;

template <>
uint32_t HwHelperHw<Family>::getMetricsLibraryGenId() const {
    return static_cast<uint32_t>(MetricsLibraryApi::ClientGen::XeHP);
}

template <>
uint32_t HwHelperHw<Family>::getComputeUnitsUsedForScratch(const HardwareInfo *pHwInfo) const {
    if (DebugManager.flags.OverrideNumComputeUnitsForScratch.get() != -1) {
        return static_cast<uint32_t>(DebugManager.flags.OverrideNumComputeUnitsForScratch.get());
    }

    // XeHP and later products return physical threads
    return std::max(pHwInfo->gtSystemInfo.MaxSubSlicesSupported, static_cast<uint32_t>(32)) * pHwInfo->gtSystemInfo.MaxEuPerSubSlice * (pHwInfo->gtSystemInfo.ThreadCount / pHwInfo->gtSystemInfo.EUCount);
}

template <>
inline bool HwHelperHw<Family>::isSpecialWorkgroupSizeRequired(const HardwareInfo &hwInfo, bool isSimulation) const {
    if (DebugManager.flags.ForceWorkgroupSize1x1x1.get() != -1) {
        return static_cast<bool>(DebugManager.flags.ForceWorkgroupSize1x1x1.get());
    } else {
        const auto &hwInfoConfig = *HwInfoConfig::get(hwInfo.platform.eProductFamily);
        return (!isSimulation && isWorkaroundRequired(REVISION_A0, REVISION_B, hwInfo) && hwInfoConfig.getLocalMemoryAccessMode(hwInfo) == LocalMemoryAccessMode::CpuAccessAllowed);
    }
}

template <>
uint32_t HwHelperHw<Family>::computeSlmValues(const HardwareInfo &hwInfo, uint32_t slmSize) {
    using SHARED_LOCAL_MEMORY_SIZE = typename Family::INTERFACE_DESCRIPTOR_DATA::SHARED_LOCAL_MEMORY_SIZE;

    auto slmValue = std::max(slmSize, 1024u);
    slmValue = Math::nextPowerOfTwo(slmValue);
    slmValue = Math::getMinLsbSet(slmValue);
    slmValue = slmValue - 9;
    DEBUG_BREAK_IF(slmValue > 7);
    slmValue *= !!slmSize;

    return slmValue;
}

template <>
void HwHelperHw<Family>::setL1CachePolicy(bool useL1Cache, typename Family::RENDER_SURFACE_STATE *surfaceState, const HardwareInfo *hwInfo) {
}

template <>
bool HwHelperHw<Family>::isBankOverrideRequired(const HardwareInfo &hwInfo) const {

    bool forceOverrideMemoryBankIndex = (HwHelper::getSubDevicesCount(&hwInfo) == 4 &&
                                         isWorkaroundRequired(REVISION_A0, REVISION_B, hwInfo));

    if (DebugManager.flags.ForceMemoryBankIndexOverride.get() != -1) {
        forceOverrideMemoryBankIndex = static_cast<bool>(DebugManager.flags.ForceMemoryBankIndexOverride.get());
    }
    return forceOverrideMemoryBankIndex;
}

template <>
bool HwHelperHw<Family>::isSipWANeeded(const HardwareInfo &hwInfo) const {
    return hwInfo.platform.usRevId <= HwInfoConfig::get(hwInfo.platform.eProductFamily)->getHwRevIdFromStepping(REVISION_B, hwInfo);
}

template <>
bool HwHelperHw<Family>::isBufferSizeSuitableForCompression(const size_t size, const HardwareInfo &hwInfo) const {
    if (DebugManager.flags.OverrideBufferSuitableForRenderCompression.get() != -1) {
        return !!DebugManager.flags.OverrideBufferSuitableForRenderCompression.get();
    }
    if (HwInfoConfig::get(hwInfo.platform.eProductFamily)->allowStatelessCompression(hwInfo)) {
        return true;
    } else {
        return size > KB;
    }
}

template <>
const StackVec<uint32_t, 6> HwHelperHw<Family>::getThreadsPerEUConfigs() const {
    return {4, 8};
}

template <>
std::string HwHelperHw<Family>::getExtensions() const {
    std::string extensions;
    extensions += "cl_intel_dot_accumulate ";
    extensions += "cl_intel_subgroup_local_block_io ";

    return extensions;
}

template <>
void MemorySynchronizationCommands<Family>::setPipeControlWAFlags(PIPE_CONTROL &pipeControl) {
    pipeControl.setCommandStreamerStallEnable(true);
    pipeControl.setHdcPipelineFlush(true);
}

template <>
void MemorySynchronizationCommands<Family>::setPipeControlExtraProperties(PIPE_CONTROL &pipeControl, PipeControlArgs &args) {
    pipeControl.setHdcPipelineFlush(args.hdcPipelineFlush);
    pipeControl.setCompressionControlSurfaceCcsFlush(args.compressionControlSurfaceCcsFlush);
    pipeControl.setWorkloadPartitionIdOffsetEnable(args.workloadPartitionOffset);
    pipeControl.setAmfsFlushEnable(args.amfsFlushEnable);

    if (DebugManager.flags.FlushAllCaches.get()) {
        pipeControl.setHdcPipelineFlush(true);
        pipeControl.setCompressionControlSurfaceCcsFlush(true);
    }
    if (DebugManager.flags.DoNotFlushCaches.get()) {
        pipeControl.setHdcPipelineFlush(false);
        pipeControl.setCompressionControlSurfaceCcsFlush(false);
    }
}

template <>
void MemorySynchronizationCommands<Family>::setPostSyncExtraProperties(PipeControlArgs &args, const HardwareInfo &hwInfo) {
    if (hwInfo.featureTable.flags.ftrLocalMemory) {
        args.hdcPipelineFlush = true;
    }
}

template <>
void MemorySynchronizationCommands<Family>::setCacheFlushExtraProperties(PipeControlArgs &args) {
    args.hdcPipelineFlush = true;
}

template <>
bool HwHelperHw<Family>::additionalPipeControlArgsRequired() const {
    return false;
}

template <>
uint32_t HwHelperHw<Family>::getDefaultRevisionId(const HardwareInfo &hwInfo) const {
    return HwInfoConfig::get(hwInfo.platform.eProductFamily)->getHwRevIdFromStepping(REVISION_B, hwInfo);
}

template class HwHelperHw<Family>;
template class FlatBatchBufferHelperHw<Family>;
template struct MemorySynchronizationCommands<Family>;
template struct LriHelper<Family>;
} // namespace NEO
