#pragma once
#include <bits/stdc++.h>

// IN PROGRESS: DIED state
enum class State {
    // newcomer, mengxin
    // it may has _then
    NEW,
    // value has been set
    // it may has _then
    READY,
    // then() has been posted as a request in loop
    POST,
    // request is done
    // value may be moved
    DONE,
    // called by future.get()
    // value is moved
    // this state cannot be reused
    // (sub)task will be break, but you should not .get() while using .then()
    DIED,
    // (sub)task will be break
    CANCEL,
};

// shared
// it may be a simple object without shared_ptr (in looper)
template <typename T>
struct ControlBlock {
    State                    _state;
    T                        _value;
    // std::exception_ptr       _exception;
    std::function<void(T&&)> _then;

    ControlBlock(): _state(State::NEW) {}
};
