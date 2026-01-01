# Single-Agent Control Baseline (v1)

This directory contains the **frozen single-agent reference harness**
used to validate SSB control freshness, tick alignment, and backlog behavior.

## Purpose

These tests answer:

- Can a single control stream remain fresh at high frequency?
- Does latest-only forwarding avoid backlog?
- Is control age bounded over time?
- Is behavior stable under sustained load?

## Scope

- Exactly **one logical agent**
- One control stream
- Latest-packet-wins semantics
- No agent routing or fan-out

These files are intentionally **not modified**.
They serve as the baseline for all multi-agent comparisons.
