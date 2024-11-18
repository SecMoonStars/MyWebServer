#include "sql_connection_pool.h"
#include <cstdlib>
#include <list>
#include <mysql/mysql.h>
#include <string>
#include <pthread.h>

using namespace std;

connection_pool::connection_pool() {
    m_FreeConn = 0;
    m_CurConn = 0;
}

void connection_pool::init(string url, string User, string Password, string DataBaseName, int Port,
                           int MaxConn, int close_log) {
    m_url = url;
    m_User = User;
    m_Password = Password;
    m_DataBaseName = DataBaseName;
    m_close_log = close_log;

    for (int i = 0; i < MaxConn; ++i) {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);

        if (conn == nullptr) {
            LOG_ERROR("Mysql Error");
            exit(-1);
        }

        //建立连接
        conn = mysql_real_connect(conn, url.c_str(), User.c_str(), Password.c_str(),
                                  DataBaseName.c_str(), Port, nullptr, 0);

        if (conn == nullptr) {
            LOG_ERROR("Mysql Error");
            exit(-1);
        }

        connlist.push_back(conn);
        ++m_FreeConn;
    }

    //设置信号量
    reserve = sem(m_FreeConn);

    m_MaxConn = m_FreeConn;
}

//释放当前连接并放回空闲连接池
bool connection_pool::ReleaseConnection(MYSQL *conn) {
    if (conn == nullptr) {
        return false;
    }

    lock.lock();

    connlist.push_back(conn);
    ++m_FreeConn;
    --m_CurConn;

    lock.unlock();

    reserve.post();
    return true;
}

//拿到一个空闲连接
MYSQL *connection_pool::Getconnection() {
    MYSQL *conn = nullptr;

    if (connlist.size() == 0) {
        return nullptr;
    }

    lock.lock();

    conn = connlist.front();
    connlist.pop_front();
    ++m_CurConn;
    --m_FreeConn;

    lock.unlock();

    reserve.post();
    return conn;
}

//销毁当前连接池
void connection_pool::DestoryPool() {
    lock.lock();

    if (connlist.size() > 0) {
        list<MYSQL *>::iterator it;

        for (it = connlist.begin(); it != connlist.end(); it++) {
            MYSQL *conn = *it;
            mysql_close(conn);
        }

        m_CurConn = 0;
        m_FreeConn = 0;
        connlist.clear();
    }

    lock.unlock();
}

int connection_pool::GetFreeConn() {
    return m_FreeConn;
}

connection_pool::~connection_pool() {
    DestoryPool();
}

connection_RAII::connection_RAII(MYSQL **conn, connection_pool *connPool) {
    *conn = connPool->get_instance()->Getconnection();

    connRAII = *conn;
    pollRAII = connPool;
}

connection_RAII::~connection_RAII() {
    pollRAII->ReleaseConnection(connRAII);
}