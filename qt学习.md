# MVC设计模式
https://www.zhihu.com/question/22886622/answer/48378638?utm_psn=1766829912575586304

mvc关键点：

1. 这里的模型（Model）或 Model Object 通常指的是应用逻辑层（也叫领域层）的对象，如 Account、Order 等等。这些对象是你开发的应用程序中的一些核心对象，负责应用的逻辑计算，有许多与业务逻辑有关的方法或操作（如 Account.sendEmail()、Order.calculateTotal()、Order.removeItem() 等等），而不是仅仅像 Value Object 那样用来传递数据（getter、setter）。把业务逻辑放在 MVC 的 Controller 中，这种设计是不合理的。
2. 把 Model Object 混同于 Value Object，从而得出 Model 没什么用的误解。
3. 混淆了 MVC 中的 View Controller 与整个应用的集中控制者 Application Controller。MVC 中的 Controller（也叫 View Controller，视图控制者）的主要职责是管理和处理用户的输入，并根据用户在 View 上的输入、系统当前状态和任务的性质，挑选后台合适的一些 Model 对象（也叫领域对象 Domain Object）来处理相应的业务逻辑，并把经处理后的用户输入请求等信息传递给 Model 对象。View Controller 本身不应该负责一个应用程序中业务逻辑的计算。

Controller有点像外观模式，调用各个Model，对View提供完整接口

# qt学习

## qtcore

### 事件循环
https://blog.csdn.net/kupepoem/article/details/121844578
https://blog.csdn.net/qq_26221775/article/details/136776793
https://blog.csdn.net/qq_43331089/article/details/124444497

Windows中窗口句柄的创建、线程事件队列的分发 -> QEventLoop、QThreadData(tls)、QObject

### 信号、槽
https://www.cnblogs.com/swarmbees/p/10816139.html


理解的两个角度：观察者模式，事件驱动

### 多线程注意事项
由于QObject的线程相关性，gui响应等因素，多线程的编程方式更加不同。尤其是在线程同步时，为避免阻塞gui，考虑qeventloop代替锁。

## network
qtcpsock的close方法是异步的，不能直接delete会崩溃。QObject析构函数为虚函数，使用QObject::deleteLater,利用事件循环延迟delete，待处理完所有事件后析构，更安全。