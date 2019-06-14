/*
Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
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
