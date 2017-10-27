#include "SimModeBase.h"
#include "Misc/MessageDialog.h"
#include "Misc/EngineVersion.h"
#include "AirBlueprintLib.h"
#include "Runtime/Launch/Resources/Version.h"
#include "controllers/Settings.hpp"
#include "Recording/RecordingThread.h"
#include "SimJoyStick/SimJoyStick.h"

ASimModeBase::ASimModeBase()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ASimModeBase::BeginPlay()
{
    Super::BeginPlay();

    try {
        readSettings();
    }
    catch (std::exception& ex) {
        UAirBlueprintLib::LogMessageString("Error occured while reading the Settings: ", ex.what(), LogDebugLevel::Failure);
    }

    if (clock_speed != 1.0f) {
        this->GetWorldSettings()->SetTimeDilation(clock_speed);
        UAirBlueprintLib::LogMessageString("Clock Speed: ", std::to_string(clock_speed), LogDebugLevel::Informational);
    }

    setStencilIDs();

    record_tick_count = 0;
    setupInputBindings();

    UAirBlueprintLib::LogMessage(TEXT("Press F1 to see help"), TEXT(""), LogDebugLevel::Informational);
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
    FRecordingThread::stopRecording();
    Super::EndPlay(EndPlayReason);
}

void ASimModeBase::readSettings()
{
    //set defaults in case exception occurs
    is_record_ui_visible = false;
    initial_view_mode = ECameraDirectorMode::CAMERA_DIRECTOR_MODE_FLY_WITH_ME;
    enable_rpc = false;
    api_server_address = "";
    default_vehicle_config = "";
    physics_engine_name = "";
    usage_scenario = "";
    enable_collision_passthrough = false;
    clock_type = "";
    engine_sound = true;
    clock_speed = 1.0f;

    typedef msr::airlib::Settings Settings;

    Settings& settings = Settings::singleton();

    //we had spelling mistake so we are currently supporting SettingsVersion or SettingdVersion :(
    settings_version_actual = settings.getFloat("SettingsVersion", settings.getFloat("SettingdVersion", 0));

    if (settings_version_actual < settings_version_minimum) {
        if ((settings.size() == 1 && 
            ((settings.getString("SeeDocsAt", "") != "") || settings.getString("see_docs_at", "") != ""))
            || (settings.size() == 0)) {
            //no warnings because we have default settings
        }
        else {
            UAirBlueprintLib::LogMessageString("Your settings file does not have SettingsVersion element."," This probably means you have old format settings file.", LogDebugLevel::Failure);
            UAirBlueprintLib::LogMessageString("Please look at new settings and update your settings.json: ","https://git.io/v9mYY", LogDebugLevel::Failure);
        }
    }

    std::string simmode_name = settings.getString("SimMode", "");
    if (simmode_name == "")
        simmode_name = "Multirotor";

    usage_scenario = settings.getString("UsageScenario", "");
    default_vehicle_config = settings.getString("DefaultVehicleConfig", "");
    if (default_vehicle_config == "") {
        if (simmode_name == "Multirotor")
            default_vehicle_config = "SimpleFlight";
        else if (simmode_name == "Car")
            default_vehicle_config = "PhysXCar4x4";
        else         
            UAirBlueprintLib::LogMessageString("SimMode is not valid: ", simmode_name, LogDebugLevel::Failure);
    }

    enable_rpc = settings.getBool("RpcEnabled", true);
    //by default we spawn server at local endpoint. Do not use 127.0.0.1 as default below
    //because for docker container default is 0.0.0.0 and people get really confused why things
    //don't work
    api_server_address = settings.getString("LocalHostIp", "");
    is_record_ui_visible = settings.getBool("RecordUIVisible", true);
    engine_sound = settings.getBool("EngineSound", false);

    std::string view_mode_string = settings.getString("ViewMode", "");

    if (view_mode_string == "") {
        if (usage_scenario == "") {
            if (simmode_name == "Multirotor")
                view_mode_string = "FlyWithMe";
            else
                view_mode_string = "SpringArmChase";
        }
        else
            view_mode_string = "SpringArmChase";
    }

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
        
    physics_engine_name = settings.getString("PhysicsEngineName", "");
    if (physics_engine_name == "") {
        if (simmode_name == "Multirotor")
            physics_engine_name = "FastPhysicsEngine";
        else
            physics_engine_name = "PhysX";
    }

    enable_collision_passthrough = settings.getBool("EnableCollisionPassthrogh", false);
    clock_type = settings.getString("ClockType", "");

    if (clock_type == "") {
        if (default_vehicle_config == "SimpleFlight")
            clock_type = "SteppableClock";
        else
            clock_type = "ScalableClock";
    }

    clock_speed = settings.getFloat("ClockSpeed", 1.0f);

    char* cameras_ids = strdup(settings.getString("RecordWithCameras", "0").c_str());
    char* tempBuff = strtok(cameras_ids, " ,;/");
    while (tempBuff != nullptr)
    {
        const int camera_id = atoi(tempBuff);
        record_with_cameras.insert(camera_id); // There might be invalid camera ids at this point
        tempBuff = strtok(nullptr, " ,;/");
    }
    free(cameras_ids);

    char* image_type_ids = strdup(settings.getString("RecordImageTypes", "0").c_str());
    tempBuff = strtok(image_type_ids, " ,;/");
    while (tempBuff != nullptr)
    {
        const int image_type_id = atoi(tempBuff);
        record_image_types.insert(image_type_id); // There might be invalid image type ids at this point
        tempBuff = strtok(nullptr, " ,;/");
    }
    free(image_type_ids);

    Settings record_settings;
    if (settings.getChild("Recording", record_settings)) {
        recording_settings.record_on_move = record_settings.getBool("RecordOnMove", recording_settings.record_on_move);
        recording_settings.record_interval = record_settings.getFloat("RecordInterval", recording_settings.record_interval);
    }
    
    UAirBlueprintLib::LogMessage("Default config: ", default_vehicle_config.c_str(), LogDebugLevel::Informational);

    // By default this is the column header. Override it in BeginPlay of pawn mode
    columns = { "Timestamp", "Position(x)", "Position(y)" , "Position(z)", "Orientation(w)", "Orientation(x)", "Orientation(y)", "Orientation(z)", "ImageName" };
}

