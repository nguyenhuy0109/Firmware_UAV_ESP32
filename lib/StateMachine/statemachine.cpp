#include "statemachine.hpp"

static void CheckImu_Do(StateMachine& sm)
{
    if (sm.ctx().imu_ok) { sm.dispatch(FlightEvent::START); }
}

static void Hold_Entry(StateMachine&)
{

}

static void Hold_Do(StateMachine&)
{

}

static void Takeoff_Do(StateMachine&)
{

}

static void Manual_Do(StateMachine&)
{

}

static void Alt_Do(StateMachine&)
{

}

static void Mission_Do(StateMachine&)
{

}

static void Acro_Do(StateMachine&)
{

}

static void Follow_Do(StateMachine&)
{

}

static void IdleStop_Do(StateMachine& sm)
{
    if (sm.ctx().auto_land) { sm.dispatch(FlightEvent::CMD_AUTO_LAND); }
    else if (sm.ctx().home_land) { sm.dispatch(FlightEvent::CMD_HOME_LAND); }
}

static void AutoLand_Do(StateMachine& sm)
{
    if (sm.ctx().landed) { sm.dispatch(FlightEvent::STOP_DONE); }
}

static void HomeLand_Do(StateMachine& sm)
{
    if (sm.ctx().landed) { sm.dispatch(FlightEvent::STOP_DONE); }
}

static void IdleFault_Do(StateMachine&)
{

}

static void LandForce_Do(StateMachine& sm)
{
    if (sm.ctx().landed) { sm.dispatch(FlightEvent::FAULT_CLEARED); }
}

static void Emergency_Do(StateMachine&)
{

}

static bool Guard_ImuOk(const StateMachine& sm) { return sm.ctx().imu_ok; }
static bool Guard_AutoLand(const StateMachine& sm) { return sm.ctx().auto_land; }
static bool Guard_HomeLand(const StateMachine& sm) { return sm.ctx().home_land; }

/**
 * @brief State table
 * @param State : the state's offset depends on the enum type Flight_State_t
 */
static constexpr Flight_State_t kStates[] = 
{   //  ||          State             ||      Mode Parent     ||      Entry Function      ||      Do Function     ||      Exit Function       ||
    {           State::CHECK_IMU        ,       Mode::IDLE      ,           nullptr         ,       CheckImu_Do     ,           nullptr         },
    {           State::HOLD             ,       Mode::RUN       ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::TAKEOFF          ,       Mode::RUN       ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::MANUAL           ,       Mode::RUN       ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::ALTITUDE         ,       Mode::RUN       ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::MISSION          ,       Mode::RUN       ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::ACRO             ,       Mode::RUN       ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::FOLLOW_TARGET    ,       Mode::RUN       ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::IDLE_STOP        ,       Mode::STOP      ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::AUTO_LAND        ,       Mode::STOP      ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::HOME_LAND        ,       Mode::STOP      ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::IDLE_FAULT       ,       Mode::FAULT     ,           nullptr         ,       IdleFault_Do    ,           nullptr         },
    {           State::LAND_FORCE       ,       Mode::FAULT     ,           nullptr         ,       nullptr         ,           nullptr         },
    {           State::EMERGENCY        ,       Mode::FAULT     ,           nullptr         ,       nullptr         ,           nullptr         }
};