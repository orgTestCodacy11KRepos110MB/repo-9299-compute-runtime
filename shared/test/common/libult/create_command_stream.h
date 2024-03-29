/*
 * Copyright (C) 2018-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace NEO {
class CommandStreamReceiver;
class ExecutionEnvironment;

using DeviceBitfield = std::bitset<4>;

extern CommandStreamReceiver *createCommandStream(ExecutionEnvironment &executionEnvironment,
                                                  uint32_t rootDeviceIndex,
                                                  const DeviceBitfield deviceBitfield);
extern bool prepareDeviceEnvironments(ExecutionEnvironment &executionEnvironment);
} // namespace NEO
