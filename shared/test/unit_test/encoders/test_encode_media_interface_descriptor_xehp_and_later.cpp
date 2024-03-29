/*
 * Copyright (C) 2021-2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/test/unit_test/fixtures/command_container_fixture.h"

using namespace NEO;

using CommandEncodeStatesTest = Test<CommandEncodeStatesFixture>;

HWTEST2_F(CommandEncodeStatesTest, givenCommandContainerWhenEncodingMediaDescriptorThenUsedSizeDidNotIncreased, IsAtLeastXeHpCore) {
    auto sizeBefore = cmdContainer->getCommandStream()->getUsed();
    EncodeMediaInterfaceDescriptorLoad<FamilyType>::encode(*cmdContainer.get());
    auto sizeAfter = cmdContainer->getCommandStream()->getUsed();
    EXPECT_EQ(sizeBefore, sizeAfter);
}
