/*
TCSS422 - Operating Systems
Problem 4

Group Members:
Zira Cook
Shaun Coleman
*/

#ifndef PCB_H
#define PCB_H

#include <time.h>

// The total number of general registers (0-7)
#define TOTAL_REG 8
#define IO_TRAP_SIZE 4
#define SYNCRO_SIZE 4
#define NO_TRAP -1

// The PID used to mark the process as the idle process
#define IDLE_PROCESS_PID 0

// Ranges for random calculations
#define TERM_COUNT_RANGE 5 + 1
#define MAX_PC_RANGE (5000 + 1 - 2000) + 2000
#define TRAP_STEP_RANGE (300 + 1 - 100) + 100


typedef struct cpu_context {
    // CPU state for the LC-3 processor
    unsigned int pc;
    unsigned int ir;
    unsigned int psr;
    unsigned int r0;
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
} CPU_context_s; // _s means this is a structure definition
typedef CPU_context_s * CPU_context_p; // _p means that this is a pointer to a structure

enum state_type {NEW, READY, RUNNING, INTERRUPTED, WAITING, HALTED};

enum process_type { NO_TYPE, IO, COMP_INTENSIVE, CONPRO_PAIR, RESOURCE_PAIR };

typedef struct pcb {
    // Process control block
    unsigned int pid; // process identification
    enum state_type state; // process state (running, waiting, etc.)
	enum process_type type; // the type of the process used for experimentation
    unsigned int parent; // parent process pid
    unsigned char priority; // 0 is highest – 15 is lowest.
    unsigned char * mem; // start of process in memory
    unsigned int size; // number of bytes in process
    unsigned char channel_no; // which I/O device or service Q
    // if process is blocked, which queue it is in
    unsigned int privileged; // a flag used to mark a process as privileged
    unsigned int maxpc; // the maximum pc value for the process
    unsigned int terminate; // the number of times the maxpc must be reached to terminate the process
    unsigned int term_count; // the number of times the maxpc was reached
    
    time_t creation; // the time when the process was created
    time_t termination; // the time when the process was terminated

    int io_1_traps[IO_TRAP_SIZE]; // An array of PC values representing Trap calls for IO device 1
    int io_2_traps[IO_TRAP_SIZE]; // An array of PC values representing Trap calls for IO device 2

	int lock_1_pcs[SYNCRO_SIZE]; // An array of PC values representing lock calls for resource 1
	int lock_2_pcs[SYNCRO_SIZE]; // An array of PC values representing lock calls for  resource 2
	int unlock_1_pcs[SYNCRO_SIZE]; // An array of PC values representing unlock calls resource 1
	int unlock_2_pcs[SYNCRO_SIZE]; // An array of PC values representing unlock calls resource 2
	int trylock_1_pcs[SYNCRO_SIZE]; // An array of PC values representing trylock calls resource 1
	int trylock_2_pcs[SYNCRO_SIZE]; // An array of PC values representing trylock calls resource 2
	int wait_1_pcs[SYNCRO_SIZE]; // An array of PC values representing wait calls resource 1
	int signal_1_pcs[SYNCRO_SIZE]; // An array of PC values representing signal calls

    CPU_context_p context; // set of cpu registers
    // other items to be added as needed.
} PCB_s;
typedef PCB_s * PCB_p;

PCB_p pcbConstruct(void); // Constructor for PCB object.

int pcbDestruct(PCB_p); // PCB destructor.

int initialize_pcb(PCB_p); // initialize all values.

unsigned int getPC(PCB_p); // getter for PC.

int setPC(PCB_p, unsigned int); // setter for PC.

unsigned int getIR(PCB_p); // getter for IR.

int setIR(PCB_p, unsigned int); // setter for IR.

unsigned int getPSR(PCB_p); // getter for PSR.

int setPSR(PCB_p, unsigned int); // setter for PSR.

unsigned int getRegister(PCB_p, unsigned int); // getter for registers.

int setRegister(PCB_p, int, unsigned int); // setter for registers.

unsigned int getPid(PCB_p); // getter for pid.

enum state_type getState(PCB_p); // getter for state.

int setState(PCB_p, enum state_type); // setter for state.

unsigned int getParent(PCB_p); // getter for parent.

int setParent(PCB_p, unsigned int); // setter for parent.

unsigned char getPriority(PCB_p); // getter for priority.

int setPriority(PCB_p, unsigned char); // setter for priority.

int setRandomPriority(PCB_p); // set random priority.

int setMem(PCB_p, unsigned int); // setter for Mem.

unsigned char getMem(PCB_p); // getter for Mem.

int setSize(PCB_p, unsigned int); // setter for Size.

unsigned char getSize(PCB_p); // getter for size.

unsigned char getChannelNo(PCB_p); // getter for channel number.

int setChannelNo(PCB_p, unsigned char); // setter for channel number.

int getPrivileged(PCB_p);// getter for the privileged flag

int setPrivileged(PCB_p, unsigned int);// setter for the privileged flag

int pcb_toString(PCB_p, char*, int); // toString for PCB.

int pcb_toSimpleString(PCB_p, char*, int); // toString for PCB to print a smaller simplified state

//// ADDED FOR PROBLEM 4

int initializeAsIdleProcess(PCB_p); // initializer to set the process as the idle process

int isIdleProcess(PCB_p); // checks if the process is the idle process

int setRandomTerminate(PCB_p); // sets a random value for the processes terminate

int setTerminate(PCB_p, int); // a setter for terminate

unsigned int getTerminate(PCB_p); // getter for terminate

int setRandomMaxPC(PCB_p); // sets a random maxpc

unsigned int getMaxPC(PCB_p); // gets the processes max pc

int setTermCount(PCB_p, unsigned int); // setter for the term count

unsigned int getTermCount(PCB_p); // getter for the term count

int setRandomIOTraps(PCB_p); // sets random values for both sets of IO traps

int setCreation(PCB_p, time_t); // sets the creation time of the process

time_t getCreation(PCB_p); // getter for the creation time of the process

int setTermination(PCB_p, time_t); // sets the termination time of the process

time_t getTermination(PCB_p); // getter for the termination time of the process

int* getIOTraps1(PCB_p); // getter for the IO1 Traps array 

int* getIOTraps2(PCB_p); // getter for the IO2 Traps array

////

int context_toString(PCB_p, char*, int); // toString for context.

#endif /* PCB_H */


