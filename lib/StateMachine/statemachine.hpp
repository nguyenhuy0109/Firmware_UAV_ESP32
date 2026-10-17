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

/*
* @details Define struct State/Mode
*/

typedef struct
{
    void(*ModeEntry);
    void(*ModeDo);
    const uint16_t tick;
    const Flight_Mode_t* NextMode[static_cast<size_t>(Mode_Trigger::COUNT)];
} Flight_Mode_t;

typedef struct
{
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

    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    StateMachine(StateMachine&&) = delete;
    StateMachine& operator=(StateMachine&&) = delete;

    private:
    StateMachine() = default;
    ~StateMachine() = default;

};
