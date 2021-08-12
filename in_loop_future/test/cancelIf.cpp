#include <bits/stdc++.h>
#include "../src/Futures.h"

int main() {
    SimpleLooper looper;
    bool stop = false;

    std::vector<std::string> players {
        "Miyauchi Renge", "Ichijou Hotaru", "Koshigaya Natsumi", "Koshigaya Komari"
    };

    ::srand(1926'08'17);
    int lucky = ::rand() % players.size();
    int remain = players.size();

    for(const auto &player : players) {
        auto playGame = makeFuture(&looper, player)
            .then([&lucky](std::string &&player) {
                std::cout << player << " is ready" << std::endl;
                bool isWinner = !(lucky--);
                return std::make_pair(std::move(player), isWinner);
            })
            .cancelIf([&stop, &remain](std::pair<std::string, bool> &info) {
                bool isWinner = info.second;
                std::cout << info.first << ( isWinner ? " is alive" : " is dead") << std::endl;
                if(!isWinner && --remain == 0) stop = true;
                return !isWinner;
            })
            .then([&stop, &remain](std::pair<std::string, bool> &&info) {
                // assert(info.second);
                std::cout << info.first << " LEVEL UP!" << std::endl;
                if(--remain == 0) stop = true;
                return nullptr;
            });
    }

    while(!stop) looper.loop();

}