#pragma once

#include <byte_transport/Config.h>

#include <cstdint>
#include <string>

namespace pendarlab::lib::comm
{
  /**
   * @brief Configuration for a UDP byte transport.
   *
   * Holds the settings needed to set up a single connected UDP socket:
   * which local port to bind (to receive datagrams) and which remote
   * address:port to connect to (to send datagrams), plus the socket
   * I/O timeouts.
   */
  class UdpByteTransportConfig : public pendarlab::lib::comm::byte_transport::Config
  {
  public:
    UdpByteTransportConfig() = default;

    /// Local port to bind for receiving datagrams.
    std::uint16_t local_port = 0;
    /// Remote address (IP or hostname) to connect to for sending datagrams.
    std::string remote_address;
    /// Remote port to connect to for sending datagrams.
    std::uint16_t remote_port = 0;
    /// SO_RCVTIMEO in milliseconds (0 = block indefinitely).
    int read_timeout_ms = 0;
    /// SO_SNDTIMEO in milliseconds (0 = block indefinitely).
    int write_timeout_ms = 0;
  };
} // namespace pendarlab::lib::comm
