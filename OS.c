/*
TCSS422 - Operating Systems
Problem 4

Group Members:
Zira Cook
Shaun Coleman
*/

#include <time.h>
#include "OS.h"

unsigned int sysStack;
unsigned int currentPC;
unsigned int iterationCount;
unsigned int quantum_post_reset;
PCB_p privilegedPCBs[MAX_PRIVILEGED];
unsigned int numPrivileged;

int timer;
int IO_1_counter;
int IO_2_counter;

PROCESS_QUEUES_p processes;

// Updated for Problem 4
// The top level of the OS simulator
int OS_Simulator() {
    char* buffer[MAX_BUFFER_SIZE];
    // Main Loop
    // One cycle is one instruction
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
        
        // Trigger timer and check for timer interupt
        if(timerDownCounter() == TIMER_INTERUPT) {
            int state = RUNNING;
            if(processes->runningProcess) state = getState(processes->runningProcess);
            // Timer interupt
            sysStack = currentPC;
            pseudoISR();
            
            if(state == HALTED) {
                printInterupt(PCB_TERMINATED);
            } else { 
                printInterupt(TIMER_INTERUPT);
            }
        }
        
        // Trigger IO1 counter and check for IO1 interupt
        if(IO_1_DownCounter() == IO_1_INTERUPT && !q_is_empty(processes->IO_1_Processes)) {
            sysStack = currentPC;
            IO_Interupt_Routine(IO_1_INTERUPT);
            printInterupt(IO_1_INTERUPT);
        }
        
        // Trigger IO2 counter and check for IO1 interupt
        if(IO_2_DownCounter() == IO_2_INTERUPT && !q_is_empty(processes->IO_2_Processes)) {
            sysStack = currentPC;
            IO_Interupt_Routine(IO_2_INTERUPT);
            printInterupt(IO_2_INTERUPT);
        }
        
        // Check for Traps (termination is checked as a trap here too)
        trapFlag = isAtTrap(processes->runningProcess);
        if(trapFlag == IO_1_TRAP || trapFlag == IO_2_TRAP || trapFlag == PCB_TERMINATED) {
            sysStack = currentPC;
            pseudoTSR(trapFlag);
            printInterupt(trapFlag);
        }

        
        //check stop condition for the simulation
        if (iterationCount >= HALT_CONDITION) {
            printf("---- HALTING SIMULATION ON ITERATION %d ----\n", iterationCount);
            break;
        }
    }

}

// Function used to simulate an ISR call
int pseudoISR() {
    // set state to interupted if the process was not halted
    if(getState(processes->runningProcess) != HALTED) {
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
    } else {
        setState(processes->runningProcess, WAITING);
    }
    
    // Activate counter if not in use (IO_Queue is empty)
    // IO Traps only
    if (trap_interupt == IO_1_TRAP && q_is_empty(processes->IO_1_Processes)) {
        IO_1_counter = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess)) * (rand() % IO_COUNTER_MULT_RANGE);
    } else if (trap_interupt == IO_2_TRAP && q_is_empty(processes->IO_2_Processes)) {
        IO_2_counter = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess)) * (rand() % IO_COUNTER_MULT_RANGE);
    }
    
    // save pc to pcb
    setPC(processes->runningProcess, currentPC);
    
    // scheduler up call
    scheduler(trap_interupt);
    
    // IRET (update current pc)
    currentPC = sysStack;
    return SUCCESSFUL;  
}

// Updated for problem 4
// Function used to simulate the scheduler in an operating system
int scheduler(int interupt) {
    // move newly created processes to the ready queue
    moveNewToReady();
    
    switch(interupt) {
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
            priority = (priority == MAX_PRIORITY ) ? 0 : priority+1;
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

    for (int i=0; i < IO_TRAP_SIZE; i++) {
        if (currentPC == pcb->io_1_traps[i]) return IO_1_TRAP;
        if (currentPC == pcb->io_2_traps[i]) return IO_2_TRAP;
    }
    
    return 0;
}

// Function used to simulate the creation of new processes
int createNewProcesses() {
    PCB_p pcb = NULL;
    Node_p node;
    for(int i = 0; i < rand() % 5; i++) {
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
    
    if(timer == 0) return TIMER_INTERUPT;
    
    return NO_INTERUPT;
}

// Added for problem 4
// A function to trigger the io1 down counter and return if the io1 interrupt occured
int IO_1_DownCounter() {
    IO_1_counter = IO_1_counter == 0 ? 0 : IO_1_counter-1;
    
    if(IO_1_counter == 0) return IO_1_INTERUPT;
    
    return NO_INTERUPT;
}

// Added for problem 4
// A function to trigger the io2 down counter and return if the io2 interrupt occured
int IO_2_DownCounter() {
    IO_2_counter = IO_2_counter == 0 ? 0 : IO_2_counter-1;
    
    if(IO_2_counter == 0) return IO_2_INTERUPT;
    
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
    
    return SUCCESSFUL;
}

// Moves proceeses from the new queue to the ready queue
int moveNewToReady() {
    // Move to Scheduler
    // set new processes to ready
    while(!q_is_empty(processes->newProcesses)) {
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
    
    // Seed RNG
    srand(time(NULL));
    
    // Initialize Vars
    initializeProcessQueues();
    
    
    // Added for Problem 3
    // SetQuantums
    for (int i = 0; i <= MAX_PRIORITY; i++) {
      // Set Quantum to ten times one more than the priority level squared
      // i.e. Priority 0 -> Quantum 10
      setQuantum(processes->readyProcesses, i, (i+1)*(i+1)*10);
    }
    
    currentPC = 0;
    sysStack = 0;
    iterationCount = 0;
    quantum_post_reset = 0;
    IO_1_counter = 0;
    IO_2_counter = 0;
    timer = getQuantum(processes->readyProcesses, getPriority(processes->runningProcess));   
    
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
    
    // main loop
    OS_Simulator();
    
    // free resources
    freeProcessQueues();
}