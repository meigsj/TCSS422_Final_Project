/*
TCSS422 - Operating Systems
Final Project

Group Members:
Shaun Coleman
Joshua Meigs
*/

/*
TODO: CHECKS
Correct output
    Need print once a Resource pair process gets both locks
check CP pair is correctly incrementing + printing number
check processes are terminating - noting some terminations and an assert making sure CP and Resource pairs are not terminated
Change IO to use a loop instead of sleep
Check that locks are used correctly for shared vars between the main thread and the IO/Timer threads
Code Cleanup
*/

#include <time.h>
#include <pthread.h>
#include <assert.h>
#include "OS.h"

SIMPLE_STACK_p sysStack;
unsigned int currentPC;
unsigned int iterationCount;
unsigned int quantum_post_reset;

int timer;
int shutting_down;
int IOSR_1_finished;
int IOSR_2_finished;
int ISR_FINISHED;
int IO_1_counter;
int IO_2_counter;

int IO_1_activated;
int IO_2_activated;
int total_deadlock_pairs;
int total_terminated;
int total_blocked;
int total_waiting;

int con_pro_name_starting_point;

int resource_pair_starting_point;
//Tests
pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timer_cond = PTHREAD_COND_INITIALIZER;

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

PROCESS_QUEUES_p processes;

// TODO - Make into structs?
int total_cp_pairs;
CP_PAIR_p cp_pairs[PRO_CON_MAX];

int total_resource_pairs;
RESOURCE_PAIR_p resource_pairs[SHARED_RESOURCE_MAX];

int total_comp_processes;

int total_io_processes;

// A flag to signal that a TSR is in progress
int trap_flag;

// A flag to signal that a TSR is in progress
int syncro_flag;

// Updated for Problem 4
// The top level of the OS simulator
void * OS_Simulator(void *arg) {

    char* buffer[MAX_BUFFER_SIZE];
    pthread_t the_timer_thread;
    pthread_create(&the_timer_thread, NULL, timer_thread, NULL);

    pthread_t the_io_1_thread;
    pthread_create(&the_io_1_thread, NULL, io1_thread, NULL);

    pthread_t the_io_2_thread;
    pthread_create(&the_io_2_thread, NULL, io2_thread, NULL);
	int deadlocks[SHARED_RESOURCE_MAX] = {0};
	int i = 0;
	// Main Loop
    // One cycle is one instruction
    for (;;) { // for spider
        int trapFlag = 0;

        // update counters
        iterationCount++;
        quantum_post_reset++;

		//DOES DEADLOCK CHECK
        simulateProgramStep();

        // Create new processes
        if ((iterationCount % NEW_PROCESS_ITERATION) == 0) {
            createNewProcesses(processes->newProcesses);
        }

        // Trigger IO counters and check for IO interupts
        // Trigger timer and check for timer interupt
        if (timer_check() && processes->runningProcess->step_finished == STEP_FINISHED) continue;
        if (IO_check() && processes->runningProcess->step_finished == STEP_FINISHED) continue;

        // Set step to finished as if replaced here the TSR will still complete
        processes->runningProcess->step_finished = STEP_FINISHED;

        // Check for IO Traps, Syncronization Traps and Termination
        trapFlag = isAtTrap(processes->runningProcess);
        syncro_flag = isAtSyncro(processes->runningProcess);
        if (trapFlag == IO_1_TRAP || trapFlag == IO_2_TRAP || trapFlag == PCB_TERMINATED) {
            printf("\nTrap Detected!\n");
            ss_push(sysStack, currentPC);
            pseudoTSR(trapFlag);
            printInterupt(trapFlag);
        } else if (syncro_flag) {
            trapFlag = getInterruptType(syncro_flag);
            printf("\nSyncronization Service Detected!\n");
			ss_push(sysStack, currentPC);

			check_for_syncro_trap(syncro_flag);

			printInterupt(trapFlag);
        }

		assert(ss_getSize(sysStack) == 0);
        assert(getTotalNodesCount(processes->readyProcesses) <= (PRO_CON_MAX + SHARED_RESOURCE_MAX + COMPUTE_PROCESS_MAX*2 + IO_PROCESS_MAX*2));

		//check stop condition for the simulation
        if (iterationCount >= HALT_CONDITION) {
            printf("---- HALTING SIMULATION ON ITERATION %d ----\n", iterationCount);
			printf("------- TOTAL TERMINATED PROCESSES %d ------\n", total_terminated);
            printf("-------- TOTAL DEADLOCKED PAIRS %d ---------\n", total_deadlock_pairs);
            pthread_mutex_lock(&global_shutdown_lock);
            shutting_down = 1;
            pthread_mutex_unlock(&global_shutdown_lock);
            break;
        }

		//check for processes type
    }

}


