#pragma once

#include <cstdint>
#include <functional>
#include <array>


/**
 * @details Flight State of quadrotor
 * This file will define the state of quadrotor and the function in FlightStateMachine class
 * @author Nguyen Huy
 */

// Define the flight mode
enum class FlightMode : uint8_t
{
   NORMAL = 0,
   ERROR,
   COUNT
};

// Define the flight states
enum class FlightState : uint8_t
{
   INIT = 0,
   CALIBRATION,
   DISARMED,
   ARMED,
   TAKEOFF,
   HOVER,
   MANUAL,
   LANDING,
   FAILSAFE,
   ERROR,
   COUNT   // The number of state, use to initialize array of states
};

// Define the events
enum class FlightEvent : uint8_t
{
   NONE = 0,
   INIT_DONE,
   CALIB,
   ARM_REQUEST,
   DISARM_REQUEST,
   TAKEOFF_REQUEST,
   LAND_REQUEST,
   HOVER_REQUEST,
   MANUAL_REQUEST,
   FAILSAFE_TRIGGER,
   ERROR_TRIGGER,
   RESET,
   COUNT
};

// Define the value after solve event
enum class StateResult : uint8_t
{
   HANDLED,
   IGNORED,
   TRANSITION
};

// Forward declaration
struct SystemCtx_t;
struct FlightState_t;
struct FlightMode_t;


// Low-Level State
struct FlightState_t
{
   void (*State_Entry)(SystemCtx_t*);
   void (*State_Do)(SystemCtx_t*);
   const uint16_t tick;          // Timeout tick
   const FlightState_t* NextState[static_cast<size_t>(FlightEvent::COUNT)];
};


// Mode
struct FlightMode_t
{
   void (*Mode_Entry)(SystemCtx_t*);
   void (*Mode_Do)(SystemCtx_t*);
   const uint16_t tick;          // Timeout tick
   const FlightMode_t* NextMode[static_cast<size_t>(FlightEvent::COUNT)];
};


class FlightStateMachine
{
   private:
   FlightStateMachine() = default;
   ~FlightStateMachine() = default;

   const FlightMode_t* current_mode_;
   const FlightState_t* current_state_;
   uint16_t timeout_;
   uint8_t error_;
   bool running_;

   static const FlightState_t STATE_IDLE;
   static const FlightState_t STATE_RUN;
   static const FlightState_t STATE_STOP;

   static const FlightMode_t MODE_NORMAL;
   static const FlightMode_t MODE_ERROR;

   public:
   FlightStateMachine(const FlightStateMachine&) = delete;
   FlightStateMachine operator=(const FlightStateMachine&) = delete;

   static FlightStateMachine& getInstance()
   {
      static FlightStateMachine instance;
      return instance;
   }

   // Initialize and Run
   FlightMode getCurrentMode() const;

   // Callback when begin the state
   using EnterCallback = std::function<void()>;
    
   // Callback when end the state
   using ExitCallback = std::function<void()>;

   // Callback solve event in the state
   using EventCallback = std::function<void()>;

   // Callback update loop in the state
   using UpdateCallback = std::function<void()>;
};