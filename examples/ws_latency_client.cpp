// runtime/ws_latency_client.cpp
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <cstdio>
#include <vector>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    constexpr double DURATION = 30.0;
    constexpr double INTERVAL = 0.1; // 100 ms

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int flag = 1;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5050);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(s, (sockaddr*)&addr, sizeof(addr));

    auto start = std::chrono::steady_clock::now();
    auto last_send = start;
    auto last_report = start;

    double sum = 0, min = 1e9, max = 0;
    double last_sum = 0;
    int count = 0, last_count = 0;

    while (true)
    {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start).count();
        if (elapsed >= DURATION) break;

        if (std::chrono::duration<double>(now - last_send).count() >= INTERVAL)
        {
            last_send = now;
            double t = elapsed;
            send(s, (char*)&t, sizeof(double), 0);

            double echo = 0;
            int r = recv(s, (char*)&echo, sizeof(double), MSG_WAITALL);
            if (r == sizeof(double))
            {
                double rtt = (std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - last_send).count()) * 1000.0;
                sum += rtt;
                min = std::min(min, rtt);
                max = std::max(max, rtt);
                count++;
            }
        }

        if (std::chrono::duration<double>(now - last_report).count() >= 5.0 && count > last_count)
        {
            double ds = sum - last_sum;
            int dn = count - last_count;
            printf("[LATENCY] Avg %.3f ms | Min %.3f | Max %.3f | Samples=%d\n",
                ds / dn, min, max, dn);
            last_sum = sum;
            last_count = count;
            min = 1e9; max = 0;
            last_report = now;
        }

        Sleep(1);
    }

    printf("[LATENCY][FINAL] Avg %.3f ms | Samples=%d\n", sum / count, count);

    closesocket(s);
    WSACleanup();
    return 0;
}
