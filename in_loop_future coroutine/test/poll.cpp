#include <unistd.h>
#include <fcntl.h>
#include <bits/stdc++.h>
#include "../src/Futures.h"

// listen keyboard and echo
int main() try {
    SimpleLooper looper;
    bool stop = false;

    int lines = 3;
    constexpr size_t BUF_SIZE = 1e5;
    char buf[BUF_SIZE];

    int flags = ::fcntl(STDIN_FILENO, F_GETFL);
    if(::fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK)) {
        throw std::runtime_error("fcntl failed: errno = " + std::to_string(errno));
    }

    auto defer = std::shared_ptr<void>(nullptr, [flags](void*) {
        if(::fcntl(STDIN_FILENO, F_SETFL, flags)) {
            throw std::runtime_error("[defer] fcntl failed: errno = " + std::to_string(errno));
        }
    });

    auto echo = makeFuture(&looper, lines)
        .poll([&buf, &stop](int &count) mutable {
            int n;
            if((n = ::read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
                ::write(STDOUT_FILENO, buf, n);
                count--;
            } else if(n < 0 && errno != EAGAIN) {
                throw std::runtime_error("read failed: errno = " + std::to_string(errno));
            }
            return count ? false : stop = true;
        });

    for(; !stop;) looper.loop();
} catch(const std::exception &e) {
    // LOG(...)
}