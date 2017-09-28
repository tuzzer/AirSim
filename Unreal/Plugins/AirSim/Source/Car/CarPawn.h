// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "VehiclePawnWrapper.h"
#include "WheeledVehicle.h"
#include "vehicles/car/api/CarRpcLibServer.hpp"
#include "CarPawn.generated.h"

class UPhysicalMaterial;
class UCameraComponent;
class USpringArmComponent;
class UTextRenderComponent;
class UInputComponent;
class UAudioComponent;

UCLASS(config = Game)
class ACarPawn : public AWheeledVehicle
{
    GENERATED_BODY()

    /** Camera component that will be our viewpoint */
    UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* Camera;

    /** SCene component for the In-Car view origin */
    UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    class USceneComponent* InternalCameraBase;

    /** Camera component for the In-Car view */
    UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    APIPCamera* InternalCamera;

    /** Text component for the In-Car speed */
    UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UTextRenderComponent* InCarSpeed;

    /** Text component for the In-Car gear */
    UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UTextRenderComponent* InCarGear;

    /** Audio component for the engine sound */
    UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
    UAudioComponent* EngineSoundComponent;

public:
    ACarPawn();

    /** The current speed as a string eg 10 km/h */
    UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly)
    FText SpeedDisplayString;

    /** The current gear as a string (R,N, 1,2 etc) */
    UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly)
    FText GearDisplayString;

    UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly)
    /** The color of the incar gear text in forward gears */
    FColor	GearDisplayColor;

    /** The color of the incar gear text when in reverse */
    UPROPERTY(Category = Display, VisibleDefaultsOnly, BlueprintReadOnly)
    FColor	GearDisplayReverseColor;

    /** Are we in reverse gear */
    UPROPERTY(Category = Camera, VisibleDefaultsOnly, BlueprintReadOnly)
    bool bInReverseGear;

    /** Initial offset of incar camera */
    FVector InternalCameraOrigin;

    void setupInputBindings();

    void enableApiControl(bool is_enabled);
    bool isApiControlEnabled();

    // Begin Actor interface
    virtual void Tick(float Delta) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    VehiclePawnWrapper* getVehiclePawnWrapper();
    void initializeForBeginPlay(bool enable_rpc, const std::string& api_server_address);

    virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation,
        FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;
protected:
    virtual void BeginPlay() override;

public:
    // End Actor interface


    /** Setup the strings used on the hud */
    void UpdateInCarHUD();

    /** Update the physics material used by the vehicle mesh */
    void UpdatePhysicsMaterial();

    /** Handle pressing forwards */
    void MoveForward(float Val);
    /** Handle pressing right */
    void MoveRight(float Val);
    /** Handle pressiong footbrake */
    void FootBrake(float Val);
    /** Handle Reverse pressed */
    void OnReversePressed();
    /** Handle Reverse released */
    void OnReverseReleased();
    /** Handle Handbrake pressed */
    void OnHandbrakePressed();
    /** Handle Handbrake released */
    void OnHandbrakeReleased();

    static const FName LookUpBinding;
    static const FName LookRightBinding;
    static const FName EngineAudioRPM;

private:
    /** Update the gear and speed strings */
    void UpdateHUDStrings();
    void startApiServer(bool enable_rpc, const std::string& api_server_address);
    void stopApiServer();
    bool isApiServerStarted();

    /* Are we on a 'slippery' surface */
    bool bIsLowFriction;
    /** Slippery Material instance */
    UPhysicalMaterial* SlipperyMaterial;
    /** Non Slippery Material instance */
    UPhysicalMaterial* NonSlipperyMaterial;

public:
    /** Returns InCarSpeed subobject **/
    FORCEINLINE UTextRenderComponent* GetInCarSpeed() const { return InCarSpeed; }
    /** Returns InCarGear subobject **/
    FORCEINLINE UTextRenderComponent* GetInCarGear() const { return InCarGear; }
    /** Returns EngineSoundComponent subobject **/
    FORCEINLINE UAudioComponent* GetEngineSoundComponent() const { return EngineSoundComponent; }

private:
    UClass* pip_camera_class_;
    class CarController;
    std::unique_ptr<msr::airlib::CarRpcLibServer> rpclib_server_;
    std::unique_ptr<msr::airlib::CarControllerBase> controller_;
    std::unique_ptr<VehiclePawnWrapper> wrapper_;

    bool api_control_enabled_ = false;
};