void check_for_syncro_trap(int syncro_flag) {
	int error_bad_pcb_for_syncro = 0;
	//TODO: Rewrite it so that Mutexs are given name fields in the structs as well, to shorten amount of space taken by array calls
	if (getType(processes->runningProcess) == CONPRO_PAIR) {
		CP_PAIR_p pair = getPCPair(processes->runningProcess);

		switch (syncro_flag) {
		case LOCK_RESOURCE_1:
			if (processes->runningProcess->pid == pair->producer->pid) {
				printf("\nProcess %s requests lock M%c\n", pair->producer_name, pair->producer_name[0]); 
			}
			else {
				printf("\nProcess %s requests lock M%c\n", pair->consumer_name, pair->consumer_name[0]);
			}		
			lock_tsr(pair->mutex);
			if (getState(processes->runningProcess) ==  INTERRUPTED) {
				printf("\nRequest on the lock was succesful\n");
			}
			else {
				printf("\nRequest on the lock not succesful, owned by PID: %d", pair->mutex->owner->pid);
			}
			break;
		case UNLOCK_RESOURCE_1:
			if (processes->runningProcess->pid == pair->producer->pid) {
				printf("\nProcess %s releases lock M%c\n", pair->producer_name, pair->producer_name[0]);
			}
			else {
				printf("\nProcess %s releases lock M%c\n", pair->consumer_name, pair->consumer_name[0]);
			}
			unlock_tsr(pair->mutex);
			break;
		case WAIT_RESOURCE_1:
			if (pair->producer->pid == processes->runningProcess->pid) {
				printf("\nProducer %s calls wait on cond with mutex %s.\n", pair->producer_name,  pair->consumer_name);
				wait_tsr(pair->mutex, pair->consumed);
			}
			else {
				printf("\nProducer %s calls wait on cond  with mutex %s.\n", pair->consumer_name,  pair->producer_name);
				wait_tsr(pair->mutex, pair->produced);
			}
			break;
		case SIGNAL_RESOURCE_1:
			if (pair->producer->pid == processes->runningProcess->pid) {
				pair->counter++;
				printf("\nProducer %s incremented variable %c%c: %d.\n",pair->producer_name , pair->producer_name[0], pair->consumer_name[0], pair->counter);
				signal_tsr(pair->mutex, pair->produced);
                pair->filled = FILLED;
				printf("\nProducer %s signals produced on cond.\n", pair->producer_name);
			}
			else {
				printf("\nConsumer %s read variable %c%c: %d\n", pair->consumer_name, pair->producer_name[0], pair->consumer_name[0], pair->counter);
				signal_tsr(pair->mutex, pair->consumed);
                pair->filled = EMPTY;
				printf("\nConsumer %s signals Consumed on cond.\n", pair->consumer_name);
			}
			break;
		default:
			break;
		}
	}
	else if (getType(processes->runningProcess) == RESOURCE_PAIR) {
		RESOURCE_PAIR_p pair = getResourcePair(processes->runningProcess);
		switch (syncro_flag) {
		case LOCK_RESOURCE_1:
			if (processes->runningProcess->pid == pair->process_1->pid) {
				printf("\nProcess %s requests lock %s\n", pair->process_1_name, pair->mutex_1_name);
			}
			else {
				printf("\nProcess %s requests lock %s\n", pair->process_2_name, pair->mutex_1_name);
			}
			lock_tsr(pair->mutex_1);
			if (getState(processes->runningProcess) == INTERRUPTED) {
				printf("\nRequest on the lock was succesful\n");
			}
			else {
				printf("\nRequest on the lock not succesful, owned by PID : %d\n", pair->mutex_1->owner->pid);
			}
			break;
		case LOCK_RESOURCE_2:
			if (processes->runningProcess->pid == pair->process_1->pid) {
				printf("\nProcess %s requests lock %s\n", pair->process_1_name, pair->mutex_2_name);
			}
			else {
				printf("\nProcess %s requests lock %s\n", pair->process_2_name, pair->mutex_2_name);
			}
			lock_tsr(pair->mutex_2);

			if (getState(processes->runningProcess) == INTERRUPTED) {
				printf("\nRequest on the lock was succesful\n");
			}
			else {
				printf("\nRequest on the lock not succesful , owned by PID : %d\n", pair->mutex_2->owner->pid);
			}

			break;
		case UNLOCK_RESOURCE_1:
			if (processes->runningProcess->pid == pair->process_1->pid) {
				printf("\nProcess %s releases lock %s\n", pair->process_1_name, pair->mutex_1_name);
			}
			else {
				printf("\nProcess %s releases lock %s\n", pair->process_2_name, pair->mutex_1_name);
			}
			unlock_tsr(pair->mutex_1);
			break;
		case UNLOCK_RESOURCE_2:
			if (processes->runningProcess->pid == pair->process_1->pid) {
				printf("\nProcess %s releases lock %s\n", pair->process_1_name, pair->mutex_2_name);
			}
			else {
				printf("\nProcess %s releases lock %s\n", pair->process_2_name, pair->mutex_2_name);
			}
			unlock_tsr(pair->mutex_2);
			break;
		default:
			break;
		}
		if (pair->mutex_1->owner && pair->mutex_2->owner) {
			printf("\nMutex's %s and %s are in use\n", pair->mutex_1_name, pair->mutex_2_name);
		}
	}
	else {
		assert(error_bad_pcb_for_syncro);
	}

}

void * timer_thread(void * s) {

    struct timespec ts;
    pthread_mutex_lock(&timer_lock);
    ts.tv_sec = 0;
    ts.tv_nsec = timer/2;//(timer*1000);
    int i = timer * IO_QUANTUM_MULTIPLIER;

    for (;;) {
        pthread_mutex_lock(&global_shutdown_lock);
        while (shutting_down) {
            break;
        }

        for (int j = 0; j < i; j++) {}

        //nanosleep(&ts, NULL);
        pthread_mutex_unlock(&global_shutdown_lock);

        while (!ISR_FINISHED) {
            pthread_cond_wait(&timer_cond, &timer_lock);
        }
        ISR_FINISHED = 0;

        //TODO add locks for grabbing the timer and setting the timer
        ts.tv_nsec = timer/2;
        i = timer * IO_QUANTUM_MULTIPLIER;
    }
}

// TODO: Check that we are correctly using locks for all vars shared by main thread
void * io1_thread(void * s) {
    struct timespec ts;
    ts.tv_sec = 0;
	ts.tv_nsec = IO_1_counter;

    // Lock signal mutex
    pthread_mutex_lock(&IO_1_lock);

    for (;;) {
        pthread_mutex_lock(&global_shutdown_lock);
        // Check if the program needs to shut down
        while (shutting_down) {
            break;
        }
        pthread_mutex_unlock(&global_shutdown_lock);

        pthread_mutex_lock(&IO_1_active_lock);
        while (!IO_1_activated) {
            pthread_cond_wait(&IO_1_active_cond, &IO_1_active_lock);
        }
		ts.tv_nsec = IO_1_counter;
        pthread_mutex_unlock(&IO_1_active_lock);

        // Sleep thread then unlock to signal IO 1 device is ready
        nanosleep(&ts, NULL);
        pthread_mutex_unlock(&IO_1_lock);

        // Wait until Interupt Service completes
       // pthread_mutex_lock(&IO_1_reset_lock);
        while (!IOSR_1_finished) {
            pthread_cond_wait(&IO_1_active_cond, &IO_1_lock);
        }
        //pthread_mutex_lock(&IO_1_lock);
        IOSR_1_finished = 0;
        ts.tv_nsec = IO_1_counter;
		//pthread_mutex_unlock(&IO_1_reset_lock);
    }
}

