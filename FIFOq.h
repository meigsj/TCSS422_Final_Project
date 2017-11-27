/*
TCSS422 - Operating Systems
Problem 4

Group Members:
Zira Cook
Shaun Coleman
*/

#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pcb.h"

// An error code to denote a null pointer was passed to a member function
#define POINTER_NULL -1

// An return code to denote all pointers passed to a member function were non-null
#define POINTER_VALID 0

// A struct used to represent a linked list node to implement a FIFO queue
typedef struct node{
    // the pcb the node represents
	PCB_p pcb;
	// pointer to the next node in the FIFO queue
	struct node * next;
} Node_s;

// A pointer to a FIFO Node
typedef Node_s * Node_p;

// A struct used to represent a FIFO queue
typedef struct {
	// the head/first node in the FIFO queue
	// null if the queue is empty
    Node_p head;
} FIFOq_s;

// A pointer to a FIFOq struct
typedef FIFOq_s * FIFOq_p;

// Allocates memory for a FIFOq and returns a FIFOq pointer
FIFOq_p construct_FIFOq();
// Allocates memory for a Node and returns a Node pointer
Node_p construct_Node();

// Frees memory used for a FIFOq pointer
int destruct_FIFOq(FIFOq_p);
// Frees memory used for a Node pointer
int destruct_Node(Node_p);

// initalizes a FIFOq to an empty(null) state
int initializeFIFOq(FIFOq_p);
// initializes a Node to an empty state
int initializeNode(Node_p);

// returns a 1 if the queue has member nodes, or 0 if the queue is empty
int q_is_empty(FIFOq_p);
// Adds the specified node to the end of the passed FIFOq
int q_enqueue(FIFOq_p, Node_p);
// Removes a Node from the head of the passed queue, retrieves/returns the stored PCB, and destructs the removed node
PCB_p q_dequeue(FIFOq_p);

// Returns a String describing the state of the passed FIFOq
int qToString(FIFOq_p, char* buffer, int b_size);

// Sets pcb for the specified node
int setNodePCB(Node_p, PCB_p);
// Returns the PCB stored in the given node
PCB_p getNodePCB(Node_p);

// Sets the next node for the specified node
int setNodeNext(Node_p, Node_p);
// Returns the specified nodes next pointer
Node_p getNodeNext(Node_p);

// Returns the head of the specified FIFOq
Node_p getHead(FIFOq_p);

// Returns the tail/end of the specified FIFOq
Node_p getTail(FIFOq_p);

// Sets the head node for the specified FIFOq
int setHead(FIFOq_p, Node_p);

// Counts the current number of nodes in the FIFIq
int nodeCount(FIFOq_p);

// ADDED FOR PROBLEM 4
// A function to set the priority of all contained PCBs to 0
int q_resetPriority(FIFOq_p);

// A function to linearly search the queue to see if the specified pcb is contained
int q_contains(FIFOq_p queue, PCB_p pcb)
