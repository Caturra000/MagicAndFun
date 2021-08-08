#pragma once
#include <bits/stdc++.h>
#include "FunctionTraits.h"
#include "FutureTraits.h"

// [experimental]
// In loop future
// lockfree?


class SimpleLooper {
public:
    // example: for(;;) loop();
    void loop() {
        for(int sz = _mq.size(); sz--;) {
            loopOnce();
        }
    }

    // unsafe
    void loopOnce() {
        debug();
        _lastEvent = std::move(_mq.front());
        _mq.pop();
        _lastEvent();
    }

    void loopOnceChecked() { if(!_mq.empty()) loopOnce(); }

    // TODO
    // receive and return a context object
    //
    // since it is a FAKE yield (will break any result and no context)
    // you should save context by yourself (via lambda reference capture)
    void yield() {
        _mq.emplace(std::move(_lastEvent));
    }

    void addEvent(std::function<void()> event) {
        if(event) {
            _mq.emplace(std::move(event));
        }
    }

private:
    void debug() {
        std::cout << "[loop msg] " << _global++ << std::endl;
    }

private:
    std::queue<std::function<void()>> _mq;
    std::function<void()>             _lastEvent;
    int                               _global {}; // debug
};


enum class State {
    NEW,
    READY,
    DONE,
    TIMEOUT,
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

template <typename T>
class Future;

template <typename T>
class Promise;

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
                // TODO remove std::move, because void(T&&) will not be actually moved, just refer to this value
                shared->_then(std::move(shared->_value));
                shared->_state = State::DONE;
            });
        }
    }

    // for looper.yield
    // use && to avoid copy or move
    void testAndSetValue(T &&value) {
        f(std::move(value));
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



template <typename T>
class Future {
public:
    Future(SimpleLooper *looper, std::shared_ptr<ControlBlock<T>> shared)
        : _looper(looper),
          _shared(shared) {}

    // TIMEOUT?
    bool hasResult() { return _shared->_state != State::NEW; }

    // ensure: has result
    T get() {
        auto value = std::move(_shared->_value);
        _shared->_state = State::DONE;
        return value;
    }

    // unsafe, hook for yield
    // ensure: has result
    // will not change the state
    T& getReference() {
        return _shared->value;
    }

    void setCallback(std::function<void(T&&)> f) {
        _shared->_then = std::move(f);
    }

    // must receive functor R(T)
    template <typename Functor,
              typename R = typename FunctionTraits<Functor>::ReturnType,
              typename Check = typename std::enable_if<
                    IsThenValid<Future<T>, Functor>::value>::type>
    Future<R> then(Functor f) {
        Promise<R> promise(_looper);
        // async request, will be set value and then callback
        setCallback([f = std::move(f), promise](T &&value) mutable {
            // f(T) will move current Future<T> value in shared
            // f(T&) or f(T&&) just use reference
            // TODO currently f(T&) is invalid
            promise.setValue(f(std::move(value)));
        });
        return promise.get();
    }

    // then() should have 2 overload method
    // if f(T), user must not use yield(), so just promise.setValue, f(T) will move current future value
    // if f(T reference), user will use yield(), test f(), then setValue (but how? callback-again?)

private:
    SimpleLooper                     *_looper;
    std::shared_ptr<ControlBlock<T>> _shared;
};
