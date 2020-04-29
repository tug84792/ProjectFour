
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
    if (sem_init(&(*sharedMemoryProcess).siddiquiSemaphore, 1, 1) == -1) {
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
    sem_destroy(&(*sharedMemoryObject).siddiquiSemaphore);

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

int handleSignals(){
    int i = 0;
    //lock, update i to get value for completed signals, then unlock
    pthread_mutex_lock(&(*sharedMemoryObject).signalMutexLock);
    i = (*sharedMemoryObject).counterForComplete;
    pthread_mutex_unlock(&(*sharedMemoryObject).signalMutexLock);
    return i;
}

//With this function, I basically V() the number of signals I got
void incrementReceivedSigCount(int getSignalType){
    //Error checking
    if (pthread_mutex_lock(&(*sharedMemoryObject).mutexLock) == -1) {
        printError();
    }
    //Use this if-else to filter out the appropriate counter.
    if(getSignalType == SIGUSR1){
        (*sharedMemoryObject).counterForSIGUSR1Received++;
    } else {
        (*sharedMemoryObject).counterForSIGUSR2Received++;
    }
    //Let go of the lick and error check too.
    if (pthread_mutex_unlock(&(*sharedMemoryObject).mutexLock) == -1) {
        printError();
    }
}

//Both of the following functions: incrementSignalsHandled(), decrementSignalsHandled(),
// work just like handleSignals(). Update a counter variable that's protected by a lock.
//In between, i have a separate function to use those functions.
int incrementSignalsHandled(int totalSignals){
    pthread_mutex_lock(&(*sharedMemoryObject).signalMutexLock);
    (*sharedMemoryObject).counterForComplete += totalSignals;
    pthread_mutex_unlock(&(*sharedMemoryObject).signalMutexLock);
}
void handleSIGUSR1(int signalArg) {
    incrementReceivedSigCount(signalArg);
}
int decrementSignalsHandled(int totalSignals){
    pthread_mutex_lock(&(*sharedMemoryObject).signalMutexLock);
    (*sharedMemoryObject).counterForComplete -= totalSignals;
    pthread_mutex_unlock(&(*sharedMemoryObject).signalMutexLock);
}
void handleSIGUSR2(int signalType){
    incrementReceivedSigCount(signalType);
}


//This works the same as the incrementReceivedSigCount function, except it's for signals sent out
void incrementSentSigCount(int sigType){

    if (pthread_mutex_lock(&(*sharedMemoryObject).mutexLock) == -1) {
        printError();
    }
    if(sigType == SIGUSR1){
        (*sharedMemoryObject).counterForSIGUSR1Sent++;
    } else {
        (*sharedMemoryObject).counterForSIGUSR2Sent++;
    }
    if (pthread_mutex_unlock(&(*sharedMemoryObject).mutexLock) == -1) {
        printError();
    }

}


void handledSignals(int signalArg){
    //Current time string
    char ctString[26];
    //built-in struct
    struct timeb currentTime;
    //Initialize averages.
    float SIGUSR1AverageTime = 0, SIGUSR2AverageTime = 0;
    //initialize all counters to 0 and last variable acts as a boolean for printing later on.
    int sentCountSigUsr1 = 0, receivedCountSigUsr1 = 0, sentCountSigUsr2 = 0, receivedCountSigUsr2 = 0, validForPrinting = 0;

    //Begin locking for updating counter.
    pthread_mutex_lock(&(*sharedMemoryObject).signalMutexLock);
    (*sharedMemoryObject).counterForComplete += 1;

    //Here I check what kind of signal it was, whether SIGUSR1 or 2
    if(signalArg == SIGUSR1){
        //Used built-in ftime() function
        ftime(&currentTime);
        //I do some calculating to get the appropriate time. The current time minus the last recorded one
        totalUSR1 = totalUSR1 +  (currentTime.time * 1000 + currentTime.millitm - finalSIGUSR1);
        //Update final user time
        finalSIGUSR1 = (currentTime.time * 1000) + currentTime.millitm;
        //update counter
        counterForUSR1++;
    }
    //Same as before, except for SIGUSR2
    else {
        ftime(&currentTime);
        totalUSR2 = totalUSR2  +  (currentTime.time * 1000 + currentTime.millitm - finalSIGUSR2);
        finalSIGUSR2 = (currentTime.time * 1000) + currentTime.millitm;
        counterForUSR2++;
    }

    if((*sharedMemoryObject).counterForComplete == 10){
        //Initialize the variables to 0, otherwise, it isn't working
        //then, calculate the average for each respective user
        SIGUSR1AverageTime = 0;
        SIGUSR1AverageTime = totalUSR1 / counterForUSR1,
        SIGUSR2AverageTime = 0;
        SIGUSR2AverageTime = totalUSR2 / counterForUSR2;

        //Error check for locking
        if (pthread_mutex_lock(&(*sharedMemoryObject).mutexLock) == -1) {
            printError();
        }
        //Update the counters for each user
        sentCountSigUsr1 = (*sharedMemoryObject).counterForSIGUSR1Sent;
        receivedCountSigUsr1 = (*sharedMemoryObject).counterForSIGUSR1Received;

        sentCountSigUsr2 = (*sharedMemoryObject).counterForSIGUSR2Sent;
        receivedCountSigUsr2 = (*sharedMemoryObject).counterForSIGUSR2Received;

        //Error checking
        if (pthread_mutex_unlock(&(*sharedMemoryObject).mutexLock) == -1) {
            printError();
        }

        //Set boolean variable to true
        validForPrinting = 1;

        //reset all counters for next time it's run
        (*sharedMemoryObject).counterForComplete = 0;
        totalUSR1 = 0, counterForUSR1 = 0, totalUSR2 = 0, counterForUSR2 = 0;
    }
    //Stop locking
    pthread_mutex_unlock(&(*sharedMemoryObject).signalMutexLock);
    //Here I use that boolean variable to see if everything checks off. And if it does, then print the information.
    if(validForPrinting) {
        //display the current time
        displayTime(ctString);
        //Print all info in a nice format
        printf("+++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("%s\n", ctString);
        printf("Number sent \n");
        printf("Total SIGUSR1's that were sent: %d\n", sentCountSigUsr1);
        printf("Total SIGUSR2 that were sent: %d\n", sentCountSigUsr2);
        printf("Number Received \n");
        printf("Total SIGUSR1 received : %d\n", receivedCountSigUsr1);
        printf("Total SIGUSR2 received : %d\n", receivedCountSigUsr2);
        printf("Averages \n");
        printf("The SIGUSR1's average time was: %f ms\n", SIGUSR1AverageTime);
        printf("The SIGUSR2's average time was: %f ms\n", SIGUSR2AverageTime);
        printf("+++++++++++++++++++++++++++++++++++++++++++++++\n");
    }
}

int main(int argc, char *argv[]) {

    //Initialize some ints for suture use.
    int SIG_USR_1 = 1, i = 0, randNum = 0, mainSignal;
    //Creating a pool of children processes and a main process (parent)
    pid_t childPids[TOTALAMOUNTOFPROCESSES], processIDs = 0;
    //signal set and masked signal
    sigset_t signalSet, maskedSignals;
    //Initialize the object using my initialization function
    sharedMemoryObject = initializeSMO();
    //built-in timeb struct and ftime() to retunr the time
    struct timeb ctMain;
    ftime(&ctMain);

    //grab both signals but ignore them in the sense that don't return anything or cause further action.
    //Then, grab the signal's time
    signal(SIGUSR1, SIG_IGN);
    finalSIGUSR1= (ctMain.time * 1000) + ctMain.millitm;

    signal(SIGUSR2, SIG_IGN);
    finalSIGUSR2= (ctMain.time * 1000) + ctMain.millitm;

    //for loop that iterates through every single process (8)
    for(i = 0; i < TOTALAMOUNTOFPROCESSES; i++){
        //Parent process that spawns everything else
        processIDs = fork();
        //Child process
        if(processIDs == 0) {
            //Load given data into the object buffer
            sharedMemoryObject = receiveSMO();
            if (i == 0) {
                //This is the process that does the reporting. It'll basically to the function before main.
                //And it will print the respective info for each SIGUSR
                signal(SIGUSR1, &handledSignals);
                signal(SIGUSR2, &handledSignals);

                //the following 3 while loops do the same thing except it depends on which process is doing it (hence the value of i):
                //1. Set all signals to full
                //2. Switch out the flag to signal mask that I created
                //3. Add signal num to the signal
                //4. handle the main signal based on the fact that you're not waiting for the signalSet and mainSignal
                while(true) {
                    sigfillset(&maskedSignals);
                    sigprocmask(SIG_SETMASK, &maskedSignals, NULL);
                    sigemptyset(&signalSet);
                    //Master child, account for everything
                    sigaddset(&signalSet, SIGUSR2);
                    sigaddset(&signalSet, SIGUSR1);
                    while (!sigwait(&signalSet, &mainSignal)) {
                        handledSignals(mainSignal);
                    }
                }
            }
            //First 2 signal handling processes
            else if ( i == 1 || i == 2 ) {
                //Signal handling
                signal(SIGUSR2, SIG_IGN);
                signal(SIGUSR1, &handleSIGUSR1);
                while(true) {
                    sigfillset(&maskedSignals);
                    sigprocmask(SIG_SETMASK, &maskedSignals, NULL);
                    sigemptyset(&signalSet);
                    //First 2 signal handling processes handle SIGUSR2
                    sigaddset(&signalSet, SIGUSR2);
                    while (!sigwait(&signalSet, &mainSignal)) {
                        handleSIGUSR2(mainSignal);
                    }
                }
            }
            //Second 2 signal handling processes
            else if ( i == 3 || i == 4 ) {
                //Signal handling
                signal(SIGUSR1, SIG_IGN);
                signal(SIGUSR2, &handleSIGUSR2);
                while(true) {
                    sigfillset(&maskedSignals);
                    sigprocmask(SIG_SETMASK, &maskedSignals, NULL);
                    sigemptyset(&signalSet);
                    //Second 2 signal handling processes handle SIGUSR1
                    sigaddset(&signalSet, SIGUSR1);
                    while (!sigwait(&signalSet, &mainSignal)) {
                        handleSIGUSR1(mainSignal);
                    }
                }
            }
            //The rest of the processes (i's 5,6,7) generate the processes
            else {
                srand(time(NULL));

                //Here I signal the process that will generate everything.
                while(true){
                    //produce a random number then make the system sleep
                    randNum = (rand() % 10) + 1;
                    usleep(randNum * 1000);
                    //signal both users
                    signal(SIGUSR1, SIG_IGN);
                    signal(SIGUSR2, SIG_IGN);

                    //Depending on what SIGUSR is returned, kill it, add to the number of signal that were sent out to the
                    //respective signal, and reset.
                    if(SIG_USR_1) {
                        kill(0, SIGUSR1);
                        incrementSentSigCount(SIGUSR1);
                        SIG_USR_1 = 0;
                    } else {
                        kill(0, SIGUSR2);
                        SIG_USR_1 = 1;
                        incrementSentSigCount(SIGUSR2);
                    }
                    //Update value for counting how many signals were sent out while protecting it with a lock
                    pthread_mutex_lock(&(*sharedMemoryObject).counterMutexLock);
                    (*sharedMemoryObject).counterForSignals++;

                    //Once the object is done what it needs to do, use the SIGTERM flag to terminate and signal finishing.
                    //It can be blocked, handles, or ignored as needed.
                    if((*sharedMemoryObject).complete){
                        kill(0, SIGTERM);
                    }
                    pthread_mutex_unlock(&(*sharedMemoryObject).counterMutexLock);
                }//end of while loop for parent
            }//end of else statement/parent
            break;
        }//end of child process
    }//end of for loop

    //continuing the parent process
    if(processIDs != 0){

        //Make the parent sleep for a little bit
        sleep(10);
        //Safely update the status of complete using locking
        pthread_mutex_lock(&(*sharedMemoryObject).counterMutexLock);
        (*sharedMemoryObject).complete = 1;
        pthread_mutex_unlock(&(*sharedMemoryObject).counterMutexLock);

        //Here, I use a for loop in order to make the parent wait for all 10 of the child processes
        for(i = 0; i < TOTALAMOUNTOFPROCESSES; i++){
            wait(NULL);
        }
        printf("Parent process is now complete and will exit\n");
    }//end of parent process
    //Clean up all usages in the program using my clean function
    cleanSMO();
    exit(EXIT_SUCCESS);
}
