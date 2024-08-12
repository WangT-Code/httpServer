/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-06-04 10:04:20
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-08-12 17:45:05
 * @FilePath: /Project/my_Server/servermain.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "./webserver/webserver.h"
int main()
{
    webserver server(8080,5000,false, 3306, "wt", "200003228631", "webdb",
    20,10,10000,true,1,1024);
    server.start();
    return 0;
}