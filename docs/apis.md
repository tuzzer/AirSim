#AirSim APIs
## Introduction
AirSim exposes APIs so you can interact with vehicle in the simulation programmatically. You can use these APIs to retrieve images, get state, control the vehicle and so on. The APIs use [msgpack-rpc protocol](https://github.com/msgpack-rpc/msgpack-rpc) over TCP/IP which allows you to use variety of programming languages including C++, C#, Python, Java etc.

## Hello Drone
Here's very quick overview of how to use AirSim APIs using C++ (see also [Python example](python.md)):

```
#include <iostream>
#include "vehicles/multirotor/api/MultirotorRpcLibClient.hpp"

int main() 
{
    using namespace std;
    msr::airlib::MultirotorRpcLibClient client;

    cout << "Press Enter to enable API control" << endl; cin.get();
    client.enableApiControl(true);

    cout << "Press Enter to arm the drone" << endl; cin.get();
    client.armDisarm(true);

    cout << "Press Enter to takeoff" << endl; cin.get();
    client.takeoff(5);

    cout << "Press Enter to move 5 meters in x direction with 1 m/s velocity" << endl; cin.get();  
    auto position = client.getPosition(); // from current location
    client.moveToPosition(position.x() + 5, position.y(), position.z(), 1);

    cout << "Press Enter to land" << endl; cin.get();
    client.land();

    return 0;
}

```

You can find a ready to run project in HelloDrone folder in the repository. See more about [Hello Drone](hello_drone.md).

## Hello Car
First make sure you have below in [settings.json](settings.md). This is because the default setting is to use multirotor.

```
{
  "SettingsVersion": 1.0,
  "SimMode": "Car"
}
```

Here's how to use AirSim APIs using C++ (see also [Python example](python.md)):

```
#include <iostream>
#include "vehicles/car/api/CarRpcLibClient.hpp"

int main() 
{
    msr::airlib::CarRpcLibClient client;
    client.enableApiControl(true); //this disables manual control
    CarControllerBase::CarControls controls;

    std::cout << "Press enter to drive forward" << std::endl; std::cin.get();
    controls.throttle = 1;
    client.setCarControls(controls);

    std::cout << "Press Enter to activate handbrake" << std::endl; std::cin.get();
    controls.handbrake = true;
    client.setCarControls(controls);

    std::cout << "Press Enter to take turn and drive backward" << std::endl; std::cin.get();
    controls.handbrake = false;
    controls.throttle = -1;
    controls.steering = 1;
    client.setCarControls(controls);

    std::cout << "Press Enter to stop" << std::endl; std::cin.get();
    client.setCarControls(CarControllerBase::CarControls());

    return 0;
}

```

You can find a ready to run project in HelloCar folder in the repository.

## Image / Computer Vision and Collision APIs
AirSim offers comprehensive images APIs to retrieve synchronized images from multiple cameras along with ground truth including depth, disparity, surface normals and vision. You can set the resolution, FOV, motion blur etc parameters in [settings.json](settings.md). There is also API for detecting collision state. See also [complete code](../Examples/StereoImageGenerator.hpp) that generates specified number of stereo images and ground truth depth with normalization to camera plan, computation of disparity image and saving it to [pfm format](pfm.md).

More on [image APIs and Computer Vision mode](image_apis.md).

### Common APIs

* `reset`: This resets the vehicle to its original starting state.
* `confirmConnection`: Checks state of connection every 1 sec and reports it in Console so user can see the progress for connection.
* `enableApiControl`: For safety reasons, by default API control for autonomous vehicle is not enabled and human operator has full control (usually via RC or joystick in simulator). The client must make this call to request control via API. It is likely that human operator of vehicle might have disallowed API control which would mean that enableApiControl has no effect. This can be checked by `isApiControlEnabled`.
* `isApiControlEnabled`: Returns true if API control is established. If false (which is default) then API calls would be ignored. After a successful call to `enableApiControl`, the `isApiControlEnabled` should return true.
* `ping`: If connection is established then this call will return true otherwise it will be blocked until timeout.

### APIs for Car
Car has followings APIs available:

* `setCarControls`: This allows you to set throttle, steering, handbrake and auto or manual gear.
* `getCarState`: This retrieves the state information including speed, current gear, velocity vector, position and orientation.
* [Image APIs](image_apis.md).

### APIs for Multirotor
Multirotor can be controlled by specifying angles, velocity vector, destination position or some combination of these. There are corresponding `move*` APIs for this purpose. When doing position control, we need to use some path following algorithm. By default AirSim uses carrot following algorithm. This is often referred to as "high level control" because you just need to specify very high level goal and the firmware takes care of the rest. Currently lowest level control available in AirSim is moveByAngle API however we will be adding more lower level controls soon as well.

#### duration and max_wait_seconds
Many API methods has parameters named `duration` or: `max_wait_seconds`.

Methods that take `float duration`, like `moveByVelocity`y return control immediately. So you can therefore choose to sleep for this duration, or you can change their mind and call something else which will automatically cancel the `moveByVelocity`.

Methods that take `float max_wait_seconds`, like `takeoff`, `land`, `moveOnPath`, `moveToPosition`, `moveToZ`, and so will block this amount of time waiting for command to be successfully completed. If the command completes before the max_wait_seconds they will return True, otherwise
if the `max_wait_seconds` times out they will return `false`.  If you want to wait for ever pass a big number. But if you want to be able to interrupt even these commands pass 0 and you can do something else or sleep in a loop while checking the drone position, etc. We would not recommend interrupting takeoff/land on a real drone, of course, as the results may be unpredictable.

#### drivetrain
There are two modes you can fly vehicle: `Drivetrain = ForwardOnly` and `Drivetrain = MaxDegreeOfFreedom`. When you specify ForwardOnly, you are saying that vehicle's front should always point in the direction of travel. So if you want drone to take left turn then it would first rotate so front points to left. This mode is useful when you have only front camera and you are operating vehicle using FPV view. This is more or less like travelling in car where you always have front view. The MaxDegreeOfFreedom means you don't care where the front points to. So when you take left turn, you just start going left like crab. Quadrotors can go in any direction regardless of where front points to. The MaxDegreeOfFreedom enables this mode.

#### yaw_mode
`yaw_mode` has two fields, `yaw_or_rate` and `is_rate`. If is_rate = true then yaw_or_rate is interpreted as angular velocity in degrees/sec which means you want vehicle to rotate continuously around its axis at that angular velocity while moving. If is_rate = false then yaw_or_rate is interpreted as angle in degrees which means you want vehicle to rotate to specific angle (i.e. yaw) and keep that angle while moving. 

You can probably see that YawMode::is_rate = true is pointless in ForwardOnly mode because you are contradicting by saying that keep front pointing ahead but also rotate continuously. However if you have YawMode::is_rate = false in ForwardOnly then you can do some funky stuff. For example, you can have drone do circles and have YawMode::yaw_or_rate = 90 so camera is always pointed to center ("super cool selfie mode"). In MaxDegreeofFreedom, you can get some funky stuff by setting YawMode::is_rate = true and say YawMode::yaw_or_rate = 20. This will cause drone to go in its path while rotating which may allow to do 360 scanning.

In most cases, you just don't want yaw to change which you can do by setting yaw rate of 0. The shothand for this is `YawMode::Zero()`. 

#### lookahead and adaptive_lookahead
When you ask vehicle to follow a path, AirSim uses "carrot following" algorithm. This algorithm operates by looking ahead on path and adjusting its velocity vector. The parameters for this algorithm is specified by `lookahead` and `adaptive_lookahead`. For most of the time you want algorithm to auto-decide the values by simply setting `lookahead = -1` and `adaptive_lookahead = 0`.

## Using APIs on Real Vehicles
We want to be able to run *same code* that runs in simulation as on real vehicle. This allows you to test your code in simulator and deploy to real vehicle. 

Generally speaking, APIs therefore shouldn't allow you to do something that cannot be done on real vehicle (for example, getting the ground truth). But, of course, simulator has much more information and it would be useful in applications that may not care about running things on real vehicle. For this reason, we clearly delineate between sim-only APIs by attaching `sim` prefix and we recommend that you don't use these APIs if you do care about real vehicle deployment.

The AirLib is self-contained library that you can put on an offboard computing module such as the Gigabyte barebone Mini PC. This module then can talk to the flight controllers such as PX4 using exact same code and flight controller protocol. The code you write for testing in the simulator remains unchanged. See [AirLib on custom drones](/custom_drone.md).


## References and Examples

* AirSim APIs using [Python](python.md)
* [move on path](https://github.com/Microsoft/AirSim/wiki/moveOnPath-demo) demo showing video of fast multirotor flight through Modular Neighborhood environment
* [building a hexacopter](https://github.com/Microsoft/AirSim/wiki/hexacopter)
* [building point clouds](https://github.com/Microsoft/AirSim/wiki/Point-Clouds)