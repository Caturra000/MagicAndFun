#include <bits/stdc++.h>


int main() {
    // 实现一个无需std::function成本的可递归lambda
    // 注意：编译器比较谨慎，无法帮你推导处返回类型为int，因此要手动声明
    auto recursive = [](auto &&func, size_t n) -> int {
        return n ? n + func(func, n-1) : 0;
    };
    std::cout << recursive(recursive, 10) << std::endl;

    // 引入中间层，让它对调用者友好
    auto accumulate = [f = recursive](size_t n) {
        return f(f, 10);
    };
    std::cout << accumulate(10) << std::endl;

    return 0;
}