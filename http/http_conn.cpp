/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-08 17:35:14
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-22 09:59:11
 * @FilePath: /Project/my_Server/http/http_conn.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */

#include "../log/log.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string>
#include "http_conn.h"
//初始化http_conn的静态变量


char* http_conn::webRoot=nullptr;
std::atomic<int> http_conn::userCount{0}; // 注意这里使用初始化列表而非store函数
void http_conn::init(int connectfd,const sockaddr_in& client)
{
    connectFd=connectfd;
    this->client=client;
    this->request.init(client);
    //初始化response
    http_conn::userCount.fetch_add(1);
}
void http_conn::process(){
    // 执行到process说明已经将当前connectFd内的数据读取完毕，但http请求可能仍然不完整
    LOG_INFO("process");
    int ret=request.parse();
    printf("method: %s\n,url: %s\n",request.getMethod().c_str(),request.getUrl().c_str());
    //request.parse()只会返回3种结果，NO_request,BAD_request,GET_request;
    if(ret==httpRequest::NO_REQUEST){
        //没有获取完整的请求
        //这里可以改一下，大文件上传。
        LOG_INFO("%s", "NO_REQUEST");
        code=NO_REQUEST;
        epollUtil::instance().modfd(connectFd,EPOLLIN);
        return;
    }
    //获取完整的请求或者错误请求
    else if(ret==BAD_REQUEST){
        //给用户写回错误信息
        code=BAD_REQUEST;
        LOG_ERROR("%s", "BAD_REQUEST");
        this->response.init(realFile,code,this->request.isLinger());
        this->response.makeResponse();
        epollUtil::instance().modfd(connectFd,EPOLLOUT);
        // return;
    }
    else{
        //获取完整的请求
        if(request.getMethod()=="POST"){
            //给用户返回成功信息
            code=GET_REQUEST;
            LOG_INFO("method:POST,url:%s",request.getUrl().c_str());
            std::string url=request.getUrl();
            realFile=string(webRoot)+url;
            this->response.init(realFile,code,this->request.isLinger());
            this->response.makeResponse();
            epollUtil::instance().modfd(connectFd,EPOLLOUT);
            // request.init();
            // return;
        }
        else if(request.getMethod() == "GET"){
            //给用户返回成功信息
            std::string url=request.getUrl();
            if(url[0]=='/'&&url.size()==1)
            {   //网站首页
                realFile=string(webRoot)+"/index.html";
            }
            else{
                //访问特定页面
                realFile=string(webRoot)+url;
            }
            LOG_INFO("request realFile:[%s]",realFile.c_str());
            //先判断页面是否存在
            printf("stat:%d\n",stat(realFile.c_str(),&fileState));
            if(stat(realFile.c_str(),&fileState)<0){
                //不存在
                code=NO_RESOURCE;
                LOG_ERROR("stat error:[%s],file is not exist!",realFile.c_str());
                this->response.init(realFile,code,this->request.isLinger());
                this->response.makeResponse();
                epollUtil::instance().modfd(connectFd,EPOLLOUT);
                // request.init();
                // return;
            }
            if(!(fileState.st_mode&S_IROTH)){
                //无权限
                code =FORBIDDEN_REQUEST;
                LOG_ERROR("stat error:[%s],file is not readable!",realFile.c_str());
                this->response.init(realFile,code,this->request.isLinger());
                this->response.makeResponse();
                epollUtil::instance().modfd(connectFd,EPOLLOUT);
                printf("makeresponse success!");
                request.init();
                return;
            }
            if(S_ISDIR(fileState.st_mode)){
                //是目录
                code=BAD_REQUEST;
                LOG_ERROR("[%s]s is dir!",realFile.c_str());
                this->response.init(realFile,code,this->request.isLinger());
                this->response.makeResponse();
                epollUtil::instance().modfd(connectFd,EPOLLOUT);
                // request.init();
                // return ;
            }
            code=FILE_REQUEST;
            LOG_INFO("[%s] is exist!file size is: %ld",realFile.c_str(),fileState.st_size);
            this->response.init(realFile,code,this->request.isLinger());
            this->response.makeResponse();
            epollUtil::instance().modfd(connectFd,EPOLLOUT);
            // request.init();
            // return ;
        }
        else{
            //不支持改http请求方法
            code=BAD_REQUEST;
            LOG_ERROR("不支持的http请求方法"); 
            this->response.init(realFile,code,this->request.isLinger());
            this->response.makeResponse();
            epollUtil::instance().modfd(connectFd,EPOLLOUT);
            // request.init();
            // return ;
        }
    }
    printf("重置request对象\n");
    request.init();
    return ;
}
void http_conn::closeConn(){
    if(connectFd!=-1){
        epollUtil::instance().removefd(connectFd);
        close(connectFd);
    }
}