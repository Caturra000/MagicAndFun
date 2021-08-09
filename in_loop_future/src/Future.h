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
    Future(SimpleLooper *looper,
           const std::shared_ptr<ControlBlock<T>> &shared)
        : _looper(looper),
          _shared(shared) {}

    Future(const Future &) = delete;
    Future(Future &&) = default;
    Future& operator=(const Future&) = delete;
    Future& operator=(Future&&) = default;

    // TIMEOUT?
    bool hasResult() { return _shared->_state == State::READY; }

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
        using CastType = typename ThenArgumentTraitsConvert<ForwardType>::Type;
        Promise<R> promise(_looper);
        auto future = promise.get();
        if(_shared->_state == State::CANCEL) {
            // return a future will never be setValue()
            promise.cancel();
            return future;
        }
        // async request, will be set value and then callback
        setCallback([f = std::forward<Functor>(f), promise = std::move(promise)](T &&value) mutable {
            // for callback f
            // f(T) and f(T&) will copy current Future<T> value in shared,
            // - T will copy to your routine. It is safe to move
            // - T& just use reference where it is in shared state
            // T&& will move the parent future value
            // - T&& just use reference, but moving this value to your routine is unsafe
            promise.setValue(f(static_cast<CastType>(value)));
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
        using CastType = typename ThenArgumentTraitsConvert<ForwardType>::Type;
        Promise<T> promise(_looper);
        auto future = promise.get();
        if(_shared->_state == State::CANCEL) {
            // return a future will never be setValue()
            promise.cancel();
            return future;
        }
        // reuse _then
        setCallback([f = std::forward<Functor>(f), promise = std::move(promise), looper = _looper](T &&value) mutable {
            if(f(static_cast<CastType>(value))) {
                // actually move
                promise.setValue(std::forward<ForwardType>(value));
            } else {
                looper->yield();
            }
        });
        return future;
    }

    // receive: bool(T&) bool(T&&)
    // return: Future<T>
    // IMPROVEMENT: return future<T>& ?
    template <typename Functor,
              bool AtLeastThenValid = IsThenValid<Future<T>, Functor>::value,
              bool DontReceiveTypeT = !std::is_same<typename FunctionTraits<Functor>::ArgsTuple, std::tuple<T>>::value,
              bool ShouldReturnBool = std::is_same<typename FunctionTraits<Functor>::ReturnType, bool>::value,
              typename CancelIfRequired = typename std::enable_if<AtLeastThenValid && DontReceiveTypeT && ShouldReturnBool>::type>
    Future<T> cancelIf(Functor &&f) {
        using ForwardType = typename std::tuple_element<0, typename FunctionTraits<Functor>::ArgsTuple>::type;
        using CastType = typename ThenArgumentTraitsConvert<ForwardType>::Type;
        Promise<T> promise(_looper);
        auto future = promise.get();
        if(_shared->_state == State::CANCEL) {
            promise.cancel();
            return future;
        }
        setCallback([f = std::forward<Functor>(f), promise = std::move(promise)](T &&value) mutable {
            if(f(static_cast<CastType>(value))) {
                promise.cancel();
            } else {
                // forward the current future to the next
                promise.setValue(std::forward<ForwardType>(value));
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
