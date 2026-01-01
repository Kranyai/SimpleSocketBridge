# Standalone Transport Benchmarks

This directory contains **isolated transport benchmarks** for
SimpleSocketBridge (SSB), executed **outside Unreal Engine, CARLA, or any
simulator**.

These benchmarks measure **raw transport capability only** and are used to
establish an upper bound on SSB performance before integration overhead.

---

## What is being measured

- Round-trip transport latency (echo-based)
- Sustained bulk throughput
- Long-duration stability (hours to 24h)

These results **do not represent simulator tick alignment, control freshness,
or end-to-end control latency inside Unreal or CARLA**.

---

## Measured Results (Localhost, Windows)

All tests were run on localhost using the reference Python servers.

### Standalone Executable (EXE)

- Average round-trip latency: **~0.143 ms**
- Sustained throughput: **~8.45 GB/s**
- Stable over long-duration runs

### Embedded / DLL Usage

- Average round-trip latency: **~0.153 ms**
- Sustained throughput: **~5.30 GB/s**

The difference reflects additional call boundaries and integration overhead,
not changes to the SSB protocol or transport semantics.

---

## Interpretation

These benchmarks represent the **best-case transport performance** of SSB.

When integrated into a simulator or engine, additional overhead from:

- tick synchronization
- control application
- engine scheduling
- physics and rendering

will increase observed end-to-end latency, which is expected and measured
separately using simulator-specific harnesses.
