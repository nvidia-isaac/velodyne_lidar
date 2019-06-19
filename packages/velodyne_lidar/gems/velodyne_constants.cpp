/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "velodyne_constants.hpp"

#include "engine/core/assert.hpp"

namespace isaac {
namespace velodyne_lidar {
namespace {
// VLP16 specific constants
constexpr uint32_t kVLP16DataPacketSize = 1248;  // Size of a packet
constexpr uint32_t kVLP16HeaderPacketSize = 42;  // Size of the packet header
constexpr uint32_t kVLP16BlocksPerPacket = 12;   // Number of blocks per packets
constexpr uint32_t kVLP16ChannelsPerBlock = 32;  // Number of channels per blocks
constexpr uint32_t kVLP16VerticalBeams = 16;     // Number of vertical beams
constexpr double kVLP16MaxRange = 100.0;         // Max range in meters
constexpr double kVLP16MinRange = 0.2;           // Min range in meters

// Vertical scanning angles in radians for VLP16
static const double kVLP16VerticalAngles[] = {
    -0.2617993878,  0.01745329252, -0.2268928028,  0.05235987756, -0.1919862177, 0.0872664626,
    -0.1570796327,  0.1221730476,  -0.1221730476,  0.1570796327,  -0.0872664626, 0.1919862177,
    -0.05235987756, 0.2268928028,  -0.01745329252, 0.2617993878};
}  // namespace

VelodyneLidarParameters GetVelodyneParameters(const VelodyneModelType model_type) {
  VelodyneLidarParameters result;
  switch (model_type) {
    case VelodyneModelType::VLP16:
      result.vertical_beams = kVLP16VerticalBeams;
      result.minimum_range = kVLP16MinRange;
      result.maximum_range = kVLP16MaxRange;
      result.packet_header_size = kVLP16HeaderPacketSize;
      result.packet_sans_header_size = kVLP16DataPacketSize - kVLP16HeaderPacketSize;
      result.block_size = 2 + 2 + 3 * kVLP16ChannelsPerBlock;  // 100
      result.channels_per_block = kVLP16ChannelsPerBlock;
      result.blocks_per_packet = kVLP16BlocksPerPacket;
      result.vertical_angles.insert(result.vertical_angles.begin(), kVLP16VerticalAngles,
                                    kVLP16VerticalAngles + 16);
      break;
    default:
      PANIC("Unknown Velodyne Model: %x", model_type);
  }
  return result;
}

}  // namespace velodyne_lidar
}  // namespace isaac
