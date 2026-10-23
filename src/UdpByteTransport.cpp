#include "udp_byte_transport/UdpByteTransport.h"

#include <cerrno>
#include <sys/socket.h>

namespace pendarlab::lib::comm
{
  struct UdpByteTransport::UdpByteTransportImpl {
    UdpByteTransportImpl(int fd) : socket_fd_(fd) {}
    int socket_fd_;
  };

  UdpByteTransport::UdpByteTransport(int socket_fd) :
      d(std::make_unique<UdpByteTransport::UdpByteTransportImpl>(socket_fd))
  {
  }

  UdpByteTransport::~UdpByteTransport() = default;

  int UdpByteTransport::read(unsigned char* buf, unsigned int buf_size)
  {
    if (d->socket_fd_ < 0) { // no valid socket
      return -1;
    }
    ssize_t n = recv(d->socket_fd_, buf, buf_size, 0);
    if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0; // read timeout with no datagram available
      }
      return -1;
    }
    return static_cast<int>(n);
  }

  int UdpByteTransport::write(const unsigned char* buf, unsigned int length)
  {
    if (d->socket_fd_ < 0) { // no valid socket
      return -1;
    }
    ssize_t n = send(d->socket_fd_, buf, length, 0);
    if (n < 0) {
      return -1;
    }
    return static_cast<int>(n);
  }
} // namespace pendarlab::lib::comm
