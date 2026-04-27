#include "DroneHUD.h"
#include "DronePawn.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "CanvasItem.h"

void ADroneHUD::DrawHUD()
{
    Super::DrawHUD();

    APlayerController* PC =
        GetOwningPlayerController();
    if (!PC) return;

    ADronePawn* Drone =
        Cast<ADronePawn>(PC->GetPawn());
    if (!Drone) return;

    FLinearColor White = FLinearColor::White;
    FLinearColor Red = FLinearColor::Red;
    FLinearColor Green = FLinearColor::Green;
    FLinearColor Cyan =
        FLinearColor(0.f, 1.f, 1.f, 1.f);
    FLinearColor Gray =
        FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
    FLinearColor Yellow = FLinearColor::Yellow;
    FLinearColor Orange =
        FLinearColor(1.f, 0.5f, 0.f, 1.f);

    DrawText(TEXT("DRONE_PACK SIMULATION"),
        20, 20, Cyan);

    DrawText(FString::Printf(
        TEXT("Speed:    %.0f cm/s"),
        Drone->GetSpeed()), 20, 50, White);
    DrawText(FString::Printf(
        TEXT("Altitude: %.0f cm"),
        Drone->GetAltitude()), 20, 75, White);

    DrawText(TEXT("── Rotor Thrust ──"),
        20, 110, Yellow);
    DrawText(FString::Printf(TEXT("FL: %.0f"),
        Drone->GetRotorFL()), 20, 135, Green);
    DrawText(FString::Printf(TEXT("FR: %.0f"),
        Drone->GetRotorFR()), 150, 135, Green);
    DrawText(FString::Printf(TEXT("BL: %.0f"),
        Drone->GetRotorBL()), 20, 160, Green);
    DrawText(FString::Printf(TEXT("BR: %.0f"),
        Drone->GetRotorBR()), 150, 160, Green);

    DrawText(TEXT("── Obstacles ──"),
        20, 200, Yellow);
    DrawText(Drone->IsObstacleAhead() ?
        TEXT("AHEAD: DANGER!") :
        TEXT("AHEAD: Clear"),
        20, 225,
        Drone->IsObstacleAhead() ? Red : Green);
    DrawText(Drone->IsObstacleLeft() ?
        TEXT("LEFT:  DANGER!") :
        TEXT("LEFT:  Clear"),
        20, 250,
        Drone->IsObstacleLeft() ? Red : Green);
    DrawText(Drone->IsObstacleRight() ?
        TEXT("RIGHT: DANGER!") :
        TEXT("RIGHT: Clear"),
        20, 275,
        Drone->IsObstacleRight() ? Red : Green);
    DrawText(Drone->IsObstacleAbove() ?
        TEXT("ABOVE: DANGER!") :
        TEXT("ABOVE: Clear"),
        20, 300,
        Drone->IsObstacleAbove() ? Red : Green);
    DrawText(Drone->IsObstacleBelow() ?
        TEXT("BELOW: DANGER!") :
        TEXT("BELOW: Clear"),
        20, 325,
        Drone->IsObstacleBelow() ?
        Orange : Green);

    DrawText(TEXT("── Missiles ──"),
        20, 365, Yellow);
    DrawText(FString::Printf(TEXT("Ammo: %d"),
        Drone->GetMissileCount()),
        20, 390,
        Drone->GetMissileCount() > 0 ?
        Green : Red);
    DrawText(Drone->HasLockedTarget() ?
        TEXT("Target: LOCKED!") :
        TEXT("Target: Searching..."),
        20, 415,
        Drone->HasLockedTarget() ? Red : Gray);

    DrawText(TEXT("── Auto Flight ──"),
        20, 455, Yellow);
    DrawText(Drone->IsAutoMode() ?
        TEXT("Mode: AUTONOMOUS") :
        TEXT("Mode: MANUAL"),
        20, 480,
        Drone->IsAutoMode() ? Cyan : Gray);

    DrawText(
        TEXT("W/S=Throttle | Arrows=Pitch/Roll"),
        20, 520, Gray);
    DrawText(
        TEXT("Q/E=Yaw | C=FPV | T=Reset"),
        20, 540, Gray);
    DrawText(
        TEXT("SPACE=Fire | F=Auto Mode"),
        20, 560, Yellow);

    if (Drone->IsCrashed() && Canvas)
        DrawText(TEXT("CRASHED! Press T!"),
            Canvas->SizeX / 2.f - 100.f,
            Canvas->SizeY / 2.f, Red);
}

void ADroneHUD::DrawText(
    const FString& Text,
    float X, float Y,
    FLinearColor Color)
{
    if (!Canvas || !GEngine) return;
    FCanvasTextItem Item(
        FVector2D(X, Y),
        FText::FromString(Text),
        GEngine->GetMediumFont(),
        Color);
    Item.Scale = FVector2D(1.5f, 1.5f);
    Canvas->DrawItem(Item);
}