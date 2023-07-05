#pragma once
#include <pthread.h>
#include <mysql/mysql.h>

void init_mutex(pthread_mutex_t* db_mutex);

void lock_mutex(pthread_mutex_t* db_mutex);

void unlock_mutex(pthread_mutex_t* db_mutex);

