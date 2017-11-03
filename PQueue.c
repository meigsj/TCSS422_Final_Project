/*
TCSS422 - Operating Systems
Problem 4

Group Members:
Zira Cook
Shaun Coleman
*/
#include "PQueue.h"

// Constructor to set memory for a Priority Queue struct
PQueue_p construct_PQueue() {
    return (PQueue_p)malloc(sizeof(PQueue_s));
}

// Destructor to free memory for the passed PQueue
int destruct_PQueue(PQueue_p pqueue) {
    if (!pqueue) return POINTER_NULL;
  
    // free all FIFOqs
    for(int i = 0; i <= MAX_PRIORITY; i++) {
        destruct_FIFOq(pqueue->queues[i]);
    }
  
    free(pqueue);
    return POINTER_VALID;
}

// Initializes a PQueue by creating an empty FIFOq for each element
int initialize_PQueue(PQueue_p pqueue) {
    if (!pqueue) return POINTER_NULL;
  
    FIFOq_p queue;
    
    for (int i = 0; i <= MAX_PRIORITY; i++) {
        pqueue->quantums[i] = 0;
    }
    
    for(int i = 0; i <= MAX_PRIORITY; i++) {
        queue = construct_FIFOq();
        initializeFIFOq(queue);
        pqueue->queues[i] = queue;
    }
 
     
    return POINTER_VALID;
}

// EDITED FOR PROBLEM 4
// Enqueues a Node into the correct FIFOq based on the 
// priority level of the Node's contained PCB
int p_enqueue(PQueue_p pqueue, Node_p node) {
    if (!pqueue || !node) return POINTER_NULL;

    // If process to be enqueued is the idle process destroy the PCB istead
    if (isIdleProcess(getNodePCB(node))) {
        pcbDestruct(getNodePCB(node));
        destruct_Node(node);
        return POINTER_VALID;
    }
    
    int priority = getPriority(getNodePCB(node));
    q_enqueue(pqueue->queues[priority], node);
 
    return POINTER_VALID;
}

// EDITED FOR PROBLEM 4
// Dequeues the head of the highest priority (lowest index)
// FIFOq that contains any nodes
PCB_p p_dequeue(PQueue_p pqueue) {
    PCB_p pcb;
    if (!pqueue) return POINTER_NULL;
    int priority = 0;
    
    // Added for problem 4
    // if pqueue is empty create and return an idle process
    // idle process has no traps, can't die and will be destroyed when a new process can run
    if (getTotalNodesCount(pqueue) == 0) {
        pcb = pcbConstruct();
        initializeAsIdleProcess(pcb);
        return pcb;
    }
    
    while(priority <= MAX_PRIORITY && q_is_empty(pqueue->queues[priority])) {
        priority++;
    }
    
    return q_dequeue(pqueue->queues[priority]);
}

// Fills the passed buffer with a String representation of the specified PQueue 
int pQToString(PQueue_p pqueue, char* buffer, int b_size) {
    if (!pqueue || !buffer) return POINTER_NULL;
 
  char output[12000];  // Magic Number
  for(int i = 0; i <= MAX_PRIORITY; i++) {
        qToString(pqueue->queues[i], output, 12000);
    strcat(buffer, output);
        strcat(buffer, "\n");
    output[0] = '\0';
  }
  
    return POINTER_VALID;
}


// Gets the size of a specific priority FIFOq in the Priority Queue
// Helper function created to ease dislaying Queue sizes
int getPrioritySize(PQueue_p pqueue, int priority) {
    int count = 0;
    
    if (!pqueue) return POINTER_NULL;
    
    if (priority < 0 || priority > MAX_PRIORITY) return 0; // Need new error
    
    return nodeCount(pqueue->queues[priority]);
}

// Fills the passed buffer with a String representation of the specified PQueue sizes
// Helper function created to print the priority queue in the format discribed in the problem 3 documentation
int pQSizeToString(PQueue_p pqueue, char* buffer, int b_size) {
    if (!pqueue || !buffer) return POINTER_NULL;
 
    char output[100];  //magic number
    for(int i = 0; i <= MAX_PRIORITY; i++) {
          sprintf(output, "Q%d: %d\n", i, getPrioritySize(pqueue, i));
      strcat(buffer, output);
  }
  
    return POINTER_VALID;
}

// A function to get the total number of nodes in the Priority Queue
// Helper function to ease the check for a specific number of processes for the break condition of the main loop
int getTotalNodesCount(PQueue_p pqueue) {
    int count = 0;
    for(int i = 0; i <= MAX_PRIORITY; i++) {
          count += getPrioritySize(pqueue, i);
    }
    
    return count;
}

// moves all nodes to the highest (0) priority
// Used to perform the reset moving once time S is reached
int resetPriority(PQueue_p pqueue) {
    FIFOq_p currentQ;
    PCB_p pcb;
    Node_p node;
    if (!pqueue) return POINTER_NULL;
    
    for (int i = 1; i <= MAX_PRIORITY; i++) {
        currentQ = pqueue->queues[i];
        while (!q_is_empty(currentQ)) {
            pcb = q_dequeue(currentQ);
            setPriority(pcb, 0);
            node = construct_Node();
            initializeNode(node);
            setNodePCB(node, pcb);
            p_enqueue(pqueue, node);
        }
    }
}

// gets the quantum for a specified priority level
// Create to interact witht he new quantums
int getQuantum(PQueue_p pqueue, int priority) {
    if (!pqueue) return POINTER_NULL;
    if (priority < 0 || priority > MAX_PRIORITY) return OUT_OF_RANGE_ERROR;
    
    return pqueue->quantums[priority];
}

// sets the quantum for a specified priority level
// Create to interact witht he new quantums
int setQuantum(PQueue_p pqueue, int priority, int quantum) {
    if (!pqueue) return POINTER_NULL;
    if (priority < 0 || priority > MAX_PRIORITY) return OUT_OF_RANGE_ERROR;
    if (quantum < 0) return OUT_OF_RANGE_ERROR;
    
    pqueue->quantums[priority] = quantum;
    
}