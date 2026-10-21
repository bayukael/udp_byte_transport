#include "udp_byte_transport/UdpByteTransportDefinition.h"

#include "udp_byte_transport/UdpByteTransport.h"
#include "udp_byte_transport/UdpByteTransportConfig.h"

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

namespace pendarlab::lib::comm
{
  using Config = pendarlab::lib::comm::byte_transport::Config;
  using ConfigParseResult = pendarlab::lib::comm::byte_transport::ConfigParseResult;
  using Transport = pendarlab::lib::comm::byte_transport::Transport;

  namespace
  {
    // Parses a decimal integer string. Returns -1 (and leaves out unset) on failure.
    int parseInt(const std::string& s, int& out)
    {
      try {
        size_t pos = 0;
        int v = std::stoi(s, &pos);
        if (pos != s.size()) {
          return -1;
        }
        out = v;
        return 0;
      } catch (const std::exception&) {
        return -1;
      }
    }

    // Parses a port string, requiring it to be in the range [0, 65535].
    bool parsePort(const std::string& s, std::uint16_t& out)
    {
      int v = 0;
      if (parseInt(s, v) != 0 || v < 0 || v > 65535) {
        return false;
      }
      out = static_cast<std::uint16_t>(v);
      return true;
    }
  } // namespace

  ConfigParseResult UdpByteTransportDefinition::parseConfig(const std::unordered_map<std::string, std::string>& cfg) const
  {
    ConfigParseResult result;
    auto udp_config = std::make_unique<UdpByteTransportConfig>();

    auto it_local_port = cfg.find("local_port");
    if (it_local_port == cfg.end()) {
      result.messages.push_back("[UdpByteTransport] : a required entry is missing: local_port");
    } else if (!parsePort(it_local_port->second, udp_config->local_port)) {
      result.messages.push_back("[UdpByteTransport] : bad value for key 'local_port': " + it_local_port->second);
    }

    auto it_remote_address = cfg.find("remote_address");
    if (it_remote_address == cfg.end()) {
      result.messages.push_back("[UdpByteTransport] : a required entry is missing: remote_address");
    } else {
      udp_config->remote_address = it_remote_address->second;
    }

    auto it_remote_port = cfg.find("remote_port");
    if (it_remote_port == cfg.end()) {
      result.messages.push_back("[UdpByteTransport] : a required entry is missing: remote_port");
    } else if (!parsePort(it_remote_port->second, udp_config->remote_port)) {
      result.messages.push_back("[UdpByteTransport] : bad value for key 'remote_port': " + it_remote_port->second);
    }

    // Optional timeouts, default 0 (block indefinitely).
    auto it_read_timeout = cfg.find("read_timeout_ms");
    if (it_read_timeout != cfg.end()) {
      int v = 0;
      if (parseInt(it_read_timeout->second, v) != 0 || v < 0) {
        result.messages.push_back("[UdpByteTransport] : bad value for key 'read_timeout_ms': " + it_read_timeout->second);
      } else {
        udp_config->read_timeout_ms = v;
      }
    }

    auto it_write_timeout = cfg.find("write_timeout_ms");
    if (it_write_timeout != cfg.end()) {
      int v = 0;
      if (parseInt(it_write_timeout->second, v) != 0 || v < 0) {
        result.messages.push_back("[UdpByteTransport] : bad value for key 'write_timeout_ms': " + it_write_timeout->second);
      } else {
        udp_config->write_timeout_ms = v;
      }
    }

    if (result.messages.empty()) {
      result.config = std::move(udp_config);
    }
    return result;
  }

  std::unique_ptr<Transport> UdpByteTransportDefinition::create(const Config& cfg) const
  {
    const auto& udp_cfg = dynamic_cast<const UdpByteTransportConfig&>(cfg);

    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
      return nullptr;
    }

    // Bind the local port to receive datagrams.
    struct sockaddr_in local_addr;
    std::memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(udp_cfg.local_port);
    if (bind(sock_fd, reinterpret_cast<struct sockaddr*>(&local_addr), sizeof(local_addr)) != 0) {
      close(sock_fd);
      return nullptr;
    }

    // Resolve the remote address (numeric IPv4 or hostname).
    struct sockaddr_in remote_addr;
    std::memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, udp_cfg.remote_address.c_str(), &remote_addr.sin_addr) != 1) {
      struct hostent* host = gethostbyname(udp_cfg.remote_address.c_str());
      if (!host || host->h_addrtype != AF_INET) {
        close(sock_fd);
        return nullptr;
      }
      std::memcpy(&remote_addr.sin_addr, host->h_addr, host->h_length);
    }
    remote_addr.sin_port = htons(udp_cfg.remote_port);

    // connect() filters received datagrams to this peer only and lets send() go to it.
    if (connect(sock_fd, reinterpret_cast<struct sockaddr*>(&remote_addr), sizeof(remote_addr)) != 0) {
      close(sock_fd);
      return nullptr;
    }

    // Configure blocking I/O with socket timeouts.
    struct timeval rcv_timeout;
    rcv_timeout.tv_sec = udp_cfg.read_timeout_ms / 1000;
    rcv_timeout.tv_usec = (udp_cfg.read_timeout_ms % 1000) * 1000;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &rcv_timeout, sizeof(rcv_timeout));

    struct timeval snd_timeout;
    snd_timeout.tv_sec = udp_cfg.write_timeout_ms / 1000;
    snd_timeout.tv_usec = (udp_cfg.write_timeout_ms % 1000) * 1000;
    setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &snd_timeout, sizeof(snd_timeout));

    return std::make_unique<UdpByteTransport>(sock_fd);
  }

  extern "C" const UdpByteTransportDefinition* getUdpByteTransportDefinition()
  {
    static UdpByteTransportDefinition udp_byte_transport_def;
    return &udp_byte_transport_def;
  }
} // namespace pendarlab::lib::comm
