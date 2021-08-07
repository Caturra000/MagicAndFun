#include <bits/stdc++.h>
#include "Future.h"

int main() {
    SimpleLooper looper;
    Promise<int> promise(&looper);
    promise.setValue(100);
    auto fut = promise.get()
        .then([](int value) {
            return std::string("str");
        })
        .then([](std::string) {
            return std::vector<int>{1, 2, 3};
        });
    return 0;
}