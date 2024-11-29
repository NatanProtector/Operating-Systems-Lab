// 205438542
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <fcntl.h>

#define MAX_ENTERS 3
#define MAX_ARGS 10
#define MAX_LENGTH 510
#define Max_Local_Var 20
#define Max_Pipes 20
#define Max_path_size 100
#define Error_msg "ERR"
#define Cd_error_msg "cd not supported"
#define Args_msg "too many args"
#define err_msg_length "command too long"

char* trimWhiteSpace(char* str);
char* getValue(char* name , char** nameArr , char** valArr);
int variableDeclaration(const char * str, int len);
void insertVar(char* str , char** nameArr , char** valArr);
void prompt(int legalArgs , int legalCommands);
int parse(char* sectionToken, char* newArg,  int len, int index);
void freeArr(char** arr, int index);
int isOutputToFileDec(char* sectionToken, int secLen, char *nameOfFile);
void createPipe(int** pipeArr, int pipeStatus);
void closePipe(int first, int second);
void dupTwo(int fileIndexOne, int fileIndexTwo, char* errorMsg);
int checkIfThreeEnterPressed(char* cmd,int *numberOfEntersPressed);
int processVariableDeclaraion(char* section, char** nameArray, char** valueArray);
char * myStrTok(char * strToTokenize, int lengthOfStr, char delim , int* pointerToLastStop);
void handlerForChildTermination();
void pauseHandler();

// global variables accessed by signal handlers.
int pidOfPaused = -1;
int pidOfLastCommand = -1;

