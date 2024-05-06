/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-06 21:30:54
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-06 22:16:21
 * @FilePath: /Project/my_Server/mysql_connection/sql_conn_pool_test.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include <stdio.h>
#include "../log/log.h"
#include "sql_conn_pool.h"
#include <string>
#include <string.h>
int main(){
    log* mylog=log::getInstance();
    std::string str="../log/logFile";
    mylog->init(false,str.c_str());
    sql_conn_pool* myMysqlPool=sql_conn_pool::instance();
    myMysqlPool->init("localhost",3306,"wt","200003228631","webdb",10);
    MYSQL* myMysqlConn;
    sqlConnect MYSQLRAII(myMysqlPool,&myMysqlConn);
    if(myMysqlConn){
        printf("成功\n");
    }
    else printf("失败");
}