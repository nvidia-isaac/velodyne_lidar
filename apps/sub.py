'''
Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
'''
from isaac import Application
import argparse
import random
import sys

if __name__ == '__main__':
    app = Application(name="sub")
    app.home_workspace_name = 'velodyne'
    app.load(
        "@com_nvidia_isaac_sdk//packages/navsim/apps/navsim_navigation.subgraph.json",
        prefix="sub")
    app.load_module("@com_nvidia_isaac_sdk//packages/sight")
    comp_sight = app.nodes["websight"]["WebsightServer"]
    comp_sight.config["webroot"] = "external/com_nvidia_isaac_sdk/packages/sight/webroot"
    comp_sight.config["assetroot"] = "../isaac_assets"
    comp_sight.config["port"] = 3000

    app.run()
