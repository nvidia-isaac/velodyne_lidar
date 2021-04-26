#####################################################################################
# Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.

# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto. Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#####################################################################################
@0xf59794ab590ab038;

using import "../external/com_nvidia_isaac_sdk/messages/geometry.capnp".Polyline2dProto;

# A parking spot detected by ParkNet.
struct FooProto {
  # Confidence [0,1] of the parking spot detection.
  confidence @0: Float32;
  # Open polyline for parking spot box projected to ground.
  box @1: Polyline2dProto;
  # The index (0-3) of the detected "entry line".
  entryLineIndex @2: Int32;
}

# A list of parking spots detected by ParkNet.
struct BarProto {
  # The list of parking spot detections.
  detections @0: List(FooProto);
}
