/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-20 10:54:29
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-21 22:04:59
 * @FilePath: /Project/my_Server/http/test_server.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-20 10:54:29
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-20 11:38:42
 * @FilePath: /Project/my_Server/http/test_server.cpp
 * @Description: 
 * 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <bits/socket.h>
#include <netdb.h>
#include <sys/socket.h>
#include "http_conn.h"
#include "../log/log.h"
#include "httprequest.h"
#include "httpresponse.h"
int main(){

    //初始化数据库连接池，日志系统
    log::getInstance()->init(false,"../log/logFile");
    sql_conn_pool::instance()->init("127.0.0.1",3306,"wt","200003228631","webdb",10);
    
    int listenFd=socket(AF_INET,SOCK_STREAM,0);
    if(listenFd<0){
        LOG_ERROR("socket error");
        return -1;
    }
    struct sockaddr_in server;
    server.sin_family=AF_INET;
    server.sin_addr.s_addr=INADDR_ANY;
    server.sin_port=htons(8080);
    
    if(bind(listenFd,(sockaddr*)&server,sizeof(server))<0){
        LOG_ERROR("bind error");
        return -1;
    }
    // 设置端口复用
    int optval=1;
    setsockopt(listenFd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));

    if(listen(listenFd,5)<0){
        LOG_ERROR("listen error");
        return -1;
    }
    int max_event_num=1024;
    struct epoll_event events[1024];
    struct epoll_event ev;
    ev.data.fd=listenFd;
    ev.events=EPOLLIN;
    
    int efd=epoll_create(5);
    epollUtil::instance().setupepollfd(efd);
    if(epoll_ctl(epollUtil::instance().getepollfd(),EPOLL_CTL_ADD,listenFd,&ev)<0){
        LOG_ERROR("epoll_ctl error");
        return -1;
    }

    // 网站根目录
    std::string tempWeb("../resources");
    http_conn::setWebRoot("../resources");
    http_conn conn[100];
    while(1){
        int res=epollUtil::instance().wait(events,max_event_num);
        // if(res=-1)LOG_ERROR("epoll wait");
        for(int i=0;i<res;i++){
            if(events[i].data.fd==listenFd){
                printf("有新客户端接入\n");
                struct sockaddr_in client;
                socklen_t len=sizeof(client);
                int connFd=accept(listenFd,(sockaddr*)&client,&len);
                printf("通信sockfd:%d\n",connFd);
                if(connFd<0){
                    LOG_ERROR("accept error");
                    return 0;
                }
                LOG_INFO("accept a new client:%d",connFd);
                http_conn::userCount++;
                conn[connFd].init(connFd,client);
                epollUtil::instance().addfd(connFd,1);
            }
            else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                http_conn::userCount--;
                printf("对方关闭连接\n");
                LOG_DEBUG("%d closed!",events[i].data.fd);
                conn[events[i].data.fd].closeConn();//关闭连接:将连接从epoll中删除，关闭了fd
            }
            else if(events[i].events&EPOLLIN){
                int err;
                printf("开始读取内容\n");
                printf("%d\n",events[i].data.fd);
                conn[events[i].data.fd].getRequest().init();
                if(conn[events[i].data.fd].read(&err)){
                    printf("读取成功\n");
                    printf("读取完成，开始处理请求\n");
                    conn[events[i].data.fd].process();
                }
                else{
                    conn[events[i].data.fd].closeConn();
                }
            }
            else if(events[i].events&EPOLLOUT){
                printf("开始返回报文\n");
                int err;
                conn[events[i].data.fd].write(&err);
                printf("%d返回完毕\n",events[i].data.fd);
            }
        }
    }

}