#pragma once
#include<mysql/mysql.h>
#include<pthread.h>
#include "mutex.h"
#include <stdio.h>
#define HOST "localhost"
#define USER "root"
#define PASSWD "Xu6735278."
#define DATABASE "myDatabase"
#define PORT 3306

class MYDB {
public:
    MYDB() {
       this->mysql = new MYSQL;
       this->res = new MYSQL_RES;
    }
    ~MYDB() {
        mysql_close(this->mysql);
        delete(mysql);
        delete(res);
    }
    void mydb_mysql_init();
    int mydb_mysql_real_connect();
    int mydb_mysql_query(const char* sql);
    MYSQL* mysql;
    MYSQL_RES* res;
    pthread_mutex_t* db_mutex;
};

