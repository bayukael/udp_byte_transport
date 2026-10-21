#include <byte_transport/ConfigParseResult.h>
#include <byte_transport/Registry.h>
#include <byte_transport/RegistryUserAccess.h>
#include <gtest/gtest.h>
#include <string>
#include <udp_byte_transport/UdpByteTransportConfig.h>
#include <udp_byte_transport/UdpByteTransportDefinition.h>
#include <unordered_map>

using Registry = pendarlab::lib::comm::byte_transport::Registry;
using RegistryUserAccess = pendarlab::lib::comm::byte_transport::RegistryUserAccess;
using ConfigParseResult = pendarlab::lib::comm::byte_transport::ConfigParseResult;
using UdpByteTransportDefinition = pendarlab::lib::comm::UdpByteTransportDefinition;

namespace
{
  class UdpByteTransportConfigTest : public testing::Test
  {
  protected:
    static constexpr const char* kType = "UdpByteTransport";

    UdpByteTransportConfigTest() :
        registry_user_(test_registry_.createUser())
    {
      test_registry_.addTransportDefinition(kType, transport_def_);
    }

    void SetUp() override {}

    ConfigParseResult parseConfig(const std::unordered_map<std::string, std::string>& cfg) const
    {
      return (*registry_user_)[kType]->parseConfig(cfg);
    }

    Registry test_registry_;
    std::unique_ptr<RegistryUserAccess> registry_user_;
    UdpByteTransportDefinition transport_def_;

    std::unordered_map<std::string, std::string> valid_config_ = {
      { "local_port", "32050" },
      { "remote_address", "127.0.0.1" },
      { "remote_port", "32051" }
    };
  };

  TEST_F(UdpByteTransportConfigTest, GoodConfigShouldParseCorrectly)
  {
    auto result = parseConfig(valid_config_);
    EXPECT_EQ(result.ok(), true);
    ASSERT_NE(result.config, nullptr);

    const auto& udp_cfg = dynamic_cast<const pendarlab::lib::comm::UdpByteTransportConfig&>(*result.config);
    EXPECT_EQ(udp_cfg.local_port, 32050);
    EXPECT_EQ(udp_cfg.remote_address, "127.0.0.1");
    EXPECT_EQ(udp_cfg.remote_port, 32051);
    EXPECT_EQ(udp_cfg.read_timeout_ms, 0);
    EXPECT_EQ(udp_cfg.write_timeout_ms, 0);
  }

  TEST_F(UdpByteTransportConfigTest, TimeoutsShouldParseWhenProvided)
  {
    std::unordered_map<std::string, std::string> cfg = valid_config_;
    cfg["read_timeout_ms"] = "50";
    cfg["write_timeout_ms"] = "100";

    auto result = parseConfig(cfg);
    EXPECT_EQ(result.ok(), true);
    ASSERT_NE(result.config, nullptr);

    const auto& udp_cfg = dynamic_cast<const pendarlab::lib::comm::UdpByteTransportConfig&>(*result.config);
    EXPECT_EQ(udp_cfg.read_timeout_ms, 50);
    EXPECT_EQ(udp_cfg.write_timeout_ms, 100);
  }

  TEST_F(UdpByteTransportConfigTest, MissingLocalPortShouldFail)
  {
    std::unordered_map<std::string, std::string> cfg = valid_config_;
    cfg.erase("local_port");

    auto result = parseConfig(cfg);
    EXPECT_EQ(result.ok(), false);
    bool found = false;
    for (const auto& msg : result.messages) {
      if (msg.find("local_port") != std::string::npos) {
        found = true;
        break;
      }
    }
    EXPECT_EQ(found, true);
  }

  TEST_F(UdpByteTransportConfigTest, MissingRemoteAddressShouldFail)
  {
    std::unordered_map<std::string, std::string> cfg = valid_config_;
    cfg.erase("remote_address");

    auto result = parseConfig(cfg);
    EXPECT_EQ(result.ok(), false);
    bool found = false;
    for (const auto& msg : result.messages) {
      if (msg.find("remote_address") != std::string::npos) {
        found = true;
        break;
      }
    }
    EXPECT_EQ(found, true);
  }

  TEST_F(UdpByteTransportConfigTest, MissingRemotePortShouldFail)
  {
    std::unordered_map<std::string, std::string> cfg = valid_config_;
    cfg.erase("remote_port");

    auto result = parseConfig(cfg);
    EXPECT_EQ(result.ok(), false);
    bool found = false;
    for (const auto& msg : result.messages) {
      if (msg.find("remote_port") != std::string::npos) {
        found = true;
        break;
      }
    }
    EXPECT_EQ(found, true);
  }

  TEST_F(UdpByteTransportConfigTest, InvalidLocalPortShouldFail)
  {
    std::unordered_map<std::string, std::string> cfg = valid_config_;
    cfg["local_port"] = "not_a_number";

    auto result = parseConfig(cfg);
    EXPECT_EQ(result.ok(), false);
    bool found = false;
    for (const auto& msg : result.messages) {
      if (msg.find("local_port") != std::string::npos) {
        found = true;
        break;
      }
    }
    EXPECT_EQ(found, true);
  }

  TEST_F(UdpByteTransportConfigTest, OutOfRangePortShouldFail)
  {
    std::unordered_map<std::string, std::string> cfg = valid_config_;
    cfg["remote_port"] = "70000";

    auto result = parseConfig(cfg);
    EXPECT_EQ(result.ok(), false);
  }

} // namespace

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  int test_result = RUN_ALL_TESTS();
  return test_result;
}
