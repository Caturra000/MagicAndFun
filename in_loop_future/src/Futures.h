#pragma once
#include <bits/stdc++.h>
#include "Future.h"
#include "Promise.h"

// TODO namespace ...

template <typename ...Args, typename Tuple = std::tuple<std::remove_reference_t<Args>...>>
inline Future<Tuple> makeTupleFuture(SimpleLooper *looper, Args &&...args) {
    Promise<Tuple> promise(looper);
    promise.setValue(std::make_tuple(std::forward<Args>(args)...));
    return promise.get();
}

template <typename T, typename R = std::remove_reference_t<T>>
inline Future<R> makeFuture(SimpleLooper *looper, T &&arg) {
    Promise<R> promise(looper);
    promise.setValue(std::forward<T>(arg));
    return promise.get();
}