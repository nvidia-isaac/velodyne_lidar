"""
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

load("@com_nvidia_isaac//engine/build:isaac.bzl", "isaac_app", "isaac_cc_module")

isaac_app(
    name = "vlp16_sample",
    app_json_file = "vlp_sample.app.json",
    modules = [
        "velodyne_lidar",
        "@com_nvidia_isaac//packages/perception",
        "@com_nvidia_isaac//packages/viewers",
    ],
)
