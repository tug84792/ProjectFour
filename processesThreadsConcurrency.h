#ifndef PROJECT4_PROCESSESTHREADSCONCURRENCY_H
#define PROJECT4_PROCESSESTHREADSCONCURRENCY_H

#include <stdio.h>    
#include <stdlib.h>   
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/mman.h>
#include<sys/wait.h>

//Here I declare a max size for multiple things, including processes, signals, and string size
#define TOTALAMOUNTOFPROCESSES 8
#define TOTALNUMBEROFSIGNALS 1000
#define TOTALSIZEOFBUFFER 1024

//Here I create a struct that is basically like a buffer. I eventually use this on the object
//that is sharing memory
struct sharedMemoryBuffer {
    sem_t  s;            /* POSIX unnamed semaphore */
    pthread_mutex_t mutexLock, signalMutexLock, counterMutexLock;
    int complete, counterForComplete, counterForSignals, counterForSIGUSR1Sent,  counterForSIGUSR2Sent, counterForSIGUSR1Received, counterForSIGUSR2Received;
};

//Error message function
const char *error = "An error has occurred.\n";
void printError(){
    perror(error);
    exit(EXIT_FAILURE);
}

#endif //PROJECT4_PROCESSESTHREADSCONCURRENCY_H
