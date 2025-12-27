import socket, struct, time

HOST = "127.0.0.1"
CMD, DATA = 5050, 5051

sc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sc.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
sc.bind((HOST, CMD)); sc.listen(1)
print(f"[Latency] wait {HOST}:{CMD}")
cmd, addr = sc.accept(); sc.close()
print("[Latency] cmd conn", addr)

data = None
print("[Latency] skip data socket (5051)")

cmd.sendall(b"L")

cmd.settimeout(0.5)
start = time.time(); last = start; n = 0
while time.time() - start < 30:
    try:
        d = cmd.recv(8)
        if not d:
            break
        cmd.sendall(d)
        n += 1
        if time.time() - last > 5:
            print(f"[Latency] {n} pings")
            last = time.time()
    except socket.timeout:
        pass

cmd.close()
print("[Latency] done", n, "pings")
