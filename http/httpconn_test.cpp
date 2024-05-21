/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-14 15:14:17
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-20 09:53:47
 * @FilePath: /Project/my_Server/http/httpconn_test.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-14 15:14:17
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-14 15:57:48
 * @FilePath: /Project/my_Server/http/httpconn_test.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include "http_conn.h"
#include "../log/log.h"
#include "httprequest.h"
#include "httpresponse.h"
using namespace std;

int main(){
    // 初始化
    
    // http_conn::webRoot=;
    // int epollfd=epoll_create(5);
    // epollUtil::epollFd=epollfd;
    // 初始化日志和数据库连接池
    log::getInstance()->init(false,"../log/logFile");
    sql_conn_pool::instance()->init("127.0.0.1",3306,"wt","200003228631","webdb",10);
    sockaddr_in client;
    client.sin_family = AF_INET;
    client.sin_port = htons(80);
    client.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    //报文
    //验证登录
    const char login_buf[8192]="GET /video.html HTTP/1.1\r\nHost: 127.0.0.1\r\nOrigin: http://127.0.0.1\r\nConnection: keep-alive\r\n\r\n";
    string str(login_buf);
    http_conn my_conn(1,client);
    memcpy(my_conn.getRequest().getBuf(),str.c_str(),str.size());
    my_conn.getRequest().setReadedID();
    my_conn.webRoot="../resources";
    my_conn.process();
    
    //看解析是否成功
    cout<<"====================================="<<endl;
    printf("mtrhod %s,len %ld\n",my_conn.getRequest().getMethod().c_str(),my_conn.getRequest().getMethod().length());
    printf("url %s,len:%ld\n",my_conn.getRequest().getUrl().c_str(),my_conn.getRequest().getUrl().length());
    printf("version %s,len:%ld\n",my_conn.getRequest().getVersion().c_str(),my_conn.getRequest().getVersion().length());
    //遍历并打印headers
    cout<<"====================================="<<endl;
    std::unordered_map<std::string,std::string> headers=my_conn.getRequest().getHeaders();
    printf("\nhreades len:%ld\n",headers.size());
    for(auto it=headers.begin();it!=headers.end();it++)
    {
        printf("%s| : |%s\n",it->first.c_str(),it->second.c_str());
    }
    cout<<"====================================="<<endl;
    printf("response content：\n");
    writev(STDOUT_FILENO,my_conn.getResponse().getIovec(),2);
    printf("\n===================================\n");
    write(STDOUT_FILENO,my_conn.getResponse().getBuf(),my_conn.getResponse().getWriteEdIdx());
    write(STDOUT_FILENO,my_conn.getResponse().getFileAddress(),my_conn.getResponse().getFileLen());
    cout<<"\n====================================="<<endl;
}