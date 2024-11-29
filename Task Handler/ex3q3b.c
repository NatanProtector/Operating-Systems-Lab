// 205438542
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>

#define max_len 128
#define Addition "ADD"
#define Subtraction "SUB"
#define Multiplication "MUL"
#define Poly_arr_name "/shmPolyArr"
#define Counter_name "/shmForCount"
#define Lock_name "/shmForLock"
#define error_msg "ERR"
#define exit_command "END"

typedef struct {
    char command[max_len];
} polynomial;

typedef struct {
    int* result;
    int* poly1;
    int* poly2;
    int operation;
    int index;
} ThreadArgs;

void displayResultAsPolynomial(int ** result,int operation,int sizeOfPoly);
int commandToInt(char * command);
int getSizeOfPolyArr(const char * arr);
char* parseFromIndex(const char * cmd, int size, int * index);
int getRealSizeOfPoly(char * subStringOne, char * subStringTwo);
void reverse(char string[], int length);
void stringPolyToIntArr(char * stringPoly , int ** polynomial, int size);
int parseCommand(char * cmd, int size , int ** polynomial_1, int ** polynomial_2, int * sizeResult);
int * ADD(int * polynomial_1, int * polynomial_2, int size);
int * SUB(int * polynomial_1,int * polynomial_2, int size);
int * MUL(const int * polynomial_1, const int * polynomial_2, int size);
int * executeOperation(int operation, int * polynomial_1, int * polynomial_2, int size);
void getSharedMemory(void ** ptr, char* name, int size);

