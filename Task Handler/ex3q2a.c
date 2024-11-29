//205438542
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

#define max_len 128
#define max_polynomials 100
#define Poly_arr_name "/shmPolyArr"
#define Counter_name "/shmForCount"
#define Lock_name "/shmForLock"
#define error_msg "ERR"
#define exit_command "END"

typedef struct {
    char command[max_len];
} polynomial;

void setSharedMemory(void** ptr,char* name,int unitSize, int size);

int main() {
    // create and initialize lock
    pthread_mutex_t * pToLock;
    setSharedMemory((void**)&pToLock,Lock_name,sizeof(pthread_mutex_t),1);
    pthread_mutex_init(pToLock, NULL);

    // create and initialize semaphore
    sem_t * ptrToCounter;
    setSharedMemory((void**)&ptrToCounter,Counter_name,sizeof(sem_t),1);
    sem_init(ptrToCounter, 1, 0);

    // create shared memory for polynomials
    polynomial* ptrToPolyArr;
    setSharedMemory((void**)&ptrToPolyArr,Poly_arr_name,sizeof(polynomial), max_polynomials);

    while (1) {
        char cmd[max_len];
        // get input
        scanf("%s",cmd);

        if (strcmp(cmd,exit_command) == 0) {
            // close consumer program
            strcpy(ptrToPolyArr[0].command, exit_command);
            sem_post(ptrToCounter);
            pthread_mutex_unlock(pToLock);
            break;
        }
        // lock critical section
        pthread_mutex_lock(pToLock);

        int semValue;
        sem_getvalue(ptrToCounter, &semValue);

        // write to shared memory using semaphore value
        strcpy(ptrToPolyArr[semValue].command, cmd);
        sem_post(ptrToCounter);

        pthread_mutex_unlock(pToLock);
    }

    // destroy semaphore
    sem_destroy(ptrToCounter);
    pthread_mutex_destroy(pToLock);

    // unlink from and close shared memory
    munmap (pToLock, sizeof(pthread_mutex_t));
    munmap (ptrToPolyArr, sizeof(polynomial));
    munmap (ptrToCounter, sizeof(sem_t));
    shm_unlink (Lock_name);
    shm_unlink (Poly_arr_name);
    shm_unlink (Counter_name);
}
// create shared memory
void setSharedMemory(void** ptr,char* name,int unitSize, int size) {
    int fd_shm;
    if ((fd_shm = shm_open (name, O_RDWR | O_CREAT, 0660)) == -1)
        perror (error_msg);
    if (ftruncate(fd_shm, (unitSize * size)) == -1)
        perror(error_msg);
    if (((*ptr) = mmap (NULL, unitSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
        perror (error_msg);
}
