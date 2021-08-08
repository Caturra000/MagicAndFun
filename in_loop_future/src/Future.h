#pragma once
#include <bits/stdc++.h>
#include "FunctionTraits.h"
#include "FutureTraits.h"
#include "SimpleLooper.h"
#include "ControlBlock.h"
#include "Promise.h"

// [experimental]
// In loop future
// lockfree?

template <typename T>
class Promise;

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

    // must receive functor R(T) R(T&) R(T&&)
    template <typename Functor,
              typename R = typename FunctionTraits<Functor>::ReturnType,
              typename Check = typename std::enable_if<
                    IsThenValid<Future<T>, Functor>::value>::type>
    Future<R> then(Functor &&f) {
        // T or T& or T&& ?
        // using ForwardType = decltype(std::get<0>(std::declval<typename FunctionTraits<Functor>::ArgsTuple>()));
        using ForwardType = typename std::tuple_element<0, typename FunctionTraits<Functor>::ArgsTuple>::type;
        Promise<R> promise(_looper);
        auto future = promise.get();
        // async request, will be set value and then callback
        setCallback([f = std::forward<Functor>(f), promise = std::move(promise)](T &&value) mutable {
            // f(T) will move current Future<T> value in shared
            // f(T&) or f(T&&) just use reference
            promise.setValue(f(std::forward<ForwardType>(value)));
        });
        return future;
    }

    // receive: bool(T&) bool(T&&)
    // return: Future<T>
    template <typename Functor,
              bool AtLeastThenValid = IsThenValid<Future<T>, Functor>::value,
              bool DontReceiveTypeT = !std::is_same<typename FunctionTraits<Functor>::ArgsTuple, std::tuple<T>>::value,
              bool ShouldReturnBool = std::is_same<typename FunctionTraits<Functor>::ReturnType, bool>::value,
              typename PollRequired = typename std::enable_if<AtLeastThenValid && DontReceiveTypeT && ShouldReturnBool>::type>
    Future<T> poll(Functor &&f) {
        using ForwardType = typename std::tuple_element<0, typename FunctionTraits<Functor>::ArgsTuple>::type;
        Promise<T> promise(_looper);
        auto future = promise.get();
        // reuse _then
        setCallback([f = std::forward<Functor>(f), promise = std::move(promise), looper = _looper](T &&value) mutable {
            if(f(std::forward<ForwardType>(value))) {
                // actually move
                promise.setValue(std::move(value));
            } else {
                looper->yield();
            }
        });
        return future;
    }

private:
    void setCallback(const std::function<void(T&&)> &f) {
        _shared->_then = f;
    }

    void setCallback(std::function<void(T&&)> &&f) {
        _shared->_then = std::move(f);
    }

private:
    SimpleLooper                     *_looper;
    std::shared_ptr<ControlBlock<T>> _shared;
};