void ASimModeBase::Tick(float DeltaSeconds)
{
    if (isRecording())
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

    UAirBlueprintLib::BindActionToKey("InputEventResetAll", EKeys::BackSpace, this, &ASimModeBase::reset);
}

bool ASimModeBase::isRecording()
{
    return FRecordingThread::isRecording();
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
    if (getFpvVehiclePawnWrapper() != nullptr)
    {
        // Create a vector of valid cameras
        const int camera_count = getFpvVehiclePawnWrapper()->getCameraCount();
        std::vector<msr::airlib::VehicleCameraBase*> cameras(camera_count);

        for (auto itr = record_with_cameras.begin(); itr != record_with_cameras.end(); ++itr)
        {
            if (*itr >= 0 && *itr < camera_count)
            {
                cameras[*itr] = getFpvVehiclePawnWrapper()->getCameraConnector(*itr);
            }
        }

        // Create a vector of valid image types
        const int image_type_count = static_cast<int> (msr::airlib::VehicleCameraBase::ImageType::Count);
        std::vector<int> image_type_ids;
        for (auto itr = record_image_types.begin(); itr != record_image_types.end(); ++itr)
        {
            if (*itr >= 0 && *itr < image_type_count)
            {
                image_type_ids.push_back(*itr);
            }
        }

        FRecordingThread::startRecording(cameras, image_type_ids,
            getFpvVehiclePawnWrapper()->getKinematics(), recording_settings, columns);
    }
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
    FRecordingThread::stopRecording();
}

