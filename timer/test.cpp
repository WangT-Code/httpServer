/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-07 20:10:44
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-07 21:26:18
 * @FilePath: /Project/my_Server/timer/test.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include <functional>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <thread>
#include <unistd.h>
#include "heaptimer.h"
typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef std::chrono::high_resolution_clock::time_point TimeStamp;

int main(){
    heapTimer timer(1000);
    for(int i=0; i<5; i++){
        timer.add(i,10000 , [](){
            std::cout << "timeout" << std::endl;
        });
        sleep(2);
    }
    for(auto& node:timer.heap){
        std::cout <<std::chrono::duration_cast<MS>(node.expires.time_since_epoch()).count()  << std::endl;
    }
    for(auto [key,val]:timer.ref){
        std::cout << key << " " << val << std::endl;
    }
    std::cout<<"============"<<std::endl;
    timer.adjust(0,100000000);
    sleep(1);
    timer.adjust(3,100000000);
    // timer.tick();
    //顺序遍历timer的heap中每个元素的expires
    for(auto& node:timer.heap){
        std::cout <<std::chrono::duration_cast<MS>(node.expires.time_since_epoch()).count()  << std::endl;
    }
    for(auto [key,val]:timer.ref){
        std::cout << key << " " << val << std::endl;
    }
    std::cout<<timer.heap.size()<<std::endl;
}