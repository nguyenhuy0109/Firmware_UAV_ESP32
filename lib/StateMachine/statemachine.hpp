#pragma once

typedef enum
{
    NONE = 0,
    NO_ERROR,
    ERROR,
} State_Change_Trigger;

typedef enum
{
    NONE = 0,
    ARM = 1,

} Mode_Trigger;

typedef struct
{
    void(*Entry);
    void(*Do);
    Flight_Mode_t *current_mode_;
    Flight_Mode_t *next_mode_;
} Flight_Mode_t;

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