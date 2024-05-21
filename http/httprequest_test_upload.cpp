/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-09 21:49:48
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-10 11:55:21
 * @FilePath: /Project/my_Server/http/httprequest_test_upload.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "./httprequest.h"
#include <arpa/inet.h>
int main()
{
    sockaddr_in client;
    //初始化日志
    log::getInstance()->init(false,"../log/logFile");
    //初始化数据库
    sql_conn_pool::instance()->init("127.0.0.1",3306,"wt","200003228631","webdb",10);
    client.sin_port = htons(80);
    client.sin_addr.s_addr = inet_addr("127.0.0.1");
    httpRequest request;
    request.init(client);

    std::string fileContent = "Hello, World!";
    // 构造 HTTP 报文
    std::stringstream requestBody;
    requestBody << "POST /upload.html HTTP/1.1\r\n";
    requestBody << "Host: example.com\r\n";
    requestBody << "User-Agent: Your-Application/1.0\r\n";
    requestBody << "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n";
    requestBody << "Content-Length: 107\r\n";
    requestBody << "\r\n";
    requestBody << "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n";
    requestBody << "Content-Disposition: form-data; name=\"file\"; filename=\"haha.txt\"\r\n";
    requestBody << "Content-Type: text/plain\r\n";
    requestBody << "\r\n";
    requestBody << fileContent << "\r\n";
    requestBody << "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

    memcpy(request.buf(),requestBody.str().c_str(),requestBody.str().size());
    request.setReadedID();
    
    request.parse();
    
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
    
    if(request.getUrl()=="/login.html"){
        //获取request中的user，并遍历
        printf("\nuser len:%ld\n",request.getUser().size());
        for(auto it=request.getUser().begin();it!=request.getUser().end();it++)
        {
            printf("%s| : |%s\n",it->first.c_str(),it->second.c_str());
        }
    }
}