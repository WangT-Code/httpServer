/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-06-02 15:46:28
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-08-14 10:01:04
 * @FilePath: /Project/my_Server/webserver/webserver.h
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "../http/http_conn.h"
#include "../lock/lock.h"
#include "../timer/heaptimer.h"

class webserver{
public:
    webserver(int port, int timeoutMs, bool optLinger,
        int sqlPort, const char* sqlUser, const char* sqlPwd,
        const char* dbName, int connPoolNum,
        int threadNum, int maxRequests,
        bool openLog, int logLevel, int logQueSize);
    ~webserver();
    void start();
private:
    void initSocket();//创建监听套接字，并向epoll注册事件
    void dealListen();
    void onRead(http_conn* client);
    void onWrite(http_conn* client);
    void onProcess(http_conn* client);
    void dealRead(http_conn* client);
    void dealWrite(http_conn* client);
    void closeConn(int fd);
    void addClient(int fd,sockaddr_in addr);
private:
    int listenSocket;
    int timeoutMs;
    bool isStop;//是否停止运行服务器
    bool openLinger;//优雅关闭
    int port;
    static const int MAX_FD=65535;
    char* webRoot;
    struct epoll_event events[1024];
    std::unique_ptr<heapTimer> timer;
    unordered_map<int,http_conn> users;
};