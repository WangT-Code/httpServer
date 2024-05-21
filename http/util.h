#ifndef UTIL_H
#define UTIL_H

#include<sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include<sys/epoll.h>
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
public:
     void setupepollfd(int eFd){
        epollFd=eFd;
    }
    int getepollfd(){
        return epollFd;
    }
    int wait(epoll_event* events,int MAXEVENTS){
        return epoll_wait(epollFd,events,MAXEVENTS,-1);
    }
     void addfd(int fd,bool oneshot){
        epoll_event event;
        event.data.fd=fd;
        event.events=EPOLLET|EPOLLIN|EPOLLRDHUP;
        if(oneshot)
            event.events|=EPOLLONESHOT;
        epoll_ctl(epollFd,EPOLL_CTL_ADD,fd,&event);
        setnonblocking(fd);
    }
     void removefd(int fd){
        epoll_ctl(epollFd,EPOLL_CTL_DEL,fd,nullptr);
        close(fd);
    }
     void modfd(int fd,int ev){
        epoll_event event;
        event.data.fd=fd;
        event.events=ev|EPOLLET|EPOLLONESHOT|EPOLLRDHUP;
        epoll_ctl(epollFd,EPOLL_CTL_MOD,fd,&event);
    }
    void setIn(int fd){
        epoll_event event;
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
};
#endif