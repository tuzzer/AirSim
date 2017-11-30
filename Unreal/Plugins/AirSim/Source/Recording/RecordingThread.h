#pragma once

#include "CoreMinimal.h"
#include "AirBlueprintLib.h"
#include "HAL/Runnable.h"
#include "VehicleCameraConnector.h"
#include "VehiclePawnWrapper.h"
#include "Recording/RecordingFile.h"
#include "physics/Kinematics.hpp"
#include "Recording/RecordingSettings.h"
#include <memory>
#include "common/ClockFactory.hpp"

class FRecordingThread : public FRunnable
{
public:
    FRecordingThread();
    virtual ~FRecordingThread();
    static void startRecording(std::vector<msr::airlib::VehicleCameraBase*> cameras, std::vector<int> image_type_ids, const msr::airlib::Kinematics::State* kinematics, const RecordingSettings& settings, std::vector <std::string> columns, VehiclePawnWrapper* wrapper);
    static void stopRecording(); 
    static bool isRecording();

private:
    virtual bool Init();
    virtual uint32 Run();
    virtual void Stop();
    virtual void Exit();


    void EnsureCompletion();

private:
    FThreadSafeCounter stop_task_counter_;
    FRenderCommandFence read_pixel_fence_;
    
    static std::unique_ptr<FRecordingThread> instance_;

    std::unique_ptr<FRunnableThread> thread_;

    RecordingSettings settings_;
    int img_count = 0;
    std::vector<msr::airlib::VehicleCameraBase*> cameras_;
    std::vector<int> image_type_ids_;
    std::unique_ptr<RecordingFile> recording_file_;
    const msr::airlib::Kinematics::State* kinematics_;
    VehiclePawnWrapper* wrapper_;

    msr::airlib::TTimePoint last_screenshot_on_;
    msr::airlib::Pose last_pose_;

    bool is_ready_;
};