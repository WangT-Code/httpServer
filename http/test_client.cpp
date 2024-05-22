/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-20 11:07:59
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-22 10:36:17
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
    const char writebuf[8192]="GET /images/1.jpg HTTP/1.1\r\nAccept: image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8\r\nAccept-Encoding: gzip, deflate\r\nAccept-Language: zh-CN,zh;q=0.9,en;q=0.8,en-GB;q=0.7,en-US;q=0.6\r\nConnection: keep-alive\r\nHost: 192.168.159.170:8080\r\nReferer: http://192.168.159.170:8080/picture.html\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36 Edg/124.0.0.0\r\n\r\n";
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
    int readEdIdx=0;
    while(1){
        int res=recv(sockfd,receiveBuf+readEdIdx,8192-readEdIdx,0);
        if(res<0){
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                continue;
            }
            else break;
        }
        if(res==0){
            printf("对方关闭连接\n");
            break;
        }
        readEdIdx+=res;
        if(readEdIdx==1566336){
            printf("接受完毕\n");
            break;
        }
    }
    printf("receive:\n%s",receiveBuf);
    close(sockfd);
}