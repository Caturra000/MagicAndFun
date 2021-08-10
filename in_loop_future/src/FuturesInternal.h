#pragma once
#include "bits/stdc++.h"
#include "Future.h"
#include "Promise.h"
namespace details {

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

template <typename ResultTuple, typename ControlBlockTupleForward>
inline auto whenAllTemplate(SimpleLooper *looper, ControlBlockTupleForward &&controlBlocks) {
    using ControlBlockTuple = typename std::remove_reference<ControlBlockTupleForward>::type;
    using Binder = std::tuple<ControlBlockTuple, ResultTuple>;
    return makeTupleFuture(looper,
                           std::forward<ControlBlockTupleForward>(controlBlocks),
                           ResultTuple{})
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
}

template <typename ResultVector, typename QueryVectorForward>
inline auto whenNTemplate(size_t n, SimpleLooper *looper, QueryVectorForward &&queries) {
    using QueryVector = typename std::remove_reference<QueryVectorForward>::type;
    using Binder = std::tuple<QueryVector, ResultVector>;
    return makeTupleFuture(looper, std::forward<QueryVectorForward>(queries), ResultVector{})
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

} // details