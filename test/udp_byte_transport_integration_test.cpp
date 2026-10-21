#include <byte_transport/ConfigParseResult.h>
#include <byte_transport/Registry.h>
#include <byte_transport/RegistryUserAccess.h>
#include <byte_transport/Transport.h>
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <udp_byte_transport/UdpByteTransportDefinition.h>
#include <unordered_map>
#include <vector>

using Registry = pendarlab::lib::comm::byte_transport::Registry;
using RegistryUserAccess = pendarlab::lib::comm::byte_transport::RegistryUserAccess;
using Transport = pendarlab::lib::comm::byte_transport::Transport;
using UdpByteTransportDefinition = pendarlab::lib::comm::UdpByteTransportDefinition;

// Two fixed loopback ports used for the two-instance round-trip.
constexpr int kPortA = 32410;
constexpr int kPortB = 32411;
// A separate port used for the timeout case in the integration test.
constexpr int kPortC = 32412;

namespace
{
  // Reads at most one datagram (or times out), returning the bytes read.
  std::vector<uint8_t> readOne(Transport& transport, unsigned int timeout_in_ms)
  {
    std::vector<uint8_t> data;
    uint8_t buf[1024];
    const auto start = std::chrono::steady_clock::now();
    while (data.empty()) {
      const auto now = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > timeout_in_ms) {
        break;
      }
      int bytes_read = transport.read(buf, sizeof(buf));
      if (bytes_read > 0) {
        data.assign(buf, buf + bytes_read);
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }
    return data;
  }

  class UdpByteTransportIntegrationTest : public testing::Test
  {
  protected:
    static constexpr const char* kType = "UdpByteTransport";

    UdpByteTransportIntegrationTest() : registry_user_(test_registry_.createUser())
    {
      test_registry_.addTransportDefinition(kType, transport_def_);
    }

    void SetUp() override {}

    std::unique_ptr<Transport> createTransport(const std::unordered_map<std::string, std::string>& cfg) const
    {
      auto parse_result = (*registry_user_)[kType]->parseConfig(cfg);
      if (!parse_result.ok()) {
        return nullptr;
      }
      return (*registry_user_)[kType]->create(*parse_result.config);
    }

    Registry test_registry_;
    std::unique_ptr<RegistryUserAccess> registry_user_;
    UdpByteTransportDefinition transport_def_;
  };

  // Two instances bound to local ports, each connected to the other as the peer.
  // Writing on A must be receivable on B, and they must construct successfully.
  TEST_F(UdpByteTransportIntegrationTest, TwoInstancesShouldRoundTrip)
  {
    std::unordered_map<std::string, std::string> cfg_a = {
      { "local_port", std::to_string(kPortA) },
      { "remote_address", "127.0.0.1" },
      { "remote_port", std::to_string(kPortB) }
    };
    std::unordered_map<std::string, std::string> cfg_b = {
      { "local_port", std::to_string(kPortB) },
      { "remote_address", "127.0.0.1" },
      { "remote_port", std::to_string(kPortA) }
    };

    auto transport_a = createTransport(cfg_a);
    auto transport_b = createTransport(cfg_b);
    ASSERT_NE(transport_a, nullptr);
    ASSERT_NE(transport_b, nullptr);

    const std::vector<uint8_t> payload = { 'h', 'e', 'l', 'l', 'o', ' ', 'u', 'd', 'p' };
    int bytes_written = transport_a->write(payload.data(), payload.size());
    EXPECT_EQ(bytes_written, static_cast<int>(payload.size()));

    auto received = readOne(*transport_b, 2000);
    EXPECT_EQ(received.size(), payload.size());
    ASSERT_EQ(received.size(), payload.size());
    for (size_t i = 0; i < payload.size(); i++) {
      EXPECT_EQ(payload[i], received[i]);
    }
  }

  // A bound instance with a short timeout returns 0 when no datagram arrives.
  TEST_F(UdpByteTransportIntegrationTest, ReadShouldTimeoutWithNoDatagram)
  {
    std::unordered_map<std::string, std::string> cfg = {
      { "local_port", std::to_string(kPortC) },
      { "remote_address", "127.0.0.1" },
      { "remote_port", std::to_string(kPortA) },
      { "read_timeout_ms", "30" }
    };
    auto transport = createTransport(cfg);
    ASSERT_NE(transport, nullptr);

    unsigned char buf[16];
    int n = transport->read(buf, sizeof(buf));
    EXPECT_EQ(n, 0);
  }

} // namespace

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  int test_result = RUN_ALL_TESTS();
  return test_result;
}
