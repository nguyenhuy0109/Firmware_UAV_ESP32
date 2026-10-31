#pragma once

#include <cstdint>
#include <cstddef>

enum class Mode : uint8_t
{
    IDLE = 0,
    RUN,
    STOP,
    FAULT,
    COUNT
};

enum class State : uint8_t
{
    CHECK_IMU = 0,
    HOLD,
    TAKEOFF,
    MANUAL,
    ALTITUDE,
    MISSION,
    ACRO,
    FOLLOW_TARGET,
    IDLE_STOP,
    AUTO_LAND,
    HOME_LAND,
    IDLE_FAULT,
    LAND_FORCE,
    EMERGENCY,
    COUNT
};

enum class FlightEvent : uint8_t
{
    NONE = 0,
    // Mode-level
    START,              // IDLE -> RUN
    ERROR_DETECTED,     // IDLE/RUN/STOP -> FAULT
    STOP_REQUEST,       // RUN -> STOP
    STOP_DONE,          // STOP -> IDLE
    FAULT_CLEARED,      // FAULT -> IDLE

    // State-level trong RUN
    CMD_HOLD,
    CMD_TAKEOFF,
    CMD_MANUAL,
    CMD_ALT,
    CMD_MISSION,
    CMD_ACRO,
    CMD_FOLLOW_TARGET,

    // State-level trong STOP
    CMD_AUTO_LAND,
    CMD_HOME_LAND,

    // State-level trong FAULT
    CMD_LAND_FORCE,
    CMD_EMERGENCY,

    COUNT
};

struct FlightContext
{
    bool imu_ok = false;
    bool auto_land = false;
    bool home_land = false;
    bool landed = false;
};

class StateMachine;

struct Flight_Mode_t
{
    Mode id;
    State initial;
};

struct Flight_State_t
{
    State id;
    Mode parent;
    void(*StateEntry)(StateMachine&);
    void(*StateDo)(StateMachine&);
    void(*StateExit)(StateMachine&);
};

struct ModeTransition
{
    Mode from;
    FlightEvent event;
    Mode to;
    bool(*GuardFcn)(const StateMachine&);
};

class StateMachine
{
    public:
    static StateMachine& getInstance(){
        static StateMachine instance;
        return instance;
    }

    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(StateMachine&&) = delete;

    // Reset Handler
    void start();

    // Run on/do of current state
    void update();

    // Push event to state machine. Return true if change status
    bool dispatch(FlightEvent ev);

    Mode mode() const { return mode_; }
    State state() const { return state_; }

    FlightContext& ctx() { return ctx_; }
    const FlightContext& ctx() const { return ctx_; }

    private:
    StateMachine() = default;
    ~StateMachine() = default;

    Mode mode_ = Mode::IDLE;
    State state_ = State::CHECK_IMU;
    FlightContext ctx_{};

};