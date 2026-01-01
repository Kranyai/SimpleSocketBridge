# ssb_control_core.py
import socket
import struct
import threading
import time

HOST = "127.0.0.1"
INGRESS_PORT = 5060
EGRESS = ("127.0.0.1", 5061)

PKT_FMT = "<IfffQ"
PACKET_SIZE = struct.calcsize(PKT_FMT)

_latest_cmd = None
_lock = threading.Lock()


def _udp_ingress():
    global _latest_cmd
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((HOST, INGRESS_PORT))
    print(f"[SSB CORE] Ingress listening on UDP {HOST}:{INGRESS_PORT}")

    while True:
        data, _ = sock.recvfrom(PACKET_SIZE)
        if len(data) == PACKET_SIZE:
            with _lock:
                _latest_cmd = struct.unpack(PKT_FMT, data)


def _udp_egress():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"[SSB CORE] Forwarding → {EGRESS}")

    last_sent = None

    while True:
        with _lock:
            cmd = _latest_cmd

        # forward only when changed
        if cmd is not None and cmd is not last_sent:
            sock.sendto(struct.pack(PKT_FMT, *cmd), EGRESS)
            last_sent = cmd


threading.Thread(target=_udp_ingress, daemon=True).start()
threading.Thread(target=_udp_egress, daemon=True).start()

print("[SSB CORE] Running (Ctrl+C to stop)")

while True:
    time.sleep(1)
