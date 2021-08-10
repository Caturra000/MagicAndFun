#include <bits/stdc++.h>
#include "Future.h"
#include "Futures.h"

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
        .poll([&looper](FakePoller poller) {
            if(!poller.poll()) {
                return false;
            }
            std::cout << "poll finished" << std::endl;
            return true;
        })
        .wait(std::chrono::milliseconds(2333), [](FakePoller &poller) {
            // null
        })
        .cancelIf([](FakePoller&) {
            std::cout << "but I try to cancel this routine. If true, looper will never stop" << std::endl;
            // return true; // never stop
            return false; // go on!
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
            .poll([](FakePoller &poller) {
                if(!poller.poll()) return false;
                return true;
            })
            // .wait(std::chrono::milliseconds(1000), [](FakePoller &poller) {
            //     // null
            // })
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






    // test whenAll

    auto collectFut0 = makeFuture(&looper, std::string("12345"));
    auto collectFut1 = makeFuture(&looper, std::vector<int>{1, 2, 3});

    auto all = whenAll(&looper, collectFut0, collectFut1)
        .then([&stopFlag](std::tuple<std::string, std::vector<int>> &&result) {
            std::cout << std::get<0>(result) << std::endl
                      << std::get<1>(result)[0] << std::endl;
            stopFlag = true;
            return nullptr;
        });

    stopFlag = false;
    for(; !stopFlag;) {
        looper.loop();
    }




    // test whenN

    auto stringFuture0 = makeFuture(&looper, std::string("12345"));
    auto stringFuture1 = makeFuture(&looper, std::string("54321"));
    auto stringFuture2 = makeFuture(&looper, std::string("13254"));

    std::vector<Future<std::string>> futures;
    futures.emplace_back(makeFuture(&looper, std::string("12345")));
    futures.emplace_back(makeFuture(&looper, std::string("54321")));
    futures.emplace_back(makeFuture(&looper, std::string("13254")));

    // auto checkN = whenN(2, &looper, stringFuture0, stringFuture1, stringFuture2)
    // auto checkN = whenN(2, &looper, futures.begin(), futures.end())
    auto checkNIf = whenNIf(2, &looper, futures.begin(), futures.end(), [](const std::string &str) {
            return str.size() > 0 && str[0] == '1';
        })
        .then([&stopFlag](std::vector<std::pair<size_t, std::string>> &collected) {
            std::cout << "whenN: " << collected.size() << std::endl;
            for(auto &&pair : collected) {
                std::cout << "[" << pair.first << ", " << pair.second << "]" << std::endl;
            }
            stopFlag = true;
            return nullptr;
        });

    stopFlag = false;
    for(; !stopFlag;) {
        looper.loop();
    }

    // test whenAny

    auto stringFuture3 = makeFuture(&looper, std::string("12345"));
    auto stringFuture4 = makeFuture(&looper, std::string("54321"));
    auto stringFuture5 = makeFuture(&looper, std::string("13254"));

    std::vector<Future<std::string>> futures2;
    futures2.emplace_back(makeFuture(&looper, std::string("12345")));
    futures2.emplace_back(makeFuture(&looper, std::string("54321")));
    futures2.emplace_back(makeFuture(&looper, std::string("13254")));
    // futures2.emplace_back(std::move(stringFuture3));
    // futures2.emplace_back(std::move(stringFuture4));
    // futures2.emplace_back(std::move(stringFuture5));

    // whenAny(&looper, stringFuture3, stringFuture4, stringFuture5)
    // whenAny(&looper, futures2.begin(), futures2.end())
    whenAnyIf(&looper, futures2.begin(), futures2.end(), [](const std::string &str) {
            return str.size() > 1 && str[1] == '4';
        })
        .then([&stopFlag](std::pair<size_t, std::string> &&any) {
            std::cout << "any" << std::endl;
            std::cout << "[" << any.first << ", " << any.second << "]" << std::endl;
            stopFlag = true;
            return nullptr;
        });

    stopFlag = false;
    for(; !stopFlag;) {
        looper.loop();
    }
    // std::cout << stringFuture5.get() << std::endl;

    return 0;
}