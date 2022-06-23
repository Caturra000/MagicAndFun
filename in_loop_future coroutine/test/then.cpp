#include <bits/stdc++.h>
#include "../src/Futures.h"

int main() {
    Looper looper;
    bool stop = false;

    using Username = std::string;
    using Password = std::string;
    using UserLoginInfo = std::pair<Username, Password>;
    using FavoritePictures = std::vector<int>; // int : picture id

    std::map<Username, FavoritePictures> database {
        {"jojo", {1, 2, 3, 4}},
        {"dio",  {5, 6, 7}}
    };

    std::function<bool(UserLoginInfo)> verifyLogin = [](UserLoginInfo) {
        return true;
    };

    auto openPixiv = [&looper](UserLoginInfo info) {
        Promise<UserLoginInfo> promise(&looper);
        promise.setValue(std::move(info));
        return promise.get();
    };

    auto surfing = openPixiv({"jojo", "I Love Dio"})
        // verify
        .then([&verifyLogin](UserLoginInfo &&info) {
            bool verified = verifyLogin(info);
            return std::make_pair(verified, info.first);
        })
        // query
        .then([&database](std::pair<bool, Username> info) {
            FavoritePictures pictures;
            auto verified = info.first;
            auto &username = info.second;
            if(verified) {
                pictures = database[username];
            }
            return pictures;
        })
        // response
        .then([&stop](FavoritePictures &pictures) {
            stop = true;
            for(auto &&picture : pictures) {
                std::cout << picture << std::endl;
            }
            return nullptr;
        });

    for(; !stop;) looper.loop();

}