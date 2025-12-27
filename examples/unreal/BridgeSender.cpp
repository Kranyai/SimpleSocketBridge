#include "BridgeSender.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Async/Async.h"
#include "HAL/PlatformProcess.h"

static FSocket* MakeTcp()
{
    FSocket* S = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("SSB"), false);
    if (S)
    {
        S->SetNonBlocking(false);
        S->SetNoDelay(true);
    }
    return S;
}

ABridgeSender::ABridgeSender()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ABridgeSender::BeginPlay()
{
    Super::BeginPlay();
    if (!bConnectOnBeginPlay) return;

    bRunning = true;
    Async(EAsyncExecution::Thread, [this]() { ListenForCommand(); });
}

void ABridgeSender::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bRunning = false;
    CloseAll();
    Super::EndPlay(EndPlayReason);
}

bool ABridgeSender::ConnectSocket(FSocket*& Out, int32 Port)
{
    if (Out) { Out->Close(); ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Out); Out = nullptr; }
    Out = MakeTcp();
    if (!Out) return false;

    FIPv4Address IP;
    if (!FIPv4Address::Parse(ServerIP, IP)) return false;

    TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    Addr->SetIp(IP.Value);
    Addr->SetPort(Port);

    while (bRunning && !Out->Connect(*Addr))
    {
        UE_LOG(LogTemp, Warning, TEXT("Retry connect to %s:%d in 3s..."), *ServerIP, Port);
        FPlatformProcess::Sleep(3.f);
    }
    return bRunning;
}

void ABridgeSender::CloseAll()
{
    if (CmdSocket) { CmdSocket->Close(); ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(CmdSocket); CmdSocket = nullptr; }
    if (DataSocket) { DataSocket->Close(); ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(DataSocket); DataSocket = nullptr; }
}

void ABridgeSender::ListenForCommand()
{
reconnect:
    if (!bRunning) return;

    if (!ConnectSocket(CmdSocket, CmdPort)) return;
    UE_LOG(LogTemp, Warning, TEXT("Connected to %s:%d"), *ServerIP, CmdPort);

    uint8 Code = 0;
    int32 Bytes = 0;

    double WaitStart = FPlatformTime::Seconds();
    bool bGotCommand = false;

    while (bRunning && (FPlatformTime::Seconds() - WaitStart) < 5.0)
    {
        if (CmdSocket->Recv(&Code, 1, Bytes))
        {
            if (Bytes > 0) { bGotCommand = true; break; }
        }
        FPlatformProcess::Sleep(0.05f);
    }

    if (!bGotCommand)
    {
        UE_LOG(LogTemp, Warning, TEXT("No command received within timeout. Reconnecting..."));
        CloseAll();
        if (bRunning) goto reconnect;
        return;
    }

    if (Code == 'T' || Code == 'E' || Code == 'C')
    {
        if (!ConnectSocket(DataSocket, DataPort))
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to connect to data port %d, skipping test."), DataPort);
            CloseAll();
            if (bRunning) goto reconnect;
            return;
        }
    }

    switch (Code)
    {
    case 'L': UE_LOG(LogTemp, Warning, TEXT("Latency test start"));       RunLatencyTest(); break;
    case 'T': UE_LOG(LogTemp, Warning, TEXT("Throughput test start"));    RunThroughputTest(); break;
    case 'E': UE_LOG(LogTemp, Warning, TEXT("Endurance test start"));     RunEnduranceTest(); break;
    case 'C': UE_LOG(LogTemp, Warning, TEXT("Combined test start"));      RunCombinedTest(); break;
    default:
        UE_LOG(LogTemp, Warning, TEXT("Unknown command: %d"), Code);
        break;
    }

    CloseAll();
    if (bRunning) goto reconnect;
}

void ABridgeSender::RunLatencyTest()
{
    const double Duration = 30.0, Interval = 0.1;
    double Start = FPlatformTime::Seconds(), Last = 0, LastReport = Start;
    double Sum = 0, Min = 9999, Max = 0, LastSum = 0; int32 Count = 0, LastCount = 0;

    while (bRunning && (FPlatformTime::Seconds() - Start) < Duration)
    {
        double Now = FPlatformTime::Seconds();
        if (Now - Last >= Interval)
        {
            Last = Now;
            double T = Now; int32 Sent = 0, Recv = 0;
            CmdSocket->Send((uint8*)&T, sizeof(double), Sent);
            double Echo = 0;
            if (CmdSocket->Recv((uint8*)&Echo, sizeof(double), Recv))
            {
                double L = (FPlatformTime::Seconds() - T) * 1000.0;
                Sum += L; Min = FMath::Min(Min, L); Max = FMath::Max(Max, L); Count++;
            }
        }
        if (FPlatformTime::Seconds() - LastReport > 5.0 && Count > LastCount)
        {
            double ds = Sum - LastSum; int32 n = Count - LastCount;
            UE_LOG(LogTemp, Warning, TEXT("[LATENCY] Avg %.3f ms | Min %.3f | Max %.3f | Samples=%d"),
                ds / n, Min, Max, n);
            LastReport = FPlatformTime::Seconds(); LastSum = Sum; LastCount = Count; Min = 9999; Max = 0;
        }
        FPlatformProcess::Sleep(0.001f);
    }
    UE_LOG(LogTemp, Warning, TEXT("Latency test end"));
}

