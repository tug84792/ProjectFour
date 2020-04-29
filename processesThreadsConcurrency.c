
#include "processesThreadsConcurrency.h"


double totalUSR1 = 0, totalUSR2 = 0;
//Used a time datatype to get the timings of start and completion
time_t counterForUSR1 = 0, finalSIGUSR1, counterForUSR2 = 0, finalSIGUSR2;
const char *pathForSharedMemory = "Multiple Processes";

struct sharedMemoryBuffer *sharedMemoryObject;

int sharedMemoryFileDescriptor;
//Here I have a struct function that basically gets the shared memory object based on
//a number of conditions
struct sharedMemoryBuffer * receiveSMO() {

    //Here, I start up generating processes using shm_open. I use a number of flags to set the read/write privileges.
    //If the file doesn't exist, I created it using O_CREAT
    int fileDescriptor = shm_open(pathForSharedMemory, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

    //error handling if getting the file doesn't work
    if (fileDescriptor == -1) {
        printError();
    }

    /* Map the object into the caller's address space */
    //Using mmap(), I'll map the shm object from previous line into the address space of the caller, basically the FD.
    //The pages can be read and/or written due to the given flags. MAP_SHARED will keep everything updated.
    struct sharedMemoryBuffer *sharedMemoryProcess = mmap(NULL, sizeof(struct sharedMemoryBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    //error handling if mapping doesn't work
    if (sharedMemoryProcess == MAP_FAILED) {
        printError();
    }
    //store the local FD to the global variable, then return the process overall
    sharedMemoryFileDescriptor = fileDescriptor;
    return sharedMemoryProcess;
}
//With this struct, I'll create a shm object and assign its size to that of the struct itself. I also initialize multiple
//things as necessary.
struct sharedMemoryBuffer* initializeSMO(){

    //Here, I start up generating processes using shm_open. I use a number of flags to set the read/write privileges.
    //If the file doesn't exist, I created it using O_CREAT
    int fileDescriptor = shm_open(pathForSharedMemory, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    //error handling if getting the file doesn't work
    if (fileDescriptor == -1) {
        printError();
    }
    
    //Here is where I do the actual setting of the size of the fd to the struct using ftruncate. I also do an error check.
    if (ftruncate(fileDescriptor, sizeof(struct sharedMemoryBuffer)) == -1) {
        printError();
    }

    //Just like before, using mmap(), I'll map the shm object from previous line into the address space of the caller, basically the FD.
    //The pages can be read and/or written due to the given flags. MAP_SHARED will keep everything updated.
    struct sharedMemoryBuffer *sharedMemoryProcess = mmap(NULL, sizeof(struct sharedMemoryBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    //Error check if mapping failed
    if (sharedMemoryProcess == MAP_FAILED) {
        printError();
    }
    /* Initialize semaphores as process-shared, with value 0 */
    //Here, I'll initialize semaphores to be shared with processes, to the address of sem s, with value of 1
    if (sem_init(&(*sharedMemoryProcess).s, 1, 1) == -1) {
        printError();
    }
    //store the local FD to the global variable.
    sharedMemoryFileDescriptor = fileDescriptor;

    (*sharedMemoryProcess).counterForSIGUSR1Sent = 0;
    (*sharedMemoryProcess).counterForSIGUSR2Sent = 0;
    (*sharedMemoryProcess).counterForSIGUSR1Received = 0;
    (*sharedMemoryProcess).counterForSIGUSR2Received = 0;
    (*sharedMemoryProcess).counterForSignals = 0;
    (*sharedMemoryProcess).counterForComplete = 0;
    (*sharedMemoryProcess).complete = 0;

    pthread_mutexattr_t attribute;
    //initialize previous thread attribute here with default values
    pthread_mutexattr_init(&attribute);
    //Here I access the value of that process-shared attribute. The flagg allows multiple processes to use the mutex and attr.
    pthread_mutexattr_setpshared(&attribute, PTHREAD_PROCESS_SHARED);

    //finally initialize the three locks I began in the header file.
    pthread_mutex_init(&(*sharedMemoryProcess).mutexLock, &attribute);
    pthread_mutex_init(&(*sharedMemoryProcess).signalMutexLock, &attribute);
    pthread_mutex_init(&(*sharedMemoryProcess).counterMutexLock, &attribute);

    //return process overall
    return sharedMemoryProcess;
}
//With this function, I basically delete everything that I initialized previously.
void cleanSMO(){
    /* Unlink the shared memory object. Even if the peer process
   is still using the object, this is okay. The object will
   be removed only after all open references are closed. */
    //Close out the fd and shared memory object path, and object itself.
    close(sharedMemoryFileDescriptor);
    shm_unlink(pathForSharedMemory);
    sem_destroy(&(*sharedMemoryObject).s);

    //Get rid of all the locks
    pthread_mutex_destroy(&(*sharedMemoryObject).mutexLock);
    pthread_mutex_destroy(&(*sharedMemoryObject).signalMutexLock);
    pthread_mutex_destroy(&(*sharedMemoryObject).counterMutexLock);

    //Finally, take out the object from the buffer
    munmap(sharedMemoryObject, sizeof(struct sharedMemoryBuffer));
}
//This function uses special data types built-in for timestamp purposes
void displayTime(char *currentTime) {
    struct timeStruct* timestamp;
    time_t t;
    //initialize time to NULL using time(), then set it appropriately.
    t = time(NULL);
    timestamp = localtime(&t);
    //Here, I use strftime to print the time.
    strftime(currentTime, 26, "%Y-%m-%d %H:%M:%S", (const struct tm *) timestamp);
}