int localVarCount = 0;
int main() {
    char*nameArray[Max_Local_Var];
    char*valueArray[Max_Local_Var];
    char cmd[MAX_LENGTH*MAX_LENGTH];
    int numberOfEntersPressed = 0;
    int legalArgsCount = 0;
    int legalCommandsCount = 0;

    signal(SIGCHLD, handlerForChildTermination);

    while (1) {
        // display the prompt, using the commands and args count
        prompt(legalArgsCount, legalCommandsCount);

        // get input from the user
        fgets(cmd, MAX_LENGTH*MAX_LENGTH, stdin);

        // Exit the loop if the user inputs "enter" three times in a row
        if (checkIfThreeEnterPressed(cmd,&numberOfEntersPressed))
            break;

        // Remove newline character from input
        cmd[strcspn(cmd, "\n")] = '\0';

        // check length
        int commandLength = strlen(cmd);
        if (commandLength > MAX_LENGTH) {
            printf("%s\n",err_msg_length);
            continue;
        }

        // outer loop for commands divided by ';'
        //char * pToCmd = cmd;
        //char *pipeSecToken = strtok_r(cmd, ";", &pToCmd);

        int indexOfStart = 0;
        char * pipeSecToken = myStrTok(cmd, commandLength, ';', &indexOfStart);

        while (pipeSecToken != NULL) {

            // create copy to not mess with original string
            char *pipeSecTokenCopy = (char*)malloc((sizeof (char)) * strlen(pipeSecToken));
            strcpy(pipeSecTokenCopy,pipeSecToken);

            if (processVariableDeclaraion(pipeSecTokenCopy, nameArray,valueArray) == 1) {
                free(pipeSecTokenCopy);
                free(pipeSecToken);
                pipeSecToken = myStrTok(cmd, commandLength, ';', &indexOfStart);
                continue;
            }

            // keep track of pipe status
            // status=0 - no pipe| status>0 - (status + 1) pipes left
            int pipeStatus = 0;
            int numOfPipes = 0;
            for (int i = 0 ; pipeSecTokenCopy[i] != '\0' ; i++ ) // count pipes
                if (pipeSecTokenCopy[i] == '|')
                    pipeStatus++;
            if (pipeStatus>0)
                pipeStatus++;

            //create pipes if needed:
            int** pipeArr = (int**)malloc(sizeof(int*) * Max_Pipes);
            if (pipeStatus>0) {
                numOfPipes = pipeStatus-1;
                createPipe(pipeArr,numOfPipes);
            }

            // parsing for pipe
            char * pToPipeSec = pipeSecTokenCopy;
            char *sectionToken = strtok_r(pipeSecTokenCopy, "|", &pToPipeSec);
            // loop for pipe
            while (sectionToken != NULL) {

                // create copy to not mess with original string
                char *sectionTokenCopy = (char*)malloc((sizeof (char)) * strlen(sectionToken));
                strcpy(sectionTokenCopy,sectionToken);

                while (strchr(sectionTokenCopy, '$') != NULL)
                    insertVar(sectionTokenCopy, nameArray, valueArray);

                // process command string into arguments
                char *args[MAX_ARGS];
                int argc = 0;
                int secLen = strlen(sectionTokenCopy);
                int backGroundFlag = 0;

                // check for backGround declaration
                if (sectionTokenCopy[secLen-1] == '&') {
                    sectionTokenCopy[secLen-1] = '\0';
                    backGroundFlag = 1;
                }

                // check for output to file declaration
                int outputToFileFlag = 0;
                char* nameOfFile = (char*)malloc(sizeof (char) * secLen);
                int indexOfDec = isOutputToFileDec(sectionTokenCopy, secLen, nameOfFile); // find '>' index
                if (indexOfDec > -1) {
                    sectionTokenCopy[indexOfDec] = '\0';
                    secLen = indexOfDec;
                    strcat(nameOfFile,".txt");
                    outputToFileFlag = 1;
                } else if(indexOfDec == -2) {
                    printf("syntax error\n");
                    free(sectionTokenCopy);
                    free(nameOfFile);
                    break;
                }

                // move paused process to background
                if (strcmp(sectionTokenCopy, "bg") == 0) {
                    if (pidOfPaused != -1) {
                        printf("continued pid: %d\n", pidOfPaused);
                        kill(pidOfPaused, SIGCONT);
                        pidOfPaused = -1;
                    } else {
                        printf( "no current paused process");
                    }
                    free(sectionTokenCopy);
                    free(nameOfFile);
                    break;
                }

                // parsing arguments
                int successfulParseFlag = 1;
                int index = 0;
                while (index != secLen) {
                    if (argc == MAX_ARGS + 1) {
                        successfulParseFlag = 0;
                        break;
                    }

                    char * newArg = (char*)malloc((sizeof (char)));
                    index = parse(sectionTokenCopy, newArg, secLen, index);

                    // insert token copy into argument array
                    args[argc] = newArg;
                    argc++;
                }

                // check if parse successful
                if (successfulParseFlag == 0) {
                    printf("%s\n",Args_msg);
                    free(nameOfFile);
                    free(sectionTokenCopy);
                    freeArr(args ,argc-1);
                    break;
                }
                // null terminate args array
                args[argc] = NULL;

                // set the sigtstp signal to activate pauseHandler
                signal(SIGTSTP, pauseHandler);

                pid_t pid = fork();

                // record last command
                pidOfLastCommand = pid;

                if (pid == -1) {
                    perror(Error_msg);
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    // Child process

                    // cd command is NOT SUPPORTED
                    if (!strcmp(args[0], "cd")) {
                        // report error
                        perror(Cd_error_msg);
                        exit(EXIT_FAILURE); // terminate child process
                    }

                    // handle pipe chains (works for one only so far)
                    if (pipeStatus == numOfPipes + 1) { // if first command in pipe
                        int nextPipe = pipeStatus - 2;
                        dupTwo(pipeArr[nextPipe][1],STDOUT_FILENO, "dup2");
                        closePipe(pipeArr[nextPipe][0],pipeArr[nextPipe][1]);
                    }
                    else if (pipeStatus > 1 && pipeStatus != numOfPipes + 1) { // if middle command in pipe
                        int nextPipe = pipeStatus - 2;
                        dupTwo(pipeArr[nextPipe][1],STDOUT_FILENO, "dup2");
                        closePipe(pipeArr[nextPipe][0],pipeArr[nextPipe][1]);
                        int thisPipe = pipeStatus - 1;
                        dupTwo(pipeArr[thisPipe][0],STDIN_FILENO, "dup2");
                        closePipe(pipeArr[thisPipe][0],pipeArr[thisPipe][1]);
                    }
                    else if (pipeStatus == 1) { // if final command in pipe
                        int thisPipe = pipeStatus - 1;
                        dupTwo(pipeArr[thisPipe][0],STDIN_FILENO, "dup2");
                        closePipe(pipeArr[thisPipe][0],pipeArr[thisPipe][1]);
                    }


                    // if output to file command was given
                    if (outputToFileFlag == 1 && (pipeStatus == 1 || pipeStatus == 0)) {
                        int file = open(nameOfFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (file < 0) {
                            perror("File opening failed");
                            exit(EXIT_FAILURE);
                        }
                        // Redirect stdout to the file
                        dupTwo(file, STDOUT_FILENO, "Duplication failed");
                        // Close the file descriptor
                        close(file);
                    }

                    // Execute the command
                    if (execvp(args[0], args) == -1) {
                        /*
                         * if execvp executes then it will dump all the memory including the dynamically
                         * allocated memory, therefor no need to free the memory unless execvp does not work.
                         * */
                        freeArr(args ,argc-1);
                        // report error
                        perror(Error_msg);
                        exit(EXIT_FAILURE);
                    }
                } else {
                    // Parent process

                    // close parent pipe
                    if (pipeStatus > 1 && pipeStatus != numOfPipes + 1) { // if not after first or last command
                        closePipe(pipeArr[pipeStatus-1][0],pipeArr[pipeStatus-1][1]);
                    }
                    else if (pipeStatus == 1) // if after last command in pipe
                        closePipe(pipeArr[0][0],pipeArr[0][1]);


                    // Wait for child to exit and save status
                    int status;
                    if (backGroundFlag == 0)
                        waitpid(pid, &status, WUNTRACED);

                    // if exit status is not a failure increment the command and variable counters
                    if (WIFEXITED(status) && WEXITSTATUS(status) != EXIT_FAILURE) {
                        legalArgsCount += (argc);
                        legalCommandsCount++;
                    }

                    freeArr(args ,argc-1);
                }

                free(nameOfFile); // free array for file name

                pipeStatus--;

                free(sectionTokenCopy);

                sectionToken = strtok_r(NULL,"|", &pToPipeSec);// next token
            }

            // close pipes and free pipe array
            for (int i = 0; i<numOfPipes ; i++)
                free(pipeArr[i]);
            free(pipeArr);

            // next token

            free(pipeSecTokenCopy);

            free(pipeSecToken);

            pipeSecToken = myStrTok(cmd, commandLength, ';', &indexOfStart);
        }
    }
    // free memory from local variables
    freeArr(nameArray , localVarCount-1);
    freeArr(valueArray , localVarCount-1);
    localVarCount = 0;

    return 0;
}
// check for '>' that is not in quotes, return its index, also create the string for the file name.
// return -1 if no declaration was found, return -2 if syntax error,
int isOutputToFileDec(char* sectionToken, int secLen, char *nameOfFile) {
    int inQuotes = 0;
    int indexOfDeclaration = -1;
    int index = 0;
    while (index != secLen) { // find index of '>'
        if (sectionToken[index] == '>' && inQuotes == 0) {
            indexOfDeclaration = index;
            break;
        } else if (sectionToken[index] == '"') {
            if (inQuotes ==0)
                inQuotes = 1;
            else
                inQuotes = 0;
        }
        index++;
    }
    if (indexOfDeclaration != -1) {
        int i = 0;
        index++;
        while (sectionToken[index] == ' ' && index != secLen)
            index++;
        while (sectionToken[index] != ' ' && index != secLen) {
            nameOfFile[i] = sectionToken[index];
            i++; index++;
        }
        nameOfFile[i] = '\0';
        if (i == 0)
            indexOfDeclaration = -2;
    }
    return indexOfDeclaration;
}
// display the command prompt in the following format:
void prompt(int legalArgs , int legalCommands) {
    char currentDirectory[Max_path_size];
    if (getcwd(currentDirectory, Max_path_size) == NULL) {
        perror(Error_msg);
    } else {
        printf("#cmd:%d|#args:%d@%s ",legalCommands,legalArgs,currentDirectory);
    }
}
// free array from index to 0
void freeArr(char** arr, int index) {
    for (int i = index; i>-1 ;i--){
        free(arr[i]);
    }
}
// trim all spaces from sides
char* trimWhiteSpace(char *str) {
    char *end;
    // Trim leading space
    while(((unsigned char)*str) == ' ') str++;

    // Check if string is all spaces
    if(*str == 0)
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && ((unsigned char)*end) == ' ') end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}
// return 1 if three enters were pressed in a row, return 0 and increment counter otherwise
int checkIfThreeEnterPressed(char* cmd,int *numberOfEntersPressed) {
    if (strcmp(cmd, "\n") == 0) {
        (*numberOfEntersPressed)++;
        if (*numberOfEntersPressed == MAX_ENTERS)
            return 1;
        else
            return 0;
    }
    else {
        (*numberOfEntersPressed) = 0;
        return 0;
    }
}

