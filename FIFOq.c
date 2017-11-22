/*
TCSS422 - Operating Systems
Final Project

Group Members:
Kirtwinder Gulati
Shaun Coleman
Ayub Tiba
Joshua Meigs
*/
#include "FIFOq.h"

// Allocates memory for a FIFOq and returns a FIFOq pointer
FIFOq_p construct_FIFOq(){
    return (FIFOq_p)malloc(sizeof(FIFOq_s));    
}

// Allocates memory for a Node and returns a Node pointer
Node_p construct_Node() {
    return (Node_p)malloc(sizeof(Node_s));
}

// Frees memory used for a FIFOq pointer
int destruct_FIFOq(FIFOq_p queue) {
    if (!queue) return POINTER_NULL;
    
    free(queue);
    
    return POINTER_VALID;
}

// Frees memory used for a Node pointer
int destruct_Node(Node_p node) {
    if (!node) return POINTER_NULL;
    
    free(node);
    
    return POINTER_VALID;
}

// initalizes a FIFOq to an empty(null) state
int initializeFIFOq(FIFOq_p queue) {
    if (!queue) return POINTER_NULL;
    
    queue->head = NULL;
}

// initializes a Node to an empty state
int initializeNode(Node_p node) {
    if (!node) return POINTER_NULL;
    
    node->pcb = NULL;
    node->next = NULL;
}

// returns a 1 if the queue has member nodes, or 0 if the queue is empty
int q_is_empty(FIFOq_p queue) {
    if (!queue) return POINTER_NULL;
    
    if (!queue->head) {
        return 1;
    } else {
        return 0;
    }
}

// Adds the specified node to the end of the passed FIFOq
int q_enqueue(FIFOq_p queue, Node_p node) {
    if (!queue || !node) return POINTER_NULL;
    
    setNodeNext(node, NULL);
 
    if (q_is_empty(queue)) {
        setHead(queue, node);
    } else {
        Node_p head = getHead(queue);
        Node_p next = getNodeNext(head);
      
        while(next) {
            head = next;
            next = getNodeNext(next);
        }
  
        setNodeNext(head, node);
    }
    
    return POINTER_VALID;
}

// Removes a Node from the head of the passed queue, retrieves/returns the stored PCB, and destructs the removed node
PCB_p q_dequeue(FIFOq_p queue) {
    // Check for valid pointers
    if (!queue) return POINTER_NULL;
    
    if (q_is_empty(queue)) return NULL;   
    
    // dequeue head node and replace with next
    Node_p head = getHead(queue);
    Node_p next = getNodeNext(head);
    
    setHead(queue, next);

    // retrieve PCB and destruct Node
    PCB_p pcb = getNodePCB(head);
    destruct_Node(head);
    
    return pcb;    
}

// Returns a String describing the state of the passed FIFOq
int qToString(FIFOq_p queue, char* buffer, int b_size) {
    if (!queue || !buffer) return POINTER_NULL;
    
    char qPIDs[1024];
    char temp[8];
    int totalNodes = 1;
    
    if (!queue) return POINTER_NULL;
    
    if (q_is_empty(queue)) {
           sprintf(buffer, "Q:Count=0: * : contents: queue is empty.");
    } else {
  
        Node_p head = getHead(queue);
        Node_p next = getNodeNext(head);
 
        sprintf(qPIDs, "P%d-", getPid(getNodePCB(head)));


        while(next) {
            totalNodes++;
            head = next;
            next = getNodeNext(next);
            sprintf(temp, ">P%d-", getPid(getNodePCB(head)));
            strcat(qPIDs, temp);
        }

        strcat(qPIDs, "* ");
 
        sprintf(buffer, "Q:Count=%d:", totalNodes);
        strcat(buffer, qPIDs);

    }
    return POINTER_VALID;    
}

// Sets pcb for the specified node
int setNodePCB(Node_p node, PCB_p pcb) {
    if (!node) return POINTER_NULL;
    
    node->pcb = pcb;
    
    return POINTER_VALID;
}

// Returns the PCB stored in the given node
PCB_p getNodePCB(Node_p node) {
    if (!node) return POINTER_NULL;
    
    return node->pcb;
}

// Sets the next node for the specified node
int setNodeNext(Node_p node, Node_p next) {
    if (!node) return POINTER_NULL;
    
    node->next = next;

    return POINTER_VALID;
}

// Returns the specified nodes next pointer
Node_p getNodeNext(Node_p node) {
    if (!node) return POINTER_NULL;
    
    return node->next;
}

// Returns the head of the specified FIFOq
Node_p getHead(FIFOq_p queue) {
    if (!queue) return POINTER_NULL;
    
    return queue->head;
}

// Returns the head of the specified FIFOq
Node_p getTail(FIFOq_p queue) {
    Node_p tempQueue = queue->head;
    while(tempQueue->next != NULL) {
        tempQueue = tempQueue->next;
    }
    return tempQueue;
}

// Sets the head node for the specified FIFOq
int setHead(FIFOq_p queue, Node_p node) {
    if (!queue) return POINTER_NULL;
    
    queue->head = node;
    
    return POINTER_VALID;
}

// returns a count of total nodes in the FIFOq
int nodeCount(FIFOq_p queue) {
	int count = 0;

    Node_p tempQueue = queue->head;
    while(tempQueue != NULL) {
        tempQueue = tempQueue->next;
        count++;
    }
	
	return count;
}

// ADDED FOR PROBLEM 4
// A function to set the priority of all contained PCBs to 0
int q_resetPriority(FIFOq_p queue) {
     Node_p head;
     Node_p next;
     
     if (!queue) return POINTER_NULL;
     
     head = getHead(queue);
     next = getNodeNext(head);
        
     while (head) {
         setPriority(getNodePCB(head), 0);
         head = next;
         next = getNodeNext(next);
     }
     
     return POINTER_VALID;
}
