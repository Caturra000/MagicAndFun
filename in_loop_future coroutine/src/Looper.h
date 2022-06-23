#pragma once

#define SKYWIND3000_CPU_LOOP_UNROLL_4X(actionx1, actionx2, actionx4, width) do { \
    unsigned long __width = (unsigned long)(width);    \
    unsigned long __increment = __width >> 2; \
    for (; __increment > 0; __increment--) { actionx4; }    \
    if (__width & 2) { actionx2; } \
    if (__width & 1) { actionx1; } \
}   while (0)

#define CATURRA_2X(action)  do { {action;} {action;} } while(0)
#define CATURRA_4X(action)  do { CATURRA_2X(action); CATURRA_2X(action); } while(0)
#define CATURRA_8X(action)  do { CATURRA_4X(action); CATURRA_4X(action); } while(0)
#define CATURRA_16X(action) do { CATURRA_8X(action); CATURRA_8X(action); } while(0)

#include <bits/stdc++.h>
#include "co.hpp"

class Looper {
public:

    void loop() { unroll4x(); }

    void stop() {}

    void unroll4x() {
        size_t n = _mq.size();
        SKYWIND3000_CPU_LOOP_UNROLL_4X(
            {
                loopOnceUnchecked();
            },
            {
                CATURRA_2X(loopOnceUnchecked());
            },
            {
                CATURRA_4X(loopOnceUnchecked());
            },
            n
        );

    }

    // unsafe
    void loopOnceUnchecked() {
        // debug();
        auto co = std::move(_mq.front());
        _mq.pop();
        co->resume();
        if(co->running()) {
            _mq.emplace(std::move(co));
        }
    }

    void loopOnce() { if(!_mq.empty()) loopOnceUnchecked(); }

    void yield() {
        co::this_coroutine::yield();
    }

    template <typename Func, typename ...Args>
    void post(Func &&func, Args &&...args) {
        _mq.emplace(_env->createCoroutine(
            std::forward<Func>(func),
            std::forward<Args>(args)...
        ));
    }

private:
    void debug() {
        std::cout << "[loop msg] " << _global++ << std::endl;
    }

private:
    co::Environment                            *_env {&co::open()};
    std::queue<std::shared_ptr<co::Coroutine>> _mq;
    int                                        _global {}; // debug
};

#undef CATURRA_16X
#undef CATURRA_8X
#undef CATURRA_4X
#undef CATURRA_2X
#undef SKYWIND3000_CPU_LOOP_UNROLL_4X
