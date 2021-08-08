最近在写一个简单的promise-future模型（STL提供的实在过于简陋了）

一般来说它的设计就是promise - shared state - future

这个时候state一般有T value / ExceptionPtr / Enum等内容

这个T value就是对应于promise.setValue()和future.get()的值

很显然，value构造的时机是setValue而不是shared state构造的时候

也就是说，最常见的内存布局会导致多调用了一次默认构造函数

常见的解决方法是用`T *pValue`来替代，用`nullptr`来表示没有构造

但这样内存布局由过于零散了，分配的空间也不在shared state的范围内

也可以用零长数组来代替，但是不是太复杂了点，毕竟shared state本身需要shared ptr作为容器封装，`make_shared`里面都不知道该怎么传参好了

然后又想到刚好有placement new这个东西，或许就是用来解决这个问题的，就尝试写一版本吧

名字先随便称为Lazy好了，表示延迟构造

（话说这个东西是不是`std::optional`的低配版？）


------------------

TODO

- 如果T是`POD`直接用`memcpy`就好了
- 构造函数是不是太难看了点


-----------

update. 知乎水贴时发现其实用`union`会比直接操作`char[sizeof]`更好实现
