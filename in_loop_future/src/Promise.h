#pragma once
#include <bits/stdc++.h>
#include "SimpleLooper.h"
#include "ControlBlock.h"

template <typename T>
class Future;

// NOTE:
// Promise is not copyable, but we need default const& operation
// For the implementation, the moved capture lambda cannot be stored in std::function
template <typename T>
class Promise {
public:
    Promise(SimpleLooper *looper)
        : _looper(looper),
          _shared(std::make_shared<ControlBlock<T>>()) {}

    // T_: a forward type of T, just reuse the code
    // case:
    // - T / T& : value copies to _state, if has _then, force cast (not actually moved, depends on then() argument)
    // - T&& : value moves to _state, if has _then, force cast (not actually moved)
    template <typename T_>
    void setValue(T_ &&value) {
        if(_shared->_state == State::NEW) {
            _shared->_value = std::forward<T_>(value);
            _shared->_state = State::READY;
            if(_shared->_then) {
                postRequest();
            }
        } else if(_shared->_state == State::CANCEL) {
            // ignore
            return;
        } else {
            throw std::runtime_error("promise can only set once.");
        }
    }

    // void setException()...

    void setCallback(std::function<void(T&&)> f) {
        _shared->_then = std::move(f);
    }

    void cancel() {
        _shared->_state = State::CANCEL;
    }

    Future<T> get() {
        return Future<T>(_looper, _shared);
    };

private:
    // ensure: NEW before setValue() or READY
    void postRequest() {
        _shared->_state = State::POST;
        _looper->addEvent([shared = _shared] {
            // shared->_value may be moved
            // _then must be T&&
            shared->_then(static_cast<T&&>(shared->_value));
            shared->_state = State::DONE;
        });
    }

private:
    SimpleLooper                     *_looper;
    std::shared_ptr<ControlBlock<T>> _shared;
};