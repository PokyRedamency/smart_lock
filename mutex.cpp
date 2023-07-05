#include"mutex.h"

void init_mutex(pthread_mutex_t* db_mutex){ // ����������) {
    pthread_mutex_init(db_mutex, NULL); // ��ʼ��������
}

void lock_mutex(pthread_mutex_t* db_mutex) {
    pthread_mutex_lock(db_mutex); // ��ȡ����������
}

void unlock_mutex(pthread_mutex_t* db_mutex) {
    pthread_mutex_unlock(db_mutex); // �ͷŻ���������
}

