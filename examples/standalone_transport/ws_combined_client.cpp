#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <cstdio>
#include <thread>
#include <atomic>
#include <vector>
#include <algorithm>
#include <cstdint>

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

int main(int argc, char** argv)
{
    double duration = (argc > 1) ? atof(argv[1]) : 86400.0; // default 24h

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    SOCKET cmd = connect_tcp(5050);
    if (cmd == INVALID_SOCKET)
        return 1;

    char code = 0;
    if (recv(cmd, &code, 1, MSG_WAITALL) != 1 || code != 'C')
        return 1;

    SOCKET data = connect_tcp(5051);
    if (data == INVALID_SOCKET)
        return 1;

    std::atomic<bool> stop{ false };

    // ---- throughput ----
    std::atomic<uint64_t> total_bytes{ 0 };
    uint64_t window_bytes = 0;
    uint64_t last_bytes_snapshot = 0;

    // ---- latency ----
    double window_lat_sum = 0.0;
    uint64_t window_lat_count = 0;

    double total_lat_sum = 0.0;
    uint64_t total_lat_count = 0;

    // ---- DATA THREAD ----
    std::thread data_thread([&]()
        {
            const size_t PACKET_SIZE = 65536;
            std::vector<uint8_t> buf(PACKET_SIZE);
            uint32_t counter = 0;

            while (!stop)
            {
                memcpy(buf.data(), &counter, sizeof(counter));
                int sent = send(data, (char*)buf.data(), (int)buf.size(), 0);
                if (sent != (int)buf.size())
                    break;

                total_bytes += sent;
                counter++;
            }
        });

    auto start_time = std::chrono::steady_clock::now();
    auto last_ping = start_time;
    auto last_report = start_time;

    while (true)
    {
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - start_time).count();
        if (elapsed >= duration)
            break;

        // ---- latency ping every 100 ms ----
        if (std::chrono::duration<double>(now - last_ping).count() >= 0.1)
        {
            last_ping = now;

            double t = elapsed;
            auto send_time = std::chrono::steady_clock::now();

            if (send(cmd, (char*)&t, sizeof(double), 0) != sizeof(double))
                break;

            double echo = 0.0;
            if (recv(cmd, (char*)&echo, sizeof(double), MSG_WAITALL) == sizeof(double))
            {
                double rtt_ms =
                    std::chrono::duration<double>(
                        std::chrono::steady_clock::now() - send_time
                    ).count() * 1000.0;

                window_lat_sum += rtt_ms;
                window_lat_count++;

                total_lat_sum += rtt_ms;
                total_lat_count++;
            }
        }

        // ---- report every 5 seconds ----
        if (std::chrono::duration<double>(now - last_report).count() >= 5.0)
        {
            double dt = std::chrono::duration<double>(now - last_report).count();
            uint64_t cur_bytes = total_bytes.load();

            uint64_t delta_bytes = cur_bytes - last_bytes_snapshot;
            last_bytes_snapshot = cur_bytes;

            double gbps_5s = (double)delta_bytes / 1e9 / dt;
            double gbps_total = (double)cur_bytes / 1e9 / elapsed;

            double lat_5s = window_lat_count ? window_lat_sum / window_lat_count : 0.0;
            double lat_total = total_lat_count ? total_lat_sum / total_lat_count : 0.0;

            printf(
                "[COMBINED][5s]  %.1f min | %.2f GB/s | lat %.3f ms | pings %llu\n"
                "[COMBINED][ALL] %.1f min | %.2f GB/s | lat %.3f ms | pings %llu\n\n",
                elapsed / 60.0,
                gbps_5s,
                lat_5s,
                window_lat_count,
                elapsed / 60.0,
                gbps_total,
                lat_total,
                total_lat_count
            );

            // reset window stats
            window_lat_sum = 0.0;
            window_lat_count = 0;
            last_report = now;
        }

        Sleep(1);
    }

    stop = true;
    data_thread.join();

    closesocket(cmd);
    closesocket(data);
    WSACleanup();

    printf(
        "[COMBINED][FINAL] %.2f GB | avg %.2f GB/s | lat %.3f ms | pings %llu\n",
        total_bytes.load() / 1e9,
        (total_bytes.load() / 1e9) /
        std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start_time
        ).count(),
        total_lat_count ? total_lat_sum / total_lat_count : 0.0,
        total_lat_count
    );

    return 0;
}
