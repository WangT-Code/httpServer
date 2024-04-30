#ifndef THREADPOOL_H
#define THREADPOOL_H
#include "../lock/lock.h"
#include <queue>
#include <thread>
#include <functional>
#include <memory>
#include <assert.h>
#include <exception>
/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-04-29 19:49:30
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-04-29 22:27:06
 * @FilePath: /Project/my_Server/threadpool/threadpool.h
 * @Description: 
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
template <typename T>
class threadpool
{
private:

    //线程数量
    int m_threadNum;
    // 任务数量
    int m_requestNum;
    // 任务队列
    std::queue<T*> m_requestQueue;
    // 锁
    mutex m_mutex;
    // 信号量
    sem m_sem;
    // 线程结束标志
    bool m_stop;
public:
    static void worker(void* arg){
        threadpool* pool=(threadpool*)arg;
        while(!(pool->m_stop)){
            pool->m_sem.wait();
            pool->m_mutex.lock();
            // 没有任务
            if(pool->m_requestQueue.size()<=0){
                pool->m_mutex.unlock();
                continue;
            }
            //取出任务
            T* request=pool->m_requestQueue.front();
            pool->m_requestQueue.pop();
            // 任务取出后就可以解锁
            pool->m_mutex.unlock();
            // 执行任务
            request->process();
        }
    }

private:
    threadpool(){};
    ~threadpool(){
        m_mutex.lock();
        m_stop=true;
        m_mutex.unlock();
    };
public:
    static threadpool* instance(){
        static threadpool my_threadpool;
        return &my_threadpool;
    }
    bool init(const int threadNum=10,const int requestNum=10000){
        if(threadNum<=0||requestNum<=0){
            return false;
        }
        m_requestNum=requestNum;
        m_threadNum=threadNum;
        //创建线程
        for(int i=0;i<threadNum;++i){
            std::thread(worker,this).detach();
        }
        return true;
    }
    //向任务队列中添加任务
    bool append(T* request){
        // 互斥访问
        m_mutex.lock();
        if(int(m_requestQueue.size())>=m_requestNum){
            // 队列已经满了，不能添加任务，返回false
            //返回前需要解锁
            m_mutex.unlock();
            return false;
        }
        // 添加任务
        m_requestQueue.push(request);
        // 解锁
        m_mutex.unlock();
        // 唤醒线程
        m_sem.post();
        return true;
    }
};
#endif