#include <bits/stdc++.h>
#include "Future.h"

int main() {
    SimpleLooper looper;
    Promise<int> promise(&looper);
    bool stopFlag = false;
    bool add = false;
    int count = 0;
    auto fut = promise.get()
        .then([&looper, &count](int value) {
            // if(count < 10) {
            //     std::cout << "will break and retry" << std::endl;
            //     looper.yield();
            //     return std::declval<std::string>(); // return a dummy object?
            // }
            return std::string("str");
        })
        .then([](std::string str) {
            std::cout << str << std::endl;
            return std::vector<int>{1, 2, 3};
        })
        .then([&stopFlag](std::vector<int> vec) {
            std::cout << "vec size: " << vec.size() << std::endl;
            stopFlag = true;
            return nullptr;
        });
    for(; !stopFlag;) {
        looper.loop();
        if(!add) {
            promise.setValue(100);
            add =  true;
        }
    }
    return 0;
}