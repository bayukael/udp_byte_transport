#include <byte_transport/ConfigParseResult.h>
#include <byte_transport/Registry.h>
#include <byte_transport/RegistryUserAccess.h>
#include <byte_transport/Transport.h>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <udp_byte_transport/UdpByteTransportDefinition.h>
#include <unordered_map>

using Registry = pendarlab::lib::comm::byte_transport::Registry;
using RegistryUserAccess = pendarlab::lib::comm::byte_transport::RegistryUserAccess;
using Transport = pendarlab::lib::comm::byte_transport::Transport;
using UdpByteTransportDefinition = pendarlab::lib::comm::UdpByteTransportDefinition;

namespace
{
  class UdpByteTransportTest : public testing::Test
  {
  protected:
    static constexpr const char* kType = "UdpByteTransport";

    UdpByteTransportTest() : registry_user_(test_registry_.createUser())
    {
      test_registry_.addTransportDefinition(kType, transport_def_);
    }

    void SetUp() override {}

    std::unique_ptr<Transport> createWith(const std::unordered_map<std::string, std::string>& cfg) const
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

  // Binding the same local port twice on the host should fail (port is taken).
  TEST_F(UdpByteTransportTest, BindingSameLocalPortTwiceShouldFailTheSecond)
  {
    std::unordered_map<std::string, std::string> cfg = {
      { "local_port", "32333" },
      { "remote_address", "127.0.0.1" },
      { "remote_port", "32334" }
    };
    auto first = createWith(cfg);
    EXPECT_NE(first, nullptr);
    auto second = createWith(cfg);
    EXPECT_EQ(second, nullptr);
  }

  // A read with no datagram available and a short timeout should return 0.
  TEST_F(UdpByteTransportTest, ReadWithNoDatagramAndTimeoutShouldReturnZero)
  {
    std::unordered_map<std::string, std::string> cfg = {
      { "local_port", "32335" },
      { "remote_address", "127.0.0.1" },
      { "remote_port", "32336" },
      { "read_timeout_ms", "30" }
    };
    auto transport = createWith(cfg);
    ASSERT_NE(transport, nullptr);

    unsigned char buf[16];
    auto start = std::chrono::steady_clock::now();
    int n = transport->read(buf, sizeof(buf));
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    EXPECT_EQ(n, 0);
    EXPECT_GE(elapsed, 20); // it actually waited for the timeout
  }

} // namespace

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  int test_result = RUN_ALL_TESTS();
  return test_result;
}
