"""
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

workspace(name = "velodyne_lidar")

# Point following dependency to Isaac SDK night Release 0528 downloaded from https://developer.nvidia.com/isaac/downloads
local_repository(
    name = "com_nvidia_isaac",
    path = "ISAAC_SDK_NIGHTLY_RELEASE_0528",
)

load("@com_nvidia_isaac//third_party:engine.bzl", "isaac_engine_workspace")
load("@com_nvidia_isaac//third_party:packages.bzl", "isaac_packages_workspace")

isaac_engine_workspace()

isaac_packages_workspace()

# Configures toolchain
load("@com_nvidia_isaac//engine/build/toolchain:toolchain.bzl", "toolchain_configure")

toolchain_configure(name = "toolchain")

####################################################################################################

# Velodyne Lidar specific dependencies

load(":repositories.bzl", "velodyne_lidar_workspace")

velodyne_lidar_workspace()
