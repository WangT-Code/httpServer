/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-10 20:21:36
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-06-04 10:32:55
 * @FilePath: /Project/my_Server/http/httpresponse.h
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-10 20:21:36
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-14 20:27:04
 * @FilePath: /Project/my_Server/http/httpresponse.h
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H
#include <string>
#include <unordered_map>
#include <sys/uio.h>
#include <sys/mman.h>
#include "../log/log.h"
using std::string;
using std::unordered_map;
class httpResponse{
private:
    enum HTTP_CODE
    {
        NO_REQUEST = 0,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDENT_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };
    
public:
    httpResponse(){
        code=-1;
        realFile="";
        isKeepAlive=false;
        fileAddress=nullptr;
        byteToSend=0;
        byteHaveSend=0;
        fileState={0};
    };
    ~httpResponse(){unMapFile();};
    void init(string& filepath,int code,bool isKeepAlive=false);
    void init(){
        memset(writeBuf,'\0',sizeof(writeBuf));
        writeEdIdx=0;
        byteToSend=0;
        realFile="";
        unMapFile();
    }
    bool makeResponse();
    bool write( int sockfd,int *saveErrno);
    char* getBuf(){
        return writeBuf;
    }
    size_t getWriteEdIdx(){
        return writeEdIdx;
    }
    char* getFileAddress(){
        return fileAddress;
    }
    size_t getFileLen() const{
        return fileState.st_size;
    }
    iovec* getIovec(){
        return writeV;
    }
    void unMapFile(){
        if(fileAddress){
            munmap(fileAddress,fileState.st_size);
            fileAddress=nullptr;
        }
    };
public:
    static const size_t WRITEBUFSIZE=8192;
private:
    
    bool addContent();
    bool addResponse(const char* format,...);
    
    void mapToWriteV(){
        writeV[0].iov_base=writeBuf;
        writeV[0].iov_len=writeEdIdx;
        writeVcnt=1;
    }
    
private:
    char writeBuf[WRITEBUFSIZE];
    size_t writeEdIdx;
    string realFile;
    bool isKeepAlive;
    int code;
    char* fileAddress;
    // char* File;
    struct iovec writeV[2];
    int writeVcnt;
    struct stat fileState;
    size_t byteToSend;//记录总共有多少字节需要发送
    size_t byteHaveSend;//已经发送多少字节
    static const unordered_map<string, string> SUFFIX_TYPE;
    static const unordered_map<int, string> CODE_STATUS;
    static const unordered_map<int, string> CODE_PATH;
};
#endif