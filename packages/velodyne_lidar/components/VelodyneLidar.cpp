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

#include "VelodyneLidar.hpp"

#include <cerrno>
#include <cmath>
#include <cstring>
#include <utility>
#include <vector>

#include "engine/core/assert.hpp"
#include "engine/core/constants.hpp"
#include "engine/core/logger.hpp"
#include "engine/gems/coms/socket.hpp"
#include "messages/tensor.hpp"

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
  if (parameters_.vertical_beams <= 0) {
    reportFailure("Number of vertical beams needs to be positive");
    return;
  }
  if (number_of_rays % parameters_.vertical_beams != 0) {
    reportFailure("Number of rays (%d) is not divisible by number of vertical beams (%d)",
                  number_of_rays, parameters_.vertical_beams);
    return;
  }
  const size_t number_of_slices = number_of_rays / parameters_.vertical_beams;
  Tensor2ui16 ranges(number_of_slices, parameters_.vertical_beams);
  Tensor2ub intensities(number_of_slices, parameters_.vertical_beams);

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
              ranges.view(), intensities.view(),
              (i * parameters_.blocks_per_packet + j) * parameters_.channels_per_block);
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
  ToProto(std::move(ranges), range_scan_proto.initRanges(), tx_scan().buffers());
  ToProto(std::move(intensities), range_scan_proto.initIntensities(), tx_scan().buffers());
  // publish
  tx_scan().publish();
}

void VelodyneLidar::stop() {
  socket_->closeSocket();
}

void VelodyneLidar::processDataBlockVPL16(const VelodyneRawDataBlock& raw_block,
                                          TensorView2ui16 ranges, TensorView2ub intensities,
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
    const size_t index = offset + i;
    ASSERT(parameters_.vertical_beams > 0, "Number of vertical beams needs to be positive");
    const size_t phi_index = index % parameters_.vertical_beams;
    const size_t theta_index = index / parameters_.vertical_beams;
    if (channel.distance < min_range_u16 || max_range_u16 < channel.distance) {
      ranges(theta_index, phi_index) = 0;
      intensities(theta_index, phi_index) = 0;
    } else {
      ranges(theta_index, phi_index) = channel.distance;
      intensities(theta_index, phi_index) = channel.reflectivity;
    }
  }
}

void VelodyneLidar::initLaser(VelodyneModelType model_type) {
  parameters_ = GetVelodyneParameters(model_type);
  raw_packet_.resize(parameters_.packet_sans_header_size, '\0');
}

}  // namespace velodyne_lidar
}  // namespace isaac
