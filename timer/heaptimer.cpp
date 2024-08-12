/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-07 16:49:54
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-06-04 11:15:56
 * @FilePath: /Project/my_Server/timer/heaptimer.cpp
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#include "heaptimer.h"

void heapTimer::swapNode(size_t i, size_t j)
{   size_t s=heap.size();
    if(i<0||j<0||i>=s||j>=s){
        return;
    }
    ref[heap[i].fd]=j;
    ref[heap[j].fd]=i;
    std::swap(heap[j],heap[i]);
}
void heapTimer::siftdown(size_t i){
    size_t s=heap.size();
    if(i<0||i>=s)return;
    size_t t=i*2+1;
    while(t<s){
        //找到左右子节点中比较小的那个，t就是结果
        if(t+1<s&&heap[t+1]<heap[t]){
            ++t;
        }
        //对比当前节点和最小左右子节点的值
        if(heap[i]<heap[t]){
            //当前节点小于左右子节点的值，直接返回
            break;
        }
        swapNode(i,t);
        i=t;
        t=t*2+1;
    }
}
void heapTimer::siftup(size_t i){
    size_t s=heap.size();
    if(i<0||i>=s) return;
    size_t t=(i-1)/2;
    while(t>=0){
        //当前节点小于父节点
        if(heap[i]<heap[t]){
            swapNode(i,t);
            i=t;
            t=(t-1)/2;
        }
        else break;
    }
}
void heapTimer::deleteAt(size_t i){
    size_t s=heap.size();
    if(i<0||i>=s)return;
    if(heap.empty())return ;
    // 将被删除的点与最后一个节点交换
    swapNode(i,s-1);
    // 从map中删除改节点的映射关系
    ref.erase(heap.back().fd);
    // 删除改节点
    heap.pop_back();
    // 调整节点
    if(!heap.empty()){
        siftdown(i);
        siftup(i);
    }
}
void heapTimer::add(int fd, int timeOut, const TimeoutCallBack& cb)
{
    if(fd<0)return;
    if(!ref.count(fd)){
        //没有出现过
        ref[fd]=heap.size();
        heap.push_back({fd,Clock::now()+MS(timeOut),cb});
        siftup(heap.size()-1);
    }
    else{
        //是在堆中已经存在的定时器
        //更新时间
        heap[ref[fd]].expires=Clock::now()+MS(timeOut);
        //更新回调函数，其实不更新也行
        heap[ref[fd]].cb=cb;
        // 调整堆
        siftdown(ref[fd]);
        siftup(ref[fd]);
    }
}
void heapTimer::adjust(int fd, int newExpires){
    if(fd<0)return;
    if(heap.empty())return;
    if(!ref.count(fd))return;
    heap[ref[fd]].expires=Clock::now()+MS(newExpires);
    siftdown(ref[fd]);
    siftup(ref[fd]);
}

void heapTimer::pop(){
    if(!heap.empty()){
        deleteAt(0);
    }
}
void heapTimer::tick(){
    LOG_INFO("tick");
    while(!heap.empty()){
        // 取第一个节点
        timeNode temp_node=heap.front();
        if(std::chrono::duration_cast<MS>(temp_node.expires - Clock::now()).count()<0){
            //当前节点过期
            // 执行当前描述符的回调函数
            LOG_INFO("socket:[%d] time is over",temp_node.fd);
            temp_node.cb();
            //弹出
            pop();
        }
        else break;
    }
}
/// @brief 返回最近到期时间
/// @return 数组为空返回-1,否则返回最近到期时间
int heapTimer::getNextTick(){
    tick();//处理过期节点
    if(heap.empty())return -1;//数组为空
    int ans=std::chrono::duration_cast<MS>(heap.front().expires - Clock::now()).count();
    if(ans<0)return 0;
    return ans;
}
