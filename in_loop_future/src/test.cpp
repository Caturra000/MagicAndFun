#include <bits/stdc++.h>
#include "Future.h"


struct FakePoller {
    bool flag = false;
    std::chrono::system_clock::time_point _start {std::chrono::system_clock::now()};
    int _count = 0;

    // may be an async-IO operations
    // test yield
    bool poll() {
        // if(_count >= 10) return true;
        // _count++;
        // return false;
        auto delta = std::chrono::system_clock::now() - _start;
        if(std::chrono::system_clock::now() - _start >= std::chrono::milliseconds(100)) return true;
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
                return false;
            }
            return true;
        })
        .then([&stopFlag](FakePoller&) {
            stopFlag = true;
            return nullptr;
        });
    for(; !stopFlag;) {
        looper.loop();
        if(!add) {
            promise2.setValue(poller);
            add = true;
        }
    }





    auto start = std::chrono::system_clock::now();


    stopFlag = false;
    add = false;
    int finished = 0;

    // cannot use promises(100, &looper); will copy same control block!
    std::vector<Promise<FakePoller>> promises;
    for(int i = 0; i < 100; ++i) {
        promises.emplace_back(&looper);
    }

    for(auto &promise : promises) {
        auto fut = promise.get()
            .poll([](FakePoller &&poller) {
                if(!poller.poll()) return false;
                return true;
            })
            .then([&finished, &stopFlag, &promises](FakePoller &&poller) {
                finished++;
                if(finished >= promises.size()) {
                    stopFlag = true;
                }
                return nullptr;
            });
    }

    for(; !stopFlag;) {
        looper.loop();
        if(!add) {
            for(auto &promise : promises) {
                promise.setValue(FakePoller{});
            }
            add = true;
        }
    }

    auto end = std::chrono::system_clock::now();

    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "cost: " << delta.count() << std::endl;

    return 0;
}