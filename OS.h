/*
TCSS422 - Operating Systems
Problem 4

Group Members:
Zira Cook
Shaun Coleman
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "FIFOq.h"
#include "pcb.h"
#include "PQueue.h"

// value to denote current interupt is a timer interupt
#define TIMER_INTERUPT 1

// value to denote current interupt is a IO 1 interupt
#define IO_1_INTERUPT 2

// value to denote current interupt is a IO 2 interupt
#define IO_2_INTERUPT 3

// value to denote a TRAP call to IO 1
#define IO_1_TRAP 4

// value to denote a TRAP call to IO 2
#define IO_2_TRAP 5

// value to denote a program was halted
#define PCB_TERMINATED 6

// value to denote no interupt or trap detected
#define NO_INTERUPT 0

// value to denote a successful function return
#define SUCCESSFUL 0

// Maximum size used for an ouput buffer string
#define MAX_BUFFER_SIZE 1024

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
#define INIT_CREATE_CALLS 20

// the max number of processes that will not terminate
#define MAX_PRIVILEGED 4

// the number of zombie processes that triggers an empty zombies function
#define MAX_ZOMBIES 4

// range used to for the multiplier to decided the counter for IO interrupts (3-5)
#define IO_COUNTER_MULT_RANGE 3 + 3

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

typedef struct custom_mutex {
	// NULL if no process holds the mutex, otherwise the pointer to the process
	PID_p owner;
	// A FIFO_q of processes blocked waiting for the mutex
	FIFOq_p blocked;
} CUSTOM_MUTEX_p;

typedef struct custom_cond {
	// int representing the state
	int state;
	// A FIFO_q of processes waiting for a state change
	FIFOq_p waiting;
} CUSTOM_COND_p;

typedef struct cp_pair {
	// PID of processes in the pair
	int consumer_pid;
	int producer_pid;

	// Shared counter to increment/read
	int counter;

	// Syncronization vars
	CUSTOM_MUTEX_p mutex;
	CUSTOM_COND_p produced;
	CUSTOM_COND_p consumed;
} CP_PAIR_p;

typedef struct resource_pair {
	// PID of processes in the pair
	int process1_pid;
	int process2_pid;

	// Syncronization vars
	CUSTOM_MUTEX_p mutex_1;
	CUSTOM_MUTEX_p mutex_2;
} RESOURCE_PAIR_p;

// A function to act as the main loop for the simulator
//void OS_Simulator();

// A function to simulate an ISR
int pseudoISR();

// A function to simulate an OS scheduler
int scheduler(int);

// A function to simulate a dispatcher for timer interrupts
int dispatcher();

// A function to simulate a an IO Interrupt routine
int IO_Interupt_Routine(int);

// A function to simulate a TSR
int pseudoTSR(int);

// A function to simulate the dispatcher for an IO Interrupt
int dispatcherIO(FIFOq_p);

// A function to simulate the dispatcher for an Trap Interrupt
int dispatcherTrap(FIFOq_p);

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
