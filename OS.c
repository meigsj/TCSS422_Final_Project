/*
TCSS422 - Operating Systems
Final Project
Group Members:
Kirtwinder Gulati
Shaun Coleman
Ayub Tiba
Joshua Meigs
*/

#include <time.h>
#include <pthread.h>

#include <assert.h> //Could be useful

#include "OS.h"
#include <assert.h>
unsigned int sysStack;
unsigned int currentPC;
unsigned int iterationCount;
unsigned int quantum_post_reset;

// Too Remove
PCB_p privilegedPCBs[MAX_PRIVILEGED];
unsigned int numPrivileged;

int scheduler_flag;
int timer;
int shutting_down;
int ISR_FINISHED;
int IOSR_1_finished;
int IOSR_2_finished;
int ISR_FINISHED;
int IO_1_counter;
int IO_2_counter;

int IO_1_activated;
int IO_2_activated;




//Tests
pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timer_cond = PTHREAD_COND_INITIALIZER;
//<<<<<<< HEAD
//...
//pthread_mutex_t io1_lock = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t io1_cond = PTHREAD_COND_INITIALIZER; 

//<<<<<<< HEAD
pthread_mutex_t io2_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t io2_cond = PTHREAD_COND_INITIALIZER; 
//=======
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

//>>>>>>> 12e1e4b6bf7127f66dd71ac490b3591a002cc4ba
//=======

pthread_mutex_t io1_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t io1_cond = PTHREAD_COND_INITIALIZER; 

pthread_mutex_t global_shutdown_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t IO_1_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t IO_1_reset_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t IO_1_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t IO_1_active_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t IO_1_active_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t IO_2_reset_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t IO_2_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t IO_2_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t IO_2_active_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t IO_2_active_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t IO_1_global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t IO_2_global_lock = PTHREAD_MUTEX_INITIALIZER;
//>>>>>>> kirt

PROCESS_QUEUES_p processes;

