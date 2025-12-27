import socket, time

HOST = "127.0.0.1"
CMD, DATA = 5050, 5051

sc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sc.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
sc.bind((HOST, CMD)); sc.listen(1)
print(f"[Throughput] wait {HOST}:{CMD}")

sd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sd.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
sd.bind((HOST, DATA)); sd.listen(1)
print(f"[Throughput] wait {HOST}:{DATA}")

cmd, addr = sc.accept()
print("[Throughput] cmd conn", addr)

cmd.sendall(b"T")

data, _ = sd.accept()
print("[Throughput] data conn")

data.settimeout(0.5)
start = time.time(); last = start; total = 0
while time.time() - start < 30:
    try:
        chunk = data.recv(65536)
        if not chunk: break
        total += len(chunk)
        if time.time() - last > 5:
            print(f"[Throughput] {total/1e9:.2f} GB")
            last = time.time()
    except socket.timeout:
        pass

cmd.close()
data.close()
print(f"[Throughput] done {total/1e9:.2f} GB")
