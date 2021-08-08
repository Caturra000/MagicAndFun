#include <bits/stdc++.h>
#include "Future.h"


struct FakePoller {
    int _count = 0;

    // may be an async-IO operations
    // test yield
    bool poll() {
        if(_count > 10) return true;
        _count++;
        return false;
    }
};

int main() {
    SimpleLooper looper;
    Promise<int> promise(&looper);
    bool stopFlag = false;
    bool add = false;
    int count = 0;
    FakePoller poller;

    auto fut = promise.get()
        // TODO
        // then argument will change state of shared->_value in different way
        // then(T) will copy the value of shared, shared value is still alive
        // then(T&) will use reference only, shared value is still alive
        // then(T&&) will move the value of shared, and cannot be used again (cannot do a fake yield if moved)
        // best practice: when you need an async-IO operation, use T& to yield (return type T as a sencond then function arguemnt) and then T&& for your routine
        .then([&looper, &count](int value) {
            return std::string("str");
        })
        .then([](std::string &&str) {
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

    Promise<FakePoller> promise2(&looper);
    stopFlag = false;
    add = false;
    auto fut22 = promise2.get()
        .poll([&looper](FakePoller &&poller) {
            if(!poller.poll()) {
                std::cout << "oops" << std::endl;
                return false;
            }
            return true;
        })
        .then([&stopFlag](FakePoller) {
            std::cout << "ok!" << std::endl;
            stopFlag = true;
            return nullptr;
        });
    for(; !stopFlag;) {
        looper.loop();
        if(!add) {
            promise2.setValue(FakePoller{});
            add = true;
        }
    }
    return 0;
}