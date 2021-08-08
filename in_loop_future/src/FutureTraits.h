#pragma once
#include <bits/stdc++.h>
#include "FunctionTraits.h"

// forward
template <typename T>
class Future;

// example: FutureInner<Future<int>>::type -> int
template <typename T>
struct FutureInner {
    using Type = T;
};

template <typename T>
struct FutureInner<Future<T>> {
    using Type = typename FutureInner<T>::Type;
};

template <typename Fut, typename Func,
// some useful info
typename T = typename FutureInner<Fut>::Type,
typename Args = typename FunctionTraits<Func>::ArgsTuple>
struct IsThenValid: public std::conditional_t<
    std::is_same<std::tuple<T>, Args>::value
    || std::is_same<std::tuple<T&>, Args>::value
    || std::is_same<std::tuple<T&&>, Args>::value,
    std::true_type,
    std::false_type> {};