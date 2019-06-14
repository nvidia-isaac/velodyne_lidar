"""
Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
"""

load("@com_nvidia_isaac//engine/build:isaac.bzl", "isaac_http_archive", "isaac_new_http_archive", "isaac_new_local_repository")

def clean_dep(dep):
    return str(Label(dep))

def velodyne_lidar_workspace():
    isaac_http_archive(
        name = "com_google_absl",
        sha256 = "c8ba586a9ab12bc4a67bb419fc0d2146200942b072bac95f50490f977b7fb04f",
        strip_prefix = "abseil-cpp-5441bbe1db5d0f2ca24b5b60166367b0966790af",
        urls = ["https://github.com/abseil/abseil-cpp/archive/5441bbe1db5d0f2ca24b5b60166367b0966790af.tar.gz"],
        licenses = ["@com_google_absl//:COPYRIGHT"],
    )
