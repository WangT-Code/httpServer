/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-09 21:49:48
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-10 11:17:50
 * @FilePath: /Project/my_Server/http/httrequest_test.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "./httprequest.h"
#include <arpa/inet.h>
int main()
{
    sockaddr_in client;
    log::getInstance()->init(false,"../log/logFile");
    sql_conn_pool::instance()->init("127.0.0.1",3306,"wt","200003228631","webdb",10);
    client.sin_port = htons(80);
    client.sin_addr.s_addr = inet_addr("127.0.0.1");
    httpRequest request;
    request.init(client);

    //验证登录
    const char login_buf[8192]="POST /login.html HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nHost: 127.0.0.1\r\nContent-Length: 44\r\nOrigin: http://127.0.0.1\r\nConnection: keep-alive\r\n\r\nusername=2421177510&password=wt200003228631";
    //验证注册
    const char register_buf[8192]="POST /register.html HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nHost: 127.0.0.1\r\nContent-Length: 44\r\nOrigin: http://127.0.0.1\r\nConnection: keep-alive\r\n\r\nusername=3114417423&password=wt200003228631";
    //验证哪个功能就传入哪个报文
    std::string str(register_buf);
    memcpy(request.buf(),str.c_str(),str.size());
    printf("len:%d,%s\n",strlen(request.buf()),request.buf());
    // printf("解析一行\n");
    request.setReadedID();
    
    printf("\n%d\n",request.parse());
    std::unordered_map<std::string,std::string> user=request.getUser();
    std::unordered_map<std::string,std::string> headers=request.getHeaders();
    //遍历并打印user
    // 打印request中的method,path,version
    printf("mtrhod %s,len %ld\n",request.getMethod().c_str(),request.getMethod().length());
    printf("url %s,len:%ld\n",request.getUrl().c_str(),request.getUrl().length());
    printf("version %s,len:%ld\n",request.getVersion().c_str(),request.getVersion().length());
    
    //遍历并打印headers
    printf("\nhreades len:%ld\n",headers.size());
    for(auto it=headers.begin();it!=headers.end();it++)
    {
        printf("%s| : |%s\n",it->first.c_str(),it->second.c_str());
    }
    printf("user size:%d\n",user.size());
    for(auto it=user.begin();it!=user.end();it++)
    {
        printf("%s:%s\n",it->first.c_str(),it->second.c_str());
    }
}