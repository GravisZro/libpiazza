# libpiazza

`libpiazza` is a C++20 library to interface with the Piazza website. It provides
typed data structures for Piazza's internal HTTP protocol and a Qt-based client
for authenticating and issuing JSON-RPC-style requests.

This is an unofficial client. It is not affiliated with or endorsed by Piazza.

## Requirements

- CMake 3.20+
- A C++20 compiler
- Qt 6 (`Core` and `Network`)
- The `libqjsonrpc` submodule

## Build

Clone with submodules and build:

```sh
git clone --recurse-submodules git@github.com:GravisZro/libpiazza.git
cd libpiazza
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

If submodules are missing:

```sh
git submodule update --init --recursive
```

The build produces `libpiazza.a` and the `piazza_test_client` smoke-test
executable. There is no install target; use `add_subdirectory()` or link against
the `piazza` target.

Standard CMake options apply, such as `CMAKE_BUILD_TYPE`, `CMAKE_PREFIX_PATH`,
and `Qt6_DIR`. The C++ standard is fixed at 20.

## Usage

```cpp
#include "protocol.h"
#include "rpc.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonObject>

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);

  piazza::RpcClient client;
  client.user_login("you@example.com", "password");

  const QJsonObject status = client.get_user_status().toObject();
  const QJsonArray networks = status.value("networks").toArray();
  const QString nid = networks.at(0).toObject().value("id").toString();
  client.set_network_id(nid);

  const QJsonObject feed = client.get_my_feed(3, 0, "updated").toObject();
  const QJsonValue cid = feed.value("feed").toArray().at(0).toObject().value("id");
  const piazza::Post post = client.content_get(cid);
}
```

The client requires a `QCoreApplication` and makes synchronous requests.

## Options

Authentication:

- `user_login(email, password)` logs in to the live site.
- `demo_login(auth)` or `demo_login({}, url)` uses the demo flow.
- `get_cookies()` and `set_cookies()` export/restore sessions.

Requests:

- `RpcClient(QString network_id)` sets an optional default network.
- `set_network_id()` selects the network used by later calls.
- `request(method, params, api_type, nid_key, include_nid)` calls any API
  method; `api_type` is `ApiType::logic` (default) or `ApiType::main`.

Errors are reported as `piazza::AuthenticationError`, `piazza::RequestError`,
`piazza::NotAuthenticatedError`, and `piazza::NoNetworkIdError`.

## Test client

```sh
./build/piazza_test_client <email> <password>
```

The optional `PIAZZA_NID` environment variable selects a network; otherwise the
first network from `user.status` is used.

## License

`libpiazza` is licensed under `AGPL-3.0-only`. See [LICENSE](LICENSE).
