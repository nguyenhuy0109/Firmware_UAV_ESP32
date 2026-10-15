#pragma once

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

}