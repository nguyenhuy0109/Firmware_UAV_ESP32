#include "topics.hpp"


OrbTopic<ControlFlagsMsg> g_control_flags_topic;
OrbTopic<ImuData> imu_topic;
OrbTopic<GpsData> gps_topic;
OrbTopic<VehiclePose> vehicle_pose;