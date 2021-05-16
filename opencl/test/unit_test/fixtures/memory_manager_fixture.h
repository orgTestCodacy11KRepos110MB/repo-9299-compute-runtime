/*
 * Copyright (C) 2018-2021 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/execution_environment/execution_environment.h"
#include "shared/source/helpers/options.h"

using namespace NEO;

class MockCommandStreamReceiver;
namespace NEO {
class MockMemoryManager;
}; // namespace NEO

class MemoryManagerWithCsrFixture {
  public:
    MockMemoryManager *memoryManager;
    ExecutionEnvironment executionEnvironment;
    std::unique_ptr<MockCommandStreamReceiver> csr;
    uint32_t taskCount = 0;
    uint32_t currentGpuTag = initialHardwareTag;

    ~MemoryManagerWithCsrFixture() = default;

    void SetUp();
    void TearDown();
};
