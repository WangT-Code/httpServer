/*
 * @Author: wt wangtuam@163.com
 * @Date: 2024-04-29 20:05:32
 * @LastEditors: wt wangtuam@163.com
 * @LastEditTime: 2024-04-29 21:55:26
 * @FilePath: /Project/my_Server/lock/lock.h
 * @Description: 线程同步机制包装类
 * 
 * Copyright (c) 2024 by ${git_name_email}, All Rights Reserved. 
 */
#ifndef LOCK_H
#define LOCK_H
#include <exception>
#include <semaphore.h>
#include <assert.h>

class sem{
public:
    sem(){
        if(sem_init(&m_sem,0,0)!=0){
            throw std::exception();
        }
    }
    sem(int num){
        if(sem_init(&m_sem,0,num)!=0){
            throw std::exception();
        }
    }
    ~sem(){
        sem_destroy(&m_sem);
    }
    bool post(){
        return sem_post(&m_sem)==0;
    }
    bool wait(){
        return sem_wait(&m_sem)==0;
    }
    
    /// @return 返回非0代表成功对信号量减一，返回-1说明信号量减一失败但不阻塞设置errno为EAGAIN
    int tryWait(){
        return sem_trywait(&m_sem);
    }
private:
    sem_t m_sem;
};


class mutex{
private:
    pthread_mutex_t m_mutex;
public:
    mutex(){
        if(pthread_mutex_init(&m_mutex,nullptr)!=0){
            throw std::exception();
        }
    }
    ~mutex(){
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock(){
        return pthread_mutex_lock(&m_mutex)==0;
    }
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex)==0;
    }

    /// @return 如果成功获取锁，则返回 0；如果互斥锁已经被其他线程锁定，则立即返回一个非零值，表示获取锁失败,errno设置为 EBUSY 或者 EINVAL。该函数不会阻塞线程，而是立即返回结果。
    int trylock(){
        return pthread_mutex_trylock(&m_mutex);
    }
    pthread_mutex_t* get(){
        return &m_mutex;
    }
};

class cond{
private:
    pthread_cond_t m_cond;
public:
    cond(){
        if(pthread_cond_init(&m_cond,nullptr)!=0){
            throw std::exception();
        }
    }
    ~cond(){
        pthread_cond_destroy(&m_cond);
    }
    bool broadcast(){
        return pthread_cond_broadcast(&m_cond)==0;
    }
    bool signal(){
        return pthread_cond_signal(&m_cond)==0;
    }
    bool wait(pthread_mutex_t* m_mutex){
        int ret=0;
        ret=pthread_cond_wait(&m_cond,m_mutex);
        return ret==0;
    }
};
#endif