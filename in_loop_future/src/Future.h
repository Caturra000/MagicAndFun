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

    // unsafe
    // ensure: has result
    T get() {
        auto value = std::move(_shared->_value);
        _shared->_state = State::DIED;
        return value;
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
        // TODO remove, std::forward is the same
        using CastType = typename ThenArgumentTraitsConvert<ForwardType>::Type;
        Promise<R> promise(_looper);
        auto future = promise.get();
        State state = _shared->_state;
        if(state == State::NEW || state == State::READY) {
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
            if(state == State::READY) {
                postRequest();
            }
        } else if(state == State::CANCEL) {
            // return a future will never be setValue()
            promise.cancel();
        }
        return future;
    }

    // receive: bool(T) bool(T&)
    // return: Future<T>
    template <typename Functor,
              bool AtLeastThenValid = IsThenValid<Future<T>, Functor>::value,
              bool WontAccpetRvalue = !std::is_same<typename FunctionTraits<Functor>::ArgsTuple, std::tuple<T&&>>::value,
              bool ShouldReturnBool = std::is_same<typename FunctionTraits<Functor>::ReturnType, bool>::value,
              typename PollRequired = typename std::enable_if<AtLeastThenValid && WontAccpetRvalue && ShouldReturnBool>::type>
    Future<T> poll(Functor &&f) {
        using ForwardType = typename std::tuple_element<0, typename FunctionTraits<Functor>::ArgsTuple>::type;
        using CastType = typename ThenArgumentTraitsConvert<ForwardType>::Type;
        Promise<T> promise(_looper);
        auto future = promise.get();
        State state = _shared->_state;
        if(state == State::NEW || state == State::READY) {
            // reuse _then
            setCallback([f = std::forward<Functor>(f), promise = std::move(promise), looper = _looper](T &&value) mutable {
                if(f(static_cast<CastType>(value))) {
                    promise.setValue(std::forward<ForwardType>(value));
                } else {
                    looper->yield();
                }
            });
            if(state == State::READY) {
                postRequest();
            }
        } else if(_shared->_state == State::CANCEL) {
            // return a future will never be setValue()
            promise.cancel();
        }
        return future;
    }

    // receive: bool(T) bool(T&)
    // return: Future<T>
    // IMPROVEMENT: return future<T>& ?
    template <typename Functor,
              bool AtLeastThenValid = IsThenValid<Future<T>, Functor>::value,
              bool WontAccpetRvalue = !std::is_same<typename FunctionTraits<Functor>::ArgsTuple, std::tuple<T&&>>::value,
              bool ShouldReturnBool = std::is_same<typename FunctionTraits<Functor>::ReturnType, bool>::value,
              typename CancelIfRequired = typename std::enable_if<AtLeastThenValid && WontAccpetRvalue && ShouldReturnBool>::type>
    Future<T> cancelIf(Functor &&f) {
        using ForwardType = typename std::tuple_element<0, typename FunctionTraits<Functor>::ArgsTuple>::type;
        using CastType = typename ThenArgumentTraitsConvert<ForwardType>::Type;
        Promise<T> promise(_looper);
        auto future = promise.get();
        State state = _shared->_state;
        if(state == State::NEW || state == State::READY) {
            setCallback([f = std::forward<Functor>(f), promise = std::move(promise)](T &&value) mutable {
                if(f(static_cast<CastType>(value))) {
                    promise.cancel();
                } else {
                    // forward the current future to the next
                    promise.setValue(std::forward<ForwardType>(value));
                }
            });
            if(state == State::READY) {
                postRequest();
            }
        } else if(_shared->_state == State::CANCEL) {
            promise.cancel();
        }
        return future;
    }

    // receive: bool(T) bool(T&)
    // return: Future<T>
    template <typename Functor,
              bool AtLeastThenValid = IsThenValid<Future<T>, Functor>::value,
              bool WontAccpetRvalue = !std::is_same<typename FunctionTraits<Functor>::ArgsTuple, std::tuple<T&&>>::value,
              bool ShouldReturnVoid = std::is_same<typename FunctionTraits<Functor>::ReturnType, void>::value,
              typename WaitRequired = typename std::enable_if<AtLeastThenValid && WontAccpetRvalue && ShouldReturnVoid>::type>
    Future<T> wait(std::chrono::milliseconds duration, Functor &&f) {
        auto start = std::chrono::system_clock::time_point{};
        return poll([f = std::forward<Functor>(f), start, duration](T &self) mutable {
            auto current = std::chrono::system_clock::now();
            // call once
            if(start == std::chrono::system_clock::time_point{}) {
                start = current;
            }
            if(current - start >= duration) {
                f(self);
                return true;
            }
            return false;
        });
    }

    // receive: bool(T) bool(T&)
    // return: Future<T>
    template <typename Functor,
              bool AtLeastThenValid = IsThenValid<Future<T>, Functor>::value,
              bool WontAccpetRvalue = !std::is_same<typename FunctionTraits<Functor>::ArgsTuple, std::tuple<T&&>>::value,
              bool ShouldReturnVoid = std::is_same<typename FunctionTraits<Functor>::ReturnType, void>::value,
              typename WaitRequired = typename std::enable_if<AtLeastThenValid && WontAccpetRvalue && ShouldReturnVoid>::type>
    Future<T> wait(std::chrono::system_clock::time_point timePoint, Functor &&f) {
        return poll([f = std::forward<Functor>(f), timePoint](T &self) mutable {
            auto current = std::chrono::system_clock::now();
            if(current >= timePoint) {
                f(self);
                return true;
            }
            return false;
        });
    }

private:
    void setCallback(const std::function<void(T&&)> &f) {
        _shared->_then = f;
    }

    void setCallback(std::function<void(T&&)> &&f) {
        _shared->_then = std::move(f);
    }

    // unsafe
    // ensure: READY & has then_
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
