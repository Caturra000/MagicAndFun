#pragma once
#include <bits/stdc++.h>
#include "SimpleLooper.h"
#include "ControlBlock.h"

template <typename T>
class Future;

template <typename T>
class Promise {
public:
    Promise(SimpleLooper *looper)
        : _looper(looper),
          _shared(std::make_shared<ControlBlock<T>>()) {}

    void setValue(T value) {
        if(_shared->_state == State::NEW) {
            _shared->_value = std::move(value);
            _shared->_state = State::READY;
        } else {
            throw std::runtime_error("promise can only set once.");
        }

        if(_shared->_then) {
            _looper->addEvent([shared = _shared] {
                // shared->_value may be moved
                shared->_then(static_cast<T&&>(shared->_value));
                shared->_state = State::DONE;
            });
        }
    }

    // void setException()...

    void setCallback(std::function<void(T&&)> f) {
        _shared->_then = std::move(f);
    }

    Future<T> get() {
        return Future<T>(_looper, _shared);
    };

private:
    SimpleLooper                     *_looper;
    std::shared_ptr<ControlBlock<T>> _shared;
};