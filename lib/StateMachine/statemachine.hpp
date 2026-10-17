#pragma once

#include <cstdint>
#include <array>

enum class State_Trigger : uint8_t
{
    NONE = 0,
    NO_ERROR,
    ERROR,
    COUNT
};

enum class Mode_Trigger : uint8_t
{
    NONE = 0,
    ARM,
    DISARM,
    TAKEOFF,
    LAND,
    EMC,
    COUNT
};

/**
* @details Define Mode/State name
*/
enum class FlightMode : uint8_t
{
    HOLD = 0,
    TAKEOFF,
    ALTITUDE,
    POSITION,
    ACRO,
    COUNT
};

enum class FlightState : uint8_t
{
    IDLE = 0,
    RUN,
    STOP,
    ERROR
};
/*
* @details Define struct State/Mode
*/

typedef struct
{
    const FlightMode mode_id_;
    void(*ModeEntry);
    void(*ModeDo);
    const uint16_t tick;
    const Flight_Mode_t* NextMode[static_cast<size_t>(Mode_Trigger::COUNT)];
} Flight_Mode_t;

typedef struct
{
    const FlightState state_id_;
    void (*StateEntry);
    void (*StateDo);
    const uint16_t tick;
    const Flight_State_t* NextState[static_cast<size_t>(State_Trigger::COUNT)];
} FLight_State_t;

class StateMachine
{
    public:
    static StateMachine& getInstance(){
        static StateMachine instance;
        return instance;
    }

    // Singleton delete function
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(StateMachine&&) = delete;

    /*
    * @details Get status of the vehicles
    */
    FlightMode getCurrentMode() const { return current_mode_->mode_id_; }
    FlightState getCurrentState() const { return current_state_->state_id_; }
    bool isRunning() const { return running_; }

    /*
    * @details Peripherals read/write function
    * @params IMU, MOTOR, WIFI, BLUETOOTH
    */
    void enableMotors();
    void disableMotors();
    void readIMU();
    void enableWiFi();
    void enableBluetooth();

    private:
    StateMachine() = default;
    ~StateMachine() = default;

    const Flight_Mode_t *current_mode_;
    const Flight_State_t *current_state_;
    uint16_t timeout_;
    uint8_t error_;
    bool running_;
};
