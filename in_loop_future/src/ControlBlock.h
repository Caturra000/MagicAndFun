#pragma once
#include <bits/stdc++.h>

enum class State {
    NEW,
    READY,
    DONE,
    TIMEOUT,
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