// Updated for Problem 4
// The top level of the OS simulator
void * OS_Simulator(void *arg) {
	char* buffer[MAX_BUFFER_SIZE];
	pthread_t the_timer_thread;
//<<<<<<< HEAD
//<<<<<<< HEAD
	pthread_t the_io1_thread;
	pthread_t the_io2_thread;
    // Main Loop
    // One cycle is one instruction
	//pthread_create(&the_timer_thread, NULL, timer_thread, NULL);
	
	//Creatting the I/O threads
	//pthread_create(&the_io1_thread, NULL,  io1_thread, NULL);
	//pthread_create(&the_io2_thread, NULL,  io2_thread, NULL);
	
//=======
	
    // Main Loop
    // One cycle is one instruction
//>>>>>>> 12e1e4b6bf7127f66dd71ac490b3591a002cc4ba
    for( ; ; ) { // for spider
        int trapFlag = 0;
        // update counters
        iterationCount++;
        quantum_post_reset++;
        
        // increments Current PC and checks MAX_PC of the running PCB
        simulateProgramStep();
        
        // Create new processes
        if ((iterationCount % NEW_PROCESS_ITERATION) == 0) {
            createNewProcesses(processes->newProcesses);
			
        }
		//scheduler_flag == 1
		if (iterationCount == 5) {//WILL NEED TO BE FIXED AS SO TO HAVE SCHEDULAR RUN AT LEAST ONCE BEFORE STARTING
			pthread_create(&the_timer_thread, NULL, timer_thread, NULL);
			//scheduler_flag = 2;
//=======
	pthread_create(&the_timer_thread, NULL, timer_thread, NULL);
	pthread_t the_io_1_thread;
	pthread_create(&the_io_1_thread, NULL, io1_thread, NULL);

	pthread_t the_io_2_thread;
	pthread_create(&the_io_2_thread, NULL, io2_thread, NULL);

	// Main Loop
	// One cycle is one instruction
	for (; ; ) { // for spider
		int trapFlag = 0;
		// update counters
		iterationCount++;
		quantum_post_reset++;

		// increments Current PC and checks MAX_PC of the running PCB
		simulateProgramStep();

		// Create new processes
		if ((iterationCount % NEW_PROCESS_ITERATION) == 0) {
			createNewProcesses(processes->newProcesses);
//>>>>>>> kirt
		}
		//In the CPU loop use the non-blocking mutex_trylock() call so that the loop doesn't block itself waiting for the timer signal
		//// TO REMOVE AND REPLACE WITH CHECK CONDITION FOR TIMER INTERUPT
		// Trigger timer and check for timer interupt
		if (pthread_mutex_trylock(&timer_lock) == 0) { //WAS: timerDownCounter() == TIMER_INTERUPT
			int state = RUNNING;
			if (processes->runningProcess) state = getState(processes->runningProcess);
			// Timer interupt
			sysStack = currentPC;

			pseudoISR();
			ISR_FINISHED = 1;
			pthread_cond_signal(&timer_cond);

			if (state == HALTED) {
				printInterupt(PCB_TERMINATED);
			}
			else {
				printInterupt(TIMER_INTERUPT);
			}
			pthread_mutex_unlock(&timer_lock);
//<<<<<<< HEAD
        }
		/////
        
        // Trigger IO1 counter and check for IO1 interupt
        if(/*IO_1_DownCounter() == IO_1_INTERUPT*/pthread_mutex_trylock(&io1_lock) == 0 /*&& !q_is_empty(processes->IO_1_Processes)//Not sure if this is needed*/) {
            sysStack = currentPC;
            IO_Interupt_Routine(IO_1_INTERUPT);
//<<<<<<< HEAD
            printInterupt(IO_1_INTERUPT);
			
			pthread_mutex_unlock(&io1_lock);
//=======
            //printInterupt(IO_1_INTERUPT);
//>>>>>>> 12e1e4b6bf7127f66dd71ac490b3591a002cc4ba
        }
        
        // Trigger IO2 counter and check for IO1 interupt
        if(/*IO_2_DownCounter() == IO_2_INTERUPT*/pthread_mutex_trylock(&io2_lock) == 0 /*&& !q_is_empty(processes->IO_2_Processes)//Not sure if this is needed*/) {
            sysStack = currentPC;
            IO_Interupt_Routine(IO_2_INTERUPT);
//<<<<<<< HEAD
            printInterupt(IO_2_INTERUPT);
			
			pthread_mutex_unlock(&io2_lock);
//=======
            //printInterupt(IO_2_INTERUPT);
//>>>>>>> 12e1e4b6bf7127f66dd71ac490b3591a002cc4ba
        }
        
        // Check for Traps (termination is checked as a trap here too)
        trapFlag = isAtTrap(processes->runningProcess);
        if(trapFlag == IO_1_TRAP || trapFlag == IO_2_TRAP || trapFlag == PCB_TERMINATED) {
            sysStack = currentPC;
            pseudoTSR(trapFlag);
            //printInterupt(trapFlag);
        }

        
        //check stop condition for the simulation
        if (iterationCount >= HALT_CONDITION) {
            printf("---- HALTING SIMULATION ON ITERATION %d ----\n", iterationCount);
//<<<<<<< HEAD
			timer = -1;
			pthread_join(the_timer_thread, NULL);
			pthread_join(the_io1_thread, NULL);
			pthread_join(the_io2_thread, NULL);
            break;
//=======
			pthread_mutex_lock(&global_lock);
//=======
		}

		// Trigger IO1 counter and check for IO1 interupt
		if (IO_1_activated && pthread_mutex_trylock(&IO_1_lock) == 0) {
			sysStack = currentPC;
			IO_Interupt_Routine(IO_1_INTERUPT);
			pthread_mutex_lock(&IO_1_global_lock);
			if (q_is_empty(processes->IO_1_Processes)) {
				IO_1_activated = 0;
			}
			pthread_mutex_unlock(&IO_1_global_lock);
			IOSR_1_finished = 1;
			pthread_cond_signal(&IO_1_active_cond);
			printInterupt(IO_1_INTERUPT);
			pthread_mutex_unlock(&IO_1_lock);

		}

		// Trigger IO2 counter and check for IO1 interupt
		if (IO_2_activated && pthread_mutex_trylock(&IO_2_lock) == 0) {
			sysStack = currentPC;
			IO_Interupt_Routine(IO_2_INTERUPT);
			pthread_mutex_lock(&IO_2_global_lock);
			if (q_is_empty(processes->IO_2_Processes)) {
				IO_2_activated = 0;
			}
			pthread_mutex_unlock(&IO_2_global_lock);
			IOSR_2_finished = 1;
			pthread_cond_signal(&IO_2_active_cond);

			printInterupt(IO_2_INTERUPT);
			pthread_mutex_unlock(&IO_2_lock);

		}

		// Check for Traps (termination is checked as a trap here too)
		trapFlag = isAtTrap(processes->runningProcess);
		if (trapFlag == IO_1_TRAP || trapFlag == IO_2_TRAP || trapFlag == PCB_TERMINATED) {
			sysStack = currentPC;
			pseudoTSR(trapFlag);
			printInterupt(trapFlag);
		}


		//check stop condition for the simulation
		if (iterationCount >= HALT_CONDITION) {
			printf("---- HALTING SIMULATION ON ITERATION %d ----\n", iterationCount);
			pthread_mutex_lock(&global_shutdown_lock);
//>>>>>>> kirt
			shutting_down = 1;
			pthread_mutex_unlock(&global_shutdown_lock);
			break;
//<<<<<<< HEAD
//>>>>>>> 12e1e4b6bf7127f66dd71ac490b3591a002cc4ba
        }
    }

//=======
		}
	}
//>>>>>>> kirt
}

