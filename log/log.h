#ifndef LOG_H
#define LOG_H

#include "blcok_queue.h"
#include <iostream>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>

using namespace std;

class Log {
public:
    // 懒汉模式，需要时再创建，无需加锁
    static Log *get_instance() {
        static Log instance;
        return &instance;
    }

    static void *flush_log_thread(void *args) { Log::get_instance()->async_write_log(); }

    bool init(const char *file_name, int close_log, int log_buf_size = 8192,
              int split_lines = 50000, int max_queue_size = 0);

    void write_log(int level, const char *format, ...);

    void flush(void);

private:
    Log();
    virtual ~Log();
    // 后端异步写文件
    void async_write_log() {
        string singal_log;
        while (m_log_queue->pop(singal_log)) {
            m_mutex.lock();
            fputs(singal_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }

private:
    char dir_name[128];               // 路径名
    char log_name[128];               // log文件名
    int m_split_lines;                // 日志最大行数
    int m_log_buf_size;               // 日志缓冲区大小
    long long m_count;                // 日志行数
    int m_today;                      // 日志时间，按时间分类
    FILE *m_fp;                       // 打开日志的指针
    char *m_buf;                      // 缓冲区
    block_queue<string> *m_log_queue; // 缓冲队列
    bool m_is_async;                  // 日志是否异步
    locker m_mutex;
    int m_close_log; // 日志是否关闭
};

// 日志等级
#define LOG_DEBUG(format, ...)                                                                     \
    if (m_close_log == 0) {                                                                        \
        Log::get_instance()->write_log(0, format, ##__VA_ARGS__);                                  \
        Log::get_instance()->flush();                                                              \
    }
#define LOG_INFO(format, ...)                                                                      \
    if (m_close_log == 0) {                                                                        \
        Log::get_instance()->write_log(1, format, ##__VA_ARGS__);                                  \
        Log::get_instance()->flush();                                                              \
    }
#define LOG_WARN(format, ...)                                                                      \
    if (m_close_log == 0) {                                                                        \
        Log::get_instance()->write_log(2, format, ##__VA_ARGS__);                                  \
        Log::get_instance()->flush();                                                              \
    }
#define LOG_ERROR(format, ...)                                                                     \
    if (m_close_log == 0) {                                                                        \
        Log::get_instance()->write_log(3, format, ##__VA_ARGS__);                                  \
        Log::get_instance()->flush();                                                              \
    }

#endif