// search for value based on name in nameArray and valueArray
char* getValue(char* name, char** nameArr , char** valArr) {
    int i = 0;
    while (i != localVarCount) {
        if (strcmp(name, nameArr[i]) == 0) {
            return valArr[i];
        }
        i++;
    }
    return "";
}

// check if token is of the format <something>$<variable name> and if so insert the variable
void insertVar(char* str , char** nameArr , char** valArr){
    long len = strlen(str);
    int i = 0;
    while (str[i] != '\0') {
        if (str[i] == '$') {

            // using the index of '$' and the length of str create an array for the input
            char* nameInput = (char*)malloc((sizeof (char))*(len - i - 1));
            nameInput[len - i + 1] = '\0';

            // copy the name of the declaration, then input into the allocated array
            int k = i+1; int j = 0;
            while (str[k] != '\0' && str[k] != ' ' && str[k] != '"' && str[k] != '/' && str[k] != '|') {
                nameInput[j] = str[k];
                k++; j++;
            }
            nameInput[j] = '\0';

            // copy the of rest of the string, then input into the allocated array
            char* restOfInput = (char*)malloc((sizeof (char))*(j));
            j = 0;
            while (str[k] != '\0') {
                restOfInput[j] = str[k];
                k++; j++;
            }
            restOfInput[j] = '\0';

            // get the value for the name
            char * value = getValue(nameInput, nameArr,  valArr);
            char * valueCopy = (char*)malloc((sizeof (char))* strlen(value));
            strcpy(valueCopy, value);
            str[i] = '\0';

            //concat both arrays and free
            printf("%s\n",str);
            strcat(str, valueCopy);
            strcat(str, restOfInput);
            printf("%s\n",str);
            free(nameInput);
            free(restOfInput);
            free(valueCopy);
            return;
        }
        i++;
    }

}
// check if legal variable declaration, return index of first '=', return -1 if not legal declaration.
int variableDeclaration(const char * str, int len) {
    int indexOfEquals = 0;
    while (indexOfEquals != len) {
        // variable declaration cannot contain ' ' or '$'
        if (str[indexOfEquals] == ' ' || str[indexOfEquals] == '$') {
            break;
        }
            // variable name cannot be empty array
        else if (str[indexOfEquals] == '=' && indexOfEquals > 0) {
            return indexOfEquals;
        }
        indexOfEquals++;
    }
    return -1;
}
int processVariableDeclaraion(char* section, char** nameArray, char** valueArray) {
    // trim spaces off the edges of the section
    char* trimSec = trimWhiteSpace(section);
    // check for variable declaration must be done here or the children won't have access to
    // the local variables when the fork() executes.
    // trim spaces off edges
    int trimSecLen = strlen(trimSec);
    // check if valid variable declaration
    int indexOfEqual = variableDeclaration(trimSec, trimSecLen);
    if (indexOfEqual != -1 && localVarCount != Max_Local_Var) {
        // create new string for the name and value in the arrays
        nameArray[localVarCount] = (char*) malloc((sizeof (char)*(indexOfEqual + 1)));
        valueArray[localVarCount] = (char*) malloc((sizeof (char)*(trimSecLen - indexOfEqual + 1)));

        // name '$'=NULL to "slice" the string into the name section and value section
        trimSec[indexOfEqual] = '\0';

        // then copy the sections
        strcpy(nameArray[localVarCount], trimSec);
        strcpy(valueArray[localVarCount] , trimSec + indexOfEqual + 1);
        localVarCount++;
        return 1;
    } else
        return 0;
}

