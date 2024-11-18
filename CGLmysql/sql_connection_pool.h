#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include <iostream>
#include "../lock/locker.h"
#include "../log/log.h"
#include <mysql/mysql.h>
#include <list>
#include <string>
#include <error.h>

using namespace std;

class connection_pool {
public:
    MYSQL *Getconnection();              //获取数据库连接
    bool ReleaseConnection(MYSQL *conn); //释放当前连接
    int GetFreeConn();                   //获取空闲连接数
    void DestoryPool();                  //销毁所有连接

    //单例模式
    static connection_pool *get_instance() {
        static connection_pool connPool;
        return &connPool;
    }

    void init(string url, string User, string Password, string DataBaseName, int Port, int MaxConn,
              int close_log);

private:
    connection_pool();
    ~connection_pool();

    int m_MaxConn;
    int m_FreeConn;
    int m_CurConn;
    locker lock;
    list<MYSQL *> connlist;
    sem reserve;

public:
    string m_url;
    string m_User;
    string m_Password;
    string m_DataBaseName;
    int m_close_log;
};

class connection_RAII {
public:
    connection_RAII(MYSQL **con, connection_pool *connPool);
    ~connection_RAII();

private:
    connection_pool *pollRAII;
    MYSQL *connRAII;
};

#endif