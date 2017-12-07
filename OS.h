/*
TCSS422 - Operating Systems
Final Project

Group Members:
Shaun Coleman
Joshua Meigs
Ayub Tiba
Kirtwinder Gulati
*/



#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include "FIFOq.h"
#include "pcb.h"
#include "PQueue.h"
#include "Simple_Stack.h"
#include "Deadlock_Monitor.h"

// value to determine if deadlock will be possible (0 for not possible otherwise possible)
#define DEADLOCK 1

// A Constant used to multiply the quantum for timer "sleep" loop
#define IO_QUANTUM_MULTIPLIER 2000

// The amount of loop iterations before creating new processes
#define NEW_PROCESS_ITERATION 9000

// The initial number of create process calls to start the program with
#define INIT_CREATE_CALLS 3
// value to denote a successful function return
#define SUCCESSFUL 0

// value to denote a successful ISR return
#define ISR_RETURNED 1

// Maximum size used for an ouput buffer string
#define MAX_BUFFER_SIZE 2048

// The amount of loop iterations before halting the simulation
#define HALT_CONDITION 100000

// The amount of time before all processes are reset to priority 0
#define RESET_QUANTUM (((((MAX_PRIORITY/2)+1) * (MAX_PRIORITY/2)+1) * 10) * 65)

// the number of zombie processes that triggers an empty zombies function
#define MAX_ZOMBIES 4

// range used to for the multiplier to decided the counter for IO interrupts (3-5)
#define IO_COUNTER_MULT_RANGE 3 + 3

// The maximum number of producer/consumer pairs
#define PRO_CON_MAX 10

// The maximum number of shared resource pairs
#define SHARED_RESOURCE_MAX 10

// The maximum number of IO process
#define IO_PROCESS_MAX 50

// The maximum number of computation intensive processes
#define COMPUTE_PROCESS_MAX 25

// The max number +1 of new producer/consumer pairs to make per process creation
#define CREATE_PRO_CON_MAX 2

// The max number +1 of new shared resource pairs to make per process creation
#define CREATE_SHARED_RESOURCE_MAX 5

// The max number +1 of new IO Processes to make per process creation
#define CREATE_IO_PROCESS_MAX 8

// The max number +1 of new computation intensive processes to make per process creation
#define CREATE_CUMPUTE_PROCESS_MAX 4

// The maximum number of characters used to name a consumer or producer process
#define MAX_NAME_SIZE_CONPRO 3

#define UPPERCASE_A 64

#define UPPERCASE_Z 89

#define LOWERCASE_A 96

// An enum used to denote which interrupt is occuring for the scheduler
enum interrupt_type { NO_INTERUPT, TIMER_INTERUPT, IO_1_INTERUPT, IO_2_INTERUPT, IO_1_TRAP
    , IO_2_TRAP, PCB_TERMINATED, LOCK_INTERRUPT, UNLOCK_INTERRUPT, WAIT_INTERRUPT, SIGNAL_INTERRUPT
};

// An enum used to denote which syncronization services was requested
enum syncro_code { NO_RESOURCE_SYNCRO, LOCK_RESOURCE_1, UNLOCK_RESOURCE_1, LOCK_RESOURCE_2, UNLOCK_RESOURCE_2
                    , SIGNAL_RESOURCE_1, WAIT_RESOURCE_1};

enum code_wait_code {EMPTY, FILLED};

typedef struct process_queues {
    // all currently used process queues and the running process pcb
    FIFOq_p newProcesses;
    FIFOq_p zombieProcesses;
    FIFOq_p IO_1_Processes;
    FIFOq_p IO_2_Processes;
    PQueue_p readyProcesses;
    PCB_p runningProcess;
} PROCESS_QUEUES_s;

typedef PROCESS_QUEUES_s* PROCESS_QUEUES_p;

typedef struct timer_device {
    pthread_mutex_t timer_lock;
    pthread_cond_t timer_cond;
} TIMER_DEVICE_s;

typedef TIMER_DEVICE_s* TIMER_DEVICE_p;

typedef struct io_device {
    int IO_activated;
    pthread_mutex_t IO_lock;
    pthread_mutex_t IO_reset_lock;
    pthread_cond_t IO_cond;
    pthread_cond_t IO_active_cond;
    pthread_mutex_t IO_active_lock;
} IO_DEVICE_s;

typedef IO_DEVICE_s* IO_DEVICE_p;

typedef struct custom_cond {
	// int representing the state
	int state;
	// A FIFO_q of processes waiting for a state change
	FIFOq_p waiting;
} CUSTOM_COND_s;

typedef CUSTOM_COND_s* CUSTOM_COND_p;

typedef struct cp_pair {
	// pointers to the processes in the pair
	PCB_p consumer;
	PCB_p producer;
	
	char consumer_name[MAX_NAME_SIZE_CONPRO];
	char producer_name[MAX_NAME_SIZE_CONPRO];

	// Shared counter to increment/read
	int counter;

    // Shared flag to simulate while loop for wait
    int filled;

	// Syncronization vars
	CUSTOM_MUTEX_p mutex;
	CUSTOM_COND_p produced;
	CUSTOM_COND_p consumed;
} CP_PAIR_s;

typedef CP_PAIR_s* CP_PAIR_p;

// A function to act as the main loop for the simulator
//void OS_Simulator();

