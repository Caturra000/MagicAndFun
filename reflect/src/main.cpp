#include <bits/stdc++.h>
#define dbg(x) std::cout << x << std::endl

template <typename T>
struct Foo {
    int a;
    std::string b;

    void func() {}

    int func2(double) { std::cout << (__PRETTY_FUNCTION__) << std::endl; return 1; }
};

// 低配版std::string_view
struct View {
    const char *base;
    size_t length;
};

enum class EofTag {};

template <char ...chs>
struct MetaString {
    constexpr const static char str[]  = { chs... };
    constexpr const static size_t capacity = sizeof...(chs);
    constexpr const char* operator()() const { return MetaString<chs...>::str; }
    constexpr const char* operator()(EofTag) const { return {chs..., '\0'}; }
    constexpr const size_t length() const { return MetaString<chs...>::capacity; }
    constexpr const size_t length(EofTag) const { return length() + 1; }
};
template <char ...chs>
constexpr const char MetaString<chs...>::str[];

inline constexpr size_t findLastPosFrom(const View view, char ch) {
    for(size_t i = view.length-1; true; --i) {
        if(view.base[i] == ch) {
            return i;
        }
        if(i == 0) {
            return static_cast<size_t>(-1);
        }
    }
    return static_cast<size_t>(-1);
}
inline constexpr View resolveTypeNameFrom(const View fullInfo) {
    size_t N = fullInfo.length;
    const char *base = fullInfo.base;
    size_t lo = findLastPosFrom(fullInfo, '=') + 2;
    size_t hi = findLastPosFrom(fullInfo, ']');
    // 注意这个view并没有'\0'
    return View { base + lo, hi - lo };
}

template <typename T>
inline constexpr auto getTypeNameView() {
    // output: constexpr auto getTypeName() [with T = <TYPE_INFO>]
    constexpr const View fullInfo {__PRETTY_FUNCTION__, sizeof __PRETTY_FUNCTION__};
    // output: <TYPE_INFO>
    return resolveTypeNameFrom(fullInfo);
}

// 获取解析好的MetaString
template <typename T>
inline constexpr const auto getTypeName() {
    constexpr auto typeNameView = getTypeNameView<T>();
    std::make_index_sequence<typeNameView.length> sequence;
    // 需要-std=c++17，否则无法保证constexpr lambda
    // TODO
    // 目前还没想好C++14及以下的兼容解决方案orz
    // 如果实在没更好的办法，那就全面升级C++17，顺便把View改为std::string_view
    return [&typeNameView]<size_t ...Is>(std::index_sequence<Is...>) {
        return MetaString<typeNameView.base[Is]..., '\0'>{};
    } (sequence);
}


int main() {
    Foo<std::vector<std::string>> foo;
    auto typeName = getTypeName<decltype(foo)>()();
    dbg(typeName); // 期望输出：Foo<std::vector<std::__cxx11::basic_string<char> > >
    return 0;
}