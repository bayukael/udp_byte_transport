# udp_byte_transport

A C++17 library that implements a UDP transport as a `byte_transport` plugin, built with CMake and installed as a consumable CMake package.

## What it does

`udp_byte_transport` provides a concrete `Transport` backed by a UDP socket. It is implemented as a plugin for the `byte_transport` registry: it subclasses `byte_transport::TransportDefinition` so that it can be registered under a type name, looked up by a `RegistryUserAccess`, and instantiated from a set of key/value config strings.

The library is split into three pieces:

- `UdpByteTransport` — a `byte_transport::Transport` that owns a connected UDP socket and forwards `read()`/`write()` to it as single datagrams. Uses the PIMPL idiom.
- `UdpByteTransportConfig` — a `byte_transport::Config` subclass carrying the fields needed to set up the socket (local port, remote address:port, socket I/O timeouts).
- `UdpByteTransportDefinition` — a `byte_transport::TransportDefinition` that parses config and creates the transport. It exports an `extern "C"` `getUdpByteTransportDefinition()` entry point so it can be discovered by a dynamic lib loader.

## Architecture

Public headers live in `include/udp_byte_transport/` and use the namespace `pendarlab::lib::comm`.

- `UdpByteTransport` — abstract read/write interface for a transport.
- `UdpByteTransportConfig` — holds UDP socket settings; derives from `byte_transport::Config`.
- `UdpByteTransportDefinition` — plugin interface: `parseConfig()` + `create()`.

`UdpByteTransport` uses the PIMPL idiom: the implementation (`UdpByteTransportImpl`) owns the socket descriptor, so the socket internals are hidden from the public header.

### One instance = one remote peer

An instance represents exactly one remote peer (construct = connect, analogous to a serial transport pointing at one device). `UdpByteTransportDefinition::create()`:

1. creates a single UDP socket,
2. **binds** it to the configured local port (to receive datagrams),
3. **connects** it to the configured remote address:port with `connect()`, so the kernel filters received datagrams to that peer only and `send()` targets it,
4. applies blocking-style socket timeouts (`SO_RCVTIMEO` / `SO_SNDTIMEO`) from the parsed config.

Each `read()` maps to one `recv()` and each `write()` maps to one `send()` (one datagram).

### Config parsing

`UdpByteTransportDefinition::parseConfig()` reads a `std::unordered_map<std::string, std::string>` and supports these keys:

| Key                | Required | Description                                        |
| ------------------ | :------: | -------------------------------------------------- |
| `local_port`       | yes      | Local port to bind for receiving datagrams (0-65535). |
| `remote_address`   | yes      | Remote address to connect to — numeric IPv4 or hostname. |
| `remote_port`      | yes      | Remote port to connect to (0-65535).               |
| `read_timeout_ms`  | no       | `SO_RCVTIMEO` in ms (default 0 = block indefinitely). |
| `write_timeout_ms` | no       | `SO_SNDTIMEO` in ms (default 0 = block indefinitely). |

If a required key is missing, a port is out of range, or a timeout is invalid, it adds a message to the result and `ok()` returns `false`. On success it returns a `UdpByteTransportConfig` populated with the parsed values.

### Read/write semantics

- `read()` / `write()` map to one datagram each (`recv` / `send`).
- A read/write **error** returns `-1`.
- A read **timeout** with no datagram available returns `0` (`EAGAIN`/`EWOULDBLOCK`).

## Quick start (build)

Requires CMake >= 3.13, the installed `PendarlabByteTransport` package, and GoogleTest (`find_package(GTest CONFIG REQUIRED)`). Point `CMAKE_PREFIX_PATH` at the directory containing the installed pendarlab packages.

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/installed/packages
cmake --build build
./build/udp_byte_transport_config_test       # config parsing
./build/udp_byte_transport_test              # create() behavior + read timeout
./build/udp_byte_transport_integration_test  # real round-trip between two instances
```

Run a single test:

```bash
./build/udp_byte_transport_config_test --gtest_filter='*GoodConfigShouldParseCorrectly*'
```

There are no lint/format targets; building and running the test executables is the only verification.

## Using the library from another project

The library installs as the CMake package `PendarlabUdpByteTransport` (exported target `pendarlab::UdpByteTransport`):

```cmake
find_package(PendarlabUdpByteTransport REQUIRED)
target_link_libraries(my_app PRIVATE pendarlab::UdpByteTransport)
```

## Versioning

The library version is declared once in `CMakeLists.txt` (`project(... VERSION x.y.z)`). At configure time `configure_file()` fills in `include/udp_byte_transport/Version.h.in` to produce `Version.h`, which exposes the version both as preprocessor macros and as `constexpr` constants in `namespace pendarlab::lib::comm`:

```cpp
#include <udp_byte_transport/Version.h>

using namespace pendarlab::lib::comm;
// versionMajor, versionMinor, versionPatch, version (const char*)
```

The shared library also carries the version via CMake's `VERSION`/`SOVERSION` properties (`libpendarlab-udp_byte_transport.so -> .so.1 -> .so.1.0.0`), and `write_basic_package_version_file()` generates a `PendarlabUdpByteTransportConfigVersion.cmake` so `find_package()` can enforce `SameMajorVersion` compatibility.

The version is not appended to the install location; to keep multiple versions side by side, install each into its own prefix (e.g. `<prefix>/1.0.0/`) and point `CMAKE_PREFIX_PATH` at the one you want.

Register the transport with a `byte_transport::Registry` under a type name, then create it from config:

```cpp
#include <byte_transport/Registry.h>
#include <memory>
#include <udp_byte_transport/UdpByteTransportDefinition.h>

using namespace pendarlab::lib::comm;
using byte_transport::Registry;

Registry registry;
UdpByteTransportDefinition def;
registry.addTransportDefinition("UdpByteTransport", def);

auto user = registry.createUser();
const auto* transport_def = (*user)["UdpByteTransport"]; // nullptr if absent

std::unordered_map<std::string, std::string> cfg = {
  { "local_port", "14550" },
  { "remote_address", "192.168.1.10" },
  { "remote_port", "14550" },
  { "read_timeout_ms", "100" },
};
auto parsed = transport_def->parseConfig(cfg);
if (parsed.ok()) {
  auto transport = transport_def->create(*parsed.config); // nullptr if setup fails
  // transport->read(buf, size); transport->write(buf, size);
}
```

## Adding a transport plugin

1. Subclass `byte_transport::Transport` for the concrete transport.
2. Subclass `byte_transport::Config` to carry transport-specific settings.
3. Subclass `byte_transport::TransportDefinition`; implement `parseConfig()` and `create()`.
4. Export an `extern "C"` getter returning the definition (see `getUdpByteTransportDefinition()`).

## Tests

- `test/udp_byte_transport_config_test.cpp` — `parseConfig()` accepts a valid config, parses optional timeouts, and rejects a missing `local_port`, missing `remote_address`, missing `remote_port`, an invalid `local_port`, or an out-of-range port.
- `test/udp_byte_transport_test.cpp` — `create()` fails when the same local port is already bound, and a read with no datagram and a short timeout returns `0`.
- `test/udp_byte_transport_integration_test.cpp` — two instances bound to local ports and connected to each other as peers perform a real send/receive round-trip, and a solo instance times out with no datagram.
