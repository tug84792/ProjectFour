
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
