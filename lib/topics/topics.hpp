#pragma once

#include "orb_topic.hpp"

enum class AutoSubMode : uint8_t
{
    NONE = 0,
    HOLD,
    TAKEOFF,
    MISSION,
    FOLLOW_TARGET,
    AUTO_LAND,
    HOME_LAND,
    LAND_FORCE
};

struct ControlFlagsMsg
{
    bool armed = false;
    bool enable_rate = false;
    bool enable_attitude = false;
    bool enable_altitude = false;
    bool enable_position = false;
    AutoSubMode auto_mode = AutoSubMode::NONE;
};

/**
 * @brief Imu Data
 */
struct ImuData
{
    uint64_t timestamp_us;

    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;
};


/**
 * @brief Gps Data
 * @include Position, Altitude
 */
struct GpsData
{

};


/**
 * @brief Vehicle Position
 * @include X, Y, Z, Yaw
 */
struct VehiclePose
{
    uint64_t timestamp_us;

    float x;
    float y;
    float z;
    float yaw;
};

extern OrbTopic<ControlFlagsMsg> g_control_flags_topic;
extern OrbTopic<ImuData> imu_topic;
extern OrbTopic<GpsData> gps_topic;
extern OrbTopic<VehiclePose> vehicle_pose;