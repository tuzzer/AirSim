#include "SimModeBase.h"
#include "Misc/MessageDialog.h"
#include "Misc/EngineVersion.h"
#include "AirBlueprintLib.h"
#include "Runtime/Launch/Resources/Version.h"
#include "controllers/Settings.hpp"
#include "SimJoyStick/SimJoyStick.h"

ASimModeBase::ASimModeBase()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASimModeBase::BeginPlay()
{
    Super::BeginPlay();

    setStencilIDs();

    recording_file_.reset(new RecordingFile());
    record_tick_count = 0;
    setupInputBindings();

    UAirBlueprintLib::LogMessage(TEXT("Press F1 to see help"), TEXT(""), LogDebugLevel::Informational);

    readSettings();
}

void ASimModeBase::setStencilIDs()
{
    TArray<AActor*> foundActors;
    UAirBlueprintLib::FindAllActor<AActor>(this, foundActors);
    TArray<UStaticMeshComponent*> components;
    int stencil = 0;
    for (AActor* actor : foundActors) {
        actor->GetComponents(components);
        if (components.Num() == 1) {
            components[0]->SetRenderCustomDepth(true);
            components[0]->CustomDepthStencilValue = (stencil++) % 256;
            components[0]->MarkRenderStateDirty();
        }
    }
}

void ASimModeBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    recording_file_.release();

    Super::EndPlay(EndPlayReason);
}

void ASimModeBase::readSettings()
{
    typedef msr::airlib::Settings Settings;

    Settings& settings = Settings::singleton();

    settings_version_actual = settings.getFloat("SettingdVersion", 0);
    if (settings_version_actual < settings_version_minimum) {
        if ((settings.size() == 1 && 
            ((settings.getString("SeeDocsAt", "") != "") || settings.getString("see_docs_at", "") != ""))
            || (settings.size() == 0)) {
            //no warnings because we have default settings
        }
        else {
            UAirBlueprintLib::LogMessageString("Your settings file is of old version and possibly not compatible!","", LogDebugLevel::Failure);
            UAirBlueprintLib::LogMessageString("Please look at new settings and update your settings.json: ","https://git.io/v9mYY", LogDebugLevel::Failure);
        }
    }

    std::string simmode_name = settings.getString("SimMode", "");
    usage_scenario = settings.getString("UsageScenario", "");
    default_vehicle_config = settings.getString("DefaultVehicleConfig", "SimpleFlight");
   
    enable_rpc = settings.getBool("RpcEnabled", true);
    //by default we spawn server at local endpoint. Do not use 127.0.0.1 as default below
    //because for docker container default is 0.0.0.0 and people get really confused why things
    //don't work
    api_server_address = settings.getString("LocalHostIp", "");
    is_record_ui_visible = settings.getBool("RecordUIVisible", true);

    std::string view_mode_string = settings.getString("ViewMode", simmode_name == "" ? "FlyWithMe" : "SpringArmChase");
    if (view_mode_string == "FlyWithMe")
        initial_view_mode = ECameraDirectorMode::CAMERA_DIRECTOR_MODE_FLY_WITH_ME;
    else if (view_mode_string == "Fpv")
        initial_view_mode = ECameraDirectorMode::CAMERA_DIRECTOR_MODE_FPV;
    else if (view_mode_string == "Manual")
        initial_view_mode = ECameraDirectorMode::CAMERA_DIRECTOR_MODE_MANUAL;
    else if (view_mode_string == "GroundObserver")
        initial_view_mode = ECameraDirectorMode::CAMERA_DIRECTOR_MODE_GROUND_OBSERVER;
    else if (view_mode_string == "SpringArmChase")
        initial_view_mode = ECameraDirectorMode::CAMERA_DIRECTOR_MODE_SPRINGARM_CHASE;
    else
        UAirBlueprintLib::LogMessage("ViewMode setting is not recognized: ", view_mode_string.c_str(), LogDebugLevel::Failure);
        
    physics_engine_name = settings.getString("PhysicsEngineName", "FastPhysicsEngine");
    enable_collision_passthrough = settings.getBool("EnableCollisionPassthrogh", false);
    clock_type = settings.getString("ClockType", 
        default_vehicle_config == "SimpleFlight" ? "SteppableClock" : "ScalableClock");

    Settings record_settings;
    if (settings.getChild("Recording", record_settings)) {
        recording_settings.record_on_move = record_settings.getBool("RecordOnMove", recording_settings.record_on_move);
        recording_settings.record_interval = record_settings.getFloat("RecordInterval", recording_settings.record_interval);
    }
    
    UAirBlueprintLib::LogMessage("Default config: ", default_vehicle_config.c_str(), LogDebugLevel::Informational);
}

void ASimModeBase::Tick(float DeltaSeconds)
{
    if (recording_file_->isRecording())
        ++record_tick_count;
    Super::Tick(DeltaSeconds);
}

void ASimModeBase::reset()
{
    //Should be overridden by derived classes
}

VehiclePawnWrapper* ASimModeBase::getFpvVehiclePawnWrapper()
{
    //Should be overridden by derived classes
    return nullptr;
}


std::string ASimModeBase::getReport()
{
    static const std::string empty_string = std::string();
    //Should be overridden by derived classes
    return empty_string;
}

void ASimModeBase::setupInputBindings()
{
    UAirBlueprintLib::EnableInput(this);
}

bool ASimModeBase::isRecording()
{
    return recording_file_->isRecording();
}

bool ASimModeBase::isRecordUIVisible()
{
    return is_record_ui_visible;
}

ECameraDirectorMode ASimModeBase::getInitialViewMode()
{
    return initial_view_mode;
}

void ASimModeBase::startRecording()
{
    recording_file_->startRecording();
}

bool ASimModeBase::toggleRecording()
{
    if (isRecording())
        stopRecording();
    else
        startRecording();

    return isRecording();
}

void ASimModeBase::stopRecording()
{
    recording_file_->stopRecording();
}

RecordingFile& ASimModeBase::getRecordingFile()
{
    return *recording_file_;
}