void * io2_thread(void * s) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = IO_2_counter;//(timer*1000);
     // Lock signal mutex
    pthread_mutex_lock(&IO_2_lock);

    for (;;) {
        pthread_mutex_lock(&global_shutdown_lock);
        // Check if the program needs to shut down
        while (shutting_down) {
            break;
        }
        pthread_mutex_unlock(&global_shutdown_lock);

        pthread_mutex_lock(&IO_2_active_lock);
        while (!IO_2_activated) {
            pthread_cond_wait(&IO_2_active_cond, &IO_2_active_lock);
        }
		ts.tv_nsec = IO_2_counter;
        pthread_mutex_unlock(&IO_2_active_lock);

        nanosleep(&ts, NULL);
        pthread_mutex_unlock(&IO_2_lock);

        // Wait until Interupt Service completes
        pthread_mutex_lock(&IO_2_reset_lock);
        while (!IOSR_2_finished) {
            pthread_cond_wait(&IO_2_active_cond, &IO_2_reset_lock);
        }
        pthread_mutex_lock(&IO_2_lock);
        IOSR_2_finished = 0;
        ts.tv_nsec = IO_2_counter;
		pthread_mutex_unlock(&IO_2_reset_lock);
    }
}

int timer_check() {
    int ret = 0;
    if (pthread_mutex_trylock(&timer_lock) == 0) {
        ret = 1;
        printf("\nTimer Interrupt Detected!\n");
        int state = RUNNING;
        if (processes->runningProcess) state = getState(processes->runningProcess);
        // Timer interupt
        ss_push(sysStack, currentPC);

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
	}

    return ret;
}

int IO_check() {
    int proc_replaced = 0;
    if (IO_1_activated && pthread_mutex_trylock(&IO_1_lock) == 0) {
        printf("\nIO1 Interrupt Detected!\n");
        ss_push(sysStack,currentPC);
        IO_Interupt_Routine(IO_1_INTERUPT);
        pthread_mutex_lock(&IO_1_global_lock);
        if (q_is_empty(processes->IO_1_Processes)) {
            IO_1_activated = 0;
        }
        pthread_mutex_unlock(&IO_1_global_lock);
        IOSR_1_finished = 1;
        //pthread_cond_signal(&IO_1_active_cond);
        printInterupt(IO_1_INTERUPT);
        pthread_mutex_unlock(&IO_1_lock);
    }

    if (IO_2_activated && pthread_mutex_trylock(&IO_2_lock) == 0) {
        printf("\nIO2 Interrupt Detected!\n");
		ss_push(sysStack, currentPC);
        IO_Interupt_Routine(IO_2_INTERUPT);
        pthread_mutex_lock(&IO_2_global_lock);
        if (q_is_empty(processes->IO_2_Processes)) {
            IO_2_activated = 0;
        }
        pthread_mutex_unlock(&IO_2_global_lock);
        IOSR_2_finished = 1;
       // pthread_cond_signal(&IO_2_active_cond);

        printInterupt(IO_2_INTERUPT);
        pthread_mutex_unlock(&IO_2_lock);
    }
}

// Function used to simulate an ISR call
int pseudoISR() {
    PCB_p running = processes->runningProcess;
    int state = getState(running);

    // set state to interupted if the process was not halted
    if (state != HALTED && state != WAITING) {
        setState(running, INTERRUPTED);
    }

    if (running) setPC(running, currentPC);

    // scheduler up call
    scheduler(TIMER_INTERUPT, NULL, NULL, NULL);

    // Update Timer
    timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
    // IRET (update current pc)
	assert(!stack_is_empty(sysStack));
    currentPC = ss_pop(sysStack);
    return SUCCESSFUL;
}

// Added for problem 4
// A function to simulate a an IO Interrupt routine
int IO_Interupt_Routine(int IO_interupt) {
    int replaced = 0;
    if (getState(processes->runningProcess) == RUNNING)
        setState(processes->runningProcess, INTERRUPTED);

    // save pc to pcb
    setPC(processes->runningProcess, currentPC);

    replaced = timer_check();

    // scheduler up call
    scheduler(IO_interupt, NULL, NULL, NULL);

    // IRET (update current pc)
	assert(!stack_is_empty(sysStack));
    currentPC = ss_pop(sysStack);
    return replaced;
}

// Added for problem 4
// A function to simulate a TSR
int pseudoTSR(int trap_interupt) {
    PCB_p running = processes->runningProcess;
    setPC(processes->runningProcess, currentPC);
    if (trap_interupt == PCB_TERMINATED) {
        setState(processes->runningProcess, HALTED);
        setTermination(processes->runningProcess, time(NULL));
    } else {
        setState(processes->runningProcess, WAITING);
    }
    
    timer_check();

    IO_check();

	//TODO: Remove Dead Code
	/*
    // Activate counter if not in use (IO_Queue is empty)
    // IO Traps only
    if (trap_interupt == IO_1_TRAP && q_is_empty(processes->IO_1_Processes)) {
		pthread_mutex_lock(&IO_1_reset_lock);
        
		pthread_mutex_unlock(&IO_1_reset_lock);
    } else if (trap_interupt == IO_2_TRAP && q_is_empty(processes->IO_2_Processes)) {
		pthread_mutex_lock(&IO_1_reset_lock);
		
		pthread_mutex_unlock(&IO_1_reset_lock);
	}
	*/

    // scheduler up call
    scheduler(trap_interupt, running, NULL, NULL);

    // activate IO devices as needed
    pthread_mutex_lock(&IO_1_active_lock);
    if (q_is_empty(processes->IO_1_Processes)) {
        IO_1_activated = 0;
    } else {
        IO_1_activated = 1;
		IO_1_counter = getQuantum(processes->readyProcesses, getPriority(running)) * (rand() % IO_COUNTER_MULT_RANGE);
        pthread_cond_signal(&IO_1_active_cond);
    }
    pthread_mutex_unlock(&IO_1_active_lock);

    pthread_mutex_lock(&IO_2_active_lock);
    if (q_is_empty(processes->IO_2_Processes)) {
        IO_2_activated = 0;
    } else {
        IO_2_activated = 1;
		IO_2_counter = getQuantum(processes->readyProcesses, getPriority(running)) * (rand() % IO_COUNTER_MULT_RANGE);
		pthread_cond_signal(&IO_2_active_cond);
    }
    pthread_mutex_unlock(&IO_2_active_lock);

    trap_flag = 0;
    // IRET (update current pc)
	assert(!stack_is_empty(sysStack));
    currentPC = ss_pop(sysStack);
    return SUCCESSFUL;
}

