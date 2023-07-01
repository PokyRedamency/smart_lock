#pragma once
#include<mysql/mysql.h>

#define HOST "localhost"
#define USER "root"
#define PASSWD "Xu6735278."
#define DATABASE "myDatabase"
#define PORT 3306
#define NULL nullptr

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
};

