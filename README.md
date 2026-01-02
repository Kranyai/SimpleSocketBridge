# SimpleSocketBridge (SSB)

Low-latency, high-throughput binary transport for tight simulation and
reinforcement-learning loops, with first-class Unreal Engine 5.5 integration.

SSB can be used either as an Unreal Engine component or as an independent
standalone runtime for simulator and control pipelines.

---

## Why

Most UE ↔ external process integrations introduce latency and jitter that break high-frequency control and sim-to-real workflows.

SSB uses a minimal binary protocol over raw sockets, keeping overhead low and behavior predictable under sustained load.

---

## Architecture (high-level)

```
UE 5.5 (C++)
└─ SimpleSocketBridge
├─ Raw socket (binary frames)
├─ Minimal fixed-size header
└─ Tight send/receive loop
↓
External server (Python commonly used)
└─ SSB protocol handler
├─ recv → decode
└─ encode → send (control / actions)
```


SSB bypasses Unreal RPC, UObject serialization, and shared memory.  
Data is exchanged as small binary frames to minimize latency and jitter.

---

## Quick smoke test (5 minutes)

SSB is exposed as a reusable Unreal Engine component and can be added directly to any Blueprint.

### Typical usage
1. Enable the SSB plugin in Unreal Engine.
2. Open any Actor Blueprint.
3. Add the **SimpleSocketBridge** component.
4. Configure the target address / port.
5. Press Play.

Unreal initiates a socket connection and begins continuous send/receive.

### Blueprint integration example

![SSB Blueprint component integration](docs/ssb_blueprint_component.png)

SSB provides Blueprint events for connection lifecycle and runtime status (connect, disconnect, failures, etc.).  
The underlying socket client is an internal implementation detail.

### Short demo video

A short unlisted demo showing SSB running with the reference test servers:

▶ https://youtu.be/cRMRFwMp0u4

## CARLA Integration Demo (128 Agents)

An unlisted technical demo showing SimpleSocketBridge driving 128 CARLA vehicles
with ~32 KB control payloads in synchronous mode.

- CARLA shipping build (headless)
- 128 agents
- 20 Hz control
- Latest-only UDP transport
- ~48 minute unattended run

📹 Video demo (unlisted):
https://youtu.be/UO_P2Vj7YOQ

---

## Reference demo implementation

The following files were used for validation and benchmarking:

- Unreal side:
  - [examples/unreal/BridgeSender.h](examples/unreal/BridgeSender.h)
  - [examples/unreal/BridgeSender.cpp](examples/unreal/BridgeSender.cpp)

- External servers:
  - [examples/servers/ssb_latency_server.py](examples/servers/ssb_latency_server.py) – round-trip latency measurement
  - [examples/servers/ssb_throughput_server.py](examples/servers/ssb_throughput_server.py) – sustained throughput test
  - [examples/servers/ssb_combined_server.py](examples/servers/ssb_combined_server.py) – combined demo

These represent the exact setup used to produce the measured results below.

---

## Testing & Methodology

SSB is validated using explicit, reproducible test harnesses designed to measure
control freshness, tick alignment, and backlog behavior before scaling to
multi-agent workloads.

### Single-Agent Baseline

A frozen single-agent reference harness is provided under:

- `examples/single_agent_v1/`

This baseline validates:

- Latest-packet-wins control semantics
- Tick-aligned control application
- Bounded control age under sustained frequency
- Absence of backlog or queue buildup

These tests are intentionally limited to **one logical agent** and serve as the
baseline for all subsequent multi-agent evaluation.

### Standalone vs Unreal Testing

The same reference test servers are used across standalone and Unreal-based tests
to ensure observed differences originate from the host environment rather than
the SSB transport or protocol.

---

## Measured Performance

- ~0.28 ms round-trip latency (local, sustained)
- ~1.9 GB/s throughput per thread
- 24h endurance test with zero disconnects

(Tested on UE 5.5 + Python 3.x, Windows)

Standalone transport benchmarks are provided separately and represent raw SSB
capability outside any game engine or simulator.

---

## Platform notes

- Current implementation: **Windows (UE 5.5)**
- All OS-specific socket code is isolated in a single file
- Core protocol and logic are OS-agnostic, making a Linux port straightforward

---

## Use cases

- Reinforcement learning environments
- Robotics and control simulation
- High-frequency sim-to-real experiments

---

## Status

Early / experimental.

Looking for engineers interested in:
- Testing under real workloads
- Benchmark comparisons
- Stressing edge cases

---

## Contact

If you’re working on simulation, RL, or robotics and want to test SSB, feel free to open an issue or email:

**k3nz3y01@gmail.com**
