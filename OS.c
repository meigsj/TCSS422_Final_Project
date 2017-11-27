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
#include <assert.h>
#include "OS.h"
#include <assert.h>
unsigned int sysStack;
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

//Tests
pthread_mutex_t timer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timer_cond = PTHREAD_COND_INITIALIZER;

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
        }
        //In the CPU loop use the non-blocking mutex_trylock() call so that the loop doesn't block itself waiting for the timer signal
        //// TO REMOVE AND REPLACE WITH CHECK CONDITION FOR TIMER INTERUPT
        // Trigger timer and check for timer interupt
         //WAS: timerDownCounter() == TIMER_INTERUPT
        timer_check();

        // Trigger IO counters and check for IO interupts
        IO_check();

        // Check for Traps (termination is checked as a trap here too)
       
        trapFlag = isAtTrap(processes->runningProcess);
        if (trapFlag == IO_1_TRAP || trapFlag == IO_2_TRAP || trapFlag == PCB_TERMINATED) {
            printf("\nTrap Detected!\n");
            sysStack = currentPC;
            pseudoTSR(trapFlag);
            printInterupt(trapFlag);
        }
        
        //check stop condition for the simulation
        if (iterationCount >= HALT_CONDITION) {
            printf("---- HALTING SIMULATION ON ITERATION %d ----\n", iterationCount);
            pthread_mutex_lock(&global_shutdown_lock);
            shutting_down = 1;
            pthread_mutex_unlock(&global_shutdown_lock);
            break;
        }
        
    }

}


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
    ts.tv_nsec = IO_FREQ;
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
        ts.tv_nsec = IO_FREQ;
    }
}

