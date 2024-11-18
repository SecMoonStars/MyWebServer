#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGLmysql/sql_connection_pool.h"

template <typename T>
class threadpool {
public:
    threadpool(int actor_moder,connection_pool* connPool,int thread_number=8,int max_request=10000);
    ~threadpool();
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    static void *work(void *args);
    void run();

private:
    int m_thread_number;           //线程池的线程数
    int m_max_request;             //最大请求数量
    pthread_t *m_threads;          //线程池数组
    list<pthread_t *> m_workqueue; //请求队列
    locker m_quelocker;            //请求队列的互斥锁
    sem m_questate;                //是否有任务要处理
    connection_pool *m_connPool;   //数据库
    int m_actor_moder;             //切换模型
};

#endif