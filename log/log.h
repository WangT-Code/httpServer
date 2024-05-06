/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-04-30 11:21:13
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-06 22:17:49
 * @FilePath: /Project/my_Server/log/log.h
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#ifndef LOG_H
#define LOG_H

#include <string>
#include <mutex>
#include <thread>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include "blockQueue.h"
#include <stdio.h>
// #include 
class log
{
public:
    //单例模式
    static log* getInstance(){
        static log mylog;
        return &mylog;
    }
    /// @brief 初始化日志系统：初始化日志名，并打开文件
    /// @param close_log 是否关闭日志，为true时，表明关闭日志，忽略其余参数
    /// @param log_path 日志存储路径
    /// @param log_buf_size 缓存大小
    /// @param split_lines 日志文件的最大行数
    /// @param max_queue_size 日志队列的大小
    bool init(bool close_log,const char* log_path="./logFile",int log_buf_size=2048,int split_lines=5000000,int max_queue_size=100);
    void write(const int level,const char*format,...);
    void writeLevelInfo(char* buf,const int level);
    static void callBack(){
        log::getInstance()->asyncWriteLog();
    }
    bool isClose(){
        return mCloseLog;
    }
    // 刷新写入流缓冲区
    void flush(){
        {
            std::lock_guard<std::mutex> lock(mMutex);
            fflush(mFp);
        }
    }
private:
    void asyncWriteLog(){
        std::string logStr;
        while(mQue->pop(logStr)){
            std::lock_guard<std::mutex> lock(mMutex);
            fputs(logStr.c_str(),mFp);

        }
        return;
    }
private:
    static const int fileNameMax=256;
    static const int logNAMEMax=256;
    bool mIsAsync;//是否开启异步
    const char * mLogPath;//日志路径名
    char * mLogName;//日志名
    int mSplitLine;//日志文件的最大行数
    int mQueueSize;//日志队列最大容量
    int mLogBufSize;//日志缓存大小
    int mCountLine;//当前的日志行数
    int mToDay;//当前是哪天
    std::mutex mMutex;
    FILE* mFp;//文件指针
    char * mBuf;//日志缓存
    blockQueue<std::string> * mQue;//日志队列
    bool mCloseLog;
private:
    log(){
        mCloseLog=true;
        mIsAsync=false;
        mLogPath=nullptr;
        mLogName=nullptr;
        mSplitLine=0;
        mQueueSize=0;
        mLogBufSize=0;
        mCountLine=0;
        mFp=nullptr;
        mBuf=nullptr;
        mQue=nullptr;
    }
    ~log(){};
};
#define LOG_DEBUG(format, ...) if(!log::getInstance()->isClose()) {log::getInstance()->write(0,format,##__VA_ARGS__); log::getInstance()->flush();}
#define LOG_INFO(format, ...) if(!log::getInstance()->isClose()) {log::getInstance()->write(1,format,##__VA_ARGS__); log::getInstance()->flush();}
#define LOG_WARN(format, ...) if(!log::getInstance()->isClose()) {log::getInstance()->write(2,format,##__VA_ARGS__); log::getInstance()->flush();}
#define LOG_ERROR(format, ...) if(!log::getInstance()->isClose()) {log::getInstance()->write(3,format,##__VA_ARGS__); log::getInstance()->flush();}





#endif 