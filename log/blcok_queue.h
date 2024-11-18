#ifndef BOLCK_QUEUE_H
#define BOLCK_QUEUE_H

#include "../lock/locker.h"
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
using namespace std;

template <class T>
class block_queue {
public:
    block_queue(int maxsize = 1000) {
        if (maxsize < 0) {
            exit(-1);
        }

        m_maxsize = maxsize;
        m_array = new T[maxsize];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }
    void clear() {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }
    ~block_queue() {
        m_mutex.lock();
        if (m_array != nullptr)
            delete[] m_array;
        m_mutex.unlock();
    }
    bool full() {
        m_mutex.lock();
        if (m_size >= m_maxsize) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool empty() {
        m_mutex.lock();
        if (m_size == 0) {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    bool front(T &value) {
        m_mutex.lock();
        if (m_size == 0) {
            m_mutex.unlock();
            return true;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }
    bool back(T &value) {
        m_mutex.lock();
        if (m_size == 0) {
            m_mutex.unlock();
            return true;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }
    int max_size() {
        int tmp;

        m_mutex.lock();
        tmp = m_maxsize;

        m_mutex.unlock();
        return tmp;
    }
    int size() {
        int tmp;

        m_mutex.lock();
        tmp = m_size;

        m_mutex.unlock();
        return tmp;
    }
    bool push(const T &item) {
        m_mutex.lock();

        if (m_size >= m_maxsize) {
            // 满了，通知其他线程消费
            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }

        m_back = (m_back + 1) % m_maxsize;
        m_array[m_back] = item;

        // 加入后通知其他线程
        m_size++;
        m_cond.broadcast();

        m_mutex.unlock();
        return true;
    }
    bool pop(T &item) {
        m_mutex.lock();
        while (m_size <= 0) {
            if (!m_cond.wait(m_mutex.get())) {
                m_mutex.unlock();
                return false;
            }
        }

        m_front = (m_front + 1) % m_maxsize;
        item = m_array[m_front];

        m_size--;

        m_mutex.unlock();
        return true;
    }
    bool pop(T &item, int ms_timeout) {
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now, NULL);

        m_mutex.lock();
        if (m_size <= 0) {
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1e6;

            if (!m_cond.timewait(m_mutex.get(), t)) {
                // 超时返回
                m_mutex.unlock();
                return false;
            }
        }
        // 双重检查避免虚等待
        if (m_size <= 0) {
            m_mutex.unlock();
            return false;
        }

        m_front = (m_front + 1) % m_maxsize;
        item = m_array[m_front];

        m_size--;

        m_mutex.unlock();
        return true;
    }

private:
    locker m_mutex;
    cond m_cond;

    T *m_array;
    int m_maxsize;
    int m_size;
    int m_front;
    int m_back;
};

#endif