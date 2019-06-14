"""
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

load("@com_nvidia_isaac//engine/build:isaac.bzl", "isaac_cc_library", "isaac_cc_module", "isaac_component")

isaac_cc_library(
    name = "gems",
    srcs = ["velodyne_constants.cpp"],
    hdrs = ["velodyne_constants.hpp"],
    visibility = ["//visibility:public"],
    deps = ["@com_nvidia_isaac//engine/core"],
)
