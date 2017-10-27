#pragma once

#include "CoreMinimal.h"
#include <string>
#include "AirBlueprintLib.h"
#include "physics/Kinematics.hpp"
#include "FileManager.h"


class RecordingFile {
public:
    RecordingFile(std::vector <std::string> columns);
    ~RecordingFile();

    void appendRecord(TArray<uint8>& compressedPng, const msr::airlib::Kinematics::State* kinematics, const std::string& camera_name, const std::string& img_type_name, const std::string& img_name);
    void appendColumnHeader(std::vector <std::string> columns);
    void startRecording();
    void stopRecording(bool ignore_if_stopped);
    bool isRecording();

private:
    void createFile(const std::string& file_path);
    void closeFile();
    void writeString(const std::string& line);
    bool isFileOpen();
    std::string getLogFileFullPath();
    std::string getLine(const msr::airlib::Kinematics::State& kinematics, const std::string& image_file_name);


private:
    std::string record_filename = "airsim_rec";     
    std::string image_path_;
    bool is_recording_ = false;
    IFileHandle* log_file_handle_ = nullptr;
    std::vector <std::string> columns;
};