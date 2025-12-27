import socket, struct, threading, time, sys, datetime

HOST = "127.0.0.1"
CMD, DATA = 5050, 5051
duration = float(sys.argv[1]) if len(sys.argv) > 1 else 3600.0  # seconds

def listen(port):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    s.bind((HOST, port)); s.listen(1)
    print(f"[Combined] wait {HOST}:{port}")
    return s

def accept_pair():
    sc = listen(CMD)
    sd = listen(DATA)
    cmd, a1 = sc.accept(); print(f"[Combined] cmd {a1}")
    try:
        cmd.sendall(b"C")
    except Exception as e:
        print("[Combined] send C failed:", e); cmd.close(); sc.close(); sd.close(); raise
    data, a2 = sd.accept(); print(f"[Combined] data {a2}")
    sc.close(); sd.close()
    return cmd, data

while True:
    try:
        cmd, data = accept_pair()

        stop = False
        total_bytes = 0
        last_id = -1
        lost = 0
        pings = 0

        def data_loop():
            global total_bytes, last_id, lost, stop
            data.settimeout(0.5)
            buf = b""
            PACKET_SIZE = 65536
            HEADER_SIZE = 4

            while not stop:
                try:
                    chunk = data.recv(65536)
                    if not chunk:
                        break
                    buf += chunk

                    while len(buf) >= PACKET_SIZE:
                        frame = buf[:PACKET_SIZE]
                        buf = buf[PACKET_SIZE:]
                        total_bytes += len(frame)

                        if len(frame) >= HEADER_SIZE:
                            pkt = struct.unpack('<I', frame[:HEADER_SIZE])[0]
                            if last_id >= 0 and pkt != last_id + 1:
                                lost += (pkt - last_id - 1)
                            last_id = pkt

                except socket.timeout:
                    pass
                except ConnectionResetError:
                    break
                except Exception as e:
                    print("[Combined] data error:", e)
                    break

        t = threading.Thread(target=data_loop, daemon=True)
        t.start()

        cmd.settimeout(0.5)
        start = time.time()
        last_print = start
        fast_until = start + 60.0

        while time.time() - start < duration:
            try:
                d = cmd.recv(8)
                if not d: break
                cmd.sendall(d)
                pings += 1
            except socket.timeout:
                pass
            except Exception as e:
                print("[Combined] cmd error:", e); break

            now = time.time()
            elapsed = now - start
            minutes = int(elapsed // 60)

            if elapsed < 60 and now - last_print >= 5.0:
                rate = (total_bytes / 1e9) / elapsed if elapsed > 0 else 0.0
                print(f"[Combined] {elapsed/60:5.1f} min | {total_bytes/1e9:8.2f} GB | "
                    f"{rate:5.2f} GB/s | pings {pings} | lost {lost}")
                last_print = now
            elif minutes > 0 and minutes != int((last_print - start) // 60):
                rate = (total_bytes / 1e9) / elapsed if elapsed > 0 else 0.0
                print(f"[Combined] {minutes:2d}.0 min | {total_bytes/1e9:8.2f} GB | "
                    f"{rate:5.2f} GB/s | pings {pings} | lost {lost}")
                last_print = now

        stop = True
        t.join(1.0)
        try:
            cmd.close(); data.close()
        finally:
            total_pkts = (last_id + 1) if last_id >= 0 else 0
            loss_pct = (lost / total_pkts * 100.0) if total_pkts else 0.0
            print(f"[Combined] done {total_bytes/1e9:.2f} GB, pings {pings}, lost {lost} pkts ({loss_pct:.6f}%)")

            one_shot = True
            if one_shot:
                print("[Combined] One-shot mode: shutting down after session end.")
                sys.exit(0)
            else:
                print("[Combined] session end; waiting for next Unreal connection...\n")

    except Exception as e:
        print(f"[Combined] ERROR: {e} -> retry in 5s")
        time.sleep(5)
