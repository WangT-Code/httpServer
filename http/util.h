#ifndef UTIL_H
#define UTIL_H

#include<sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include<sys/epoll.h>
#include "../log/log.h"
class epollUtil{
private:
    epollUtil(){};
public:
    static epollUtil& instance(){
        static epollUtil instance;
        return instance;
    }
private:
    int epollFd;
    struct epoll_event* pevents;
    int epoll_event_num;
public:
    void init(epoll_event* p,int event_num){
        pevents=p;
        epoll_event_num=event_num;
    }
     void setupepollfd(int eFd){
        if(eFd<0){
            LOG_ERROR("epoll create error!");
        }
        epollFd=eFd;
    }
    int getepollfd(){
        return epollFd;
    }
    int wait(int timeoutMs){
        return epoll_wait(epollFd,pevents,epoll_event_num,timeoutMs);
    }
     void addfd(int fd,bool oneshot){
        epoll_event event={0};
        event.data.fd=fd;
        event.events=EPOLLET|EPOLLIN|EPOLLRDHUP;
        if(oneshot)
            event.events|=EPOLLONESHOT;
        epoll_ctl(epollFd,EPOLL_CTL_ADD,fd,&event);
        setnonblocking(fd);
    }
     void removefd(int fd){
        if (fd < 0) return;
        epoll_event ev = {0};
        ev.data.fd = fd;
        epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
    }
     void modfd(int fd,int ev){
        LOG_INFO("set %d EPOLLIN",fd);
        epoll_event event={0};
        event.data.fd=fd;
        event.events=ev|EPOLLET|EPOLLONESHOT|EPOLLRDHUP;
        epoll_ctl(epollFd,EPOLL_CTL_MOD,fd,&event);
    }
    void setIn(int fd){
        epoll_event event={0};
        event.data.fd=fd;
        event.events=EPOLLIN|EPOLLET|EPOLLONESHOT|EPOLLRDHUP;
        epoll_ctl(epollFd,EPOLL_CTL_MOD,fd,&event);
    }
     int  setnonblocking(int fd){
        int old_option=fcntl(fd,F_GETFL);
        int new_option=old_option|O_NONBLOCK;
        fcntl(fd,F_SETFL,new_option);
        return old_option;
    }
    int getFd(int i){
        return pevents[i].data.fd;
    }
    uint32_t getEvents(int i){
        return pevents[i].events;
    }
};
#endif