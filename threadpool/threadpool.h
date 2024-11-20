#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cstddef>
#include <list>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGLmysql/sql_connection_pool.h"

template <typename T>
class threadpool
{
public:
    threadpool(int actor_moder, connection_pool *connPool, int thread_number = 8,
               int max_request = 10000);
    ~threadpool();
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    static void *worker(void *args);
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

template <typename T>
threadpool<T>::threadpool(int actor_moder, connection_pool *connPool, int thread_number,
                          int max_request)
    : m_actor_moder(actor_moder), m_connPool(connPool), m_thread_number(thread_number),
      m_max_request(max_request)
{

    if (max_request < 0 || thread_number < 0)
        throw std::exception();
    m_threads = new pthread_t[m_thread_number];
    if (m_threads == nullptr)
        throw std::exception();

    for (int i = 0; i < m_thread_number; i++) {
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i])) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}

template <typename T>
bool threadpool<T>::append(T *request, int state)
{
    m_quelocker.lock();
    if (m_workqueue.size() > m_max_request) {
        m_quelocker.unlock();
        return false;
    }
    request->m_state = state;
    m_workqueue.push_back(request);
    m_quelocker.unlock();

    m_questate.post();
    return true;
}

template <typename T>
bool threadpool<T>::append_p(T *request)
{
    m_quelocker.lock();
    if (m_workqueue.size() > m_max_request) {
        m_quelocker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_quelocker.unlock();

    m_questate.post();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *args)
{
    threadpool *pool = (threadpool *)args;
    pool->run();
    return pool;
}

//reator模型：异步主线程一直监听客户端然后注册，然后工作线程收到信息后从队列取出请求处理，处理读写仍然是同步的
//两种模型，前者reator后者proactor，前者执行I/O以及业务逻辑，后者只处理业务逻辑
template <typename T>
void threadpool<T>::run()
{
    while (true) {
        m_questate.wait();
        m_quelocker.lock();
        if (m_workqueue.size() == 0) {
            m_quelocker.unlock();
            return;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        --m_thread_number;
        m_quelocker.unlock();
        //reator
        if (m_actor_moder == 1) {
            //读
            if (request->m_state == 0) {
                if (request->read_once()) {
                    request->improv = 1;
                    connection_RAII mysqlconn(request->mysql, m_connPool);
                    request->process();
                }
                else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
                //写
            }
            else {
                if (request->write_once()) {
                    request->improv = 1;
                }
                else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        //proactor
        else {
            connection_RAII mysqlconn(request->mysql, m_connPool);
            request->process();
        }
    }
}

#endif