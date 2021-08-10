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
    static_assert(sizeof...(futs) > 0, "whenAll should receive future");
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
                })
                .then([](Binder &&binder) {
                    auto res = std::move(std::get<1>(binder));
                    return res;
                });
    return boot;
}

// futs : Future<T>, Future<T>, Future<T>...
// return: Future<std::vector<std::pair<size_t, T>>>
// returns index and result
template <typename Fut, typename ...Futs>
inline auto whenN(size_t n, SimpleLooper *looper, Fut &fut, Futs &...futs) {
    // assert(N <= 1 + sizeof...(futs));
    using T = typename FutureInner<Fut>::Type;
    using ControlBlockType = std::shared_ptr<ControlBlock<T>>;
    using QueryPair = std::pair<size_t, ControlBlockType>;
    using ResultPair = std::pair<size_t, T>; // index and result
    using QueryVector = std::vector<QueryPair>;
    using ResultVector = std::vector<ResultPair>;
    using Binder = std::tuple<QueryVector, ResultVector>;
    QueryVector queries;

    // collect
    {
        std::vector<ControlBlockType> controlBlocks {
            fut.getControlBlock(), futs.getControlBlock()...
        };
        for(size_t index = 0, N = controlBlocks.size(); index != N; ++index) {
            queries.emplace_back(index, std::move(controlBlocks[index]));
        }
    };


    return makeTupleFuture(looper, std::move(queries), ResultVector{})
        .poll([noresponse = QueryVector{}, remain = n](Binder &binder) mutable {
            auto &queries = std::get<0>(binder);
            auto &results = std::get<1>(binder);
            while(remain && !queries.empty()) {
                // pair: index + controllBlock
                auto &query = queries.back();
                State state = query.second->_state;
                bool hasValue = (state == State::READY) || (state == State::POST) || (state == State::DONE);
                if(!hasValue) {
                    noresponse.emplace_back(std::move(query));
                } else {
                    results.emplace_back(query.first, query.second->_value);
                    remain--;
                }
                queries.pop_back();
            }
            noresponse.swap(queries);
            return remain == 0;
        })
        .then([](Binder &&binder) {
            auto res = std::move(std::get<1>(binder));
            // sort by index
            std::sort(res.begin(), res.end(), [](auto &&lhs, auto &&rhs) {
                return lhs.first < rhs.first;
            });
            return res;
        });
}

// futs : Future<T>, Future<T>, Future<T>...
// return: Future<std::pair<size_t, T>>
// returns index and result
template <typename Fut, typename ...Futs>
inline auto whenAny(SimpleLooper *looper, Fut &fut, Futs &...futs) {
    using T = typename FutureInner<Fut>::Type;
    return whenN(1, looper, fut, futs...)
        .then([](std::vector<std::pair<size_t, T>> &&results) {
            auto result = std::move(results[0]);
            return result;
        });
}