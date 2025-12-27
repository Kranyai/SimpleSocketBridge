#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <atomic>
#include "BridgeSender.generated.h"

class FSocket;

UCLASS()
class ABridgeSender : public AActor
{
    GENERATED_BODY()

public:
    ABridgeSender();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, Category = "Socket")
    FString ServerIP = TEXT("127.0.0.1");

    UPROPERTY(EditAnywhere, Category = "Socket")
    int32 CmdPort = 5050;

    UPROPERTY(EditAnywhere, Category = "Socket")
    int32 DataPort = 5051;

    UPROPERTY(EditAnywhere, Category = "Socket")
    bool bConnectOnBeginPlay = true;

    UPROPERTY(EditAnywhere, Category = "Socket")
    double CombinedDuration = 86400.0;

private:
    FSocket* CmdSocket = nullptr;
    FSocket* DataSocket = nullptr;

    std::atomic<bool> bRunning{ false };
    std::atomic<bool> bStopCombined{ false };

    void ListenForCommand();
    bool ConnectSocket(FSocket*& Out, int32 Port);
    void CloseAll();

    void RunLatencyTest();
    void RunThroughputTest();
    void RunEnduranceTest();
    void RunCombinedTest();
};
