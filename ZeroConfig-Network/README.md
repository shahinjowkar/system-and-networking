# Zero Configuration Service (ZCS)

A lightweight zero-configuration networking library for service discovery in C. Enables automatic service discovery over local networks using UDP multicast, eliminating manual configuration or centralized servers.

## Features

- **Zero Configuration**: Services automatically discover each other
- **UDP Multicast**: Efficient one-to-many communication (224.1.1.1:5000)
- **Dual Node Types**: APP nodes (consumers) and SERVICE nodes (providers)
- **Attribute-Based Discovery**: Services advertise with key-value attributes
- **Automatic Heartbeats**: Services send periodic heartbeats (every 3 seconds)
- **Graceful Leaving & Rejoining**: Nodes are automatically removed from registries when heartbeats timeout, and re-added when they return
- **Thread-Safe**: Mutex-protected shared registry state
- **Local Registry**: Timestamp-tracked registry of discovered services

## Architecture

Multicast-based architecture:
- **SERVICE nodes** advertise presence with attributes (e.g., type, location, make)
- **APP nodes** send DISCOVERY messages to find available services
- All nodes listen on the same multicast group for automatic discovery
- Services send heartbeat messages every 3 seconds
- **Heartbeat Timeout**: If a service fails to send heartbeats, APP nodes automatically mark it unavailable and remove it from their local registry
- **Automatic Rejoining**: When a service resumes sending heartbeats after being offline, it is automatically re-discovered and re-added to registries

## Building

```bash
gcc -o zcs_lib zcs.c multicast.c -lpthread
```

## Usage

### Service Node

```c
#include "zcs.h"

int main() {
    zcs_init(ZCS_SERVICE_TYPE);
    zcs_attribute_t attribs[] = {
        { .attr_name = "type", .value = "speaker" },
        { .attr_name = "location", .value = "kitchen" }
    };
    zcs_start("speaker-X", attribs, 2);
    sleep(3600);
    zcs_shutdown();
}
```

### App Node

```c
#include "zcs.h"

int main() {
    zcs_init(ZCS_APP_TYPE);
    sleep(2);  // Wait for discovery
    char *names[10];
    int count = zcs_query("type", "speaker", names, 10);
}
```

## API Reference

- `zcs_init(int type)` - Initialize as APP or SERVICE node
- `zcs_start(char *name, zcs_attribute_t attr[], int num)` - Start service with attributes
- `zcs_post_ad(char *ad_name, char *ad_value)` - Post advertisement (planned)
- `zcs_query(char *attr_name, char *attr_value, ...)` - Query services (planned)
- `zcs_get_attribs(char *name, ...)` - Get service attributes (planned)
- `zcs_listen_ad(char *name, zcs_cb_f callback)` - Listen for advertisements (planned)
- `zcs_shutdown()` - Gracefully shutdown ZCS

## Dependencies

- POSIX threads (`pthread`)
- Standard C library
- Network sockets (Linux/macOS)
