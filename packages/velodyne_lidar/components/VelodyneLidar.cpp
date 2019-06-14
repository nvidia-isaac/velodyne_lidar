/*
Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/
#include "VelodyneLidar.hpp"

#include <cerrno>
#include <cmath>
#include <cstring>
#include <vector>

#include "engine/core/assert.hpp"
#include "engine/core/constants.hpp"
#include "engine/core/logger.hpp"
#include "engine/gems/coms/socket.hpp"

namespace isaac {
namespace velodyne_lidar {

namespace {
constexpr uint16_t kBlockFlag = 0xEEFF;
constexpr int kNumberOfAccumulatedPackets = 5;
}  // namespace

void VelodyneLidar::start() {
  initLaser(get_type());
  previous_packet_ = std::nullopt;

  socket_.reset(Socket::CreateRxUDPSocket(get_ip(), get_port()));
  int res = socket_->startSocket();
  if (res < 0) {
    reportFailure("Could not start socket: code=%d, errno=%d", res, errno);
    return;
  }

  tickBlocking();
}

void VelodyneLidar::tick() {
  // Start preparing the outgoing message
  auto range_scan_proto = tx_scan().initProto();
  range_scan_proto.setRangeDenormalizer(kDistanceToMeters * 65535.0f);
  range_scan_proto.setIntensityDenormalizer(kMaxIntensity);
  range_scan_proto.setInvalidRangeThreshold(parameters_.minimum_range);
  range_scan_proto.setOutOfRangeThreshold(parameters_.maximum_range);
  range_scan_proto.setDeltaTime(kDeltaTime);

  // Prepares vertical angles
  range_scan_proto.initPhi(parameters_.vertical_beams);
  for (uint32_t i = 0; i < parameters_.vertical_beams; i++) {
    range_scan_proto.getPhi().set(i, parameters_.vertical_angles[i]);
  }

  // We need room for one package more as we need it to interpolate the missing azimuth angles.
  std::vector<double> azimuths((kNumberOfAccumulatedPackets + 1) * parameters_.blocks_per_packet);

  // Prepare the rays
  const uint32_t number_of_rays =
      kNumberOfAccumulatedPackets * parameters_.blocks_per_packet * parameters_.channels_per_block;
  auto rays_proto = range_scan_proto.initRays(number_of_rays);

  // For the very first time we run this we do not have a previous package and need to do some
  // special handling.
  const bool is_first_batch = !previous_packet_;

  // read and parse packages
  for (uint32_t i = 0; i < kNumberOfAccumulatedPackets + 1; i++) {
    // acquire package
    if (i == 0) {
      // as the first package we use the last package from the previous run
      if (previous_packet_) {
        raw_packet_ = *previous_packet_;
      }
    } else {
      // read data from lidar proto
      raw_packet_.resize(parameters_.packet_sans_header_size, '\0');
      const uint32_t res = socket_->readPacket(reinterpret_cast<char*>(raw_packet_.data()),
                                               parameters_.packet_sans_header_size);
      if (res != parameters_.packet_sans_header_size) {
        reportFailure("Empty message or timeout: code=%u, errno=%d", res, errno);
        return;
      }
    }
    // process rays
    if (i == kNumberOfAccumulatedPackets) {
      // The last package we read will be saved and published next time.
      previous_packet_ = raw_packet_;
    } else {
      if (!is_first_batch) {
        // Extract rays from package blocks.
        for (uint32_t j = 0; j < parameters_.blocks_per_packet; j++) {
          processDataBlockVPL16(
              *reinterpret_cast<VelodyneRawDataBlock*>(raw_packet_.data() +
                                                       j * parameters_.block_size),
              rays_proto, (i * parameters_.blocks_per_packet + j) * parameters_.channels_per_block);
        }
      }
    }
    // process angles
    if (!is_first_batch) {
      // Extract angles from package blocks.
      for (uint32_t j = 0; j < parameters_.blocks_per_packet; j++) {
        const double azimuth =
            -DegToRad(static_cast<double>(reinterpret_cast<VelodyneRawDataBlock*>(
                                              raw_packet_.data() + j * parameters_.block_size)
                                              ->azimuth) /
                      100.0);
        azimuths[i * parameters_.blocks_per_packet + j] = azimuth;
      }
    }
  }
  // If this is the first time we run this loop we are only interested in the last package
  // and won't publish any data.
  if (is_first_batch) {
    return;
  }
  // Fill in the missing azimuths: every second angle is missing and interpolated
  ASSERT(parameters_.vertical_beams * 2 == parameters_.channels_per_block,
         "Expecting one azimuth every second set.");
  const size_t number_of_azimuth_used = kNumberOfAccumulatedPackets * parameters_.blocks_per_packet;
  auto thetas_proto = range_scan_proto.initTheta(2 * number_of_azimuth_used);
  for (size_t i = 0; i < number_of_azimuth_used; i++) {
    const double a1 = azimuths[i];
    const double a2 = azimuths[i + 1];
    thetas_proto.set(2 * i, a1);
    thetas_proto.set(2 * i + 1, a1 + 0.5 * DeltaAngle(a2, a1));
  }
  // publish
  tx_scan().publish();
}

void VelodyneLidar::stop() {
  socket_->closeSocket();
}

void VelodyneLidar::processDataBlockVPL16(const VelodyneRawDataBlock& raw_block,
                                          capnp::List<RangeScanProto::Ray>::Builder& rays,
                                          int offset) {
  const uint16_t min_range_u16 =
      static_cast<uint16_t>(parameters_.minimum_range / kDistanceToMeters);
  const uint16_t max_range_u16 =
      static_cast<uint16_t>(parameters_.maximum_range / kDistanceToMeters);
  // Block structure depends on the channels available
  for (size_t i = 0; i < parameters_.channels_per_block; i++) {
    if (raw_block.dataBlockFlag != kBlockFlag) {
      LOG_ERROR("Invalid raw_packet");
    }
    const VelodyneRawChannel& channel = raw_block.channels[i];
    if (channel.distance < min_range_u16 || max_range_u16 < channel.distance) {
      rays[offset + i].setRange(0);
      rays[offset + i].setIntensity(0);
    } else {
      rays[offset + i].setRange(channel.distance);
      rays[offset + i].setIntensity(channel.reflectivity);
    }
  }
}

void VelodyneLidar::initLaser(VelodyneModelType model_type) {
  parameters_ = GetVelodyneParameters(model_type);
  raw_packet_.resize(parameters_.packet_sans_header_size, '\0');
}

}  // namespace velodyne_lidar
}  // namespace isaac