// Updated for problem 4
// Function used to simulate the scheduler in an operating system
int scheduler(int interupt, PCB_p running, CUSTOM_MUTEX_p mutex, CUSTOM_COND_p cond) {//WAS COND_s
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
        assert(running);
        dispatcherTrap(processes->IO_1_Processes, running);
        timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
        break;
    case IO_2_TRAP:
        assert(running);
        dispatcherTrap(processes->IO_2_Processes, running);
        timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
        break;
    case LOCK_INTERRUPT:
        assert(running && mutex);
        dispatcherLock(running, mutex);
        timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
        break;
    case UNLOCK_INTERRUPT:
        assert(mutex);
        dispatcherUnlock(mutex);
        break;
    case WAIT_INTERRUPT:
        assert(running && mutex && cond);
        dispatcherWait(running, mutex, cond);
        timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
        break;
    case SIGNAL_INTERRUPT:
        assert(mutex && cond);
        dispatcherSignal(mutex, cond);
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
		check_for_deadlock();
    }

    return SUCCESSFUL;
}

// Function used to simulate the dispatcher of an operating system
int dispatcher() {
    Node_p node = construct_Node();
    int priority;
    initializeNode(node);
    
    // update context
	assert(!stack_is_empty(sysStack));
    setPC(processes->runningProcess, ss_pop(sysStack));
    
    // check the state of the running proccess to correctly switch contexts
    switch (getState(processes->runningProcess)) {
        case HALTED:
            // enqueue to zombieProcesses
            setNodePCB(node, processes->runningProcess);
            q_enqueue(processes->zombieProcesses, node);
			total_terminated++;

            if (getType(processes->runningProcess) == IO) {
                total_io_processes--;
            } else if(getType(processes->runningProcess) == COMP_INTENSIVE) {
                total_comp_processes--;
            } else {
                printf("Terminating a non IO or COMP_INTENSIVE process");
                assert(0);
            }

            break;
        case INTERRUPTED:
            // set state
            setState(processes->runningProcess, READY);
    
            // update priority
            priority = getPriority(processes->runningProcess);
            priority = (priority == MAX_PRIORITY ) ? 0 : priority+1;
            setPriority(processes->runningProcess, priority);
    
            // enqueue
            setNodePCB(node, processes->runningProcess);
            p_enqueue(processes->readyProcesses, node);
            break;
        case WAITING:
            // do not enqueue running proccess if waiting
            // process will be enqueued to proper IO device
            // once the TSR resumes
        default:
            break;
    }
      
    // dequeue
    processes->runningProcess = p_dequeue(processes->readyProcesses);

    // update state to running
    // set state
    setState(processes->runningProcess, RUNNING);
    
    ss_push(sysStack, getPC(processes->runningProcess));
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

    ss_pop(sysStack);
    ss_push(sysStack, processes->runningProcess->pid);

    return SUCCESSFUL;
}

// Added for problem 4
// A function to simulate the dispatcher for an Trap Interrupt
int dispatcherTrap(FIFOq_p IO_Queue, PCB_p running) {
	Node_p node = construct_Node();
	initializeNode(node);

	// update context
	assert(!stack_is_empty(sysStack));
	setPC(running, ss_pop(sysStack));

	// enqueue
	setNodePCB(node, running);
	q_enqueue(IO_Queue, node);

    //printf("\nP->Running %d, Running %d\n", processes->runningProcess->pid, running->pid);

	if (processes->runningProcess->pid == running->pid) {
		// dequeue
		processes->runningProcess = p_dequeue(processes->readyProcesses);
		// update state to running
		// set state
		setState(processes->runningProcess, RUNNING);
	}

	ss_push(sysStack, getPC(processes->runningProcess));

	return SUCCESSFUL;
}

int lock_tsr(CUSTOM_MUTEX_p mutex) {
    PCB_p running = processes->runningProcess;
    setPC(running, currentPC);
    
    if (mutex->owner == NULL || mutex->owner->pid == processes->runningProcess->pid) {
        setState(running, INTERRUPTED);
		
    } else {
        setState(running, WAITING);
    }

    timer_check();
    IO_check();

    if (getState(running) == WAITING) {
        scheduler(LOCK_INTERRUPT, running, mutex, NULL);
    } else {
        mutex->owner = running;
        ss_pop(sysStack);
        ss_push(sysStack, getPC(processes->runningProcess));
    }

    syncro_flag = NO_RESOURCE_SYNCRO;

    // IRET (update current pc)
	assert(!stack_is_empty(sysStack));
    currentPC = ss_pop(sysStack);
    return SUCCESSFUL;
}

int unlock_tsr(CUSTOM_MUTEX_p mutex) {
    setPC(processes->runningProcess, currentPC);
    setState(processes->runningProcess, INTERRUPTED);
    
    timer_check();

    IO_check();
    
    if (q_is_empty(mutex->blocked)) {
        mutex->owner = NULL;
        ss_pop(sysStack);
        ss_push(sysStack, getPC(processes->runningProcess));
    } else {
        scheduler(UNLOCK_INTERRUPT, NULL, mutex, NULL);
    }

    syncro_flag = NO_RESOURCE_SYNCRO;

    // IRET (update current pc)
	assert(!stack_is_empty(sysStack));
    currentPC = ss_pop(sysStack);
  
    return SUCCESSFUL;
}

int wait_tsr(CUSTOM_MUTEX_p mutex, CUSTOM_COND_p cond) {
    PCB_p running = processes->runningProcess;
    setPC(running, currentPC);

    setState(running, WAITING);

    timer_check();
    IO_check();

    scheduler(WAIT_INTERRUPT, running, mutex, cond);
   
    syncro_flag = NO_RESOURCE_SYNCRO;

    // IRET (update current pc)
	assert(!stack_is_empty(sysStack));
    currentPC = ss_pop(sysStack);
    return SUCCESSFUL;
}

int signal_tsr(CUSTOM_MUTEX_p mutex, CUSTOM_COND_p cond) {
    setPC(processes->runningProcess, currentPC);

    timer_check();
    IO_check();
	
    if (q_is_empty(cond->waiting)) {
        ss_pop(sysStack);
        ss_push(sysStack, getPC(processes->runningProcess));
    } else {
        scheduler(SIGNAL_INTERRUPT, NULL, mutex, cond);
    }

    syncro_flag = NO_RESOURCE_SYNCRO;

    // IRET (update current pc)
	assert(!stack_is_empty(sysStack));
    currentPC = ss_pop(sysStack);
    return SUCCESSFUL;
}

int dispatcherLock(PCB_p process, CUSTOM_MUTEX_p mutex) {
    Node_p node = construct_Node();
    initializeNode(node);
    setNodePCB(node, process);

	// update context
	assert(!stack_is_empty(sysStack));
	setPC(process, ss_pop(sysStack));   
    setState(process, WAITING);

    q_enqueue(mutex->blocked, node);
	total_blocked++;

	if (processes->runningProcess->pid == process->pid) {
		processes->runningProcess = p_dequeue(processes->readyProcesses);
		setState(processes->runningProcess, RUNNING);
	}

	ss_push(sysStack, getPC(processes->runningProcess));
}

