/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-06-02 15:46:44
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-06-04 11:14:18
 * @FilePath: /Project/my_Server/webserver/webserver.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "webserver.h"
#include "../threadpool/threadpool.h"
#include "../http/util.h"
#include "../timer/heaptimer.h"
webserver::webserver(int port,  int timeoutMs, bool optLinger,
        int sqlPort, const char* sqlUser, const char* sqlPwd,
        const char* dbName, int connPoolNum,
        int threadNum, int maxRequests,
        bool openLog, int logLevel, int logQueSize):timer(new heapTimer(1024)){
            this->port=port;
            this->timeoutMs=timeoutMs;
            this->openLinger=optLinger;
            this->isStop=false;
            webRoot=getcwd(nullptr,256);//获取当前工作目录
            strncat(webRoot,"/resources",16);
            
            epollUtil::instance().setupepollfd(epoll_create(10));
            epollUtil::instance().init(&events[0],timeoutMs);
            // 创建监听套接字,并绑定
            initSocket();
            // 初始化线程池
            threadpool<http_conn>::instance()->init(threadNum,maxRequests);

            // 初始化数据库连接池
            sql_conn_pool::instance()->init("localhost",sqlPort,sqlUser,sqlPwd,dbName,connPoolNum);
            
            //初始化http_conn::userCount和http_conn::webRoot
            http_conn::userCount.store(0,std::memory_order_relaxed);
            http_conn::webRoot=this->webRoot;

            // 初始化日志系统
            if(openLog){
                printf("开启日志\n");
                log::getInstance()->init(false,"/home/wt/Project/my_Server/log/logFile",10240,5000000,logQueSize);
                LOG_INFO("========== Server init ==========");
                LOG_INFO("Port:%d, OpenLinger: %s", port, optLinger? "true":"false");
                
                LOG_INFO("LogSys level: %d", logLevel);
                LOG_INFO("srcDir: %s", webRoot);
                LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
            }
            
        }
webserver::~webserver(){
    close(listenSocket);
    isStop=true;
    free(webRoot);
    sql_conn_pool::instance()->destroy();
}
void webserver::start(){
    int timeMs=-1;//默认一直阻塞
    if(!isStop){
        LOG_INFO("========= Server start =========");  
    }
    else{
        LOG_INFO("========= Server stop =========");
    }
    while(!isStop){
        if(timeoutMs>0){
            //获取当前最近要到期的时间
            timeMs=timer->getNextTick();
        }
        //定时事件是通过epollwait实现，而不是通过信号来实现
        int eventCnt=epollUtil::instance().wait(timeMs);
        LOG_INFO("enentCnt:[%d]",eventCnt);
        for(int i=0;i<eventCnt;i++){
            int fd=epollUtil::instance().getFd(i);
            uint32_t events=epollUtil::instance().getEvents(i);
            if(fd==listenSocket){
                //有新连接
                dealListen();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                closeConn(fd);
            }
            else if(events & EPOLLIN){
                //有数据可读
                dealRead(fd);
            }
            else if(events & EPOLLOUT){
                //有数据可写
                dealWrite(fd);
            }else{
                LOG_ERROR("unknown event");
            }
        }
    }
}
void webserver::addClient(int fd,sockaddr_in addr){
    //重置http_conn
    users[fd].init(fd,addr);

    ///std::memory_order_relaxed保证写操作的原子性，是最宽松的级别
    http_conn::userCount.fetch_add(1,std::memory_order_relaxed);
    //添加计时器
    if(timer){
        timer->add(fd,timeoutMs,std::bind(&webserver::closeConn,this,fd));
    }
    //将fd添加到epoll中
    epollUtil::instance().addfd(fd,true);
    LOG_INFO("Client[%d](%s:%d) in, user count:%d",fd,users[fd].getIp(),users[fd].getPort(),http_conn::userCount.load(std::memory_order_acquire));
}
void webserver::dealListen(){
    if(http_conn::userCount>=MAX_FD){
        LOG_ERROR("too many clients");
        return;
    }
    //因为时水平触发模式，所以要一次性读取完
    struct sockaddr_in addr;
    socklen_t len=sizeof(addr);
    while(true){
        int connfd=accept(listenSocket,(struct sockaddr*)&addr,&len);
        if(connfd<0){
            LOG_ERROR("accept error");
            break;
        }
        addClient(connfd,addr);
        users[connfd].init(connfd,addr);
    }
    return;
}
void webserver::dealRead(int fd){
    int err;
    if(users[fd].read(&err)){
        //读取完毕，添加任务
        threadpool<http_conn>::instance()->append(&users[fd]);
        //重置定时器
        timer->adjust(fd,timeoutMs);
    }
    else {
        if(err==EAGAIN||err==EWOULDBLOCK){
            if(users[fd].getRequest().getCode()==0){
                //请求不完整，从新注册EPOLLIN|EPOLLONESHOT事件
                epollUtil::instance().modfd(fd,EPOLLIN);
                timer->adjust(fd,timeoutMs);
            }
            else{
                //请求出错
                closeConn(fd);
            }
        }
        else{
            //读错误
            closeConn(fd);
        }
    }
}
void webserver::dealWrite(int fd){
    int err;
    if(users[fd].write(&err)){
        //epollout事件的注册已经下放到write中
        //写完了，添加任务
        timer->adjust(fd,timeoutMs);
        // if(!users[fd].getRequest().isLinger()){
        //     // 不是长连接，直接关闭连接
        //     closeConn(fd);
        // }
    }
    else{
        //出错
        closeConn(fd);
    }
}
void webserver::closeConn(int fd){
    if(fd<0){
        return;
    }
    //删除定时器
    timer->deleteWithFd(fd);
    //关闭连接
    users[fd].closeConn();
    LOG_INFO("ip:[%s],port:[%d],socket:[%d] quit",users[fd].getIp(),users[fd].getPort(),fd);
    
}
void webserver::initSocket(){
    int ret;
    struct sockaddr_in addr;
    if(port>65536||port<0){
        LOG_ERROR("port :%d error",port);
        isStop=true;
        return;
    }
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_family=AF_INET;
    addr.sin_port=htons(port);
    struct linger optLinger={0};
    if(openLinger){
        optLinger.l_onoff=1;
        optLinger.l_linger=20;
    }
    listenSocket=socket(AF_INET,SOCK_STREAM,0);
    if(listenSocket<0){
        LOG_ERROR("create Listensocket error");
        isStop=true;
        return;
    }
    ret=setsockopt(listenSocket,SOL_SOCKET,SO_LINGER,&optLinger,sizeof(optLinger));
    if(ret<0){
        close(listenSocket);
        LOG_ERROR("set socket linger error");
        isStop=true;
        return;
    }
    int optval=1;
    ret=setsockopt(listenSocket,SOL_SOCKET,SO_REUSEADDR,(const void*)&optval,sizeof(optval));
    if(ret<0){
        close(listenSocket);
        LOG_ERROR("set socket reuse error");
        isStop=true;
        return;
    }
    ret=bind(listenSocket,(struct sockaddr*)&addr,sizeof(addr));
    if(ret<0){
        close(listenSocket);
        LOG_ERROR("bind error");
        isStop=true;
        return;
    }
    ret=listen(listenSocket,5);
    if(ret<0){
        close(listenSocket);
        LOG_ERROR("listen error");
        isStop=true;
        return ;
    }
    epollUtil::instance().addfd(listenSocket,false);
    LOG_INFO("Server port: %d",port);
    return;
}