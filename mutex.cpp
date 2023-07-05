#include"mutex.h"

void init_mutex(pthread_mutex_t* db_mutex){ // »¥³âËø¶ÔÏó) {
    pthread_mutex_init(db_mutex, NULL); // ³õÊ¼»¯»¥³âËø
}

void lock_mutex(pthread_mutex_t* db_mutex) {
    pthread_mutex_lock(db_mutex); // »ñÈ¡»¥³âËøµÄËø
}

void unlock_mutex(pthread_mutex_t* db_mutex) {
    pthread_mutex_unlock(db_mutex); // ÊÍ·Å»¥³âËøµÄËø
}

