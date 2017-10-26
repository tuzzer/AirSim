// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef air_RpcLibClientBase_hpp
#define air_RpcLibClientBase_hpp

#include "common/Common.hpp"
#include "common/CommonStructs.hpp"
#include "controllers/VehicleCameraBase.hpp"


namespace msr { namespace airlib {

class RpcLibClientBase {
public:
    enum class ConnectionState : uint {
        Initial = 0, Connected, Disconnected, Reset, Unknown
    };
public:
    RpcLibClientBase(const string& ip_address = "localhost", uint16_t port = 42451, uint timeout_ms = 60000);
    ConnectionState getConnectionState();
    bool ping();

    vector<VehicleCameraBase::ImageResponse> simGetImages(vector<VehicleCameraBase::ImageRequest> request);
    vector<uint8_t> simGetImage(int camera_id, VehicleCameraBase::ImageType type);
    msr::airlib::GeoPoint getHomeGeoPoint();

    void simSetPose(const Pose& pose, bool ignore_collison);
    Pose simGetPose();

    void confirmConnection();
    bool isApiControlEnabled();
    void enableApiControl(bool is_enabled);
    void reset();

    virtual ~RpcLibClientBase();    //required for pimpl

protected:
    void* getClient();

private:
    struct impl;
    std::unique_ptr<impl> pimpl_;
};

}} //namespace
#endif
