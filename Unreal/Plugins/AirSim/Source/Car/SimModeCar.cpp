#include "SimModeCar.h"
#include "ConstructorHelpers.h"
#include "AirBlueprintLib.h"
#include "Car/CarPawn.h"

ASimModeCar::ASimModeCar()
{
    static ConstructorHelpers::FClassFinder<APIPCamera> external_camera_class(TEXT("Blueprint'/AirSim/Blueprints/BP_PIPCamera'"));
    external_camera_class_ = external_camera_class.Succeeded() ? external_camera_class.Class : nullptr;
    static ConstructorHelpers::FClassFinder<ACameraDirector> camera_director_class(TEXT("Blueprint'/AirSim/Blueprints/BP_CameraDirector'"));
    camera_director_class_ = camera_director_class.Succeeded() ? camera_director_class.Class : nullptr;
    vehicle_pawn_class_ = ACarPawn::StaticClass();
}

void ASimModeCar::BeginPlay()
{
    Super::BeginPlay();

    createVehicles(vehicles_);
}

void ASimModeCar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    spawned_actors_.Empty();
    if (CameraDirector != nullptr) {
        fpv_vehicle_pawn_wrapper_ = nullptr;
        CameraDirector = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

VehiclePawnWrapper* ASimModeCar::getFpvVehiclePawnWrapper()
{
    return fpv_vehicle_pawn_wrapper_;
}

void ASimModeCar::setupVehiclesAndCamera(std::vector<VehiclePtr>& vehicles)
{
    //get player controller
    APlayerController* player_controller = this->GetWorld()->GetFirstPlayerController();
    FTransform actor_transform = player_controller->GetActorTransform();
    //put camera little bit above vehicle
    FTransform camera_transform(actor_transform.GetLocation() + FVector(-300, 0, 200));

    //we will either find external camera if it already exist in evironment or create one
    APIPCamera* external_camera;

    //find all BP camera directors in the environment
    {
        TArray<AActor*> camera_dirs;
        UAirBlueprintLib::FindAllActor<ACameraDirector>(this, camera_dirs);
        if (camera_dirs.Num() == 0) {
            //create director
            FActorSpawnParameters camera_spawn_params;
            camera_spawn_params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            CameraDirector = this->GetWorld()->SpawnActor<ACameraDirector>(camera_director_class_, camera_transform, camera_spawn_params);
            spawned_actors_.Add(CameraDirector);

            //create external camera required for the director
            external_camera = this->GetWorld()->SpawnActor<APIPCamera>(external_camera_class_, camera_transform, camera_spawn_params);
            spawned_actors_.Add(external_camera);
        }
        else {
            CameraDirector = static_cast<ACameraDirector*>(camera_dirs[0]);
            external_camera = CameraDirector->getExternalCamera();
        }
    }

    //find all vehicle pawns
    {
        TArray<AActor*> pawns;
        UAirBlueprintLib::FindAllActor<TVehiclePawn>(this, pawns);

        //if no vehicle pawns exists in environment
        if (pawns.Num() == 0) {
            //create vehicle pawn
            FActorSpawnParameters pawn_spawn_params;
            pawn_spawn_params.SpawnCollisionHandlingOverride = 
                ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            TVehiclePawn* spawned_pawn = this->GetWorld()->SpawnActor<TVehiclePawn>(
                vehicle_pawn_class_, actor_transform, pawn_spawn_params);

            spawned_actors_.Add(spawned_pawn);
            pawns.Add(spawned_pawn);
        }

        //set up vehicle pawns
        for (AActor* pawn : pawns)
        {
            //initialize each vehicle pawn we found
            TVehiclePawn* vehicle_pawn = static_cast<TVehiclePawn*>(pawn);
            vehicle_pawn->initializeForBeginPlay(enable_rpc, api_server_address);

            //chose first pawn as FPV if none is designated as FPV
            VehiclePawnWrapper* wrapper = vehicle_pawn->getVehiclePawnWrapper();
            if (enable_collision_passthrough)
                wrapper->config.enable_passthrough_on_collisions = true;  
            if (wrapper->config.is_fpv_vehicle || fpv_vehicle_pawn_wrapper_ == nullptr)
                fpv_vehicle_pawn_wrapper_ = wrapper;
        }
    }

    CameraDirector->initializeForBeginPlay(getInitialViewMode(), fpv_vehicle_pawn_wrapper_, external_camera);
}


void ASimModeCar::createVehicles(std::vector<VehiclePtr>& vehicles)
{
    //find vehicles and cameras available in environment
    //if none available then we will create one
    setupVehiclesAndCamera(vehicles);
}



