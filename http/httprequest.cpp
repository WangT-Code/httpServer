/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-08 20:23:34
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-08-13 16:35:11
 * @FilePath: /Project/my_Server/http/httprequest.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "httprequest.h"
#include <sys/socket.h>
#include <regex>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../mysql_connection/sql_conn_pool.h"
const char CRLF[] = "\r\n";
const unordered_set<string> httpRequest::DEFAULTHTML
{
    //全部加上.html
    "/index.html","/register.html", "/login.html",
    "/welcome.html", "/video.html", "/picture.html",
    "/upload.html", "/download.html"
};
const unordered_set<string> httpRequest::DEFAULTHTMLWITHOUTHTML
{
    "/index", "/register", "/login",
    "/welcome", "/video", "/picture",
    "/upload", "/download"
};
const unordered_map<string, int> httpRequest::DEFAULTHTMLTAG
{
    {"/register.html", 0},
    {"/register", 0},
    {"/login.html", 1},
    {"/login", 1}
};
/**
 * @brief 初始化httpRequest对象
 * 
 * 该函数用于初始化httpRequest的成员变量，为后续的HTTP请求处理做准备。
 * 初始化内容包括清空读取缓冲区、设置读取索引和检查索引、初始化解析状态和内容长度等。
 * 
 * @param 无
 * @return 无
 */
