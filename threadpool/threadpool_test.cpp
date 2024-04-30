/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-04-29 21:40:15
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-04-29 22:29:08
 * @FilePath: /Project/my_Server/threadpool/threadpool_test.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include <iostream>
#include "threadpool.h"
#include <thread>
struct task{
    int val;
    task(int num){
        val=num;
    }
    void process(){
        std::cout<<val<<std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
};
int main(){
    threadpool<task>* my_pool=threadpool<task>::instance();
    my_pool->init(3,100);
    for(int i=0;i<100;i++){
        std::cout<<"添加任务第"<<i+1<<"个任务"<<std::endl;
        my_pool->append(new task(i));
    }
    int n,m;
    std::cin>>n>>m;
    return 0;
}