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
    template <typename ControlBlockTuple>
    static auto makeResultTuple(ControlBlockTuple &&tup) {
        // decay to lvalue
        return std::make_tuple(std::get<Is>(tup)->_value...);
    }
};

// whenAll return copied values
// futures must be lvalue
// futs: Future<T1>, Future<T2>, Future<T3>...
// return: Future<std::tuple<T1, T2, T3...>>
template <typename ...Futs>
inline auto whenAll(SimpleLooper *looper, Futs &...futs) {
    using ControlBlockTuple = std::tuple<std::shared_ptr<ControlBlock<typename FutureInner<Futs>::Type>>...>;
    using ResultTuple = std::tuple<typename FutureInner<Futs>::Type...>;
    using Binder = std::tuple<ControlBlockTuple, ResultTuple>;
    auto boot = makeTupleFuture(looper,
                                /*controlBlocks*/std::make_tuple(futs.getControlBlock()...),
                                /*results*/ResultTuple{})
                // at most X times?
                .poll(/*X, */[](Binder &binder) {
                    bool pass = true;
                    auto &cbs = std::get<0>(binder);
                    auto &res = std::get<1>(binder);
                    TupleHelper<ControlBlockTuple>::forEach(cbs, [&pass](auto &&elem) mutable {
                        // IMPROVEMENT: O(n) algorithm
                        // or divide-and-conqure by yourself
                        // note: cannot cache pass result for the next poll, because state will die or cancel
                        auto state = elem->_state;
                        bool hasValue = (state == State::READY) || (state == State::POST) || (state == State::DONE);
                        return pass &= hasValue;
                    });
                    // lock values here if true
                    if(pass) {
                        using Sequence = typename std::make_index_sequence<std::tuple_size<ResultTuple>::value>;
                        res = TupleHelper<Sequence>::makeResultTuple(cbs);
                        // now it's safe to then()
                    }
                    return pass;
                }).then([](Binder &&binder) {
                    auto res = std::move(std::get<1>(binder));
                    return res;
                });
    return boot;
}
