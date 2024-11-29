// 205438542
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define max_len 128
#define Addition "ADD"
#define Subtraction "SUB"
#define Multiplication "MUL"

void displayResultAsPolynomial(int ** result, int operation, int sizeOfPoly);
int commandToInt(char * command);
int getSizeOfPolyArr(const char * arr);
char* parseFromIndex(const char * cmd, int size, int * index);
int getRealSizeOfPoly(char * subStringOne, char * subStringTwo);
void reverse(char string[], int length);
void stringPolyToIntArr(char * stringPoly , int ** polynomial, int size);
int parseCommand(char * cmd, int size , int ** polynomial_1, int ** polynomial_2, int * sizeResult);
int * ADD(const int * polynomial_1,const  int * polynomial_2, int size);
int * SUB(const int * polynomial_1,const int * polynomial_2, int size);
int * MUL(const int * polynomial_1, const int * polynomial_2, int size);
int * executeOperation(int operation, int * polynomial_1, int * polynomial_2, int size);

int main() {
    while (1) {
        char cmd[max_len];
        int cmdLen = strlen(cmd);
        // get input
        scanf("%s",cmd);

        // to exit
        if (strcmp(cmd,"END")==0)
            break;

        int * polynomial_1;
        int * polynomial_2;
        int sizeOfPoly;

        // string to integer array
        int operation = parseCommand(cmd, cmdLen, &polynomial_1, &polynomial_2, &sizeOfPoly);
        // calculate result
        int * result = executeOperation(operation, polynomial_1, polynomial_2, sizeOfPoly);
        // display result
        displayResultAsPolynomial(&result, operation, sizeOfPoly);
        free(result);
        free(polynomial_1);
        free(polynomial_2);
    }
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
int * ADD(const int * polynomial_1,const  int * polynomial_2, int size) {
    int * result = (int *) malloc(sizeof(int) * size);
    result[0] = polynomial_1[0] + polynomial_2[0];
    //add each coefficient
    for (int i = 0 ; i < size ; i++) {
        result[i] = polynomial_1[i] + polynomial_2[i];
    }
    return result;
}
int * SUB(const int * polynomial_1,const int * polynomial_2, int size) {
    int * result = (int *) malloc(sizeof(int) * size);
    // subtract each coefficient
    for (int i = 0 ; i < size ; i++) {
        result[i] = polynomial_1[i] - polynomial_2[i];
    }
    return result;
}
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