// reset downcounter by quantum in seperate function

// To Complete
/*
The timer is an independent thread that puts itself to sleep for some number of milliseconds (the standard sleep function in Linux is in seconds so use the nanosleep() function (time.h)
-you may need to experiment with how many the timer should sleep to approximate a single quantum). When it wakes up it will need to "signal" the CPU thread that an interrupt has
occurred through the use of a mutex. In the CPU loop use the non-blocking mutex_trylock() call so that the loop doesn't block itself waiting for the timer signal.
After throwing  the  interrupt  signal  it  puts  itself  to  sleep  again  for  the  designated  quantum.  The  timer  has  the
highest priority with respect to interrupt processing. It must be accommodated before any I/O interrupt. If an  I/O  interrupt  is  processing  when  a
timer interrupt occurs  you  should  call the timer  pseudo_ISR  from inside the I/O pseudo_ISR to simulate these priority relation
in os change timer to check for trylock
*/
void * timer_thread(void * s) {
	struct timespec ts;
	pthread_mutex_lock(&timer_lock);
	ts.tv_sec = 0;
	ts.tv_nsec = timer;//(timer*1000);


	for (;;) {
		pthread_mutex_lock(&global_shutdown_lock);
		while (shutting_down) {
			break;
		}
		nanosleep(&ts, NULL);
		pthread_mutex_unlock(&global_shutdown_lock);
		while (!ISR_FINISHED) {
			pthread_cond_wait(&timer_cond, &timer_lock);
		}
		ISR_FINISHED = 0;
		ts.tv_nsec = timer;

	}
}

