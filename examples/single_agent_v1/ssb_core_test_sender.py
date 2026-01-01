# ssb_core_test_sender.py
import socket
import struct
import time

TARGET = ("127.0.0.1", 5060)

PKT_FMT = "<IfffQ"   # seq, throttle, steer, brake, send_ns
PACKET_SIZE = struct.calcsize(PKT_FMT)

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

seq = 0
HZ = 100
period = 1.0 / HZ

print(f"[SENDER] Sending @ {HZ} Hz → {TARGET}")

try:
    while True:
        seq = (seq + 1) & 0xFFFFFFFF
        send_ns = time.perf_counter_ns()  # monotonic, cross-process safe

        pkt = struct.pack(
            PKT_FMT,
            seq,
            0.6,   # throttle
            0.0,   # steer
            0.0,   # brake
            send_ns
        )

        sock.sendto(pkt, TARGET)
        time.sleep(period)

except KeyboardInterrupt:
    print("[SENDER] Stopped")
