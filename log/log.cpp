/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-04-30 11:21:03
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-06 22:17:26
 * @FilePath: /Project/my_Server/log/log.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "log.h"
#include <stdio.h>
#include <mutex>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <thread>
#include <string>
#include <string.h>
bool log::init(bool close_log,const char* log_path,int log_buf_size,int split_lines,int max_queue_size){
    if(close_log)return false;
    mCloseLog=false;
    if(log_buf_size<=0||split_lines<=0||max_queue_size<0)return false;
    mLogPath=log_path;
    mLogBufSize=log_buf_size;
    mQueueSize=max_queue_size;
    mSplitLine=split_lines;
    mBuf=new char[mLogBufSize];
    memset(mBuf,'\0',log_buf_size);

    if(max_queue_size>0){
        // 开启异步日志
        mIsAsync=true;
        // 创建日志队列
        mQue=new blockQueue<std::string>(max_queue_size);
        // 创建线程
        std::thread writeThread(callBack);
        //  分离线程
        writeThread.detach();
    }


    // 获取当前天数
    time_t t=time(nullptr);
    struct tm* sysTime=localtime(&t);
    struct tm myTime=*sysTime;
    mToDay=myTime.tm_mday;

    // 初始化文件名
    mLogName=new char[fileNameMax];
    memset(mLogName,'\0',fileNameMax);
    snprintf(mLogName,log::fileNameMax-1,"%d_%02d_%02d",myTime.tm_year + 1900, myTime.tm_mon + 1, myTime.tm_mday);
    
    // 文件名字的绝对路径
    char fileName[log::fileNameMax]={0};
    snprintf(fileName,log::fileNameMax-1,"%s/%s-%d",mLogPath, mLogName,0);
    // 初始化文件指针
    mFp=fopen(fileName,"a");
    if(!mFp){
        return false;
    }
    return true;
}
void log::write(const int level,const char*format,...){
    char s[16]={0};
    writeLevelInfo(s,level);
    // 获取当前系统的事件
    struct timeval now{0,0};
    gettimeofday(&now,nullptr);
    time_t t=now.tv_sec;
    struct tm* sys_tm=localtime(&t);
    struct tm my_tm=*sys_tm;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        fflush(mFp);//将上一个日期或文件内的内容写入文件
        
    }
    
    // 判断日志日期是否改变或当前日志行数为最大行数的整数倍，创建新的日志文件
    if(mToDay!=my_tm.tm_mday||(mCountLine%mSplitLine==0)){
        std::lock_guard<std::mutex> lock(mMutex);
        fflush(mFp);//将上一个日期或文件内的内容写入文件
        fclose(mFp);
        
        //创建新日志文件
        memset(mLogName,'\0',fileNameMax);
        snprintf(mLogName,log::fileNameMax-1,"%d_%02d_%02d",my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday);
        
        char newLog[256]={0};
        if(mToDay!=my_tm.tm_mday){
            mToDay=my_tm.tm_mday;
            snprintf(newLog,log::fileNameMax-1,"%s/%s",mLogPath, mLogName);
            mCountLine=0;
        }
        else{
            snprintf(newLog,log::fileNameMax-1,"%s/%s-%d",mLogPath, mLogName,mCountLine/mSplitLine);
        }
        mFp=fopen(newLog,"a");
    }
    {   //向缓存中写入数据，加锁
        std::lock_guard<std::mutex> lock(mMutex);
        std::string logStr;//存储日志消息
        ++mCountLine;
        va_list vaList;
        va_start(vaList, format);
        //写入的具体时间内容格式
        int n = snprintf(mBuf, 48, "%d-%02d-%02d %02d:%02d:%02d.%06ld %s ",
                        my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday,
                        my_tm.tm_hour, my_tm.tm_min, my_tm.tm_sec, now.tv_usec, s);
        // 写入具体内容
        int m = vsnprintf(mBuf+n,mLogBufSize-1,format,vaList);
        mBuf[m+n]='\n';
        mBuf[m+n+1]='\0';
        logStr=mBuf;
        //异步，且不满
        if(mIsAsync&&!mQue->full()){
            mQue->push(logStr);
        }
        else{
            fputs(logStr.c_str(),mFp);
        }
        va_end(vaList);
    }
}

/// @brief 根据日志等级，向buf中写入日志等级标签
/// @param buf 
/// @param level 
void log::writeLevelInfo(char* buf,const int level){
    switch (level)
    {
    case 0:
        strcpy(buf, "[debug]:");
        break;
    case 1:
        strcpy(buf, "[info]:");
        break;
    case 2:
        strcpy(buf, "[warn]:");
        break;
    case 3:
        strcpy(buf, "[erro]:");
        break;
    default:
        strcpy(buf, "[info]:");
        break;
    }
    return ;
}