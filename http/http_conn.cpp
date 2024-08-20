/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-08 17:35:14
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-08-12 17:26:44
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

unsigned char FromHex(unsigned char x)
{
	unsigned char y=0;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	// else assert(0);s
	return y;
}
std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		if (str[i] == '+') strTemp += ' ';
		else if (str[i] == '%')
		{
			// assert(i + 2 < length);s
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}
char* http_conn::webRoot=nullptr;
std::atomic<int> http_conn::userCount; // 注意这里使用初始化列表而非store函数
void http_conn::init(int connectfd,const sockaddr_in client)
{
    connectFd=connectfd;
    this->client=client;
    this->request.init(client,webRoot);
    //初始化response
    http_conn::userCount.fetch_add(1);
}
void http_conn::process(){
    // 执行到process说明已经将当前connectFd内的数据读取并解析完毕
    // LOG_INFO("process");
    //获取请求解析的结果
    ret=request.getCode();
    if(ret==httpRequest::NO_REQUEST){
        //没有获取完整的请求
        LOG_INFO("NO_REQUEST");
        code=NO_REQUEST;
        epollUtil::instance().modfd(connectFd,EPOLLIN);
        return;
    }
    //获取完整的请求或者错误请求
    else if(ret==BAD_REQUEST){
        //给用户写回错误信息
        code=BAD_REQUEST;
        LOG_ERROR("BAD_REQUEST");
        this->response.init(realFile,code,this->request.isLinger());
        this->response.makeResponse();
        epollUtil::instance().modfd(connectFd,EPOLLOUT);
        // return;
    }
    else{
        //获取完整的请求
        LOG_ERROR("")
        if(request.getMethod()=="POST"){
            //给用户返回成功信息
            code=GET_REQUEST;
            // LOG_INFO("method:POST,url:%s",request.getUrl().c_str());
            std::string url=request.getUrl();
            realFile=string(webRoot)+url;
            this->response.init(realFile,code,this->request.isLinger());
            this->response.makeResponse();
            epollUtil::instance().modfd(connectFd,EPOLLOUT);
            // request.init();
            // return;
        }
        else if(request.getMethod() == "GET"){
            // printf("GET 方法\n");
            //给用户返回成功信息
            std::string url=request.getUrl();
            if(url[0]=='/'&&url.size()==1)
            {   //网站首页
                realFile=string(webRoot)+"/index.html";
            }
            else{
                //访问特定页面
                string tempWebRoot=string(webRoot);
                //对url解码
                string decondeUrl=UrlDecode(url);
                realFile=tempWebRoot+decondeUrl;
                //如果访问的时download页面则需要更新list.json文件
                if(url=="/download.html"||url=="/download"){
                    //获取files文件目录下的所有文件名字
                    LOG_INFO("upload list.json");
                    auto files=getFiles(tempWebRoot+"/files");
                    Json::Value root;
                    Json::Value file;
                    for (int i = 0; i < (int)files.size(); i++)
                    {   
                        // LOG_INFO("s",files[i].c_str());
                        file["filename"] = files[i];
                        root.append(file);
                    }
                    std::ofstream ofs(tempWebRoot+"/list.json");
                    writeJson(ofs,root);
                    ofs.close();
                }
            }

            // LOG_INFO("request realFile:[%s]",realFile.c_str());
            //先判断页面是否存在
            // printf("stat:%d\n",stat(realFile.c_str(),&fileState));
            if(stat(realFile.c_str(),&fileState)<0){
                //不存在
                code=NO_RESOURCE;
                LOG_ERROR("stat error:[%s],file is not exist!",realFile.c_str());
                this->response.init(realFile,code,this->request.isLinger());
                this->response.makeResponse();
                epollUtil::instance().modfd(connectFd,EPOLLOUT);
                request.init();
                return;
            }
            if(!(fileState.st_mode&S_IROTH)){
                //无权限
                code =FORBIDDENT_REQUEST;
                LOG_ERROR("stat error:[%s],file is not readable!",realFile.c_str());
                this->response.init(realFile,code,this->request.isLinger());
                this->response.makeResponse();
                epollUtil::instance().modfd(connectFd,EPOLLOUT);
                // printf("makeresponse success!");
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
            // LOG_INFO("[%s] is exist!file size is: %ld",realFile.c_str(),fileState.st_size);
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
    // printf("重置request对象\n");
    request.init();
    return ;
}
void http_conn::closeConn(){
    response.unMapFile();
    request.clear();
    if(connectFd!=-1){
        epollUtil::instance().removefd(connectFd);
        close(connectFd);
        userCount--;
        LOG_INFO("close %d success!",connectFd);
    }
}
vector<string> http_conn::getFiles(string dir){
    // LOG_INFO("getfiles:[%s]",dir.c_str());
    vector<string> files;
    DIR* pDir=nullptr;
    struct dirent* pEnt=NULL;
    pDir=opendir(dir.c_str());
    if(pDir==nullptr){
        LOG_ERROR("open dir error");
        return files;
    }
    while (1)
    {
        pEnt=readdir(pDir);
        if(pEnt!=NULL){
            //忽略隐藏目录
            if(strcmp(pEnt->d_name,".")==0||strcmp(pEnt->d_name,"..")==0)continue;
            files.push_back(pEnt->d_name);
        }
        else break;
    }
    return files;
}
void http_conn::writeJson(std::ofstream& ofs,Json::Value& root){
    std::ostringstream os;
    Json::StreamWriterBuilder writerBuilder;
    std::unique_ptr<Json::StreamWriter> jsonWriter(writerBuilder.newStreamWriter());
    jsonWriter->write(root,&os);
    ofs<<os.str();
    return;
}