int dispatcherUnlock(CUSTOM_MUTEX_p mutex) {
    mutex->owner = q_dequeue(mutex->blocked);
    setState(mutex->owner, READY);
    ss_pop(sysStack);

    //Added node here so that p_enque would have a node passed in and not the PCB
    Node_p node = construct_Node();
    initializeNode(node);
    setNodePCB(node, mutex->owner);
    p_enqueue(processes->readyProcesses, node);
    total_blocked--;
    ss_push(sysStack, getPC(processes->runningProcess));
}

int dispatcherWait(PCB_p process, CUSTOM_MUTEX_p mutex, CUSTOM_COND_p cond) {
    Node_p node = construct_Node();
    initializeNode(node);
    setNodePCB(node, process);

	// update context
	assert(!stack_is_empty(sysStack));
	setPC(process, ss_pop(sysStack));

    //Added node here so that p_enque would have a node passed in and not the PCB
    setState(process, WAITING);
    q_enqueue(cond->waiting, node);
	total_waiting++;
    
    // unlock lock TODO: replace with lock tsr call?
    if (q_is_empty(mutex->blocked)) {
        mutex->owner = NULL;
    } else {
		Node_p another_node = construct_Node();
		initializeNode(another_node);
        mutex->owner = q_dequeue(mutex->blocked);
        setState(mutex->owner, READY);
		setNodePCB(another_node, mutex->owner);
        p_enqueue(processes->readyProcesses, another_node);
		total_blocked--;
    }

	if (processes->runningProcess->pid == process->pid) {
		processes->runningProcess = p_dequeue(processes->readyProcesses);
		setState(processes->runningProcess, RUNNING);
	}

	ss_push(sysStack, getPC(processes->runningProcess));
}

int dispatcherSignal(CUSTOM_MUTEX_p mutex, CUSTOM_COND_p cond) {
    Node_p node = construct_Node();
    PCB_p wokePcb;
    int priority;

    initializeNode(node);

    // dequeue from IO queue
    wokePcb = q_dequeue(cond->waiting);
	total_waiting--;

    // update state to waiting for locked to unlock
    setState(wokePcb, WAITING);

    // Try to grab lock used by wait (assumed to fail per CP_pair)
    // TODO: use unlock TSR call instead?
    setNodePCB(node, wokePcb);
    q_enqueue(mutex->blocked, node);
	total_blocked++;

    ss_pop(sysStack);
    ss_push(sysStack, processes->runningProcess->pid);
}


// Added for problem 4
// A function to check if the specified process is at an IO trap or is ready to terminate
int isAtTrap(PCB_p pcb) {
    int terminate, term_count;
    
    if (!pcb) return 0;
    
    terminate = getTerminate(pcb);
    term_count = getTermCount(pcb);
    
    if (terminate && terminate <= term_count) return PCB_TERMINATED;

    if (!pcb->io_1_traps[0] && !pcb->io_2_traps[0]) return 0;

    for (int i=0; i < IO_TRAP_SIZE; i++) {
        if (currentPC == pcb->io_1_traps[i]) return IO_1_TRAP;
        if (currentPC == pcb->io_2_traps[i]) return IO_2_TRAP;
    }
    
    return 0;
}

// A function to check if the specified process is at a syncronization service request
int isAtSyncro(PCB_p pcb) {
    int type;

    if (!pcb) return NO_RESOURCE_SYNCRO;
    type = getType(pcb);
    
    if (type == CONPRO_PAIR) {
        for (int i = 0; i < SYNCRO_SIZE; i++) {
            if (currentPC == pcb->lock_1_pcs[i]) return LOCK_RESOURCE_1;
            if (currentPC == pcb->unlock_1_pcs[i]) return UNLOCK_RESOURCE_1;
            if (currentPC == pcb->wait_1_pcs[i]) return WAIT_RESOURCE_1;
            if (currentPC == pcb->signal_1_pcs[i]) return SIGNAL_RESOURCE_1;
        }
    }

    if (type == RESOURCE_PAIR) {
		for (int i = 0; i < SYNCRO_SIZE; i++) {
			if (currentPC == pcb->lock_1_pcs[i]) return LOCK_RESOURCE_1;
			if (currentPC == pcb->unlock_1_pcs[i]) return UNLOCK_RESOURCE_1;
			if (currentPC == pcb->lock_2_pcs[i]) return LOCK_RESOURCE_2;
			if (currentPC == pcb->unlock_2_pcs[i]) return UNLOCK_RESOURCE_2;
		}
    }

    return NO_RESOURCE_SYNCRO;
}

// Function used to simulate the creation of new processes
int createNewProcesses() {
    int newIO = rand() % CREATE_IO_PROCESS_MAX;
    int newComp = rand() % CREATE_CUMPUTE_PROCESS_MAX;
    int newCP = rand() % CREATE_PRO_CON_MAX;
    int newShared = rand() % CREATE_SHARED_RESOURCE_MAX;
    //printf("\n Trying to create: %d IO, %d Comp, %d CP Pairs, %d Shared Pairs\n", newIO, newComp, newCP, newShared);

    for(int i = 0; i < newIO; i++) {
        if (total_io_processes >= IO_PROCESS_MAX) break;
        createIOProcess();
    }

    for (int i = 0; i < newComp; i++) {
        if (total_comp_processes >= COMPUTE_PROCESS_MAX) break;
        createComputeIntensiveProcess();
    }

    for (int i = 0; i < newCP; i++) {
        if (total_cp_pairs >= PRO_CON_MAX) break;
        createConsumerProducerPair();
    }

    for (int i = 0; i < newShared; i++) {
        if (total_resource_pairs >= SHARED_RESOURCE_MAX) break;
        createSharedResourcePair();
    }
}

// Create a process with IO Trap calls
int createIOProcess() {
    PCB_p pcb = NULL;
    Node_p node;

    pcb = pcbConstruct();
    initialize_pcb(pcb);

    setType(pcb, IO);

    // give PCB random Max PC, Terminate and Traps
    setRandomMaxPC(pcb);
    setRandomTerminate(pcb);
    setRandomIOTraps(pcb);

    node = construct_Node();
    initializeNode(node);
    setNodePCB(node, pcb);
    q_enqueue(processes->newProcesses, node);

    total_io_processes++;
}

