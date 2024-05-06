/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-02 21:57:23
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-06 18:36:02
 * @FilePath: /Project/my_Server/log/log_test.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include <iostream>
#include "log.h"
using namespace std;
int main(){
    log* my_log=log::getInstance();
    string logpath("./logFile");
    my_log->init(false,logpath.c_str());
    for(unsigned int i=0;i<100;++i){
        
        my_log->write(1,"data:%d",i);
    }
    

}