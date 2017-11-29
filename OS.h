/*
TCSS422 - Operating Systems
Final Project

Group Members:
Shaun Coleman
Joshua Meigs
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "FIFOq.h"
#include "pcb.h"
#include "PQueue.h"

// value to denote no interupt or trap detected
#define  0

// value to denote a successful function return
#define SUCCESSFUL 0

// value to denote a successful ISR return
#define ISR_RETURNED 1

// Maximum size used for an ouput buffer string
#define MAX_BUFFER_SIZE 2048

// The amount of loop iterations before halting the simulation
// Set low for output txt, tested at 500,000 iterations and no halting condition
#define HALT_CONDITION 50000

// The amount of loop iterations before creating new processes
#define NEW_PROCESS_ITERATION 4000

// The amount of time before all processes are reset to priority 0
// 4 times the quantum size of the middle priority
// Note: Originally was using a higher multiplier but was reduced
// to demonstrate the priority reset for the test output
#define RESET_QUANTUM (((((MAX_PRIORITY/2)+1) * (MAX_PRIORITY/2)+1) * 10) * 15)

// The initial number of create process calls to start the program with
#define INIT_CREATE_CALLS 5

// the max number of processes that will not terminate
#define MAX_PRIVILEGED 4

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
#define CREATE_SHARED_RESOURCE_MAX 2

// The max number +1 of new IO Processes to make per process creation
#define CREATE_IO_PROCESS_MAX 6

// The max number +1 of new computation intensive processes to make per process creation
#define CREATE_CUMPUTE_PROCESS_MAX 3

// A Constant used to test the timer's frequency
#define TIMER_FREQ 1000

// A Constant used to test the io device's interrupt frequency
#define IO_FREQ 10000

// An enum used to denote which interrupt is occuring for the scheduler
enum interrupt_type { NO_INTERUPT, TIMER_INTERUPT, IO_1_INTERUPT, IO_2_INTERUPT, IO_1_TRAP
    , IO_2_TRAP, PCB_TERMINATED, LOCK_INTERRUPT, UNLOCK_INTERRUPT, WAIT_INTERRUPT, SIGNAL_INTERRUPT
};

// An enum used to denote which syncronization services was requested
enum syncro_code { NO_RESOURCE_SYNCRO, LOCK_RESOURCE_1, UNLOCK_RESOURCE_1, LOCK_RESOURCE_2, UNLOCK_RESOURCE_2
                    , SIGNAL_RESOURCE_1, WAIT_RESOURCE_1};

enum code_wait_code {COND_NOT_READY, COND_READY};

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

typedef struct custom_mutex {
	// NULL if no process holds the mutex, otherwise the pointer to the process
	PCB_p owner;
	// A FIFO_q of processes blocked waiting for the mutex
	FIFOq_p blocked;
} CUSTOM_MUTEX_s;

typedef CUSTOM_MUTEX_s* CUSTOM_MUTEX_p;

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

	// Shared counter to increment/read
	int counter;

	// Syncronization vars
	CUSTOM_MUTEX_p mutex;
	CUSTOM_COND_p produced;
	CUSTOM_COND_p consumed;
} CP_PAIR_s;

typedef CP_PAIR_s* CP_PAIR_p;

typedef struct resource_pair {
	// pointers to the processes in the pair
	PCB_p process_1;
	PCB_p process_2;

	// Syncronization vars
	CUSTOM_MUTEX_p mutex_1;
	CUSTOM_MUTEX_p mutex_2;
} RESOURCE_PAIR_s;

typedef RESOURCE_PAIR_s* RESOURCE_PAIR_p;

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

void* timer_thread();

void* io1_thread();

void* io2_thread();

int createConsumerProducerPair();

void initialize_CP_Pair(CP_PAIR_p pair);

void initialize_Custom_Mutex(CUSTOM_MUTEX_p);

void initialize_Custom_Cond(CUSTOM_COND_p);

int is_mutex_free(CUSTOM_MUTEX_p);

CP_PAIR_p getPCPair(PCB_p);

CP_PAIR_p getPCPair(PCB_p);

int simulate_mutex_lock(PCB_p, CUSTOM_MUTEX_p);

int simulate_mutex_unlock(CUSTOM_MUTEX_p);

int simulate_cond_wait(PCB_p, CUSTOM_COND_p);

int simulate_cond_signal(PCB_p, CUSTOM_COND_p);

int createIOProcess();

int createComputeIntensiveProcess();

int createSharedResourcePair();

void initialize_Resource_Pair(RESOURCE_PAIR_p pair);

void initialize_CP_Pair(CP_PAIR_p);

void timer_check();

void IO_check();