// Create a process with no io
int createComputeIntensiveProcess() {
    PCB_p pcb = NULL;
    Node_p node;

    pcb = pcbConstruct();
    initialize_pcb(pcb);

    setType(pcb, COMP_INTENSIVE);

    setRandomMaxPC(pcb);
    setRandomTerminate(pcb);

    node = construct_Node();
    initializeNode(node);
    setNodePCB(node, pcb);
    q_enqueue(processes->newProcesses, node);

    total_comp_processes++;
}

// Create two processes in a consumer/producer pair
int createConsumerProducerPair() {
    PCB_p producer = NULL;
    PCB_p consumer = NULL;
    Node_p pro_node;
    Node_p con_code;
    CP_PAIR_p pair; 

    if (total_cp_pairs >= PRO_CON_MAX) return 1; // MAGIC NUMBER

    producer = pcbConstruct();
    consumer = pcbConstruct();
    initialize_pcb(producer);
    initialize_pcb(consumer);

    // Mark each processes as a consumer/producer pair
    setType(producer, CONPRO_PAIR);
    setType(consumer, CONPRO_PAIR);

    // Set to not terminate
    setTerminate(producer, 0);
    setTerminate(consumer, 0);

    // Set syncro trap calls
    producer->lock_1_pcs[0] = 7;
    producer->wait_1_pcs[0] = 9;
    producer->signal_1_pcs[0] = 11;
    producer->unlock_1_pcs[0] = 12;
    producer->maxpc = 100;

    consumer->lock_1_pcs[0] = 7;
    consumer->wait_1_pcs[0] = 9;
    consumer->signal_1_pcs[0] = 11;
    consumer->unlock_1_pcs[0] = 12;
    consumer->maxpc = 100;


    // initalize CP_PAIR
    pair = (CP_PAIR_p)malloc(sizeof(CP_PAIR_s));
    initialize_CP_Pair(pair);
    pair->producer = producer;
    pair->consumer = consumer;

    pair->filled = EMPTY;

    cp_pairs[total_cp_pairs] = pair;
    total_cp_pairs++;

	//give names to the pair
	for (int i = 0; i < MAX_NAME_SIZE_CONPRO-1; i++) {
		pair->producer_name[i] = con_pro_name_starting_point + total_cp_pairs;
	}
	pair->producer_name[MAX_NAME_SIZE_CONPRO-1] = '\0'; 
	con_pro_name_starting_point++;
	for (int i = 0; i < MAX_NAME_SIZE_CONPRO-1; i++) {
		pair->consumer_name[i] = con_pro_name_starting_point + total_cp_pairs;
	}
	pair->consumer_name[MAX_NAME_SIZE_CONPRO-1] = '\0';

    // enqueue
    pro_node = construct_Node();
    con_code = construct_Node();
    initializeNode(pro_node);
    initializeNode(con_code);
    setNodePCB(pro_node, producer);
    setNodePCB(con_code, consumer);
	q_enqueue(processes->newProcesses, pro_node);
	q_enqueue(processes->newProcesses, con_code);
	


    return SUCCESSFUL;
}

// Creates a two processes in a Shared Resource Pairintialize_Resource_Pair
int createSharedResourcePair() {
    PCB_p process_1 = NULL;
    PCB_p process_2 = NULL;
    Node_p node_1;
    Node_p node_2;
    RESOURCE_PAIR_p pair;

    if (total_resource_pairs >= SHARED_RESOURCE_MAX) return 1; // MAGIC NUMBER

    process_1 = pcbConstruct();
    process_2 = pcbConstruct();
    initialize_pcb(process_1);
    initialize_pcb(process_2);

    // Mark each processes as a resource pair
    setType(process_1, RESOURCE_PAIR);
    setType(process_2, RESOURCE_PAIR);

    process_1->maxpc = 1200;
    process_2->maxpc = 1200;

    // Set to not terminate
    setTerminate(process_1, 0);
    setTerminate(process_2, 0);

    // Set up
    process_1->lock_1_pcs[0] = 300;
    process_1->lock_2_pcs[0] = 100;
    process_1->unlock_2_pcs[0] = 1100;
    process_1->unlock_1_pcs[0] = 1000;
    
    process_2->lock_1_pcs[0] = 100;
    process_2->lock_2_pcs[0] = 300;
    process_2->unlock_2_pcs[0] = 1000;
    process_2->unlock_1_pcs[0] = 1100;
    

    // initalize RESOURCE_PAIR
    pair = (RESOURCE_PAIR_p)malloc(sizeof(RESOURCE_PAIR_s));
    initialize_Resource_Pair(pair);
    pair->process_1 = process_1;
    pair->process_2 = process_2;
    resource_pairs[total_resource_pairs] = pair;
    total_resource_pairs++;

	// give names to the pair //TODO MAGIC
	if (resource_pair_starting_point > UPPERCASE_Z && resource_pair_starting_point < LOWERCASE_A) {
		resource_pair_starting_point = LOWERCASE_A;
	}
	for (int i = 0; i < 3; i++) {
		pair->process_1_name[i] = resource_pair_starting_point + total_resource_pairs;
	}
	pair->process_1_name[MAX_NAME_SIZE_RESOURCE-1] = '\0';
	resource_pair_starting_point++;
	for (int i = 0; i < MAX_NAME_SIZE_RESOURCE - 1; i++) {
		pair->process_2_name[i] = resource_pair_starting_point + total_resource_pairs;
	}
	pair->process_2_name[MAX_NAME_SIZE_RESOURCE - 1] = '\0';

	pair->mutex_1_name[0] = 'M';
	pair->mutex_2_name[0] = 'M';

	pair->mutex_1_name[1] = pair->process_1_name[0];
	pair->mutex_1_name[2] = pair->process_2_name[0];
	pair->mutex_1_name[3] = '\0';

	pair->mutex_2_name[1] = pair->process_2_name[0];
	pair->mutex_2_name[2] = pair->process_1_name[0];
	pair->mutex_2_name[3] = '\0';


    // enqueue
    node_1 = construct_Node();
    node_2 = construct_Node();
    initializeNode(node_1);
    initializeNode(node_2);
    setNodePCB(node_1, process_1);
    setNodePCB(node_2, process_2);
    q_enqueue(processes->newProcesses, node_1);
    q_enqueue(processes->newProcesses, node_2);

    return SUCCESSFUL;
}

