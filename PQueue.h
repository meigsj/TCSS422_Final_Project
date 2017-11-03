/*
TCSS422 - Operating Systems
Problem 4

Group Members:
Zira Cook
Shaun Coleman
*/

#pragma once
#include "FIFOq.h"
#include "pcb.h"

// The maximum priority allowed for a process
#define MAX_PRIORITY 15

// An error code to describe a parameter being outside the valid range
#define OUT_OF_RANGE_ERROR -2

// A Struct reprepresenting a Priority Queue implemented as an array
// storing a head pointer to a FIFO queue were each index is the priority
// level of the processes contained in that FIFOq
typedef struct {
	FIFOq_p queues[MAX_PRIORITY+1];
  // Added in problem 3
  // Used to hold the quantum for each priority in a parallel array
  int quantums[MAX_PRIORITY+1];
} PQueue_s;

typedef PQueue_s * PQueue_p;

// Constructor to set memory for a Priority Queue struct
PQueue_p construct_PQueue();

// Destructor to free memory for the passed PQueue
int destruct_PQueue(PQueue_p);

// Initializes a PQueue by creating an empty FIFOq for each element
int initialize_PQueue(PQueue_p);

// Enqueues a Node into the correct FIFOq based on the 
// priority level of the Node's contained PCB
int p_enqueue(PQueue_p, Node_p);

// Dequeues the head of the highest priority (lowest index)
// FIFOq that contains any nodes
PCB_p p_dequeue(PQueue_p);

// Fills the passed buffer with a String representation of the specified PQueue 
int pQToString(PQueue_p, char*, int);

// Fills the passed buffer with a String representation of the specified PQueue sizes
int pQSizeToString(PQueue_p, char*, int);

// Gets a count of nodes contained in all prioirty levels of the PQueue
int getTotalNodesCount(PQueue_p);

// Gets the size of a specific priority FIFOq in the Priority Queue
int getPrioritySize(PQueue_p, int);

// moves all nodes to the highest (0) priority
int resetPriority(PQueue_p);

// gets the quantum for a specified priority level
int getQuantum(PQueue_p, int);

// sets the quantum for a specified priority level
int setQuantum(PQueue_p, int, int);