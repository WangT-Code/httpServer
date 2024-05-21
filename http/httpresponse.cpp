#include "httpresponse.h"
#include "../log/log.h"
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/uio.h>
#include "util.h"

const unordered_map<string, string> httpResponse::SUFFIX_TYPE = 
{
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/msword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};
//定义http响应的一些状态信息
const char *ok_200_title = "OK";
const char *error_400_title = "Bad Request";
const char *error_400_form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error_403_title = "Forbidden";
const char *error_403_form = "You do not have permission to get file form this server.\n";
const char *error_404_title = "Not Found";
const char *error_404_form = "The requested file was not found on this server.\n";
const char *error_500_title = "Internal Error";
const char *error_500_form = "There was an unusual problem serving the request file.\n";
void httpResponse::init(string &filePath,int code,bool isKeepAlive){
    this->isKeepAlive=isKeepAlive;
    writeEdIdx=0;
    this->code=code;
    realFile=filePath;
    byteToSend=0;
    byteHaveSend=0;
    fileAddress=nullptr;
    writeVcnt=0;
    memset(writeBuf,'\0',WRITEBUFSIZE);
}
bool httpResponse::addResponse(const char*format,...){
    if(writeEdIdx>=WRITEBUFSIZE){
        return false;
    }
    va_list arg_list;
    va_start(arg_list,format);
    int len=vsnprintf(writeBuf+writeEdIdx,WRITEBUFSIZE-1-writeEdIdx,format,arg_list);
    if(len>=(WRITEBUFSIZE-1-writeEdIdx)){
        va_end(arg_list);
        return false;
    }
    writeEdIdx+=len;
    
    va_end(arg_list);
    
    return true;
}
bool httpResponse::makeResponse(){
    if(code==INTERNAL_ERROR){
        addResponse("HTTP/1.1 %d %s\r\n",500,error_500_title);
        addResponse("Connection:%s\r\n", (isKeepAlive == true) ? "keep-alive" : "close");
        addResponse("Content-Length:%d\r\n", strlen(error_500_form));
        addResponse("Content-Type:text/html\r\n\r\n");
        addResponse("%s",error_500_form);
        byteToSend=writeEdIdx;
        mapToWriteV();
        LOG_INFO("addResponse:%s",writeBuf);
        return true;
    }
    else if(code==BAD_REQUEST){
        addResponse("HTTP/1.1 %d %s\r\n",400,error_400_title);
        addResponse("Connection:%s\r\n", (isKeepAlive == true) ? "keep-alive" : "close");
        addResponse("Content-Length:%d\r\n", strlen(error_400_form));
        addResponse("Content-Type:text/html\r\n\r\n");
        addResponse("%s",error_400_form);
        byteToSend=writeEdIdx;
        mapToWriteV();
        LOG_INFO("addResponse:%s",writeBuf);
        return true;

    }
    else if(code==FORBIDDENT_REQUEST){
        addResponse("HTTP/1.1 %d %s\r\n",403,error_403_title);
        addResponse("Connection:%s\r\n", (isKeepAlive == true) ? "keep-alive" : "close");
        addResponse("Content-Length:%d\r\n", strlen(error_403_form));
        addResponse("Content-Type:text/html\r\n\r\n");
        addResponse("%s",error_403_form);
        byteToSend=writeEdIdx;
        mapToWriteV();
        LOG_INFO("addResponse:%s",writeBuf);
        return true;

    }
    else if(code==FILE_REQUEST||code==GET_REQUEST){
        if(GET_REQUEST==code){
            LOG_INFO("user login success and begin to make response.");
        }
        LOG_INFO("add response line");
        addResponse("HTTP/1.1 %d %s\r\n",200,ok_200_title);
        LOG_INFO("add response line success,begin to add headers");
        addResponse("Connection:%s\r\n", (isKeepAlive == true) ? "keep-alive" : "close");
        LOG_INFO("add connection success and begin to run addContent()");
        byteToSend=writeEdIdx;
        return addContent();
    }
    else if(code==NO_RESOURSE){
        printf("无资源\n");
        printf("开始添加响应行\n");
        addResponse("HTTP/1.1 %d %s\r\n",404,error_404_title);
        printf("相应请求行添加完毕,开始添加响应头\n");
        addResponse("Content-Length:%d\r\n", strlen(error_404_form));
        addResponse("Connection:%s\r\n", (isKeepAlive == true) ? "keep-alive" : "close");
        addResponse("Content-Type:text/html\r\n\r\n");
        printf("响应头添加完毕,开始添加响应体\n");
        addResponse("%s",error_404_form);
        printf("writeV开始设置\n");
        byteToSend=writeEdIdx;
        mapToWriteV();
        printf("映射完毕\n");
        printf("返回内容：%s",writeBuf);
        printf("writeEdIdx:%ld",writeEdIdx);
        LOG_INFO("addResponse:%s",writeBuf);
        printf("\n添加完毕\n");
        return true;

    }
    else{
        addResponse("HTTP/1.1 %d %s\r\n",500,error_500_title);
        addResponse("Content-Length:%d\r\n", strlen(error_500_form));
        addResponse("Connection:%s\r\n", (isKeepAlive == true) ? "keep-alive" : "close");
        addResponse("Content-Type:text/html\r\n\r\n");
        addResponse("%s",error_500_form);
        mapToWriteV();
        LOG_INFO("addResponse:%s",writeBuf);
        return true;

    }
    return true;

}
bool httpResponse::addContent(){
    LOG_DEBUG("addContent file:[%s]",realFile.c_str());
    stat(realFile.c_str(),&fileState);
    //打开文件
    int fd=open(realFile.c_str(),O_RDONLY);
    LOG_DEBUG("open file:[%s] success!",realFile.c_str());
    if(-1==fd){
        code=INTERNAL_ERROR;
        LOG_ERROR("open file error:[%s]s",realFile.c_str());
        return false;
    }
    //映射文件到内存
    fileAddress=(char*)mmap(0,fileState.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);
    if(fileAddress==MAP_FAILED){
        code=INTERNAL_ERROR;
        LOG_ERROR("mmap error:[%s]",realFile.c_str());
        return false;
    }
    LOG_INFO("mmap file [%s] success!",realFile.c_str());
    addResponse("Content-Length:%d\r\n",fileState.st_size);
    //获取文件类型
    string::size_type fileType=realFile.find_last_of('.');
    if(fileType==string::npos)addResponse("Content-Type:%s\r\n\r\n", "text/plain");
    string suffix=realFile.substr(fileType);
    if(SUFFIX_TYPE.count(suffix)){
        addResponse("Content-Type:%s\r\n\r\n", SUFFIX_TYPE.find(suffix)->second.c_str());
    }
    else{
        addResponse("Content-Type:%s\r\n\r\n", "text/plain");
    }
    
    writeV[0].iov_base=writeBuf;
    writeV[0].iov_len=writeEdIdx;
    writeV[1].iov_base=fileAddress;
    writeV[1].iov_len=fileState.st_size;
    writeVcnt=2;
    byteToSend=writeEdIdx+fileState.st_size;
    return true;
}
bool httpResponse::write( int sockfd,int *saveErrno){
    if(byteToSend==0){
        printf("bytesToSend is zero\n");
        epollUtil::instance().modfd(sockfd,EPOLLIN);
        unMapFile();
        return true;
    }
    while(1){
        int bytes_write=writev(sockfd,writeV,writeVcnt);
        printf("%d此次返回%d字节数据\n",sockfd,bytes_write);
        if(bytes_write==0)sleep(2);
        if(-1==bytes_write){
            if(errno==EAGAIN){
                epollUtil::instance().modfd(sockfd,EPOLLOUT);
                return true;
            }
            else{
                *saveErrno=errno;
                unMapFile();
                return false;
            }
        }
        byteHaveSend+=bytes_write;
        byteToSend-=bytes_write;
        if(byteHaveSend>=writeV[0].iov_len){
            writeV[0].iov_len=0;
            writeV[1].iov_base=(char*)fileAddress+byteHaveSend-writeEdIdx;
            writeV[1].iov_len=byteToSend;
        }
        else{
            writeV[0].iov_base=writeBuf+byteHaveSend;
            writeV[0].iov_len-=byteHaveSend;
        }
        if(byteToSend<=0){
            LOG_INFO("write complete");
            unMapFile();
            epollUtil::instance().modfd(sockfd,EPOLLIN);
            if(isKeepAlive){
                // 重置
                init();
                return true;
            }
            else return false;
        }
    }
}