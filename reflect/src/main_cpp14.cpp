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
    template <char ...rchs>
    using Append = MetaString<chs..., rchs...>;

    constexpr const static char str[]  = { chs... };
    constexpr const static size_t capacity = sizeof...(chs);
    constexpr const char* operator()() const { return str; }
    constexpr const char* operator()(EofTag) const { return Append<'\0'>::str; }
    constexpr const size_t length() const { return capacity; }
    constexpr const size_t length(EofTag) const { return length() + 1; }
};
template <char ...chs>
constexpr const char MetaString<chs...>::str[];

template <size_t N>
inline constexpr size_t findFirstOf(const char (&buffer)[N], char ch) {
    constexpr size_t npos = static_cast<size_t>(-1);
    for(size_t i = 0; i != N; ++i) {
        if(buffer[i] == ch) {
            return i;
        }
    }
    return npos;
}

template <typename T>
inline constexpr size_t getClassNameLength() {
    constexpr auto& buffer = __PRETTY_FUNCTION__;
    size_t lo = findFirstOf(buffer, '=') + 2;
    size_t hi = findFirstOf(buffer, ';');
    return hi - lo;
}

template <typename T, size_t ...Is>
inline constexpr const auto makeTypeName(std::index_sequence<Is...>) {
    constexpr auto& buffer = __PRETTY_FUNCTION__;
    constexpr size_t lo = findFirstOf(buffer, '=') + 2;
    return MetaString<buffer[lo + Is]..., '\0'>{};
}

// 获取解析好的MetaString
template <typename T>
inline constexpr const auto getTypeName() {
    constexpr size_t length = getClassNameLength<T>();
    std::make_index_sequence<length> sequence;
    return makeTypeName<T>(sequence);
}

int main() {
    Foo<std::vector<std::string>> foo;
    constexpr auto typeName = getTypeName<decltype(foo)>()();
    dbg(typeName); // 期望输出：Foo<std::vector<std::__cxx11::basic_string<char> > >
    return 0;
}