void * io2_thread(void * s) {

    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 10000;//(timer*1000);
     // Lock signal mutex
    pthread_mutex_lock(&IO_2_lock);

    for (;;) {
        //// Lock signal mutex
        //pthread_mutex_lock(&IO_2_lock);

        pthread_mutex_lock(&global_shutdown_lock);

        // Check if the program needs to shut down
        while (shutting_down) {
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

void timer_check() {
    if (pthread_mutex_trylock(&timer_lock) == 0) {
        printf("\nTimer Interrupt Detected!\n");
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
    }
}

void IO_check() {
    if (IO_1_activated && pthread_mutex_trylock(&IO_1_lock) == 0) {
        printf("\nIO1 Interrupt Detected!\n");
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

    if (IO_2_activated && pthread_mutex_trylock(&IO_2_lock) == 0) {
        printf("\nIO2 Interrupt Detected!\n");
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
}

// Function used to simulate an ISR call
int pseudoISR() {
    PCB_p running = processes->runningProcess;
    int state = getState(running);

    // set state to interupted if the process was not halted
    if (state != HALTED || state != WAITING) {
        setState(running, INTERRUPTED);
    }

    if (running) setPC(running, currentPC);

    // scheduler up call
    scheduler(TIMER_INTERUPT, NULL);

    // Update Timer
    timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
    // IRET (update current pc)
    currentPC = sysStack;
    return SUCCESSFUL;
}

// Added for problem 4
// A function to simulate a an IO Interrupt routine
int IO_Interupt_Routine(int IO_interupt) {
    if (getState(processes->runningProcess) == RUNNING)
        setState(processes->runningProcess, INTERRUPTED);

    // save pc to pcb
    setPC(processes->runningProcess, currentPC);

    timer_check();

    // scheduler up call
    scheduler(IO_interupt, NULL);

    // IRET (update current pc)
    currentPC = sysStack;
    return SUCCESSFUL;
}

// Added for problem 4
// A function to simulate a TSR
int pseudoTSR(int trap_interupt) {
    PCB_p running = processes->runningProcess;
    setPC(running, currentPC);
    if (trap_interupt == PCB_TERMINATED) {
        setState(running, HALTED);
        setTermination(running, time(NULL));
    } else {
        setState(running, WAITING);
    }
    
    timer_check();
    IO_check();

    // Activate counter if not in use (IO_Queue is empty)
    // IO Traps only
    // TODO - REMOVE
    if (trap_interupt == IO_1_TRAP && q_is_empty(processes->IO_1_Processes)) {
        IO_1_counter = getQuantum(processes->readyProcesses, getPriority(running)) * (rand() % IO_COUNTER_MULT_RANGE);
    } else if (trap_interupt == IO_2_TRAP && q_is_empty(processes->IO_2_Processes)) {
        IO_2_counter = getQuantum(processes->readyProcesses, getPriority(running)) * (rand() % IO_COUNTER_MULT_RANGE);
    }

    // scheduler up call
    scheduler(trap_interupt, running);

    // activate IO devices as needed
    pthread_mutex_lock(&IO_1_active_lock);
    if (q_is_empty(processes->IO_1_Processes)) {
        IO_1_activated = 0;
    } else {
        IO_1_activated = 1;
        pthread_cond_signal(&IO_1_active_cond);
    }
    pthread_mutex_unlock(&IO_1_active_lock);

    pthread_mutex_lock(&IO_2_active_lock);
    if (q_is_empty(processes->IO_2_Processes)) {
        IO_2_activated = 0;
    } else {
        IO_2_activated = 1;
        pthread_cond_signal(&IO_2_active_cond);
    }
    pthread_mutex_unlock(&IO_2_active_lock);

    trap_flag = 0;
    // IRET (update current pc)
    currentPC = sysStack;
    return SUCCESSFUL;
}

// Updated for problem 4
// Function used to simulate the scheduler in an operating system
int scheduler(int interupt, PCB_p running) {
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
int dispatcherTrap(FIFOq_p IO_Queue, PCB_p running) {
    Node_p node = construct_Node();
    initializeNode(node);

    // update context
    setPC(running, sysStack);

    // enqueue
    setNodePCB(node, running);
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

    if (!pcb->io_1_traps[0] && !pcb->io_2_traps[0]) return 0; // MAGIC NUMBER

    for (int i=0; i < IO_TRAP_SIZE; i++) {
        if (currentPC == pcb->io_1_traps[i]) return IO_1_TRAP;
        if (currentPC == pcb->io_2_traps[i]) return IO_2_TRAP;
    }
    
    return 0;
}

// Function used to simulate the creation of new processes
int createNewProcesses() {
    int newIO = rand() % CREATE_IO_PROCESS_MAX;
    int newComp = rand() % CREATE_CUMPUTE_PROCESS_MAX;
    int newCP = rand() % CREATE_PRO_CON_MAX;
    int newShared = rand() % CREATE_SHARED_RESOURCE_MAX;
    printf("\n Trying to create: %d IO, %d Comp, %d CP Pairs, %d Shared Pairs\n", newIO, newComp, newCP, newShared);

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

    // give PCB random Max PC, and Traps
    setRandomMaxPC(producer);
    setRandomMaxPC(consumer);
    
    // Set to not terminate
    setTerminate(producer, 0);
    setTerminate(consumer, 0);

    // initalize CP_PAIR
    pair = (CP_PAIR_p)malloc(sizeof(CP_PAIR_s));
    initialize_CP_Pair(pair);
    pair->producer = producer;
    pair->consumer = consumer;
    //pair->producer = producer;
    //pair->consumer = consumer;
    
    cp_pairs[total_cp_pairs] = pair;
    total_cp_pairs++;

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
    Node_p pro_node;
    Node_p con_code;
    RESOURCE_PAIR_p pair;

    if (total_resource_pairs >= SHARED_RESOURCE_MAX) return 1; // MAGIC NUMBER

    process_1 = pcbConstruct();
    process_2 = pcbConstruct();
    initialize_pcb(process_1);
    initialize_pcb(process_2);

    // Mark each processes as a resource pair
    setType(process_1, RESOURCE_PAIR);
    setType(process_2, RESOURCE_PAIR);

    setRandomMaxPC(process_1);
    setRandomMaxPC(process_2);

    // Set to not terminate
    setTerminate(process_1, 0);
    setTerminate(process_2, 0);

    // initalize RESOURCE_PAIR
    pair = (RESOURCE_PAIR_p)malloc(sizeof(RESOURCE_PAIR_s));
    initialize_Resource_Pair(pair);
    pair->process_1 = process_1;
    pair->process_2 = process_2;
    resource_pairs[total_resource_pairs] = pair;
    total_resource_pairs++;

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
    currentPC++;
    // need to check that max_pc is correct
    // setters/getters inside of pcb maybe?
    if(currentPC > getMaxPC(processes->runningProcess)) {
        currentPC = 0;
        setTermCount(processes->runningProcess, getTermCount(processes->runningProcess)+1);
    }
    
    // TODO simulate counter and possibly syncro services

    return SUCCESSFUL;
}

int simulate_mutex_lock(PCB_p process, CUSTOM_MUTEX_p mutex) {
    if (!mutex->owner) {
        mutex->owner = process;
    } else {
        Node_p node = construct_Node();
        initializeNode(node);
        setNodePCB(node, process);
        //Added node here so that p_enque would have a node passed in and not the PCB
        // TODO: Need to be done in scheduler?
        setState(process, WAITING);
        q_enqueue(mutex->blocked, node);
    }
}

int simulate_mutex_unlock(CUSTOM_MUTEX_p mutex) {
    // TODO check for correctness
    if (!mutex->owner && !q_is_empty(mutex->blocked)) {

        // TODO: Need to be done in scheduler?
        mutex->owner = q_dequeue(mutex->blocked);
        setState(mutex->owner, READY);
        //Added node here so that p_enque would have a node passed in and not the PCB
        Node_p node = construct_Node();
        initializeNode(node);
        setNodePCB(node, mutex->owner);
        p_enqueue(processes->readyProcesses, node);
    } else {
        mutex->owner = NULL;
    }
}

int simulate_cond_wait(PCB_p process, CUSTOM_COND_p cond) {
    // TODO check state var?
    Node_p node = construct_Node();
    initializeNode(node);
    setNodePCB(node, process);
    //Added node here so that p_enque would have a node passed in and not the PCB
    setState(process, WAITING);
    q_enqueue(cond->waiting, node);
}

int simulate_cond_signal(PCB_p process, CUSTOM_COND_p cond) {
    // move next process to ready (or all?)
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
    }
    printf("Running PCB: PID:%d\n", getPid(processes->runningProcess));
    pQSizeToString(processes->readyProcesses, buffer, MAX_BUFFER_SIZE);
    printf(buffer);
    printf("IO1: %d\n", nodeCount(processes->IO_1_Processes));
    printf("IO2: %d\n", nodeCount(processes->IO_2_Processes));
    printf("Type Counts: Comp %d, IO %d, CP Pairs %d, Shared Resource %d\n\n", 
            total_comp_processes, total_io_processes, total_cp_pairs, total_resource_pairs);
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
    total_cp_pairs = 0;
    timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));
    // create starting processes
    // set a process to running
    setState(processes->runningProcess, RUNNING);
    setTerminate(processes->runningProcess, 0);
    setRandomMaxPC(processes->runningProcess);
    setRandomIOTraps(processes->runningProcess);
    trap_flag = 0;

    for (int i = 0; i < INIT_CREATE_CALLS; i++) {
        createNewProcesses();
    }

    // Start OS Thread
    pthread_create(&os, NULL, OS_Simulator, NULL);

    // Wait until the OS Thread completes
    pthread_join(os, NULL);

    // free resources
    freeProcessQueues();
// TODO Add frees for syncro services vars and structs
}