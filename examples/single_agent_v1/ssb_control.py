# ssb_control.py
import sys
import time
import socket
import struct
import math

# ---- CARLA import ----
sys.path.append(
    r"C:\carla\Unreal\CarlaUE4\Content\WindowsNoEditor\PythonAPI\carla\dist\carla-0.9.15-py3.7-win-amd64.egg"
)
import carla

# ---- SSB CORE EGRESS ----
SSB_EGRESS_PORT = 5061

PKT_FMT = "<IfffQ"
PACKET_SIZE = struct.calcsize(PKT_FMT)

WINDOW_S = 5.0


def pct(values, p):
    if not values:
        return None
    values = sorted(values)
    idx = int(len(values) * p / 100)
    return values[min(idx, len(values) - 1)]


def main():
    # ---- UDP listener ----
    ssb_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    ssb_sock.bind(("127.0.0.1", SSB_EGRESS_PORT))
    ssb_sock.setblocking(False)

    print(f"[CARLA] Listening for SSB core on UDP {SSB_EGRESS_PORT}")

    # ---- CARLA setup ----
    client = carla.Client("127.0.0.1", 2000)
    client.set_timeout(5.0)

    world = client.get_world()

    # spawn vehicle FIRST
    bp_lib = world.get_blueprint_library()
    vehicle_bp = bp_lib.filter("vehicle.*model3*")[0]
    vehicle = None
    for sp in world.get_map().get_spawn_points():
        vehicle = world.try_spawn_actor(vehicle_bp, sp)
        if vehicle:
            break

    # NOW apply sync settings
    settings = world.get_settings()
    settings.synchronous_mode = True
    settings.fixed_delta_seconds = 1.0 / 100
    settings.substepping = False
    world.apply_settings(settings)
    world.tick()

    s = world.get_settings()
    print(
        "[CARLA SETTINGS CONFIRMED]",
        "sync=", s.synchronous_mode,
        "dt=", s.fixed_delta_seconds,
        "substep=", s.substepping
    )

    tick_hz = 1.0 / settings.fixed_delta_seconds
    tick_period_ms = settings.fixed_delta_seconds * 1000.0

    print(f"[CARLA SETTINGS] sync=True dt={settings.fixed_delta_seconds} Hz={tick_hz}")

    # ---- spawn vehicle ----
    bp_lib = world.get_blueprint_library()
    vehicle_bp = bp_lib.filter("vehicle.*model3*")[0]

    vehicle = None
    for sp in world.get_map().get_spawn_points():
        vehicle = world.try_spawn_actor(vehicle_bp, sp)
        if vehicle:
            break

    if vehicle is None:
        raise RuntimeError("Failed to spawn vehicle")

    print("✅ Vehicle spawned:", vehicle.id)

    # ---- metrics state ----
    last_applied_seq = None

    win_start_ns = time.perf_counter_ns()
    win_ticks = 0
    win_fresh = 0

    ages_ms = []
    adapter_us = []
    wait_ticks = []

    prev_tick_ns = None
    max_tick_gap_ms = 0.0

    try:
        while True:
            tick_start_ns = time.perf_counter_ns()

            # tick gap
            if prev_tick_ns is not None:
                gap_ms = (tick_start_ns - prev_tick_ns) / 1e6
                max_tick_gap_ms = max(max_tick_gap_ms, gap_ms)
            prev_tick_ns = tick_start_ns

            # ---- drain UDP ----
            latest = None
            while True:
                try:
                    data, _ = ssb_sock.recvfrom(PACKET_SIZE)
                except BlockingIOError:
                    break

                recv_ns = time.perf_counter_ns()
                seq, thr, steer, brake, send_ns = struct.unpack(PKT_FMT, data)
                latest = (seq, thr, steer, brake, send_ns, recv_ns)

            fresh = False
            if latest:
                seq, thr, steer, brake, send_ns, recv_ns = latest
                if last_applied_seq is None or seq != last_applied_seq:
                    fresh = True

            t0_ns = time.perf_counter_ns()

            if fresh:
                # command age
                ages_ms.append((tick_start_ns - recv_ns) / 1e6)

                # backlog proxy
                recv_to_apply_ms = (tick_start_ns - recv_ns) / 1e6
                wait = int(math.ceil(recv_to_apply_ms / tick_period_ms))
                wait_ticks.append(wait)

                vehicle.apply_control(
                    carla.VehicleControl(
                        throttle=thr,
                        steer=steer,
                        brake=brake,
                    )
                )

                t1_ns = time.perf_counter_ns()
                adapter_us.append((t1_ns - t0_ns) / 1e3)

                last_applied_seq = seq
                win_fresh += 1

            world.tick()
            win_ticks += 1

            # ---- window report ----
            now_ns = time.perf_counter_ns()
            if (now_ns - win_start_ns) >= WINDOW_S * 1e9:
                win_s = (now_ns - win_start_ns) / 1e9
                achieved_hz = win_ticks / win_s
                fresh_pct = 100.0 * win_fresh / max(1, win_ticks)

                print(
                    f"[{tick_hz:.0f}Hz][{win_s:.1f}s] "
                    f"tick={achieved_hz:.1f}Hz "
                    f"fresh={fresh_pct:.1f}% | "
                    f"age_ms p50={pct(ages_ms,50):.1f} "
                    f"p95={pct(ages_ms,95):.1f} "
                    f"max={max(ages_ms):.1f} | "
                    f"adapter_us p95={pct(adapter_us,95):.0f} "
                    f"max={max(adapter_us):.0f} | "
                    f"max_wait={max(wait_ticks)} | "
                    f"max_tick_gap_ms={max_tick_gap_ms:.1f}"
                )

                # reset window
                win_start_ns = now_ns
                win_ticks = win_fresh = 0
                ages_ms.clear()
                adapter_us.clear()
                wait_ticks.clear()
                max_tick_gap_ms = 0.0

    except KeyboardInterrupt:
        pass
    finally:
        vehicle.destroy()


if __name__ == "__main__":
    main()
