#ifndef LOCKER_H
#define LOCKER_H

#include<pthread.h>
#include<exception>
#include<semaphore.h>

class sem{
public:
    sem(){
        //信号量，是否共享，信号量的值
        if(sem_init(&m_sem,0,0) != 0){
            throw std::exception();
        }
    }
    sem(int num){
        if(sem_init(&m_sem,0,num)!= 0){
            throw std::exception();
        }
    }
    ~sem(){
        sem_destroy(&m_sem);
    }
    bool wait(){
        //阻塞并减一计数
        return sem_wait(&m_sem) == 0;
    }
    bool post(){
        //释放一个
        return sem_post(&m_sem) == 0; 
    }
private:
    sem_t m_sem;
};

class locker
{
public:
    locker(){
        //锁，锁的类型，NULL是缺省的普通锁
        if(pthread_mutex_init(&m_mutex,NULL) != 0){
            throw std::exception();
        }
    }
    ~locker(){
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock(){
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock(){
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t* get(){
        return &m_mutex;
    }
private:
    pthread_mutex_t m_mutex;
};

class cond
{
public:
    cond(){
        //同样是缺省，前者只在同一进程，share在多个进程
        if(pthread_cond_init(&m_cond,NULL) != 0){
            throw std::exception();
        }
    }
    ~cond(){
        pthread_cond_destroy(&m_cond);
    }
    bool wait(pthread_mutex_t* m_metux){
        int ret;
        ret =pthread_cond_wait(&m_cond,m_metux) ==0;
        return ret==0;
    }
    bool timewait(pthread_mutex_t* m_metux,struct timespec t){
        //超时阻塞
        int ret;
        ret =pthread_cond_timedwait(&m_cond,m_metux,&t);
        return ret==0;
    }
    bool signal(){
        return pthread_cond_signal(&m_cond) == 0;
    }
    bool broadcast(){
        return pthread_cond_broadcast(&m_cond) == 0;
    }
private:
    pthread_cond_t m_cond;
};

#endif