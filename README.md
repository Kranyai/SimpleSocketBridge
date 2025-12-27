# SimpleSocketBridge (SSB)

Raw-socket Unreal Engine 5.5 ↔ Python bridge designed for tight simulation
and reinforcement learning loops.

## Why
Most UE↔Python integrations introduce latency and overhead that break
high-frequency control and sim-to-real workflows.

SSB uses a minimal binary protocol over raw sockets to keep overhead low.

## Architecture (high-level)

```
UE 5.5 (C++)
└─ SSB Plugin
├─ Raw socket (binary messages)
├─ Minimal fixed-size header
└─ Tight send/receive loop
↓
Python 3.x
└─ SSB Client
├─ recv → decode
└─ encode → send (control / actions)
```

SSB bypasses Unreal RPC, shared memory, and object serialization.
Data is exchanged as small binary frames over raw sockets to minimize latency and jitter.

## Measured Performance
- ~0.28 ms round-trip latency (local, sustained)
- ~1.9 GB/s throughput per thread
- 24h endurance test with zero disconnects

(Tested on UE 5.5 + Python 3.x, Windows)

## Use cases
- Reinforcement learning environments
- Robotics / control simulation
- High-frequency sim-to-real experiments

## Status
Early / experimental.
Looking for engineers interested in testing, benchmarking, or breaking it.

## Contact
If you’re working on sim / RL / robotics and want to test this,
feel free to open an issue or email: k3nz3y01@gmail.com
