Overnight TCP Transport Benchmark (8 Hours)

This document describes an 8-hour continuous localhost TCP stress test of the SSB raw transport runtime on Windows, including methodology, system configuration, and observed behavior over time.

The goal was to evaluate:

• tail-latency stability
• sustained throughput
• CPU saturation
• memory growth
• transport errors

This is not a product demo — it is a transport-level benchmark harness.

Test Configuration

OS: Windows 10 Pro 22H2
CPU: Intel Core Ultra 9 285K (24 cores)
RAM: 128 GB DDR5
Compiler: MSVC 19.38
Transport: TCP loopback (127.0.0.1)

Threads: 16
Payload: 32 KB
Duration: ~8 hours

Command used:

ssb_stress_client_public.exe ^
  --host 127.0.0.1 ^
  --port 49502 ^
  --duration 28800 ^
  --threads 16 ^
  --payload 32768 ^
  --csv overnight_8h.csv

Metrics Collected

• per-window RTT percentiles (p50 / p95 / p99)
• requests/sec
• throughput (MB/s)
• CPU usage across all cores
• RSS memory
• cumulative errors

Summary of Results

Over the full 8-hour run:

• throughput remained stable
• latency percentiles showed no upward drift
• RSS memory stayed flat
• CPU utilization was consistent
• zero transport errors were recorded

Raw CSV:
docs/data/overnight_8h.csv

System specs:
docs/data/system_specs.txt

Limitations

• localhost only
• kernel bypass not involved
• no NIC queues
• Windows loopback path
• single machine

Future runs will include physical NIC tests and CPU pinning.

Notes on Baseline Comparison

An equivalent ASIO-based harness was run using the same protocol framing and measurement window sizes to avoid tool-induced bias.

Linux Verification Run (WSL2, 30 Minutes)

In addition to the Windows overnight runs, I built and published Linux verification binaries from the same transport layer and exercised the identical stress harness for a shorter endurance run.

Release: v0.2-transport-proof-linux

Environment

OS: Ubuntu 22.04 (WSL2)
Kernel: WSL2 default
Compiler: GCC 11.4
CPU: Ryzen 7950X

Duration: 30 minutes
Threads: 8
Payload: 32 KB

Command used:

./ssb_stress_client_public \
  --threads 8 \
  --payload 32768 \
  --duration 1800 \
  --csv endurance_30m_t8_p32k.csv


Summary

• sustained throughput remained stable over the run
• latency percentiles did not drift
• RSS memory stayed flat
• no disconnects or transport errors were observed

Raw CSV:
docs/data/endurance_30m_t8_p32k.csv

Linux binaries + hashes:
[(link the GitHub release)](https://github.com/Kranyai/SimpleSocketBridge/releases/tag/v0.2-transport-proof-linux)

These Linux runs were intended to validate portability and instrumentation consistency rather than replace the longer Windows endurance tests.
