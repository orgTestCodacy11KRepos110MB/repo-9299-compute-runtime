#!/bin/bash
#
# Copyright (C) 2019-2021 Intel Corporation
#
# SPDX-License-Identifier: MIT
#

git fetch -t
git clone ../compute-runtime neo
docker info
docker build -f scripts/docker/Dockerfile-fedora-31-copr -t neo-fedora-31-copr:ci .
