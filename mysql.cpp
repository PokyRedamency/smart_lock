#include"mysql.h"

void MYDB::mydb_mysql_init() {
    mysql_init(this->mysql);
}

int MYDB::mydb_mysql_real_connect() {
    if (mysql_real_connect(this->mysql, HOST, USER, PASSWD, DATABASE, 0, NULL, 0) == NULL) {
        return -1;
    }
    return 0;
}

int MYDB::mydb_mysql_query(const char* sql) {
    // 执行数据库操作
    if (mysql_query(this->mysql, sql) != 0) {
        unlock_mutex(this->db_mutex);
        return -1;
    }
    this->res = mysql_store_result(this->mysql);

    return 0;
}




