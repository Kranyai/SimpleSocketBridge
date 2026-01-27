SSB Public Transport Benchmarks

This package contains test-only executables for evaluating the raw transport runtime behind SSB (SimpleSocketBridge).

These tools are intended for:

• independent performance verification
• endurance testing
• CPU / memory profiling
• latency distribution analysis
• comparison against ASIO baselines

They do not include CARLA adapters, Unreal plugins, orchestration logic, or production APIs.

Included Binaries
SSB

• ssb_raw_server_public.exe
Standalone TCP echo server built on SSB raw runtime.

• ssb_stress_client_public.exe
Multi-threaded endurance stress client with CSV logging, percentiles, CPU/RAM tracking.

• ssb_microbench_public.exe
Thread/payload sweep microbenchmark.

Baseline

• asio_baseline_public.exe
Identical benchmark harness implemented directly with standalone ASIO.

Important Notes

These binaries:

• are localhost-focused by default
• use a fixed echo protocol
• expose no simulator adapters
• contain no CARLA integration
• contain no orchestration layer
• are not production deployments

They are benchmark tools only.

System Requirements

• Windows x64
• 8+ logical cores recommended
• Built with MSVC Release
• Loopback (127.0.0.1) testing by default

Quick Start

Open two terminals in the folder containing the EXEs.

Start the server
ssb_raw_server_public.exe --port 49502


Leave running.

Run stress client
ssb_stress_client_public.exe ^
  --host 127.0.0.1 ^
  --port 49502 ^
  --duration 60 ^
  --threads 8 ^
  --payload 4096 ^
  --csv run.csv

Endurance Test (Overnight)

Example 24-hour run:

ssb_stress_client_public.exe ^
  --host 127.0.0.1 ^
  --port 49502 ^
  --duration 86400 ^
  --threads 16 ^
  --payload 32768 ^
  --report-every 15 ^
  --csv 24h.csv


Disable system sleep before running long tests.

Output Format

Console output every report window:

[300s] iters=1234567
rps=380000 MB/s=11800
p50=42us p95=58us p99=66us
cpu=410% rss=5MB errors=0


CSV columns:

time_s,iters,rps,mbps,p50_us,p95_us,p99_us,cpu_pct,rss_mb,errors

What These Numbers Represent

• RTT = send → echo → receive
• Percentiles are per-window
• CPU is total across all cores (100% = 1 core)
• RSS is resident memory of the client process
• MB/s = rps × payload size
• errors are cumulative

What This Package Proves

These tools demonstrate:

• raw SSB runtime performance
• kernel-socket path efficiency
• multi-core scaling
• tail-latency stability
• long-run endurance
• memory safety
• baseline comparison fairness

About CARLA / Unreal

CARLA integration, Unreal Engine plugins, and simulator adapters are separate components and are not included in this bundle.

These benchmarks validate the transport layer used by those systems.