// initialize the passed CP_PAIR struct
void initialize_CP_Pair(CP_PAIR_p pair) {
    pair->producer = NULL;
    pair->consumer = NULL;
    pair->counter = 0;
    pair->mutex = (CUSTOM_MUTEX_p)malloc(sizeof(CUSTOM_MUTEX_s));
    pair->produced = (CUSTOM_COND_p)malloc(sizeof(CUSTOM_COND_s));
    pair->consumed = (CUSTOM_COND_p)malloc(sizeof(CUSTOM_COND_s));

    initialize_Custom_Mutex(pair->mutex);
    initialize_Custom_Cond(pair->produced);
    initialize_Custom_Cond(pair->consumed);
}

// initialize the passed RESOURCE_PAIR struct
void initialize_Resource_Pair(RESOURCE_PAIR_p pair) {
    pair->process_1 = NULL;
    pair->process_2 = NULL;
    pair->mutex_1 = (CUSTOM_MUTEX_p)malloc(sizeof(CUSTOM_MUTEX_s));
    pair->mutex_2 = (CUSTOM_MUTEX_p)malloc(sizeof(CUSTOM_MUTEX_s));

    initialize_Custom_Mutex(pair->mutex_1);
    initialize_Custom_Mutex(pair->mutex_2);
}

// Initialize the passed CUSTOM_MUTEX struct
void initialize_Custom_Mutex(CUSTOM_MUTEX_p mutex) {
    mutex->owner = NULL;
    mutex->blocked = construct_FIFOq();
    initializeFIFOq(mutex->blocked);
}

// Initialize the passed CUSTOM_COND struct
void initialize_Custom_Cond(CUSTOM_COND_p cond) {
    cond->state = 0;
    cond->waiting = construct_FIFOq();
    initializeFIFOq(cond->waiting);
}

// Helpwer method to see it the passed custom mutex is unlocked
int is_mutex_free(CUSTOM_MUTEX_p mutex) {
    if (mutex->owner) return 1;

    return 0;
}

// Performs a linear search for a Producer/Consumer pair using the passed process
CP_PAIR_p getPCPair(PCB_p process) {
    for (int i = 0; i < PRO_CON_MAX; i++) {
        if (cp_pairs[i]->producer == process || cp_pairs[i]->consumer == process) {
            return cp_pairs[i];
        }
    }

    return NULL;
}

// Performs a linear search to see if the passed process is a producer
int isProducer(PCB_p process) {
    for (int i = 0; i < PRO_CON_MAX; i++) {
        if (cp_pairs[i]->producer == process) {
            return 1;
        }
    }

    return 0;
}

// Performs a linear search for a Producer/Consumer pair using the passed process
RESOURCE_PAIR_p getResourcePair(PCB_p process) {
    for (int i = 0; i < SHARED_RESOURCE_MAX; i++) {
        if (resource_pairs[i]->process_1 == process || resource_pairs[i]->process_2 == process) {
            return resource_pairs[i];
        }
    }

    return NULL;
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
    // if step is unfinished do not increment pc to avoid skipping steps
    if (processes->runningProcess->step_finished == STEP_UNFINISHED) return STEP_UNFINISHED;
    
    // Otherwise step is finished, increment to next step and set to unfinished
    processes->runningProcess->step_finished = STEP_UNFINISHED;
    currentPC++;

    // Reset PC if past maximum PC
    if(currentPC > getMaxPC(processes->runningProcess)) {
        currentPC = 0;
        setTermCount(processes->runningProcess, getTermCount(processes->runningProcess)+1);
    }

    // Simulate while check for consumer/producer pairs
    if (getType(processes->runningProcess) == CONPRO_PAIR) {
        CP_PAIR_p pair = getPCPair(processes->runningProcess);
        int is_producer = (processes->runningProcess == pair->producer);
        int isAtWhile = 0;

        for (int i = 0; i < SYNCRO_SIZE; i++) {
            if (processes->runningProcess->wait_1_pcs[i] == currentPC) {
                isAtWhile = 1;
                printf("While Detected @ PC %d, Filled: %d ", currentPC, pair->filled);
                break;
            }
        }

        // If while condition is false don't fall unto while (skip a PC)
        // Producer: while(filled == FILLED)
        // Consumer: while(filled == EMPTY)
        if (isAtWhile) {
            if (is_producer && pair->filled == EMPTY) {
                printf(" and Producer skipped");
                currentPC++;
            } else if (!is_producer && pair->filled == FILLED) {
                printf(" and Consumer skipped");
                currentPC++;
            }

            printf("\n");
        }

        
    }

    return SUCCESSFUL;
}



// Moves proceeses from the new queue to the ready queue
int moveNewToReady() {
    int i = 0;
    int newInQ = nodeCount(processes->newProcesses);
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
        i++;
    }

    printf("Moved %d processes.  %d detected in new queue.", i, newInQ);
}

int countAllNodes() {
    int total = 1; // Running Process

    total += getTotalNodesCount(processes->readyProcesses);
    total += nodeCount(processes->newProcesses);
    total += nodeCount(processes->IO_1_Processes);
    total += nodeCount(processes->IO_2_Processes);

    for (int i = 0; i < total_cp_pairs; i++) {
        total += nodeCount(cp_pairs[i]->mutex->blocked);
        total += nodeCount(cp_pairs[i]->produced->waiting);
        total += nodeCount(cp_pairs[i]->consumed->waiting);
    }

    for (int i = 0; i < total_resource_pairs; i++) {
        total += nodeCount(resource_pairs[i]->mutex_1->blocked);
        total += nodeCount(resource_pairs[i]->mutex_2->blocked);
    }
    return total;
}

// Updated for problem 4
// Prints a message showing the current state of the ready queue
int printInterupt(int interupt) {
    char buffer[MAX_BUFFER_SIZE];
    buffer[0] = '\0';
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
	case LOCK_INTERRUPT:
		printf("\nLock Trap Call @ Iteration: %d\n", iterationCount);
		break;
	case UNLOCK_INTERRUPT:
		printf("\nUnlock Trap Call @ Iteration: %d\n", iterationCount);
		break;
	case SIGNAL_INTERRUPT:
		printf("\nSignal Trap Call @ Iteration: %d\n", iterationCount);
		break;
	case WAIT_INTERRUPT:
		printf("\nWait Trap Call @ Iteration: %d\n", iterationCount);
		break;
	default:
		printf("\nUnknown Interrupt %d @ Iteration: %d\n", interupt, iterationCount);
		break;
    }
    printf("Running PCB: PID:%d @ PC %d\n", getPid(processes->runningProcess), currentPC);
    pQSizeToString(processes->readyProcesses, buffer, MAX_BUFFER_SIZE);
    printf(buffer);
    printf("IO1: %d\n", nodeCount(processes->IO_1_Processes));
    printf("IO2: %d\n", nodeCount(processes->IO_2_Processes));
	printf("Blocked: %d\n", total_blocked);
	printf("Waiting: %d\n", total_waiting);
    printf("Type Counts: Comp %d, IO %d, CP Pairs %d, Shared Resource %d\n", 
            total_comp_processes, total_io_processes, total_cp_pairs, total_resource_pairs);
    printf("Total Nodes: %d\n\n", countAllNodes());
}

