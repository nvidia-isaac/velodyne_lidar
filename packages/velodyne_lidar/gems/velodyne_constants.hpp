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
#pragma once

#include <cstdint>
#include <vector>

namespace isaac {
namespace velodyne_lidar {

// VLP model specific parameters
struct VelodyneLidarParameters {
  double minimum_range;                 // Min range in meters
  double maximum_range;                 // Max range in meters
  uint32_t packet_header_size;          // Size of packet header in bytes
  uint32_t packet_sans_header_size;     // Size of packet without header in bytes
  uint32_t block_size;                  // Size of block in bytes
  uint32_t channels_per_block;          // Number of channels per blocks
  uint32_t blocks_per_packet;           // Number of blocks per packets
  uint32_t vertical_beams;              // Number of vertical beams
  std::vector<double> vertical_angles;  // Vertical scanning angles in radians
};

// Mode the laser in is (what it sends us back)
constexpr uint8_t kVelodyneModeStrong = 0x37;
constexpr uint8_t kVelodyneModeLast = 0x38;
constexpr uint8_t kVelodyneModeDual = 0x39;
constexpr uint32_t kMaxIntensity = 100;  // Max value for the intensity (100%)
constexpr double kDistanceToMeters = 0.002f;
constexpr uint16_t kDeltaTime = 50;  // Time between firings in microseconds

#pragma pack(push, 1)

struct VelodyneRawChannel {
  uint16_t distance;
  uint8_t reflectivity;
};  // 3 bytes

struct VelodyneRawDataBlock {
  uint16_t dataBlockFlag;  // Should be 0xFFEE
  uint16_t azimuth;
  VelodyneRawChannel channels[];
};  // 100 bytes for VLP16

enum class VelodyneModelType { VLP16, INVALID };

#pragma pack(pop)

// Factory method to retrieve parameters for specific VLP model
VelodyneLidarParameters GetVelodyneParameters(const VelodyneModelType);

}  // namespace velodyne_lidar
}  // namespace isaac