void * io1_thread(void * s) {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 10000;//(timer*1000);
	// Lock signal mutex
	pthread_mutex_lock(&IO_1_lock);

	for (;;) {
//<<<<<<< HEAD
		pthread_mutex_lock(&io1_lock);
		//....
		nanosleep(&ts, NULL);
		//....
		pthread_mutex_unlock(&io1_lock);
		//....
		nanosleep(&ts, NULL);
		
		pthread_cond_wait(&io1_cond, &io1_lock);
		IO_1_DownCounter();
		if (IO_1_counter == 0) {
			pthread_cond_signal(&io1_cond);
		}	
		
		if (IO_1_counter == -1) {
//=======
		pthread_mutex_lock(&global_shutdown_lock);
		// Check if the program needs to shut down
		while (shutting_down) {
//>>>>>>> kirt
			break;
		}
		pthread_mutex_unlock(&global_shutdown_lock);

		pthread_mutex_lock(&IO_1_active_lock);
		//printf("Checking IO One Activated: %d\n", IO_1_activated);
		while (!IO_1_activated) {
			pthread_cond_wait(&IO_1_active_cond, &IO_1_active_lock);
		}
		//printf("Check IO One Completed: %d\n", IO_1_activated);
		pthread_mutex_unlock(&IO_1_active_lock);

		// Sleep thread then unlock to signal IO 1 device is ready
		nanosleep(&ts, NULL);
		pthread_mutex_unlock(&IO_1_lock);

		// Wait until Interupt Service completes
		//printf("Is IOSR 1 Finished?\n");
		pthread_mutex_lock(&IO_1_reset_lock);
		while (!IOSR_1_finished) {
			pthread_cond_wait(&IO_1_active_cond, &IO_1_reset_lock);
		}
		//printf("IOSR 1 Finished!!!!!\n");
		// Lock signal mutex
		pthread_mutex_lock(&IO_1_lock);
		IOSR_1_finished = 0;
		pthread_mutex_unlock(&IO_1_reset_lock);
		ts.tv_nsec = 10000;
	}
}

void * io2_thread(void * s) {

	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 10000;//(timer*1000);
	 // Lock signal mutex
	pthread_mutex_lock(&IO_2_lock);

	for (;;) {
//<<<<<<< HEAD
		pthread_mutex_lock(&io2_lock);
		//....
		nanosleep(&ts, NULL);
		//....
		pthread_mutex_unlock(&io2_lock);
		//....
		nanosleep(&ts, NULL);
		
		pthread_cond_wait(&io2_cond, &io2_lock);
		IO_2_DownCounter();
		if (IO_2_counter == 0) {
			pthread_cond_signal(&io2_cond);
		}	
		
		if (IO_2_counter == -1) {
//=======
		//// Lock signal mutex
		//pthread_mutex_lock(&IO_2_lock);

		pthread_mutex_lock(&global_shutdown_lock);

		// Check if the program needs to shut down
		while (shutting_down) {
//>>>>>>> kirt
			break;
		}
		pthread_mutex_unlock(&global_shutdown_lock);

		pthread_mutex_lock(&IO_2_active_lock);
		//printf("Checking IO Two Activated: %d\n", IO_2_activated);
		while (!IO_2_activated) {
			pthread_cond_wait(&IO_2_active_cond, &IO_2_active_lock);
		}
		//printf("Checking IO Two Completed: %d\n", IO_2_activated);
		pthread_mutex_unlock(&IO_2_active_lock);
		// Sleep thread then unlock to signal IO 2 device is ready
		nanosleep(&ts, NULL);
		pthread_mutex_unlock(&IO_2_lock);

		// Wait until Interupt Service completes
		pthread_mutex_lock(&IO_2_reset_lock);
		//printf("Is IOSR 2 Finished?\n");
		while (!IOSR_2_finished) {
			pthread_cond_wait(&IO_2_active_cond, &IO_2_reset_lock);
		}
		//printf("IOSR 2 Finished!!!!!\n");
		// Lock signal mutex
		pthread_mutex_lock(&IO_2_lock);
		IOSR_2_finished = 0;
		pthread_mutex_unlock(&IO_2_reset_lock);
		ts.tv_nsec = 10000;
	}
}

//////

// Function used to simulate an ISR call
int pseudoISR() {
	// set state to interupted if the process was not halted
	if (getState(processes->runningProcess) != HALTED) {
		setState(processes->runningProcess, INTERRUPTED);
	}

	// save pc to pcb
	setPC(processes->runningProcess, currentPC);

	// scheduler up call
	scheduler(TIMER_INTERUPT);

	// Update Timer
	timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
	// IRET (update current pc)
	currentPC = sysStack;
	return SUCCESSFUL;
}

// Added for problem 4
// A function to simulate a an IO Interrupt routine
int IO_Interupt_Routine(int IO_interupt) {
	setState(processes->runningProcess, INTERRUPTED);

	// save pc to pcb
	setPC(processes->runningProcess, currentPC);

	// scheduler up call
	scheduler(IO_interupt);

	// IRET (update current pc)
	currentPC = sysStack;
	return SUCCESSFUL;
}

// Added for problem 4
// A function to simulate a TSR
int pseudoTSR(int trap_interupt) {
	if (trap_interupt == PCB_TERMINATED) {
		setState(processes->runningProcess, HALTED);
		setTermination(processes->runningProcess, time(NULL));
	}
	else {
		setState(processes->runningProcess, WAITING);
	}

	// Activate counter if not in use (IO_Queue is empty)
	// IO Traps only
	if (trap_interupt == IO_1_TRAP && q_is_empty(processes->IO_1_Processes)) {
		IO_1_counter = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess)) * (rand() % IO_COUNTER_MULT_RANGE);
	}
	else if (trap_interupt == IO_2_TRAP && q_is_empty(processes->IO_2_Processes)) {
		IO_2_counter = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess)) * (rand() % IO_COUNTER_MULT_RANGE);
	}

	// save pc to pcb
	setPC(processes->runningProcess, currentPC);

	// scheduler up call
	scheduler(trap_interupt);

	// activate IO devices as needed
	pthread_mutex_lock(&IO_1_active_lock);
	if (q_is_empty(processes->IO_1_Processes)) {
		IO_1_activated = 0;
	}
	else {
		IO_1_activated = 1;
		pthread_cond_signal(&IO_1_active_cond);
	}
	pthread_mutex_unlock(&IO_1_active_lock);

	pthread_mutex_lock(&IO_2_active_lock);
	if (q_is_empty(processes->IO_2_Processes)) {
		IO_2_activated = 0;
	}
	else {
		IO_2_activated = 1;
		pthread_cond_signal(&IO_2_active_cond);
	}
	pthread_mutex_unlock(&IO_2_active_lock);

	// IRET (update current pc)
	currentPC = sysStack;
	return SUCCESSFUL;
}

