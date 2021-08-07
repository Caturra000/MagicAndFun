#include <bits/stdc++.h>

struct LazyPolicy {
    struct Lazy {};
    struct Construct {};
};

template <typename T>
class Lazy {
public:
    Lazy(): _actived(false) {}

    Lazy(LazyPolicy::Lazy): Lazy() {}

    template <typename ...Args, typename Ignored = void>
    Lazy(LazyPolicy::Construct, Args &&...args) {
        init(std::forward<Args>(args)...);
    }

    ~Lazy() {
        if(_actived) {
            get().~T();
            _actived = false;
        }
    }

    Lazy(const Lazy &that) {
        if(!that._actived) {
            _actived = false;
        } else {
            this->copy(that.get());
        }
    }

    Lazy(Lazy &&that) {
        if(!that._actived) {
            _actived = false;
        } else {
           this->move(std::move(that.get()));
        }
    }

    Lazy& operator=(const Lazy &that) {
        if(this == &that) return *this;
        this->~Lazy();
        if(that._actived) {
            this->copy(that.get());
        }
        return *this;
    }

    Lazy& operator=(Lazy &&that) {
        if(this == &that) return *this;
        this->~Lazy();
        if(that._actived) {
            this->move(static_cast<T&&>(that.get()));
        }
        return *this;
    }

    T& get() {
        return *reinterpret_cast<T*>(_storage);
    }

    const T& get() const {
        return *reinterpret_cast<const T*>(_storage);
    }

private:
    template <typename ...Args>
    void init(Args &&...args) {
        new (_storage) T(std::forward<Args>(args)...);
        _actived = true;
    }

    void copyOrMove(T instance) {
        new (_storage) T(std::move(instance));
        _actived = true;
    }

    void copy(const T &instance) {
        new (_storage) T(instance);
        _actived = true;
    }

    void move(T &&instance) {
        new (_storage) T(std::move(instance));
        _actived = true;
    }

private:
    // TODO force align?
    char _storage[sizeof(T)];
    bool _actived;
};