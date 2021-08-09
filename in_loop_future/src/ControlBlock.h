#pragma once
#include <bits/stdc++.h>

// IN PROGRESS: DIED state
enum class State {
    // newcomer, mengxin
    NEW,
    // value has been set, and then()s have not been called
    READY,
    // set value and call then_
    // but it is possible to add future.then() after promise.setValue()
    // in this case, the routine will go on and the state keeps DONE
    // note that value may be moved, please use T reference
    DONE,
    // called by future.get()
    // value is moved
    // this state cannot be used
    // (sub)task will be break, but you should not .get() while using .then()
    DIED,
    // TODO remove
    TIMEOUT,
    // (sub)task will be break
    CANCEL,
};

// shared
// it may be a simple object without shared_ptr (in looper)
template <typename T>
struct ControlBlock {
    State                    _state;
    T                        _value;
    std::exception_ptr       _exception;
    std::function<void(T&&)> _then;

    ControlBlock(): _state(State::NEW) {}
};
