/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-02 14:50:32
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-10 20:05:54
 * @FilePath: /Project/my_Server/mysql_connection/sql_conn_pool.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "sql_conn_pool.h"
#include <exception>
#include "../log/log.h"
sql_conn_pool::sql_conn_pool(/* args */)
{   
    this->mMaxConnNum=0;
}

sql_conn_pool::~sql_conn_pool()
{   
    destroy();
}
void sql_conn_pool::init(const char* host, int port,
              const char* user, const char* pwd,
              const char* dbName, int maxConnCnt){
    if(maxConnCnt<=0){
        throw std::exception();
    }
    
    // 创建mysql连接
    for(int i=0;i<maxConnCnt;++i){
        MYSQL* conn=nullptr;
        conn=mysql_init(conn);
        if(!conn){
            LOG_ERROR("MySQL init error!");
            throw std::exception();
        }
        conn=mysql_real_connect(conn,host,user,pwd,dbName,port,nullptr,0);
        if(!conn){
            LOG_ERROR("MySQL Connect error!");
            throw std::exception();
        }
        mQue.push(conn);
    }
    mSem=sem(maxConnCnt);
}
void sql_conn_pool::destroy(){
    mMutex.lock();
    while (!mQue.empty())
    {
        MYSQL* temConn=mQue.front();
        mQue.pop();
        mysql_close(temConn);
    }
    mMutex.unlock();
}
// 获取一个mysql连接
MYSQL* sql_conn_pool::getMySqlConn(){
    mSem.wait();
    mMutex.lock();
    MYSQL* mysqlConn=mQue.front();
    mQue.pop();
    mMutex.unlock();
    return mysqlConn;
}
void sql_conn_pool::releaseMySqlConn(MYSQL* conn){
    if(conn==nullptr)return;
    mMutex.lock();
    mQue.push(conn);
    mMutex.unlock();
    mSem.post();
}
//
sqlConnect::sqlConnect(sql_conn_pool* connPool,MYSQL**conn){
    if(!connPool){
        LOG_ERROR("MYSQL RAII error!");
        throw std::exception();
    }
    this->sqlConn=connPool->getMySqlConn();
    LOG_INFO("MYSQL RAII Successful!");
    *conn=this->sqlConn;
    this->sqlConnectPool=connPool;
} 
sqlConnect::~sqlConnect(){
    LOG_INFO("MYSQL RAII Release!")
    this->sqlConnectPool->releaseMySqlConn(this->sqlConn);
}