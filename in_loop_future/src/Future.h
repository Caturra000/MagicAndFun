#include <bits/stdc++.h>
#include "FunctionTraits.h"

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
    std::function<void()> _lastEvent;
    int _global {}; // debug
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
    State _state;
    T _value;
    std::exception_ptr _exception;
    std::function<void(T)> _then;

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
                shared->_then(std::move(shared->_value));
                shared->_state = State::DONE;
            });
        }
    }

    // void setException()...

    void setCallback(std::function<void(T)> f) {
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

    void setCallback(std::function<void(T)> f) {
        _shared->_then = std::move(f);
    }

    // must receive functor R(T)
    template <typename Functor,
              typename R = typename FunctionTraits<Functor>::ReturnType,
              typename Check = std::enable_if<std::is_same<
                    typename FunctionTraits<Functor>::ArgsTuple,
                    std::tuple<T>
                    >::value>>
    Future<R> then(Functor f) {
        Promise<R> promise(_looper);
        // async request, will be set value and then callback
        this/*Future<T>*/->setCallback([f = std::move(f), promise](T value) mutable {
            // Question. yield?
            promise.setValue(f(std::move(value)));
        });
        return promise.get();
    }

private:
    SimpleLooper                     *_looper;
    std::shared_ptr<ControlBlock<T>> _shared;
};
