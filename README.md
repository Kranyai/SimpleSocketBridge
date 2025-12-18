# SimpleSocketBridge (SSB)

Raw-socket Unreal Engine 5.5 ↔ Python bridge designed for tight simulation
and reinforcement learning loops.

## Why
Most UE↔Python integrations introduce latency and overhead that break
high-frequency control and sim-to-real workflows.

SSB uses a minimal binary protocol over raw sockets to keep overhead low.

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
