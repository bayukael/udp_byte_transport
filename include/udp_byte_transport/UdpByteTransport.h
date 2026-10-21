#pragma once

#include <byte_transport/Transport.h>

#include <memory>

namespace pendarlab::lib::comm
{
  /**
   * @brief A byte_transport::Transport backed by a connected UDP socket.
   *
   * One instance represents exactly one remote peer: the socket is bound to a
   * local port, connected to the peer with connect(), and every read/write maps
   * to one datagram (recv/send). Blocking I/O with socket timeouts.
   */
  class UdpByteTransport : public pendarlab::lib::comm::byte_transport::Transport
  {
  public:
    /// Takes ownership of an already bound + connected UDP socket descriptor.
    UdpByteTransport(int socket_fd);
    ~UdpByteTransport();

    virtual int read(unsigned char* buf, unsigned int buf_size) override;
    virtual int write(const unsigned char* buf, unsigned int length) override;

  private:
    struct UdpByteTransportImpl;
    std::unique_ptr<UdpByteTransportImpl> p_impl_;
  };
} // namespace pendarlab::lib::comm
