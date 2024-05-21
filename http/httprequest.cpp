/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-08 20:23:34
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-21 16:51:42
 * @FilePath: /Project/my_Server/http/httprequest.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "httprequest.h"
#include <json/json.h>
#include <sys/socket.h>
#include <regex>
#include <arpa/inet.h>
#include <fstream>
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
void httpRequest::init(sockaddr_in client)
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
    clientAddr=client;
    method="";
    path="";
    version="";
    body="";
    headers.clear();
    postUser.clear();
    fileInfo.clear();
}
void httpRequest::init(){
     // 初始化读取缓冲区为全0
    memset(&readBuf, '\0', READBUFSIZE);
    // 初始化已读取索引和已检查索引为0
    readedIdx=0;
    checkedIdx=0;
    startLine=0;
    // 初始化解析状态为请求行解析状态
    parseState=REQUEST_LINE;
    // 初始化内容长度为0
    contentLen=0;
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
    if(readedIdx>=READBUFSIZE){
        return false;
    }
    readedIdx=0;
    int byteRead=0;
    LOG_DEBUG("begin recv");
    while(true){
        byteRead=recv(sockfd,readBuf+readedIdx,READBUFSIZE-readedIdx,0);
        if(byteRead<0){
            printf("读取的内容:%s\n",readBuf);
            if(errno==EAGAIN||errno==EWOULDBLOCK){
                LOG_DEBUG("EAGIN");
                *saveError=errno;
                break;
            }
            return false;
        }
        else if(byteRead==0){
            //对方关闭连接
            LOG_DEBUG("对方关闭连接");
            return false;
        }
        readedIdx+=byteRead;
    }
    return true;
}
httpRequest::LINE_STATUS httpRequest::parseLine()
{
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
        parseState=BODY;
        return NO_REQUEST;
    }
    //没有请求体，说明已经解析完了
    else return GET_REQUEST;
}

httpRequest::HTTP_CODE httpRequest::parseRequestBody(char*text){
    if(readedIdx<(contentLen+checkedIdx)){
        return NO_REQUEST;
    }
    text[readedIdx] = '\0';
    parseState=FINISH;
    body=text;
    //登录或者注册
    if(method=="POST"&&headers["Content-Type"]=="application/x-www-form-urlencoded")
    {   
        parseFromUrlEncoded();//解析表单数据
        if(DEFAULTHTMLTAG.count(path)){
            int tag=DEFAULTHTMLTAG.find(path)->second;
            LOG_DEBUG("client ip is:%s , tag:%d",inet_ntoa(clientAddr.sin_addr),tag);
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
        parseFormData();
        LOG_INFO("client ip is:%s upload file:%s success!",inet_ntoa(clientAddr.sin_addr),fileInfo["filename"].c_str());
        std::ofstream ofs;
        ofs.open("./resources/response.txt", std::ios::ate);
        ofs << "./resources/files/" << fileInfo["filename"];
        ofs.close();
        path = "/response.txt";
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
    size_t st = 0, ed = 0;
    ed = body.find(CRLF);
    string boundary = body.substr(0, ed);

    // 解析文件信息
    st = body.find("filename=\"", ed) + strlen("filename=\"");
    ed = body.find("\"", st);
    fileInfo["filename"] = body.substr(st, ed - st);
    
    // 解析文件内容，文件内容以\r\n\r\n开始
    st = body.find("\r\n\r\n", ed) + strlen("\r\n\r\n");
    ed = body.find(boundary, st) - 2; // 文件结尾也有\r\n
    string content = body.substr(st, ed - st);
    std::ofstream ofs;
    // 如果文件分多次发送，应该采用app，同时为避免重复上传，应该用md5做校验
    ofs.open("/home/wt/Project/my_Server/resources/" +fileInfo["filename"], std::ios::ate);
    if(ofs.is_open())
    {
        ofs << content;
        ofs.close();
    }
    else{
        LOG_ERROR("open file error");
    }
}
httpRequest::HTTP_CODE httpRequest::parse(){
    LINE_STATUS lineState=LINE_OK;
    HTTP_CODE ret;
    char* text=0;
    while((parseState==BODY&&lineState==LINE_OK)||((lineState=parseLine())==LINE_OK)){
        //获取刚刚解析的行内容
        text=getLine();
        //更新startLine
        startLine=checkedIdx;
        LOG_INFO("%s",text);
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
                }
                lineState=LINE_OPEN;
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
            snprintf(sqlOrder, 256,"INSERT INTO user(username, passwd) VALUES('%s','%s')", postUser["username"].c_str(), postUser["passwd"].c_str());
            LOG_DEBUG("sqlOrder:%s", sqlOrder);
            //插入成功返回0，失败返回非0
            if(mysql_query(mysql, sqlOrder)){
                //插入失败
                LOG_ERROR("INSERT error:%s", mysql_error(mysql));
                return false;
            }
            else {
                //插入成功
                LOG_DEBUG("INSERT success:%s %s",postUser["username"].c_str(), postUser["passwd"].c_str());
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