void httpRequest::init(sockaddr_in client,const char* root)
{   
    // 初始化读取缓冲区为全0
    memset(&readBuf, '\0', READBUFSIZE);
    // 初始化已读取索引和已检查索引为0
    readedIdx=0;
    checkedIdx=0;
    // 初始化是否等待数据标志为false
    linger=false;
    startLine=0;
    // 初始化解析状态为请求行解析状态
    parseState=REQUEST_LINE;
    // 初始化内容长度为0
    contentLen=0;
    totalBytes=0;
    readedIdx=0;
    onceReadBodylen=0;
    code=0;
    clientAddr=client;
    curBody=0;
    method="";
    path="";
    version="";
    body="";
    boundary="";
    webRoot=root;
    headers.clear();
    postUser.clear();
    fileInfo.clear();
}
void httpRequest::init(){
     // 初始化读取缓冲区为全0
    // LOG_INFO("init() run once");
    memset(&readBuf, '\0', READBUFSIZE);
    // 初始化已读取索引和已检查索引为0
    readedIdx=0;
    checkedIdx=0;
    startLine=0;
    totalBytes=0;
    st=0;
    ed=0;
    // 初始化内容长度为0
    contentLen=0;
    code=0;
    onceReadBodylen=0;
    // 初始化解析状态为请求行解析状态
    parseState=REQUEST_LINE;
    curBody=0;
    method="";
    path="";
    version="";
    body="";
    headers.clear();
    postUser.clear();
    fileInfo.clear();
}
bool httpRequest::read(int sockfd,int * saveError){
    // sockfd为ET模式
    int byteRead=0;
    // LOG_DEBUG("ip:[%s],sockfd:[%d],begin recv",inet_ntoa(clientAddr.sin_addr),sockfd);
    memset(readBuf,'\0',READBUFSIZE);
    while(true){
        // int flag=fcntl(sockfd, F_GETFD);
        // if(flag==-1){
        //     LOG_ERROR("套接字:%d无效或者关闭",sockfd);
        // }else{
        //     LOG_INFO("套接字:%d有效",sockfd);
        // }
        byteRead=recv(sockfd,readBuf,READBUFSIZE-1,0);
        // printf("%d本次读取%d字节的数据,errno:%d\n",sockfd,byteRead,errno);
        // LOG_DEBUG("%d本次读取%d字节的数据,errno:%d",sockfd,byteRead,errno);s
        if(byteRead<0){
            //没有更多数据可读
            if((errno==EAGAIN||errno==EWOULDBLOCK)){
                if(code==GET_REQUEST){
                    // printf("total read %d bytesa \n",totalBytes);
                    // LOG_DEBUG("read all data success,total read %d bytesa",totalBytes);
                    *saveError=errno;
                    return true;
                }
                else{
                    // LOG_DEBUG("continue to read data,and registration EPOLLIN event");
                    *saveError=errno;
                    return false;
                }
            }
            LOG_DEBUG("read error");
            *saveError=errno;
            return false;
        }
        else if(byteRead==0){
            //对方关闭连接
            // LOG_DEBUG("对方关闭连接");
            return false;
        }
        onceReadBodylen=byteRead;
        readedIdx+=byteRead;
        totalBytes+=byteRead;
        code=parse();
        memset(readBuf,'\0',READBUFSIZE);
        // printf("code:%d\n",code);
        startLine=0;
    }
    code=parse();
    // printf("code:%d\n",code);
    return true;
}
httpRequest::LINE_STATUS httpRequest::parseLine()
{   
    // 请求体不用解析
    // LOG_INFO("parseLine");
    char temp;
    for(;checkedIdx<readedIdx;++checkedIdx){
        temp=readBuf[checkedIdx];
        if(temp=='\r'){
            if((checkedIdx+1)==readedIdx){
                return LINE_OPEN;
            }
            else if(readBuf[checkedIdx+1]=='\n'){
                readBuf[checkedIdx++]='\0';
                readBuf[checkedIdx++]='\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if(temp=='\n'){
            if((checkedIdx>1)&&(readBuf[checkedIdx-1]=='\r')){
                readBuf[checkedIdx-1]='\0';
                readBuf[checkedIdx++]='\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}
char* httpRequest::getLine(){
    return readBuf+startLine;
}
httpRequest::HTTP_CODE httpRequest::parseRequestLine(char* text){
    string requestLine=string(text);
    std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if (regex_match(requestLine, subMatch, pattern))
    {
        method = subMatch[1];
        path = subMatch[2];
        if(DEFAULTHTML.count(path)==0&&DEFAULTHTMLWITHOUTHTML.count(path)==1){
            path+=".html";
        }
        version = subMatch[3];
        return NO_REQUEST; //request isn't completed
    }
    LOG_ERROR("RequestLine Error");
    return BAD_REQUEST;
}
httpRequest::HTTP_CODE httpRequest::parseRequestHead(char* text){
    string line=string(text);
    //解析请求头中每一行的正则表达式
    std::regex pattern("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line,subMatch,pattern)){
        headers[subMatch[1]] = subMatch[2]; 
        if(subMatch[1] == "Content-Length"){
            contentLen = stoi(subMatch[2]);
        }
        if(subMatch[1] == "Connection"){
            linger = (subMatch[2] == "keep-alive") ;
        }
        return NO_REQUEST;
    }
    //解析到\r\n空行了
    else if(contentLen){
        onceReadBodylen-=checkedIdx;
        LOG_INFO("请求体的长度：%d",onceReadBodylen);
        parseState=BODY;
        return NO_REQUEST;
    }
    //没有请求体，说明已经解析完了
    else return GET_REQUEST;
}

httpRequest::HTTP_CODE httpRequest::parseRequestBody(char*text){
    
    //登录或者注册
    if(method=="POST"&&headers["Content-Type"]=="application/x-www-form-urlencoded")
    {   
        if(readedIdx<(contentLen+checkedIdx)){
            //可以将部分数据写入文件
            return NO_REQUEST;
        }
        text[readedIdx] = '\0';
        parseState=FINISH;
        body=text;
        curBody=text;
        parseFromUrlEncoded();//解析表单数据
        if(DEFAULTHTMLTAG.count(path)){
            int tag=DEFAULTHTMLTAG.find(path)->second;
            // LOG_DEBUG("client ip is:%s , tag:%d",inet_ntoa(clientAddr.sin_addr),tag);
            if(checkUser(tag)){
                LOG_INFO("client ip is:%s login success",inet_ntoa(clientAddr.sin_addr));
                path="/welcome.html";
            }
            else{
                LOG_INFO("client ip is:%s login fail",inet_ntoa(clientAddr.sin_addr));
                path="/error.html";
            }
        }      
    }
    //文件上传
    else if(method=="POST"&&headers["Content-Type"].find("multipart/form-data")!=std::string::npos)
    {   
        body=text;
        curBody=text;
        if(fileInfo.find("filename")==fileInfo.end()){
            //解析form-data的部分数据
            parseFormData();
        }
        //此时的checkedIdx是请求体的起始位置
        // LOG_DEBUG("readedIdx:%d",readedIdx);
        // LOG_DEBUG("checkedIdx:%d",checkedIdx);
        // LOG_DEBUG("contentLen:%d",contentLen);
        if(readedIdx<=(checkedIdx+contentLen)){
            //可以将部分数据写入文件
            //先判断是否已经获得文件名字，如果没有则返回NO_REQUEST,否则打开文件将部分数据先写入文件
            if(fileInfo.find("filename")!=fileInfo.end()){
                std::ofstream ofs;
                // 如果文件分多次发送，应该采用app，同时为避免重复上传，应该用md5做校验
                ofs.open(webRoot+"/files/" +fileInfo["filename"], std::ios::app|std::ios::binary);
                if(!ofs.is_open()){
                    LOG_ERROR("open file error");
                }
                // 解析文件内容，文件内容以\r\n\r\n开始
                // LOG_DEBUG("body:%s",body.c_str());
                if(st&&(st=body.find("\r\n\r\n"))!=string::npos){
                    st+=strlen("\r\n\r\n");
                    //这里可以加个标志比如flag=1,然后在if里面进行判断，这样就不用每次都body.find("\r\n\r\n")
                }
                else st=0;
                // LOG_DEBUG("开始位置st:%lu",st);
                if(readedIdx>=(checkedIdx+contentLen)){
                    //说明是最后一次读取
                    for(size_t i=0;i<boundary.size();++i){
                        // LOG_DEBUG("compare content is %s",text+onceReadBodylen-boundary.size()-i);
                        if(memcmp(text+onceReadBodylen-boundary.size()-i,boundary.c_str(),boundary.size())==0){
                            ed=onceReadBodylen-boundary.size()-i-2;
                            // LOG_DEBUG("onceReadBodylen :%d,checkedIdx:%d,boundary.size():%d,i:%d",onceReadBodylen,checkedIdx,boundary.size(),i);
                            // LOG_DEBUG("find ok:%s",&text[onceReadBodylen-boundary.size()-i]);
                            break;
                        }
                    }
                    // LOG_DEBUG("结束位置ed:%u",ed); 
                    ofs.write(&text[st],ed-st);
                    text[ed]='\0';
                    // LOG_DEBUG("写入的结束位置:%d",ed);
                    // LOG_INFO("client ip is:%s append content to file:[%s]!,add content length is:%d",inet_ntoa(clientAddr.sin_addr),fileInfo["filename"].c_str(),ed-st);
                    ofs.close();
                }
                else {
                    // LOG_DEBUG("结束位置ed:%lu",onceReadBodylen);
                    if(st){
                        //第一次读取时，还要去掉请求行和请求头的内容长度
                        ofs.write(&text[st],onceReadBodylen-st);
                        // LOG_DEBUG("写入的结束位置:%d",onceReadBodylen);
                        // LOG_INFO("client ip is:%s append content to file:[%s]!,add content length is:%d",inet_ntoa(clientAddr.sin_addr),fileInfo["filename"].c_str(),onceReadBodylen-st);
                        st=0;
                    }
                    else{
                        ofs.write(&text[st],onceReadBodylen-st);
                        // LOG_DEBUG("写入的结束位置:%d",onceReadBodylen);
                        // LOG_INFO("client ip is:%s append content to file:[%s]!,add content length is:%d",inet_ntoa(clientAddr.sin_addr),fileInfo["filename"].c_str(),onceReadBodylen-st);
                    }
                    ofs.close();  
                } 
            }
            if(readedIdx==(checkedIdx+contentLen)){
                // LOG_INFO("client ip is:%s upload file:%s success!",inet_ntoa(clientAddr.sin_addr),fileInfo["filename"].c_str());
                std::ofstream ofs1;
                ofs1.open(webRoot+"/response.txt", std::ios::ate);
                ofs1 << webRoot+"/response.txt" << fileInfo["filename"];
                ofs1.close();
                path = "/response.txt";
                return GET_REQUEST;
            }
            else return NO_REQUEST;
        }
        else return BAD_REQUEST;
    }
    return GET_REQUEST;
}
int httpRequest::convertHex(char ch){
    if(ch>='A'&&ch<='F')
        return ch-'A'+10;
    else if(ch>='a'&&ch<='f')
        return ch-'a'+10;
    else
        return ch-'0';
}
void httpRequest::parseFromUrlEncoded(){
    //从body中解析出x-www-form-urlencoded编码的参数
    if(body.size()==0){
        return;
    }
    char temp_body[body.size()+1];
    int len=0;
    for(size_t i=0;i<body.size();){
        if(body[i]=='+'){
            temp_body[len++]=' ';
            ++i;
        }
        else if(body[i]=='%'){
            temp_body[len++]=convertHex(body[i+1])*16+convertHex(body[i+2]);
            i=i+3;
        }
        else{
            temp_body[len++]=body[i];
            ++i;
        }
    }
    int i=0;
    for(int j=i+1;j<len;){
        if(temp_body[j]=='='){
            int z=j+1;
            for(;z<len;z++){
                if(temp_body[z]=='&'){
                    temp_body[j]='\0';
                    temp_body[z]='\0';
                    postUser[temp_body+i]=temp_body+j+1;
                    i=z+1;
                    j=i+1;
                    break;
                }
            }
            if(z==len){
                temp_body[j]='\0';
                temp_body[z]='\0';
                postUser[temp_body+i]=temp_body+j+1;
                return ;
            }
        }
        else ++j;
    }
}
void httpRequest::parseFormData(){
    if (body.size() == 0) return;
    ed = body.find(CRLF);
    boundary = body.substr(0, ed);
    // printf("parse boundary:%s\n",boundary.c_str());
    // 解析文件信息
    st = body.find("filename=\"", ed) + strlen("filename=\"");
    ed = body.find("\"", st);
    fileInfo["filename"] = body.substr(st, ed - st);
    string filepath=webRoot+"/files/" +fileInfo["filename"];
    if(access(filepath.c_str(),F_OK)==0){
        //文件存在,删除原来的文件
        // LOG_INFO("[%s] file is exist!",filepath.c_str());
        if(unlink(filepath.c_str())==0){
            LOG_INFO("remove [%s] file success!",filepath.c_str());
        }
        else{
            LOG_INFO("remove [%s] file file!",filepath.c_str());
        }
    }

}
httpRequest::HTTP_CODE httpRequest::parse(){
    LINE_STATUS lineState=LINE_OK;
    HTTP_CODE ret;
    char* text=0;
    while((parseState==BODY&&lineState==LINE_OK)||((lineState=parseLine())==LINE_OK)){
        //获取刚刚解析的行内容
        // if(parseState==REQUEST_LINE){
        //     LOG_INFO("当前的解析状态是解析:REQUEST_LINE");
        // }
        // else if(parseState==HEADERS){
        //     LOG_INFO("当前的解析状态是解析:HEADERS");
        // }
        // else{
        //     LOG_INFO("当前的解析状态是解析:BODY");
        // }
        text=getLine();
        //更新startLine
        startLine=checkedIdx;
        // LOG_INFO("此次的行是：%s",text);
        // LOG_INFO("checkedIdx: %lu",checkedIdx);
        switch(parseState){
            case REQUEST_LINE:
                ret=parseRequestLine(text);
                if(ret==BAD_REQUEST){
                    return BAD_REQUEST;
                }
                parseState=HEADERS;
                break;
            case HEADERS:
                ret=parseRequestHead(text);
                if(ret==BAD_REQUEST){
                    return BAD_REQUEST;
                }else if(ret==GET_REQUEST){
                    return GET_REQUEST;
                }   
                break;
            case BODY:
                // 解析body,登录，文件上传
                ret=parseRequestBody(text);
                if(ret==GET_REQUEST){
                    return GET_REQUEST;
                }else if(ret==BAD_REQUEST){
                    return BAD_REQUEST;
                }else if(ret==NO_REQUEST){
                    return NO_REQUEST;
                }
                break;
            default:
                break;
        }
    }
    return NO_REQUEST;
}
//tag断定是登录还是注册
bool httpRequest::checkUser(int tag){
    LOG_DEBUG("verify name: %s pwd: %s",postUser["username"].c_str(),postUser["password"].c_str());
    MYSQL* mysql;
    sqlConnect(sql_conn_pool::instance(),&mysql);
    if(!mysql)return false;
    char sqlOrder[256]={0};
    //先查询
    if(tag){
        LOG_DEBUG("login");
        //登录
        sprintf(sqlOrder,"SELECT username,passwd FROM user WHERE username='%s' LIMIT 1",postUser["username"].c_str());
        LOG_DEBUG("sqlOrder:%s",sqlOrder);
        //查询失败
        if(mysql_query(mysql,sqlOrder)){
            LOG_ERROR("Query error: %s",mysql_error(mysql));
            return false;
        }
        //保存结果
        MYSQL_RES* result=mysql_store_result(mysql);
        if(MYSQL_ROW row=mysql_fetch_row(result)){
            LOG_DEBUG("MYSQL row: %s %s",row[0],row[1]);
            std::string pwd(row[1]);
            //密码正确，登录成功返回true
            if(postUser["password"]==pwd){
                LOG_DEBUG("Login success!");
                mysql_free_result(result);
                return true;
            }else{
                //密码错误，返回false
                LOG_DEBUG("Login failed!");
                mysql_free_result(result);
                return false;
            }
        }
        else {
            //没有用户
            mysql_free_result(result);
            LOG_DEBUG("No user!");
            return false;
        }
        
    }
    else{
        //注册
        LOG_DEBUG("Register!");
        sprintf(sqlOrder,"SELECT username,passwd FROM user WHERE username='%s' LIMIT 1",postUser["username"].c_str());
        LOG_DEBUG("sqlOrder:%s",sqlOrder);
        //查询失败
        if(mysql_query(mysql,sqlOrder)){
            LOG_ERROR("Query error: %s",mysql_error(mysql));
            return false;
        }
        //保存结果
        MYSQL_RES* result=mysql_store_result(mysql);
        MYSQL_ROW row=mysql_fetch_row(result);
        if(row==nullptr){
            //信息没有被使用可以注册
            bzero(sqlOrder, 256);
            snprintf(sqlOrder, 256,"INSERT INTO user(username, passwd) VALUES('%s','%s')", postUser["username"].c_str(), postUser["password"].c_str());
            LOG_DEBUG("sqlOrder:%s", sqlOrder);
            //插入成功返回0，失败返回非0
            if(mysql_query(mysql, sqlOrder)){
                //插入失败
                LOG_ERROR("INSERT error:%s", mysql_error(mysql));
                return false;
            }
            else {
                //插入成功
                LOG_DEBUG("INSERT success:%s %s",postUser["username"].c_str(), postUser["password"].c_str());
                LOG_DEBUG("user regirster success!");
                return true;
            }
        }
        else{
            //信息被使用，不能注册
            LOG_DEBUG("user used!");
            mysql_free_result(result);
            return false;
        }
    }
}
bool httpRequest::isKeepAlive()const{
    return false;
}
