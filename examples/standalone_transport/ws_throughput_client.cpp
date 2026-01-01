// runtime/ws_throughput_client.cpp
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <cstdio>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

static SOCKET connect_tcp(int port)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return INVALID_SOCKET;

    int flag = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(s, (sockaddr*)&addr, sizeof(addr)) != 0)
    {
        closesocket(s);
        return INVALID_SOCKET;
    }
    return s;
}

int main()
{
    constexpr double DURATION = 30.0;
    constexpr size_t BUF_SIZE = 65536;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup failed\n");
        return 1;
    }

    // ---- CMD socket (handshake) ----
    SOCKET cmd = connect_tcp(5050);
    if (cmd == INVALID_SOCKET)
    {
        printf("Failed to connect CMD socket\n");
        return 1;
    }

    char code = 0;
    int r = recv(cmd, &code, 1, MSG_WAITALL);
    if (r != 1 || code != 'T')
    {
        printf("Did not receive 'T' from server (got %d)\n", code);
        closesocket(cmd);
        return 1;
    }

    // ---- DATA socket ----
    SOCKET data = connect_tcp(5051);
    if (data == INVALID_SOCKET)
    {
        printf("Failed to connect DATA socket\n");
        closesocket(cmd);
        return 1;
    }

    std::vector<char> buf(BUF_SIZE);

    auto start = std::chrono::steady_clock::now();
    auto last_report = start;
    long long total = 0;
    long long last_total = 0;

    while (true)
    {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();
        if (elapsed >= DURATION)
            break;

        int sent = send(data, buf.data(), (int)buf.size(), 0);
        if (sent <= 0)
            break;

        total += sent;

        if (std::chrono::duration<double>(now - last_report).count() >= 5.0)
        {
            double dt = std::chrono::duration<double>(now - last_report).count();
            double gbps = (total - last_total) / 1e9 / dt;

            printf("[THROUGHPUT] %.2f GB/s | %.2f GB total\n",
                gbps, total / 1e9);

            last_total = total;
            last_report = now;
        }
    }

    double total_time =
        std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start).count();

    printf("[THROUGHPUT][FINAL] %.2f GB/s | %.2f GB total\n",
        (total / 1e9) / total_time,
        total / 1e9);

    closesocket(data);
    closesocket(cmd);
    WSACleanup();
    return 0;
}
