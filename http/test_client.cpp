/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-20 11:07:59
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-21 12:17:19
 * @FilePath: /Project/my_Server/http/test_client.cpp
 * @Description:    
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <string.h>
#include <iostream>
#include <unistd.h>
using std::cout;
using std::endl;
int main(){
    const char writebuf[8192]="GET /my HTTP/1.1\r\nHost: 127.0.0.1\r\nOrigin: http://127.0.0.1\r\nConnection: keep-alive\r\n\r\n";
    std::string str(writebuf);
    cout<<"发送个字节数量：\n"<<str.length()<<endl;
    char receiveBuf[8192];
    memset(receiveBuf,'\0',8192);
    int sockfd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr=inet_addr("192.168.159.170");
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(8080);

    connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr));
    if(send(sockfd,writebuf,str.size(),0)<0){
        printf("错误\n");
    }
    printf("发送成功\n");
    
    int ret=recv(sockfd,receiveBuf,8192,0);
    printf("receive:\n%s",receiveBuf);
    close(sockfd);
}