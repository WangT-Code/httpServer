/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-08 17:35:02
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-23 14:59:37
 * @FilePath: /Project/my_Server/http/http_conn.h
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#ifndef HTTPCONN_CPP
#define HTTPCONN_CPP
#include "../mysql_connection/sql_conn_pool.h"
#include <sys/epoll.h>
#include <netinet/in.h>
#include <atomic>
#include <unistd.h>
#include <string>
#include <string>
#include "util.h"
#include "../log/log.h"
#include "httprequest.h"
#include "httpresponse.h"
using std::atomic;
class http_conn
{

public:
    static const int FILENAMELEN=200;
    static const int READBUFSIZE=8192;
    static const int WRITEBUFSIZE=8192;
    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE
    {
        NO_REQUEST=0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
public:
    http_conn(){};
    http_conn(int connectfd,const sockaddr_in& client){init(connectfd,client);};
    ~http_conn(){};
public:
    void init(int connectfd,const sockaddr_in& client);
    void process();
    void closeConn();
    bool read(int* saveErrno){
        return request.read(connectFd,saveErrno);
    };
    bool write(int* saveErrno){
        bool ans = response.write(connectFd,saveErrno);
        request.init();
        return ans;
    };
    // bool process_read();
    // bool process_write();
public:
    static atomic<int> userCount; 
    static char* webRoot;
    static void setWebRoot(char* webRoot){
        http_conn::webRoot=webRoot;
    }
private:
    std::string realFile;
    int connectFd;//通信套接字
    sockaddr_in client;//客户信息
    HTTP_CODE code;//状态码
    struct stat fileState;
    char* fileAddress;
    int ret;
    httpRequest request;
    httpResponse response;
public:
    httpRequest& getRequest(){return request;}
    httpResponse& getResponse(){return response;}
};
#endif