/* this function parses a string from a starting index up to the end of the section we are from, or
 * until the algorithm halts. return the index we ended the algorithm at. runs at o(n) time.
 * the algorithm: (assignment assumption: every " will have a closing ")
 * we save an integer recording the current state we are currently at, then we loop through the section starting
 * at the index input until we either reach the end of the section, or we get to the end of and argument.
 * we determine if we are at the end of an argument using the state as following:
 *   state 0 initial state - we are at the start of the search, ignore spaces and go to state 2 if we find quotes
 *                           or state 1 if we find something else.
 *   state 1 in argument - we are in an argument, keep recording char unless we hit a space or the end, in which
 *                         case we go to state 3 and halt. or we hit quotes, in which case we switch to state 2
 *   state 2 in first quote - we are in quotes, keep recording chars until we reach closing quotes
 *   state 3 in second quote - we found the second quotes, and now we want to find the rest of the argument
 *                             by recording every char until we reach a space.
 *   state 4 done - break the while loop.
 * */

int parse(char* sectionToken, char* newArg , int len , int index) {
    int i = index;
    int newArgIndex = 0;
    int state = 0;
    while (i != len) {
        switch (state) {
            case 0:
                if (sectionToken[i] == '"') {
                    state = 2;
                } else if (sectionToken[i] != ' ') {
                    state = 1;
                    newArg[newArgIndex++] = sectionToken[i];
                }
                break;
            case 1:
                if (sectionToken[i] == ' ' && newArgIndex > 0) {
                    state = 4;
                } else {
                    if (sectionToken[i] == '"')
                        state = 2;
                    else
                        newArg[newArgIndex++] = sectionToken[i];
                }
                break;
            case 2:
                if (sectionToken[i] == '"')
                    state = 3;
                else
                    newArg[newArgIndex++] = sectionToken[i];
                break;
            case 3:
                if (sectionToken[i] == ' ')
                    state = 4;
                else
                if (sectionToken[i] != '"')
                    newArg[newArgIndex++] = sectionToken[i];
        }
        i++;
        if (state == 4)
            break;
    }
    // null terminate the string.
    newArg[newArgIndex++] = '\0';
    return i;
}
// custom tokenizer, ignores tokens inside quotes, and does not mess with original string.
char * myStrTok(char * strToTokenize, int lengthOfStr, char delim , int* pointerToLastStop) {
    int index = *pointerToLastStop;
    if (index >= lengthOfStr) {
        return NULL;
    }
    int inQuotes = 0;
    int sizeOfNewToken = 0;
    while (strToTokenize[index] != '\0') {
        if (inQuotes == 0 && strToTokenize[index] == delim) {
            break;
        }
        if (strToTokenize[index] == '"') {
            inQuotes++;
            inQuotes = inQuotes%2;
        }
        sizeOfNewToken++;
        index++;
    }
    char * newToken = (char*)malloc((sizeof (char)) * (sizeOfNewToken));
    for (int i = 0 ; i < sizeOfNewToken ; i++) {
        newToken[i] = strToTokenize[(*pointerToLastStop) + i];
    }
    newToken[sizeOfNewToken] = '\0';
    index++;
    *pointerToLastStop = index;
    return newToken;
}

// create pipes
void createPipe(int** pipeArr, int pipeStatus) {
    for (int i = 0 ; i < pipeStatus ; i++) {
        pipeArr[i] = (int*)malloc((sizeof (int))*3);
        pipeArr[i][3] = '\0';
        if (pipe(pipeArr[i]) == -1) {
            printf("pipe creation did not work\n");
        }
    }
    pipeArr[pipeStatus] = NULL;
}
// close pipe if not -1
void closePipe(int first, int second) {
    if (first != -1)
        close(first);
    if (second != -1)
        close(second);
}
// dup2, only to be used by child
void dupTwo(int fileIndexOne, int fileIndexTwo, char* errorMsg) {
    if (dup2(fileIndexOne, fileIndexTwo) == -1) {
        perror(errorMsg);
        exit(EXIT_FAILURE);
    }
}

// signal handlers for background children.
void handlerForChildTermination () {
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
}
void pauseHandler() {
    if (pidOfPaused == -1) {
        kill(pidOfLastCommand, SIGSTOP);
        pidOfPaused = pidOfLastCommand;
        printf("paused %d\n", pidOfPaused);
    }
}