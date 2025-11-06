# SOSPF Link State Routing Protocol

A distributed Java implementation of the SOSPF (Simplified Open Shortest Path First) routing protocol that simulates a network of routers exchanging Link State Advertisements (LSAs) to build complete network topology and compute optimal routes using Dijkstra's algorithm.

## Architecture

The implementation consists of multiple routers running concurrently, each maintaining:
- **Link State Database (LSD)**: Stores topology information for all routers in the network
- **Socket-based Communication**: Multi-threaded server handles incoming connections; clients establish outgoing links
- **SOSPF Protocol Messages**: HELLO/HELLO-ACK/HELLO-TWO-WAY for neighbor discovery, LSAUPDATE for topology flooding, ATTACH/DISCONNECT for link management

## Key Features

- **Dijkstra's Algorithm**: Implements shortest path calculation using priority queue with parent tracking
- **LSA Flooding**: Routers broadcast updated link state information to all TWO-WAY neighbors
- **Neighbor Discovery**: THREE-WAY handshake establishes bidirectional connections
- **Thread-Safe Operations**: Synchronized access to shared data structures for concurrent message handling
- **Sequence Numbers**: LSA sequence numbers prevent duplicate processing and ensure newer updates propagate
- **Multi-Port Support**: Each router supports up to 4 ports for network connections

## Building

Build the project using Maven:

```bash
mvn clean package
```

The executable JAR with dependencies will be generated at `target/COMP535-1.0-SNAPSHOT-jar-with-dependencies.jar`.

## Usage

Launch each router in a separate terminal with its configuration file:

```bash
java -jar target/COMP535-1.0-SNAPSHOT-jar-with-dependencies.jar conf/router1.conf
```

## Configuration

Each router requires a configuration file specifying:
- `socs.network.router.ip`: Simulated IP address (e.g., "192.168.1.1")
- `socs.network.router.port`: Process port number for socket communication (e.g., 3000)
- `socs.network.router.processIP`: Host IP address, typically "127.0.0.1" for local testing

Example (`conf/router1.conf`):
```
socs.network.router.ip="192.168.1.1"
socs.network.router.port=3000
socs.network.router.processIP="127.0.0.1"
```

## Terminal Commands

| Command | Description |
|---------|-------------|
| `attach <processIP> <processPort> <simulatedIP>` | Establish a physical link to a remote router |
| `connect <processIP> <processPort> <simulatedIP>` | Attach link and immediately broadcast LSA update |
| `start` | Broadcast HELLO messages to establish TWO-WAY connections |
| `detect <destinationIP>` | Compute and display shortest path to destination router |
| `disconnect <port>` | Remove link on specified port and update topology |
| `neighbors` | List all connected neighbors |
| `printLSD` | Display the complete Link State Database |
| `quit` | Gracefully disconnect all neighbors and exit |

## Protocol Details

**Message Types:** HELLO (0), HELLO-ACK (1), HELLO-TWO-WAY (2), LSAUPDATE (3), ATTACH (4), ATTACH-ACK (5), ATTACH-REJECT (6), DISCONNECT (7)

**Router States:** `IDLE` → `INIT` → `TWO_WAY` (bidirectional connection established, LSA exchange enabled)

## Project Structure

```
src/main/java/socs/network/
├── Main.java                          # Entry point
├── node/
│   ├── Router.java                    # Main router logic with terminal interface
│   ├── LinkStateDatabase.java         # Topology storage and Dijkstra's algorithm
│   ├── Server.java                    # Multi-threaded server for incoming connections
│   └── Client.java                    # Client socket for outgoing connections
├── message/
│   ├── SOSPFPacket.java               # Protocol packet with message types
│   ├── LSA.java                       # Link State Advertisement with sequence numbers
│   └── LinkDescription.java           # Link metadata
└── util/
    └── Configuration.java             # Configuration file parser
```

## Technologies

Java (Object-oriented design, multithreading, socket programming), Maven, Typesafe Config
