#pragma once

#include <byte_transport/ConfigParseResult.h>
#include <byte_transport/Transport.h>
#include <byte_transport/TransportDefinition.h>

#include <memory>

namespace pendarlab::lib::comm
{
  /**
   * @brief TransportDefinition for the UDP byte transport.
   *
   * Parses a key/value configuration into a UdpByteTransportConfig and
   * creates UdpByteTransport instances from it.
   */
  class UdpByteTransportDefinition : public pendarlab::lib::comm::byte_transport::TransportDefinition
  {
    using Config = pendarlab::lib::comm::byte_transport::Config;
    using ConfigParseResult = pendarlab::lib::comm::byte_transport::ConfigParseResult;
    using Transport = pendarlab::lib::comm::byte_transport::Transport;

  public:
    virtual ConfigParseResult parseConfig(const std::unordered_map<std::string, std::string>& cfg) const override;
    virtual std::unique_ptr<Transport> create(const Config& cfg) const override;
  };
} // namespace pendarlab::lib::comm