int getInterruptType(int flag) {
	switch (flag)	{
	case LOCK_RESOURCE_1:
	case LOCK_RESOURCE_2:
		return LOCK_INTERRUPT;
		break;
	case UNLOCK_RESOURCE_1:
	case UNLOCK_RESOURCE_2:
		return UNLOCK_INTERRUPT;
		break;
	case WAIT_RESOURCE_1:
		return WAIT_INTERRUPT;
		break;
	case SIGNAL_RESOURCE_1:
		return SIGNAL_INTERRUPT;
		break;
	default:
		break;
	}

	return -1;
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

int initialize_test_resource_pairs() {
	createSharedResourcePair();
	resource_pairs[(total_resource_pairs - 1)]->mutex_1->owner = resource_pairs[(total_resource_pairs - 1)]->process_2;
	resource_pairs[(total_resource_pairs - 1)]->mutex_2->owner = resource_pairs[(total_resource_pairs - 1)]->process_1;
	Node_p pro_node = construct_Node();
	initializeNode(pro_node);
	setNodePCB(pro_node, resource_pairs[(total_resource_pairs - 1)]->process_1);
	Node_p con_node = construct_Node();
	initializeNode(con_node);
	setNodePCB(con_node, resource_pairs[(total_resource_pairs - 1)]->process_2);
	q_enqueue(resource_pairs[(total_resource_pairs - 1)]->mutex_1->blocked, pro_node);
	q_enqueue(resource_pairs[(total_resource_pairs - 1)]->mutex_2->blocked, con_node);
}

void destruct_Resource_Pair(RESOURCE_PAIR_p the_pair) {
	destruct_Custom_Mutex(the_pair->mutex_1);
	destruct_Custom_Mutex(the_pair->mutex_2);
	free(the_pair);
}

void destruct_CP_Pair(CP_PAIR_p pair) {
	destruct_Custom_Mutex(pair->mutex);
	destruct_Custom_Cond(pair->produced);
	destruct_Custom_Cond(pair->consumed);
	free(pair);
}

void destruct_Custom_Mutex(CUSTOM_MUTEX_p mutex) {
	destruct_FIFOq(mutex->blocked);
	free(mutex);
}

void destruct_Custom_Cond(CUSTOM_COND_p cond) {
	destruct_FIFOq(cond->waiting);
	free(cond);
}

void check_for_deadlock() {

	int* deadlocks = (int*)malloc(sizeof(int) * SHARED_RESOURCE_MAX);
	int deadlock_amount = 0;
	int i = 0;

	for (int j = 0; j < SHARED_RESOURCE_MAX; j++) {
		deadlocks[j] = 0;
	}

    printf("\nChecking for Deadlocks. . .\n");

	testResourcePairs(resource_pairs, deadlocks, total_resource_pairs);
	for (i = 0; i < total_resource_pairs; i++) {
		if (deadlocks[i] == DEADLOCK_FOUND) {
			printf("Deadlock Found between  PID: %d and PID: %d\n", resource_pairs[i]->process_1->pid, resource_pairs[i]->process_2->pid);

			printf("Process %s PID: %d\n", resource_pairs[i]->process_1_name, resource_pairs[i]->process_1->pid);
			printf("Process %s PID: %d\n", resource_pairs[i]->process_2_name, resource_pairs[i]->process_2->pid);

			printf("Mutex 1 owner PID: %d\n", resource_pairs[i]->mutex_1->owner->pid);
			printf("Mutex 2 owner PID: %d\n", resource_pairs[i]->mutex_2->owner->pid);

			destruct_Resource_Pair(resource_pairs[i]);
			resource_pairs[i] = resource_pairs[total_resource_pairs - 1];
			deadlocks[i] = deadlocks[total_resource_pairs - 1];
			deadlock_amount++;
			total_resource_pairs--;
            total_blocked -= 2; //decrement by both processes in pair from total blocked
			i--;
		}
	}

    total_deadlock_pairs += deadlock_amount;

    if (deadlock_amount > 0) {
        printf("Found %d Deadlocks.  Deadlocked processes terminated.\n", deadlock_amount);
    } else {
        printf("No Deadlocks detected.\n");
    }

}

void free_syncro_service_processes() {
	for (int i = 0; i < total_resource_pairs; i++) {
		destruct_Resource_Pair(resource_pairs[i]);
	}

	for (int i = 0; i < total_cp_pairs; i++) {
		destruct_CP_Pair(cp_pairs[i]);
	}
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
    sysStack = createSimpleStack();
	currentPC = 0;
    iterationCount = 0;
    quantum_post_reset = 0;
    IOSR_1_finished = 0;
    IOSR_2_finished = 0;
    IO_1_counter = 0;
    IO_2_counter = 0;
    IO_1_activated = 0;
    IO_2_activated = 0;
    total_cp_pairs = 0;
	total_resource_pairs = 0;
	total_deadlock_pairs = 0;
	total_terminated = 0;
	total_blocked = 0;
	total_waiting = 0;
    total_comp_processes = 1;
    total_io_processes = 0;
	//TODO: put into the structs to clean up globals?
	con_pro_name_starting_point = 64;
	resource_pair_starting_point = 64;

    timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
    // create starting processes
    // set a process to running
    setState(processes->runningProcess, RUNNING);
    setTerminate(processes->runningProcess, 0);
    setRandomMaxPC(processes->runningProcess);
    trap_flag = 0;
    syncro_flag = 0;

    for (int i = 0; i < INIT_CREATE_CALLS; i++) {
        createNewProcesses();
    }

    // Start OS Thread
    pthread_create(&os, NULL, OS_Simulator, NULL);

    // Wait until the OS Thread completes
    pthread_join(os, NULL);

    // free resources
    freeProcessQueues();
	destructStack(sysStack);
	free_syncro_service_processes();
}