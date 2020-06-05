/*
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/tools/source/sysman/pci/linux/os_pci_imp.h"

#include "level_zero/tools/source/sysman/linux/fs_access.h"

#include "sysman/linux/os_sysman_imp.h"
#include "sysman/pci/pci_imp.h"

namespace L0 {

const std::string LinuxPciImp::deviceDir("device");
const std::string LinuxPciImp::resourceFile("device/resource");
const std::string LinuxPciImp::maxLinkSpeedFile("device/max_link_speed");
const std::string LinuxPciImp::maxLinkWidthFile("device/max_link_width");
constexpr uint8_t maxPciBars = 6;
// Linux kernel would report 255 link width, as an indication of unknown.
constexpr uint32_t unknownPcieLinkWidth = 255u;

ze_result_t LinuxPciImp::getPciBdf(std::string &bdf) {
    std::string bdfDir;
    ze_result_t result = pSysfsAccess->readSymLink(deviceDir, bdfDir);
    if (ZE_RESULT_SUCCESS != result) {
        return result;
    }
    const auto loc = bdfDir.find_last_of('/');
    bdf = bdfDir.substr(loc + 1);
    return ZE_RESULT_SUCCESS;
}

ze_result_t LinuxPciImp::getMaxLinkSpeed(double &maxLinkSpeed) {
    double val;
    ze_result_t result = pSysfsAccess->read(maxLinkSpeedFile, val);
    if (ZE_RESULT_SUCCESS != result) {
        maxLinkSpeed = 0;
        return result;
    }

    maxLinkSpeed = val;
    return ZE_RESULT_SUCCESS;
}

ze_result_t LinuxPciImp::getMaxLinkWidth(uint32_t &maxLinkwidth) {
    int intVal;

    ze_result_t result = pSysfsAccess->read(maxLinkWidthFile, intVal);
    if (ZE_RESULT_SUCCESS != result) {
        return result;
    }
    if (intVal == static_cast<int>(unknownPcieLinkWidth)) {
        intVal = 0;
    }
    maxLinkwidth = intVal;
    return ZE_RESULT_SUCCESS;
}

ze_result_t LinuxPciImp::getLinkGen(uint32_t &linkGen) {
    double maxLinkSpeed;
    getMaxLinkSpeed(maxLinkSpeed);
    if (maxLinkSpeed == 2.5) {
        linkGen = 1;
    } else if (maxLinkSpeed == 5) {
        linkGen = 2;
    } else if (maxLinkSpeed == 8) {
        linkGen = 3;
    } else if (maxLinkSpeed == 16) {
        linkGen = 4;
    } else if (maxLinkSpeed == 32) {
        linkGen = 5;
    } else {
        linkGen = 0;
    }

    return ZE_RESULT_SUCCESS;
}

void getBarBaseAndSize(std::string readBytes, uint64_t &baseAddr, uint64_t &barSize, uint64_t &barFlags) {

    unsigned long long start, end, flags;
    std::stringstream sStreamReadBytes;
    sStreamReadBytes << readBytes;
    sStreamReadBytes >> std::hex >> start;
    sStreamReadBytes >> end;
    sStreamReadBytes >> flags;

    flags &= 0xf;
    barFlags = flags;
    baseAddr = start;
    barSize = end - start + 1;
}

ze_result_t LinuxPciImp::initializeBarProperties(std::vector<zet_pci_bar_properties_t *> &pBarProperties) {
    std::vector<std::string> ReadBytes;
    ze_result_t result = pSysfsAccess->read(resourceFile, ReadBytes);
    if (result != ZE_RESULT_SUCCESS) {
        return result;
    }
    for (uint32_t i = 0; i <= maxPciBars; i++) {
        uint64_t baseAddr, barSize, barFlags;
        getBarBaseAndSize(ReadBytes[i], baseAddr, barSize, barFlags);
        if (baseAddr) {
            zet_pci_bar_properties_t *pBarProp = new zet_pci_bar_properties_t;
            pBarProp->index = i;
            pBarProp->base = baseAddr;
            pBarProp->size = barSize;
            // Bar Flags Desc.
            // Bit-0 - Value 0x0 -> MMIO type BAR
            // Bit-0 - Value 0x1 -> I/O Type BAR
            // Bit-1 -  Reserved
            // Bit-2 - Valid only for MMIO type BAR
            //         Value  0x1 -> 64bit BAR*/
            pBarProp->type = (barFlags & 0x1) ? ZET_PCI_BAR_TYPE_VGA_IO : ZET_PCI_BAR_TYPE_MMIO;
            if (i == 6) { // the 7th entry of resource file is expected to be ROM BAR
                pBarProp->type = ZET_PCI_BAR_TYPE_ROM;
            }
            pBarProperties.push_back(pBarProp);
        }
    }
    if (pBarProperties.size() == 0) {
        result = ZE_RESULT_ERROR_UNKNOWN;
    }
    return result;
}

LinuxPciImp::LinuxPciImp(OsSysman *pOsSysman) {
    LinuxSysmanImp *pLinuxSysmanImp = static_cast<LinuxSysmanImp *>(pOsSysman);

    pSysfsAccess = &pLinuxSysmanImp->getSysfsAccess();
}

OsPci *OsPci::create(OsSysman *pOsSysman) {
    LinuxPciImp *pLinuxPciImp = new LinuxPciImp(pOsSysman);
    return static_cast<OsPci *>(pLinuxPciImp);
}

} // namespace L0
