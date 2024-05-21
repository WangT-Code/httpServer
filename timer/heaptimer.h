/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-07 16:49:44
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-07 21:35:09
 * @FilePath: /Project/my_Server/timer/heaptimer.h
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#ifndef HEAPTIMER_H
#define HEAPTIMER_H

#include <unordered_map>
#include <functional>
#include <chrono>
#include <cstdint>
#include <cassert>
#include <time.h>
#include <vector>
#include "../log/log.h"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef std::chrono::high_resolution_clock::time_point TimeStamp;
//定时器节点
class timeNode{
    public:
        int fd;
        TimeStamp expires;
        TimeoutCallBack cb;
        bool operator<(const timeNode& t) const{
            return expires<t.expires;
        }
};
//定时器类
class heapTimer{
    public:
        heapTimer(int capacity=1000){
            heap.reserve(capacity);
        };
        ~heapTimer(){
            clear();
        };
        // 添加一个定时器到堆中
        void add(int fd, int timeOut, const TimeoutCallBack& cb);
        // 调整定时器的到期时间
        void adjust(int fd, int newExpires);
        // 弹出堆顶的定时器
        void pop();
        // 处理过期的定时器，如果过期执行回调函数
        void tick();
        //获取最近的到期时间
        int getNextTick();
        // 回收空间
        void clear(){
            heap.clear();
            ref.clear();
        };
   private:
        void siftup(size_t i);
        void siftdown(size_t i);
        void deleteAt(size_t i);
        void swapNode(size_t i, size_t j);
        
    public:
        std::vector<timeNode> heap;
        std::unordered_map<int, size_t> ref;
    
        
 };
#endif