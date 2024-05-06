/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-02 14:50:22
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-06 21:25:31
 * @FilePath: /Project/my_Server/mysql_connection/sql_conn_pool.h
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#ifndef SQL_CONN_POOL_H
#define SQL_CONN_POOL_H
#include <mysql/mysql.h>
#include "../lock/lock.h"
#include <string>
// #include <mutex>
#include <semaphore.h>
#include <thread>
#include <queue>
class sql_conn_pool
{
private:

    int mMaxConnNum;
    mutex mMutex;
    sem mSem;
    std::queue<MYSQL* > mQue;
public:
    static sql_conn_pool*  instance(){
        static sql_conn_pool mysqlPool;
        return &mysqlPool;
    }
    void destroy();
    void init(const char* host, int port,
              const char* user, const char* pwd,
              const char* dbName, int maxConnCnt);
    MYSQL* getMySqlConn();
    void releaseMySqlConn(MYSQL* conn);
private:
    sql_conn_pool();
    ~sql_conn_pool();
};

// RAII机制
class sqlConnect{
private:
    sql_conn_pool* sqlConnectPool;
    MYSQL* sqlConn;
public:
    sqlConnect(sql_conn_pool* connPool,MYSQL** conn);
    ~sqlConnect();
};
#endif