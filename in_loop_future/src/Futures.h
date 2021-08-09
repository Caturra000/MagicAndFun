#pragma once
#include <bits/stdc++.h>
#include "Future.h"
#include "Promise.h"

// TODO namespace ...

template <typename ...Args, typename Tuple = std::tuple<std::decay_t<Args>...>>
inline Future<Tuple> makeTupleFuture(SimpleLooper *looper, Args &&...args) {
    Promise<Tuple> promise(looper);
    promise.setValue(std::make_tuple(std::forward<Args>(args)...));
    return promise.get();
}

template <typename T, typename R = std::decay_t<T>>
inline Future<R> makeFuture(SimpleLooper *looper, T &&arg) {
    Promise<R> promise(looper);
    promise.setValue(std::forward<T>(arg));
    return promise.get();
}

template <typename Tuple>
struct TupleHelper {
public:
    using DecayTuple = std::decay_t<Tuple>;

    template <typename Functor>
    static void forEach(DecayTuple &tup, Functor &&f) {
        test<0>(tup, /*std::forward<Functor>*/(f));
    }

private:
    template <size_t I, typename Functor>
    static std::enable_if_t<(I+1 < std::tuple_size<DecayTuple>::value)>
    test(DecayTuple &tup, Functor &&f) {
        // true : continue
        if(f(std::get<I>(tup))) {
            test<I+1>(tup, std::forward<Functor>(f));
        }
    }

    // last
    template <size_t I, typename Functor>
    static std::enable_if_t<(I+1 == std::tuple_size<DecayTuple>::value)>
    test(DecayTuple &tup, Functor &&f) {
        f(std::get<I>(tup));
    }
};

template <size_t ...Is>
struct TupleHelper<std::index_sequence<Is...>> {
    template <typename Tuple>
    static auto makeTupleFromValue(Tuple &&tup) {
        // decay to lvalue
        return std::make_tuple(std::get<Is>(tup)->_value...);
    }
};

// whenAll return copied values
template <typename ...Futs>
inline auto whenAll(SimpleLooper *looper, Futs &...futs) {
    auto cbTuple = std::make_tuple(futs.getControlBlock()...);
    using CbTupleType = decltype(cbTuple);

    auto boot = makeFuture(looper, std::move(cbTuple))
                // at most X times?
                .poll(/*X, */[](CbTupleType &cbTuple) {
                    bool pass = true;
                    TupleHelper<decltype(cbTuple)>::forEach(cbTuple, [&pass](auto &&elem) mutable {
                        // IMPROVEMENT: O(n) algorithm
                        // or divide-and-conqure by yourself
                        // note: cannot cache pass result for the next poll, because state will die or cancel
                        auto state = elem->_state;
                        bool hasValue = (state == State::READY) || (state == State::POST) || (state == State::DONE);
                        return pass &= hasValue;
                    });
                    return pass;
                }).then([](CbTupleType &&cbTuple) {
                    using Is = typename std::make_index_sequence<std::tuple_size<std::decay_t<CbTupleType>>::value>;
                    return TupleHelper<Is>::makeTupleFromValue(cbTuple);
                });
    return boot;
}