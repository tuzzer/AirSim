#include "RecordingFile.h"
#include "HAL/PlatformFilemanager.h"
#include "FileHelper.h"
#include <sstream>
#include "ImageUtils.h"
#include "common/ClockFactory.hpp"
#include "common/common_utils/FileSystem.hpp"


RecordingFile::RecordingFile(std::vector <std::string> columns)
{
    this->columns = columns;
}

void RecordingFile::appendRecord(TArray<uint8>& image_data, const msr::airlib::Kinematics::State* kinematics, std::string dir_name)
{
    if (image_data.Num() == 0)
        return;

    bool imageSavedOk = false;
    FString filePath;

    std::string filename = std::string("img_").append(std::to_string(images_saved_)).append(".png");
    if (dir_name != "")
    {
        filename = common_utils::FileSystem::combine(dir_name, filename);
    }

    try {    
        FString image_file_path = FString(common_utils::FileSystem::combine(image_path_, filename).c_str());
        imageSavedOk = FFileHelper::SaveArrayToFile(image_data, *image_file_path);
    }
    catch(std::exception& ex) {
        UAirBlueprintLib::LogMessage(TEXT("Image file save failed"), FString(ex.what()), LogDebugLevel::Failure);        
    }
    // If render command is complete, save image along with position and orientation

    if (imageSavedOk) {
        writeString(getLine(*kinematics, filename));

        UAirBlueprintLib::LogMessage(TEXT("Screenshot saved to:"), filePath, LogDebugLevel::Success);
        images_saved_++;
    }
}

void RecordingFile::appendColumnHeader(std::vector <std::string> columns)
{
    std::string line;
    for (int i = 0; i < columns.size()-1; i++) 
    {
        line.append(columns[i]).append("\t");
    }
    line.append(columns[columns.size() - 1]).append("\n");

    writeString(line);
}

std::string RecordingFile::getLine(const msr::airlib::Kinematics::State& kinematics, const std::string& image_file_name)
{
    uint64_t timestamp_millis = static_cast<uint64_t>(msr::airlib::ClockFactory::get()->nowNanos() / 1.0E6);

    //TODO: because this bug we are using alternative code with stringstream
    //https://answers.unrealengine.com/questions/664905/unreal-crashes-on-two-lines-of-extremely-simple-st.html

    std::string line;
    line.append(std::to_string(timestamp_millis)).append("\t")
        .append(std::to_string(kinematics.pose.position.x())).append("\t")
        .append(std::to_string(kinematics.pose.position.y())).append("\t")
        .append(std::to_string(kinematics.pose.position.z())).append("\t")
        .append(std::to_string(kinematics.pose.orientation.w())).append("\t")
        .append(std::to_string(kinematics.pose.orientation.x())).append("\t")
        .append(std::to_string(kinematics.pose.orientation.y())).append("\t")
        .append(std::to_string(kinematics.pose.orientation.z())).append("\t")
        .append(image_file_name)
        .append("\n");

    return line;

    //std::stringstream ss;
    //ss << timestamp_millis << "\t";
    //ss << kinematics.pose.position.x() << "\t" << kinematics.pose.position.y() << "\t" << kinematics.pose.position.z() << "\t";
    //ss << kinematics.pose.orientation.w() << "\t" << kinematics.pose.orientation.x() << "\t" << kinematics.pose.orientation.y() << "\t" << kinematics.pose.orientation.z() << "\t";
    //ss << "\n";
    //return ss.str();
}

void RecordingFile::createFile(const std::string& file_path)
{
    try {
        closeFile();

        IPlatformFile& platform_file = FPlatformFileManager::Get().GetPlatformFile();
        log_file_handle_ = platform_file.OpenWrite(*FString(file_path.c_str()));
        appendColumnHeader(this->columns);
    }
    catch(std::exception& ex) {
        UAirBlueprintLib::LogMessageString(std::string("createFile Failed for ") + file_path, ex.what(), LogDebugLevel::Failure);        
    }
}

bool RecordingFile::isFileOpen()
{
    return log_file_handle_ != nullptr;
}

void RecordingFile::closeFile()
{
    if (isFileOpen())
        delete log_file_handle_;

    log_file_handle_ = nullptr;
}

void RecordingFile::writeString(const std::string& str)
{
    try {    
        if (log_file_handle_) {
            FString line_f = FString(str.c_str());
            log_file_handle_->Write((const uint8*)TCHAR_TO_ANSI(*line_f), line_f.Len());
        }
        else
            UAirBlueprintLib::LogMessageString("Attempt to write to recording log file when file was not opened", "", LogDebugLevel::Failure);
    }
    catch(std::exception& ex) {
        UAirBlueprintLib::LogMessageString(std::string("file write to recording file failed "), ex.what(), LogDebugLevel::Failure);        
    }
}

RecordingFile::~RecordingFile()
{
    stopRecording(true);
}

void RecordingFile::startRecording()
{
    try {
        std::string log_folderpath = common_utils::FileSystem::getLogFolderPath(true);
        image_path_ = common_utils::FileSystem::ensureFolder(log_folderpath, "images");
        std::string log_filepath = common_utils::FileSystem::getLogFileNamePath(log_folderpath, record_filename, "", ".txt", false);
        if (log_filepath != "")
            createFile(log_filepath);
        else {
            UAirBlueprintLib::LogMessageString("Cannot start recording because path for log file is not available", "", LogDebugLevel::Failure);
            return;
        }

        if (isFileOpen()) {
            is_recording_ = true;

            UAirBlueprintLib::LogMessage(TEXT("Recording"), TEXT("Started"), LogDebugLevel::Success);
        }
        else
            UAirBlueprintLib::LogMessageString("Error creating log file", log_filepath.c_str(), LogDebugLevel::Failure);
    }
    catch(...) {
        UAirBlueprintLib::LogMessageString("Error in startRecording", "", LogDebugLevel::Failure);
    }
}

void RecordingFile::stopRecording(bool ignore_if_stopped)
{
    is_recording_ = false;
    if (! isFileOpen()) {
        if (ignore_if_stopped)
            return;

        UAirBlueprintLib::LogMessage(TEXT("Recording Error"), TEXT("File was not open"), LogDebugLevel::Failure);
    }
    else
        closeFile();

    UAirBlueprintLib::LogMessage(TEXT("Recording"), TEXT("Stopped"), LogDebugLevel::Success);
}

bool RecordingFile::isRecording()
{
    return is_recording_;
}