// Updated for problem 4
// Function used to simulate the scheduler in an operating system
int scheduler(int interupt) {
	// move newly created processes to the ready queue
	moveNewToReady();

	switch (interupt) {
	case PCB_TERMINATED:
	case TIMER_INTERUPT:
		dispatcher();
		timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
		break;
	case IO_1_INTERUPT:
		dispatcherIO(processes->IO_1_Processes);
		if (!q_is_empty(processes->IO_1_Processes)) {
			PCB_p head = getNodePCB(getHead(processes->IO_1_Processes));
			IO_1_counter = (int)getQuantum(processes->readyProcesses, getPriority(head)) * (rand() % IO_COUNTER_MULT_RANGE);
		}
		break;
	case IO_2_INTERUPT:
		dispatcherIO(processes->IO_2_Processes);
		if (!q_is_empty(processes->IO_2_Processes)) {
			PCB_p head = getNodePCB(getHead(processes->IO_2_Processes));
			IO_2_counter = (int)getQuantum(processes->readyProcesses, getPriority(head)) * (rand() % IO_COUNTER_MULT_RANGE);
		}
		break;
	case IO_1_TRAP:
		dispatcherTrap(processes->IO_1_Processes);
		timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
		break;
	case IO_2_TRAP:
		dispatcherTrap(processes->IO_2_Processes);
		timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
		break;
	default:
		// error handling as needed
		break;
	}

	// Housekeeping

	// Empty zombie queue if 10 or more
	if (nodeCount(processes->zombieProcesses) >= MAX_ZOMBIES) {
		emptyZombies(processes->zombieProcesses);
	}

	// reset the priority queue if the set time period
	if (quantum_post_reset > RESET_QUANTUM) {
		// reset priorities in all queues
		resetPriority(processes->readyProcesses);
		q_resetPriority(processes->IO_1_Processes);
		q_resetPriority(processes->IO_2_Processes);
		setPriority(processes->runningProcess, 0);

		quantum_post_reset = 0;
	}
	if (scheduler_flag == 0) {
		scheduler_flag = 1;
	}

	return SUCCESSFUL;
}