// A function to simulate an ISR
int pseudoISR();

// A function to simulate an OS scheduler
int scheduler(int, PCB_p, CUSTOM_MUTEX_p, CUSTOM_COND_p);

// A function to simulate a dispatcher for timer interrupts
int dispatcher();

// A function to simulate a an IO Interrupt routine
int IO_Interupt_Routine(int);

// A function to simulate a TSR
int pseudoTSR(int);

// A function to simulate the dispatcher for an IO Interrupt
int dispatcherIO(FIFOq_p);

// A function to simulate the dispatcher for an Trap Interrupt
int dispatcherTrap(FIFOq_p, PCB_p);

// A function to check if the specified process is at an IO trap or is ready to terminate
int isAtTrap(PCB_p);

// A function to create new processes and place them in the new queue
int createNewProcesses();

// A function to print an enqueued PCB
void printEnqueuedPCB(PCB_p);

// A function to empty the zombie queue by freeing all resources from processes in the queue
int emptyZombies(FIFOq_p);

// ADDED
// A function to trigger the timer down counter and return if the timer interrupt occured
int timerDownCounter();

// A function to trigger the io1 down counter and return if the io1 interrupt occured
int IO_1_DownCounter();

// A function to trigger the io2 down counter and return if the io2 interrupt occured
int IO_2_DownCounter();

// A function to simulate the program executing one instruction and stepping to the next
int simulateProgramStep();

// A function to move all new processes to the ready queue
int moveNewToReady();

// A function used to print the current trace when an interrupt occurs
int printInterupt(int);

// A function used to initialize the processes struct
void initializeProcessQueues();

// A function used to free the processes struct
void freeProcessQueues();

// A thread used to simulate the timer device
void* timer_thread();

// A thread used to simulate an IO device
void* io1_thread();

// A thread used to simulate an IO device
void* io2_thread();

// A function used to create a producer/consumer process pair
int createConsumerProducerPair();

// A function to initialize a consumer/producer pair struct
void initialize_CP_Pair(CP_PAIR_p pair);

// A function to initialize a Custum Mutex struct
void initialize_Custom_Mutex(CUSTOM_MUTEX_p);

// A function to initalize a Custom Condition struct
void initialize_Custom_Cond(CUSTOM_COND_p);

// A function to determin if a mutex is free
// Returns 1 if free, and 0 if the process is owned
int is_mutex_free(CUSTOM_MUTEX_p);

// Returns a PC pair that the passed PCB_p belongs to
CP_PAIR_p getPCPair(PCB_p);

// A function used to create an IO process
int createIOProcess();

// A function used to create a computationally intensive process
int createComputeIntensiveProcess();

// A function used to create a shared resource pair of processes
int createSharedResourcePair();

// A function used to initialize a shared resource process struct
void initialize_Resource_Pair(RESOURCE_PAIR_p pair);

// A function used to initialize a consumer/producer process struct
void initialize_CP_Pair(CP_PAIR_p);

// A function used to check for timer interrupts
int timer_check();

// A function used to check for IO interrupts
int IO_check();

// A function used to simulate a lock trap service routine
int lock_tsr(CUSTOM_MUTEX_p);

// A function used to simulate an unlock trap service routine
int unlock_tsr(CUSTOM_MUTEX_p mutex);

// A function used to simulate a wait trap service routine
int wait_tsr(CUSTOM_MUTEX_p mutex, CUSTOM_COND_p cond);

// A function used to simulate a signal trap service routine
int signal_tsr(CUSTOM_MUTEX_p mutex, CUSTOM_COND_p cond);

// A function used to simulate a lock dispatcher
int dispatcherLock(PCB_p process, CUSTOM_MUTEX_p mutex);

// A function used to simulate an unlock dispatcher
int dispatcherUnlock(CUSTOM_MUTEX_p mutex);

// A function used to simulate a wait dispatcher
int dispatcherWait(PCB_p process, CUSTOM_MUTEX_p mutex, CUSTOM_COND_p cond);

// A function used to simulate a signal dispatcher
int dispatcherSignal(CUSTOM_MUTEX_p mutex, CUSTOM_COND_p cond);

// A function used to check if a process is requesting a syncronization service
int isAtSyncro(PCB_p pcb);

// A function used to get a resource pair struct that the passed PCB_p belongs too
RESOURCE_PAIR_p getResourcePair(PCB_p process);

// A function used to free resources used for a shared resource pair struct
void destruct_Resource_Pair(RESOURCE_PAIR_p the_pair);

// A function used to free resources used for a consumer/producer pair struct
void destruct_CP_Pair(CP_PAIR_p pair);

// A function used to free resources used for a custom mutex struct
void destruct_Custom_Mutex(CUSTOM_MUTEX_p mutex);

// A function used to free resources used for a custom cond struct
void destruct_Custom_Cond(CUSTOM_COND_p cond);

// A function used to call the deadlock monitor and evaluate the results
void check_for_deadlock();

// A function used to return the type of interrupt currently being processed
int getInterruptType(int);

// A function used to count all processes in all queues and running
int countAllNodes();

// A function used to evaluate the current syncronization request based on the passed syncronization type
void check_for_syncro_trap(int syncro_flag);

// A function used to check if the current process is a producer in a CP Pair
int isProducer(PCB_p process);