int main() {
    // set mutex lock, counter semaphore, and the memory for polynomials
    pthread_mutex_t * pToLock;
    getSharedMemory((void**)&pToLock,Lock_name ,sizeof(pthread_mutex_t));
    sem_t * ptrToCounter;
    getSharedMemory((void**)&ptrToCounter,Counter_name ,sizeof(sem_t));
    polynomial* ptr;
    getSharedMemory((void**)&ptr,Poly_arr_name ,sizeof(polynomial));

    while (1) {

        sem_wait(ptrToCounter);

        // lock critical section
        pthread_mutex_lock(pToLock);

        int semValue;
        sem_getvalue(ptrToCounter, &semValue);
        // copy command using semaphore value
        char cmd[max_len];
        strcpy(cmd,ptr[semValue].command);
        int cmdLen = strlen(cmd);
        // exit loop
        if (strcmp(cmd,exit_command) == 0) {
            pthread_mutex_unlock(pToLock);
            break;
        }
        int * polynomial_1;
        int * polynomial_2;
        int sizeOfPoly;

        int operation = parseCommand(cmd, cmdLen, &polynomial_1, &polynomial_2, &sizeOfPoly);
        int * result = executeOperation(operation, polynomial_1, polynomial_2, sizeOfPoly);

        displayResultAsPolynomial(&result,operation,sizeOfPoly);

        free(result);
        free(polynomial_1);
        free(polynomial_2);

        // unlock critical section
        pthread_mutex_unlock(pToLock);
    }

    // unlink from shared memory
    munmap (pToLock, sizeof(pthread_mutex_t));
    munmap (ptr, sizeof(polynomial));
    munmap (ptrToCounter, sizeof(int));
}
// display the integer array of coefficients as a polynomial;
void displayResultAsPolynomial(int ** result, int operation, int sizeOfPoly) {
    int resultSize = sizeOfPoly;
    if (operation == 2)
        resultSize = 2*resultSize - 1;
    int firstCoefficientWasPrinted = 0;
    for (int i = 0 ; i < resultSize - 1 ; i++) {
        int currentCoefficient = (*result)[i];
        int degree = resultSize - i - 1;
        if (currentCoefficient == 0)
            continue;
        if (currentCoefficient<0 && firstCoefficientWasPrinted == 1)
            currentCoefficient *= -1;
        if (degree == 1)
            printf("%dx", currentCoefficient);
        else
            printf("%dx^%d", currentCoefficient, degree);
        firstCoefficientWasPrinted = 1;
        if ((*result)[i+1] < 0)
            printf(" - ");
        else
            printf(" + ");
    }
    int lastCoefficient = (*result)[resultSize-1];
    if (lastCoefficient<0)
        lastCoefficient *= -1;
    printf("%d\n", lastCoefficient);
}
// 0 Addition, 1 Subtraction, 2 Multiplication
int commandToInt(char * command) {
    int result;
    if (strcmp(command, Addition) == 0) {
        result = 0;
    } else if (strcmp(command, Subtraction) == 0) {
        result = 1;
    } else if (strcmp(command, Multiplication) == 0) {
        result = 2;
    } else
        result = -1; // should not happen
    return result;
}
// extract the degree from the string and return the appropriate size.
int getSizeOfPolyArr(const char * arr) {
    char* sizeInChar = (char*)malloc(sizeof(char) * max_len);
    int size;
    int i = 0;
    while(arr[i] != ':') {
        sizeInChar[i] = arr[i];
        i++;
    }
    sizeInChar[i] = '\0';
    size = atoi(sizeInChar);
    free(sizeInChar);
    return (size+1);
}
// subroutine of parse command.
char* parseFromIndex(const char * cmd, int size, int * index) {
    char* subString = (char*)malloc(sizeof(char) * size);
    int i = 0;
    while (cmd[(*index)] != ')') {
        subString[i] = cmd[(*index)];
        (*index)++; i++;
    }
    subString[i] = '\0';
    (*index)++;
    return subString;
}
// both coefficients must be the same size, determine the size from the strings.
int getRealSizeOfPoly(char * subStringOne, char * subStringTwo) {
    int size1 = getSizeOfPolyArr(subStringOne);
    int size2 = getSizeOfPolyArr(subStringTwo);
    int realSize;

    if(size1 < size2)
        realSize = size2;
    else
        realSize = size1;

    return realSize;
}
// reverse a string, used by stringPolyToIntArr() to correct for direction of string extraction.
void reverse(char string[], int length) {
    int temp;
    for (int i = 0, j = length - 1; i < j; i++, j--) {
        temp = string[i];
        string[i] = string[j];
        string[j] = temp;
    }
}
// create and int array of the polynomials coefficients from the string.
void stringPolyToIntArr(char * stringPoly , int ** polynomial, int size) {
    (*polynomial) = (int*) malloc(sizeof(int) * size);
    int indexOfPolynomial = size - 1;
    char * numberInString = (char*) malloc(sizeof(char) * max_len);
    int i = strlen(stringPoly) - 1; // remove 1 so that we start from before NULL
    while (stringPoly[i] != ':') {
        int j = 0;
        while (stringPoly[i] != ',' && stringPoly[i] != ':') {
            numberInString[j] = stringPoly[i];
            j++;i--;
        }
        numberInString[j] = '\0';
        reverse(numberInString,j);
        int number = atoi(numberInString);
        (*polynomial)[indexOfPolynomial] = number;
        indexOfPolynomial--;
        if (stringPoly[i] != ':')
            i--;
    }
    while (indexOfPolynomial >= 0) {
        (*polynomial)[indexOfPolynomial] = 0;
        indexOfPolynomial--;
    }
    free(numberInString);
}
// slice the command into three strings representing the first and second polynomials and the command
int parseCommand(char * cmd, int size , int ** polynomial_1, int ** polynomial_2, int * sizeResult) {
    int index = 1;
    char operation[3];
    char* subStringOne = parseFromIndex(cmd, size, &index);

    int i = 0;
    while (cmd[index] != '(') {
        operation[i] = cmd[index];
        i++;index++;
    }
    index++;
    char* subStringTwo = parseFromIndex(cmd, size, &index);
    int realSize = getRealSizeOfPoly(subStringOne, subStringTwo);
    (*sizeResult) = realSize;

    stringPolyToIntArr(subStringOne, polynomial_1, realSize);
    stringPolyToIntArr(subStringTwo, polynomial_2, realSize);

    free(subStringOne);
    free(subStringTwo);

    return commandToInt(operation);
}
// thread function, calculate addition/subtraction based on operation,
// get arguments from struct return results to the struct
void* addOrSubPolynomials(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;
    int* result = args->result;
    int* poly1 = args->poly1;
    int* poly2 = args->poly2;
    int operation = args->operation;
    int index = args->index;

    result[index] = poly1[index] + (poly2[index]*operation);
    pthread_exit(NULL);
}
// connect to shared memory
void getSharedMemory(void ** ptr, char* name, int size) {
    int fd_shm;
    if ((fd_shm = shm_open (name, O_RDWR , 0)) == -1)
        perror (error_msg);
    if (((*ptr) = mmap (NULL,size , PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0)) == MAP_FAILED)
        perror (error_msg);
}
// multi thread add or sub based on operation: 1 = add, -1 = sub
int* addOrSubMultiThreaded(int * polynomial_1,int * polynomial_2, int size, int operation) {
    int* result = (int*)malloc(sizeof(int) * size);
    pthread_t threads[size];
    ThreadArgs args[size];

    for (int i = 0; i < size; i++) {
        args[i].result = result;
        args[i].poly1 = polynomial_1;
        args[i].poly2 = polynomial_2;
        args[i].operation = operation;
        args[i].index = i;
        pthread_create(&threads[i], NULL, addOrSubPolynomials, (void*)&args[i]);
    }

    for (int i = 0; i < size; i++) {
        pthread_join(threads[i], NULL);
    }

    return result;
}
// pass arguments to threaded add/sub based on the argument 1 = addition, -1 = subtraction
int* ADD(int * polynomial_1,int * polynomial_2, int size) {
    return addOrSubMultiThreaded(polynomial_1,polynomial_2,size,1);
}
int * SUB(int * polynomial_1,int * polynomial_2, int size) {
    return addOrSubMultiThreaded(polynomial_1,polynomial_2,size,-1);
}
// multiplication works the as the previous assignments
int * MUL(const int * polynomial_1, const int * polynomial_2, int size) {
    int * result = (int*)malloc(sizeof(int) * (2 * size - 1));  // Resulting polynomial can have a maximum degree of 2 * size - 2
    // Initialize the result array with zeros
    for (int i = 0; i < 2 * size - 1; i++) {
        result[i] = 0;
    }
    // Multiply the polynomials
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            result[i + j] += polynomial_1[i] * polynomial_2[j];
        }
    }
    return result;
}
int * executeOperation(int operation, int * polynomial_1, int * polynomial_2, int size) {
    switch (operation) {
        case 0:
            return ADD(polynomial_1, polynomial_2, size);
        case 1:
            return SUB(polynomial_1, polynomial_2, size);
        case 2:
            return MUL(polynomial_1, polynomial_2, size);
    }
    perror("OPERATION ERR");
    exit(EXIT_FAILURE);
}