// Function used to simulate the dispatcher of an operating system
int dispatcher() {
	Node_p node = construct_Node();
	int priority;
	initializeNode(node);

	// update context
	setPC(processes->runningProcess, sysStack);

	// check the state of the running proccess to correctly switch contexts
	switch (getState(processes->runningProcess)) {
	case HALTED:
		// enqueue to zombieProcesses
		setNodePCB(node, processes->runningProcess);
		q_enqueue(processes->zombieProcesses, node);
		break;
	case INTERRUPTED:
		// set state
		setState(processes->runningProcess, READY);

		// update priority
		priority = getPriority(processes->runningProcess);
		priority = (priority == MAX_PRIORITY) ? 0 : priority + 1;
		setPriority(processes->runningProcess, priority);

		// enqueue
		setNodePCB(node, processes->runningProcess);
		p_enqueue(processes->readyProcesses, node);
		break;
	default:
		break;
	}

	// dequeue
	processes->runningProcess = p_dequeue(processes->readyProcesses);

	// update state to running
	// set state
	setState(processes->runningProcess, RUNNING);

	sysStack = getPC(processes->runningProcess);

	return SUCCESSFUL;
}

// Added for problem 4
// A function to simulate the dispatcher for an IO Interrupt
int dispatcherIO(FIFOq_p IO_Queue) {
	Node_p node = construct_Node();
	PCB_p finishedPcb;
	int priority;

	initializeNode(node);

	// dequeue from IO queue
	finishedPcb = q_dequeue(IO_Queue);

	// update state to ready
	setState(finishedPcb, READY);

	//enqueue to ready queue
	setNodePCB(node, finishedPcb);
	p_enqueue(processes->readyProcesses, node);

	return SUCCESSFUL;
}

// Added for problem 4
// A function to simulate the dispatcher for an Trap Interrupt
int dispatcherTrap(FIFOq_p IO_Queue) {
	Node_p node = construct_Node();
	initializeNode(node);

	// update context
	setPC(processes->runningProcess, sysStack);

	// enqueue
	setNodePCB(node, processes->runningProcess);
	q_enqueue(IO_Queue, node);

	// dequeue
	processes->runningProcess = p_dequeue(processes->readyProcesses);

	// update state to running
	// set state
	setState(processes->runningProcess, RUNNING);

	sysStack = getPC(processes->runningProcess);

	return SUCCESSFUL;
}

// Added for problem 4
// A function to check if the specified process is at an IO trap or is ready to terminate
int isAtTrap(PCB_p pcb) {
	int terminate, term_count;

	if (!pcb) return 0;

	terminate = getTerminate(pcb);
	term_count = getTermCount(pcb);

	if (terminate && terminate <= term_count) return PCB_TERMINATED;

	for (int i = 0; i < IO_TRAP_SIZE; i++) {
		if (currentPC == pcb->io_1_traps[i]) return IO_1_TRAP;
		if (currentPC == pcb->io_2_traps[i]) return IO_2_TRAP;
	}

	return 0;
}

