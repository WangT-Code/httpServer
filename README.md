## 描述  

- 使用 Reactor 高并发模型处理多个连接，使得服务器可以同时处理多个客户端连接；
- 实现了HTTP协议的解析，支持GET和POST请求；
- 使用 Epoll 多路复用机制高效处理I/O，监听和处理客户端连接事件和数据传输事件；
- 实现定时功能，关闭超时的非活动连接，并用小根堆作为容器管理定时器；
- 设计并实现了异步日志模块，使用单例模式和阻塞队列实现日志的异步写入，避免了同步写入的性能损失；
- 利用RAII机制实现数据库连接池，减少数据库连接建立与关闭的开销；
- 实现文件上传功能；

```shell
make
./bin/mywebserver
```

端口默认为8080

## 效果展示

![image](https://github.com/WangT-Code/httpServer/blob/master/1.gif)
![image](https://github.com/WangT-Code/httpServer/blob/master/2.gif)
![image](https://github.com/WangT-Code/httpServer/blob/master/3.gif)
