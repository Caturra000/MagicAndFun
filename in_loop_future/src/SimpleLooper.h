#pragma once
#include <bits/stdc++.h>

class SimpleLooper {
public:
    // example: for(;;) loop();
    void loop() {
        for(int sz = _mq.size(); sz--;) {
            loopOnce();
        }
    }

    // unsafe
    void loopOnce() {
        // debug();
        _lastEvent = std::move(_mq.front());
        _mq.pop();
        _lastEvent();
    }

    void loopOnceChecked() { if(!_mq.empty()) loopOnce(); }

    // TODO
    // receive and return a context object
    //
    // since it is a FAKE yield (will break any result and no context)
    // you should save context by yourself (via lambda reference capture)
    void yield() {
        _mq.emplace(std::move(_lastEvent));
    }

    void addEvent(std::function<void()> event) {
        if(event) {
            _mq.emplace(std::move(event));
        }
    }

private:
    void debug() {
        std::cout << "[loop msg] " << _global++ << std::endl;
    }

private:
    std::queue<std::function<void()>> _mq;
    std::function<void()>             _lastEvent;
    int                               _global {}; // debug
};