// Function used to simulate the creation of new processes
int createNewProcesses() {
	PCB_p pcb = NULL;
	Node_p node;
	for (int i = 0; i < rand() % 5; i++) {
		pcb = pcbConstruct();
		initialize_pcb(pcb);

		// give PCB random Max PC, Terminate and Traps
		setRandomMaxPC(pcb);
		setRandomTerminate(pcb);
		setRandomIOTraps(pcb);

		node = construct_Node();
		initializeNode(node);
		setNodePCB(node, pcb);
		q_enqueue(processes->newProcesses, node);
	}

	// updated for problem 4
	// Sets a PCB to privileged if under maximum allowed privileged PCBs 
	if (numPrivileged < MAX_PRIVILEGED && pcb) {
		privilegedPCBs[numPrivileged] = pcb;
		setTerminate(pcb, 0);
		numPrivileged++;
	}
}

// Prints a message describing the enqueued PCB
void printEnqueuedPCB(PCB_p pcb) {
	char buffer[MAX_BUFFER_SIZE];

	pcb_toString(pcb, buffer, MAX_BUFFER_SIZE);
	printf("Enqueued: ");
	printf(buffer);
	printf("\n\n");
	buffer[0] = '\0';
}

// Deallocates resources from halted processes
int emptyZombies(FIFOq_p zombiesList) {
	while (!(q_is_empty(zombiesList))) {
		PCB_p this_pcb = q_dequeue(zombiesList);
		pcbDestruct(this_pcb);
	}
}

// Added for problem 4
// A function to trigger the timer down counter and return if the timer interrupt occured
int timerDownCounter() {
	timer--;

	if (timer == 0) return TIMER_INTERUPT;

	return NO_INTERUPT;
}

// Added for problem 4
// A function to trigger the io1 down counter and return if the io1 interrupt occured
int IO_1_DownCounter() {
	IO_1_counter = IO_1_counter == 0 ? 0 : IO_1_counter - 1;

	if (IO_1_counter == 0) return IO_1_INTERUPT;

	return NO_INTERUPT;
}

// Added for problem 4
// A function to trigger the io2 down counter and return if the io2 interrupt occured
int IO_2_DownCounter() {
	IO_2_counter = IO_2_counter == 0 ? 0 : IO_2_counter - 1;

	if (IO_2_counter == 0) return IO_2_INTERUPT;

	return NO_INTERUPT;
}

// Added for problem 4
// A function to simulate the program executing one instruction and stepping to the next
int simulateProgramStep() {
	currentPC++;
	// need to check that max_pc is correct
	// setters/getters inside of pcb maybe?
	if (currentPC > getMaxPC(processes->runningProcess)) {
		currentPC = 0;
		setTermCount(processes->runningProcess, getTermCount(processes->runningProcess) + 1);
	}

	return SUCCESSFUL;
}

// Moves proceeses from the new queue to the ready queue
int moveNewToReady() {
	// Move to Scheduler
	// set new processes to ready
	while (!q_is_empty(processes->newProcesses)) {
		// initialize new node
		Node_p node = construct_Node();
		initializeNode(node);

		// dequeue and print next pcb
		PCB_p pcb = q_dequeue(processes->newProcesses);
		setState(pcb, READY);

		// enqueue 
		setNodePCB(node, pcb);
		p_enqueue(processes->readyProcesses, node);
	}
}