void ABridgeSender::RunThroughputTest()
{
    const double Duration = 30.0;
    TArray<uint8> Buf; Buf.SetNumUninitialized(65536);
    double Start = FPlatformTime::Seconds(), LastReport = Start;
    int64 Total = 0, LastTotal = 0; 
    int32 Sent = 0;

    while (bRunning && (FPlatformTime::Seconds() - Start) < Duration)
    {
        DataSocket->Send(Buf.GetData(), Buf.Num(), Sent);
        Total += Sent;

        double Now = FPlatformTime::Seconds();
        if (Now - LastReport > 5.0)
        {
            double DeltaBytes = Total - LastTotal;
            double DeltaTime = Now - LastReport;
            double GBs = (DeltaBytes / 1e9) / DeltaTime;
            LastTotal = Total;

            UE_LOG(LogTemp, Warning, TEXT("[THROUGHPUT] %.2f GB/s (%.2f GB total)"), GBs, Total / 1e9);
            LastReport = Now;
        }
    }

    double End = FPlatformTime::Seconds();
    double GBs = (Total / 1e9) / (End - Start);
    UE_LOG(LogTemp, Warning, TEXT("[THROUGHPUT] FINAL %.2f GB/s (%.2f GB total)"), GBs, Total / 1e9);
    UE_LOG(LogTemp, Warning, TEXT("Throughput test end"));
}


void ABridgeSender::RunEnduranceTest()
{
    const double Duration = 30.0;
    TArray<uint8> Buf; Buf.SetNumUninitialized(4096);
    double Start = FPlatformTime::Seconds(), LastReport = Start;
    int64 Count = 0; int32 Sent = 0;

    while (bRunning && (FPlatformTime::Seconds() - Start) < Duration)
    {
        DataSocket->Send(Buf.GetData(), Buf.Num(), Sent);
        if (Sent > 0) Count++;

        double Now = FPlatformTime::Seconds();
        if (Now - LastReport > 5.0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ENDURANCE] t=%.0f s packets=%lld"), Now - Start, Count);
            LastReport = Now;
        }
        FPlatformProcess::Sleep(0.001f);
    }
    UE_LOG(LogTemp, Warning, TEXT("[ENDURANCE] FINAL t=%.0f s packets=%lld"),
        FPlatformTime::Seconds() - Start, Count);
    UE_LOG(LogTemp, Warning, TEXT("Endurance test end"));
}

void ABridgeSender::RunCombinedTest()
{
    const double Duration = CombinedDuration;
    bStopCombined = false;

    TArray<uint8> ThrBuf;
    ThrBuf.SetNumUninitialized(65536);
    auto Sender = [this, Duration, ThrBuf]() mutable
        {
            double Start = FPlatformTime::Seconds();
            double LastReport = Start;
            int64 Total = 0; int32 Sent = 0;

            uint32 Counter = 0;

            bStopCombined = false;
            UE_LOG(LogTemp, Warning, TEXT("[COMBINED] Starting new Combined test session"));

            while (bRunning && !bStopCombined && (FPlatformTime::Seconds() - Start) < Duration)
            {
                if (!DataSocket) break;

                if (DataSocket->GetConnectionState() != SCS_Connected)
                    break;

                FMemory::Memcpy(ThrBuf.GetData(), &Counter, sizeof(uint32));
                Counter++;

                Sent = 0;
                bool bOK = DataSocket->Send(ThrBuf.GetData(), ThrBuf.Num(), Sent);

                if (!bOK || Sent != ThrBuf.Num() || DataSocket->GetConnectionState() != SCS_Connected)
                {
                    UE_LOG(LogTemp, Warning, TEXT("[COMBINED][THR] Connection closed or send failed — stopping sender"));
                    break;
                }

                Total += Sent;


                double Now = FPlatformTime::Seconds();
                if (Now - LastReport > 5.0)
                {
                    double GBs = (Total / 1e9) / (Now - Start);
                    UE_LOG(LogTemp, Warning, TEXT("[COMBINED][THR] %.2f GB/s (%.2f GB)"), GBs, Total / 1e9);

                    LastReport = Now;
                }
            }
        };
    Async(EAsyncExecution::Thread, Sender);

    double Start = FPlatformTime::Seconds();
    double LastPing = 0.0, LastReport = Start;
    double Sum = 0.0, Min = 9999.0, Max = 0.0;
    int32 Count = 0;

    while (bRunning && (FPlatformTime::Seconds() - Start) < Duration)
    {
        double Now = FPlatformTime::Seconds();

        if (Now - LastPing >= 0.1)
        {
            LastPing = Now;
            double T = Now; int32 Sent = 0, Recv = 0;
            if (!CmdSocket) break;
            CmdSocket->Send((uint8*)&T, sizeof(double), Sent);

            double Echo = 0;
            double WaitStart = FPlatformTime::Seconds();
            while (bRunning && (FPlatformTime::Seconds() - WaitStart) < 0.05)
            {
                if (CmdSocket->Recv((uint8*)&Echo, sizeof(double), Recv, ESocketReceiveFlags::None))
                {
                    double L = (FPlatformTime::Seconds() - T) * 1000.0;
                    Sum += L; Min = FMath::Min(Min, L); Max = FMath::Max(Max, L); Count++;
                    break;
                }
                FPlatformProcess::Sleep(0.0005);
            }
        }

        if (Now - LastReport > 5.0)
        {
            UE_LOG(LogTemp, Warning, TEXT("[COMBINED][LAT] Avg %.3f ms | Min %.3f | Max %.3f | Samples=%d"),
                (Count > 0 ? Sum / Count : 0.0), Min, Max, Count);
            LastReport = Now;
        }

        FPlatformProcess::Sleep(0.0005f);
    }

    bStopCombined = true;
    FPlatformProcess::Sleep(0.5f);

    UE_LOG(LogTemp, Warning, TEXT("Combined test end"));

}
