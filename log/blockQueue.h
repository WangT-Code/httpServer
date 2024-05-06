/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-05-02 19:51:17
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-05-06 18:12:23
 * @FilePath: /Project/my_Server/log/blockQueue.h
 * @Description: 
 * 
 * Copyright (c) 2024 by wt(wangtuam@163.com), All Rights Reserved. 
 */
#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <mutex>
#include <queue>
#include <condition_variable>
#include <sys/time.h>
#include <exception>
// #include <sys/time.h>
template<typename T>
class blockQueue
{
private:
    //日志队列
    std::queue<T> mLogQue; 
    //日志队列最大长度
    size_t mLogNum;
    int isClose;
    std::mutex mMutex;
    std::condition_variable mConsumer;
    std::condition_variable mProducer;
public:
    blockQueue(size_t lognum);
    ~blockQueue();
    bool push(const T& item);
    bool pop( T&item);
    bool full(){
        bool ans;
        {    
            std::lock_guard<std::mutex> lock(mMutex);
            ans=(mLogQue.size()>=mLogNum);
        }
        return ans;
    }
};

template<typename T>
blockQueue<T>::blockQueue(size_t lognum)
{
    if(lognum<=0){
        throw std::exception();
    }
    mLogNum=lognum;
    isClose=false;
}
template<typename T>
blockQueue<T>::~blockQueue()
{
    {
        //获取锁
        std::lock_guard<std::mutex> locker(mMutex);
        // 队列置空
        mLogQue=std::queue<T>();
        isClose=true;
    }
    //通知所有线程结束工作
    mConsumer.notify_all();
    mProducer.notify_all();
}
/// @brief 向队列中添加item
/// @tparam T ：日志
/// @param item 
/// @return 插入成功返回true，否则返回false
template<typename T>
bool blockQueue<T>::push(const T& item){
    //先抢锁
    std::unique_lock<std::mutex> lock(mMutex);
    // 队列已经满，生产者阻塞
    while(mLogQue.size()>=mLogNum){
        // 生产者阻塞
        mProducer.wait(lock);
        if(isClose){
            return false;
        }
    }
    mLogQue.push(item);
    // 激活消费者
    mConsumer.notify_one();
    return true;
}

/// @brief 弹出item
/// @tparam T 
/// @param item 
/// @return 成功true，失败false
template<typename T>
bool blockQueue<T>::pop( T& item){
    //抢锁
    std::unique_lock<std::mutex> lock(mMutex);
    // 队列为空，消费者阻塞
    while (mLogQue.empty())
    {
        //消费者阻塞
        mConsumer.wait(lock);
        if(isClose){
            return false;
        }
    }
    // 从队列中取出任务
    item=mLogQue.front();
    mLogQue.pop();
    // 通知生产者可以生产
    mProducer.notify_one();
    return true;
}
#endif