// Updated for problem 4
// Prints a message showing the current state of the ready queue and privileged processes
int printInterupt(int interupt) {
	char buffer[MAX_BUFFER_SIZE];
	switch (interupt) {
	case TIMER_INTERUPT:
		printf("\nTimer Interupt @ Iteration: %d\n", iterationCount);
		break;
	case IO_1_INTERUPT:
		printf("\nIO 1 Interupt @ Iteration: %d\n", iterationCount);
		break;
	case IO_2_INTERUPT:
		printf("\nIO 2 Interupt @ Iteration: %d\n", iterationCount);
		break;
	case IO_1_TRAP:
		printf("\nIO 1 Trap Call @ Iteration: %d\n", iterationCount);
		break;
	case IO_2_TRAP:
		printf("\nIO 2 Trap Call @ Iteration: %d\n", iterationCount);
		break;
	case PCB_TERMINATED:
		printf("\nProcess Terminated @ Iteration: %d\n", iterationCount);
		break;
	}
	printf("Running PCB: PID:%d\n", getPid(processes->runningProcess));
	pQSizeToString(processes->readyProcesses, buffer, MAX_BUFFER_SIZE);
	printf(buffer);
	printf("IO1: %d\n", nodeCount(processes->IO_1_Processes));
	printf("IO2: %d\n", nodeCount(processes->IO_2_Processes));
	for (int i = 0; i < numPrivileged; i++) {
		pcb_toSimpleString(privilegedPCBs[i], buffer, MAX_BUFFER_SIZE);
		printf(buffer);
		buffer[0] = '\0';
		printf("\n");
	}
	printf("\n");
}

// Added for problem 4
// A function used to initialize the processes struct
void initializeProcessQueues() {
	processes = (PROCESS_QUEUES_p)malloc(sizeof(PROCESS_QUEUES_s));

	processes->newProcesses = construct_FIFOq();
	processes->zombieProcesses = construct_FIFOq();
	processes->IO_1_Processes = construct_FIFOq();
	processes->IO_2_Processes = construct_FIFOq();
	processes->readyProcesses = construct_PQueue();
	processes->runningProcess = pcbConstruct();

	initializeFIFOq(processes->newProcesses);
	initializeFIFOq(processes->zombieProcesses);
	initializeFIFOq(processes->IO_1_Processes);
	initializeFIFOq(processes->IO_2_Processes);
	initialize_PQueue(processes->readyProcesses);
	initialize_pcb(processes->runningProcess);
}

// Added for problem 4
// A function used to free the processes struct
void freeProcessQueues() {
	destruct_FIFOq(processes->newProcesses);
	destruct_FIFOq(processes->zombieProcesses);
	destruct_FIFOq(processes->IO_1_Processes);
	destruct_FIFOq(processes->IO_2_Processes);
	destruct_PQueue(processes->readyProcesses);
	pcbDestruct(processes->runningProcess);

	free(processes);
}

int main() {
	pthread_t os;
	shutting_down = 0;
	ISR_FINISHED = 0;
	// Seed RNG
	srand(time(NULL));

	// Initialize Queues
	initializeProcessQueues();


	// Added for Problem 3
	// SetQuantums
	for (int i = 0; i <= MAX_PRIORITY; i++) {
		// Set Quantum to ten times one more than the priority level squared
		// i.e. Priority 0 -> Quantum 10
		setQuantum(processes->readyProcesses, i, (i + 1)*(i + 1) * 10);
	}

	// Initialize Global Vars
	currentPC = 0;
	sysStack = 0;
	iterationCount = 0;
	quantum_post_reset = 0;
	IOSR_1_finished = 0;
	IOSR_2_finished = 0;
	IO_1_counter = 0;
	IO_2_counter = 0;
	IO_1_activated = 0;
	IO_2_activated = 0;
	timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
	scheduler_flag = 0;
	// create starting processes
	// set a process to running
	setState(processes->runningProcess, RUNNING);
	setTerminate(processes->runningProcess, 0);
	setRandomMaxPC(processes->runningProcess);
	setRandomIOTraps(processes->runningProcess);
	privilegedPCBs[0] = processes->runningProcess;
	numPrivileged = 1;

	for (int i = 0; i < INIT_CREATE_CALLS; i++) {
		createNewProcesses();
	}

	// Start OS Thread
	pthread_create(&os, NULL, OS_Simulator, NULL);

	//


	// Wait until the OS Thread completes
	pthread_join(os, NULL);
	// free resources
	freeProcessQueues();
}
