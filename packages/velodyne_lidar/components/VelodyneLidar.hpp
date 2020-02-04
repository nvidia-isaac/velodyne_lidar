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

#include <memory>
#include <string>
#include <vector>

#include "engine/alice/alice_codelet.hpp"
#include "engine/core/byte.hpp"
#include "engine/core/optional.hpp"
#include "messages/range_scan.capnp.h"
#include "packages/velodyne_lidar/gems/velodyne_constants.hpp"

namespace isaac {

class Socket;

namespace velodyne_lidar {
// Serialization helper for :VelodyneModelType to JSON
NLOHMANN_JSON_SERIALIZE_ENUM(VelodyneModelType, {
                                                    {VelodyneModelType::VLP16, "VLP16"},
                                                    {VelodyneModelType::INVALID, nullptr},
                                                });

// A driver for the Velodyne VLP16 Lidar.
class VelodyneLidar : public alice::Codelet {
 public:
  void start() override;
  void tick() override;
  void stop() override;

  // A range scan slice published by the Lidar
  ISAAC_PROTO_TX(RangeScanProto, scan);

  // The IP address of the Lidar device
  ISAAC_PARAM(std::string, ip, "192.168.2.201");
  // The port at which the Lidar device publishes data.
  ISAAC_PARAM(int, port, 2368);
  // The type of the Lidar (currently only VLP16 is supported).
  ISAAC_PARAM(VelodyneModelType, type, VelodyneModelType::VLP16);

 private:
  // Process a packet of data coming on the wire with VLP16Format
  void processDataBlockVPL16(const VelodyneRawDataBlock& raw_block, TensorView2ui16 ranges,
                             TensorView2ub intensities, int offset);

  // Configures some member variables according to the lidar type
  void initLaser(VelodyneModelType model_type);

  std::unique_ptr<Socket> socket_;
  std::optional<std::vector<byte>> previous_packet_;
  std::vector<byte> raw_packet_;

  // Model specific parameters
  VelodyneLidarParameters parameters_;
};

}  // namespace velodyne_lidar
}  // namespace isaac

ISAAC_ALICE_REGISTER_CODELET(isaac::velodyne_lidar::VelodyneLidar);
