/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-08 20:23:25
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-21 10:34:59
 * @FilePath: /Project/my_Server/http/httprequest.h
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <errno.h>
#include <mysql/mysql.h>
#include <regex>
#include <netinet/in.h>
#include "../log/log.h"
#include "../mysql_connection/sql_conn_pool.h"
using std::string;
using std::unordered_map;
using std::unordered_set;
//负责处理用户的http请求报文
class httpRequest{
public:
enum HTTP_CODE
{
    NO_REQUEST = 0,
    GET_REQUEST,
    BAD_REQUEST,
    NO_RESOURSE,
    FORBIDDENT_REQUEST,
    FILE_REQUEST,
    INTERNAL_ERROR,
    CLOSED_CONNECTION
};

enum PARSE_STATE
{
    REQUEST_LINE = 0,
    HEADERS,
    BODY,
    FINISH
};
enum LINE_STATUS
{
    LINE_OK = 0,
    LINE_BAD,
    LINE_OPEN
};
public:
    httpRequest(){};
    ~httpRequest()=default;
    void init(sockaddr_in client);
    void init();
    //从sockfd中读取数据到readBuf中
    bool read(int sockfd,int * saveError);
    char* buf(){
        return readBuf; 
    };
    //解析readBuf中的数据
    HTTP_CODE parse();
    bool isKeepAlive() const;
private:
    void parseFromUrlEncoded();
    void parseFormData();
    int convertHex(char ch);
    //从readBuf中解析一行
    LINE_STATUS parseLine();

    //获取解析好的一行数据
    char* getLine();

    //解析请求行
    HTTP_CODE parseRequestLine(char* text);
    // 解析请求头
    HTTP_CODE parseRequestHead(char* text);
    // 解析请求体
    HTTP_CODE parseRequestBody(char* text);
    // 校验用户信息
    bool checkUser(int tag);
public:
    char* getBuf(){
        return readBuf;
    }
    void setReadedID(){
        readedIdx=strlen(readBuf)+1;
    }
    int getrewrite(){
        return readedIdx;
    }
    string& getMethod(){
        return method;
    }
    string& getUrl(){
        return path;
    }
    //生成version和body的getter
    string& getVersion(){
        return version;
    }
    string& getBody(){
        return body;
    }
    //生成linger,contentLen的getter
    bool isLinger() const{
        return linger;
    }
    size_t getContentLen()const{
        return contentLen;
    }
    int getReadedIdx()const{
        return readedIdx;
    }
    std::unordered_map<std::string,std::string>& getHeaders(){
        return headers;
    }
    std::unordered_map<std::string,std::string>& getUser(){
        return postUser;
    }

private:
    static const int READBUFSIZE=8192;
    sockaddr_in clientAddr;
    size_t readedIdx;//读缓冲区中已经读入的字符个数，也就是读取字符的下一个位置
    size_t checkedIdx;//指向已经解析字符的下一个位置
    size_t startLine;
    char readBuf[READBUFSIZE];//存储报文数据
    PARSE_STATE parseState;//当前的解析状态
    string method,path,version,body;
    bool linger;//是否是长连接
    size_t contentLen;//存储请求体的长度
    unordered_map<string,string> headers;//存储请求头中的信息
    unordered_map<string,string> postUser;//存储用户登陆注册时的信息
    unordered_map<string,string> fileInfo;//存储用户上传文件的信息
    static const unordered_set<string> DEFAULTHTML;//网站默认页面带.html的页面
    static const unordered_set<string> DEFAULTHTMLWITHOUTHTML;//网站页面不带.html
    static const unordered_map<string,int> DEFAULTHTMLTAG;//用户post的页面
    
};
#endif