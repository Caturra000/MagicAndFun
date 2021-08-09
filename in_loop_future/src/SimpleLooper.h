#pragma once
#include <bits/stdc++.h>

#define CPU_LOOP_UNROLL_4X(actionx1, actionx2, actionx4, width) do { \
    unsigned long __width = (unsigned long)(width);    \
    unsigned long __increment = __width >> 2; \
    for (; __increment > 0; __increment--) { actionx4; }    \
    if (__width & 2) { actionx2; } \
    if (__width & 1) { actionx1; } \
}   while (0)

class SimpleLooper {
public:
    // example: for(;;) loop();
    void loop(nullptr_t) {
        for(int sz = _mq.size(); sz--;) {
            loopOnce();
        }
    }

    // just for fun
    void loop() { unroll4x(); }

    void unroll4x() {
        size_t n = _mq.size();
        CPU_LOOP_UNROLL_4X(
            {
                loopOnce();
            },
            {
                loopOnce();
                loopOnce();
            },
            {
                loopOnce();
                loopOnce();
                loopOnce();
                loopOnce();
            },
        n);

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

#undef CPU_LOOP_UNROLL_4X