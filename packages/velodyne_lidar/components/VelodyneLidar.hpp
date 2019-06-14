/*
Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/alice/alice_codelet.hpp"
#include "engine/core/byte.hpp"
#include "engine/core/optional.hpp"
#include "messages/messages.hpp"
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
  void processDataBlockVPL16(const VelodyneRawDataBlock& raw_block,
                             capnp::List<RangeScanProto::Ray>::Builder& rays, int offset);

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
