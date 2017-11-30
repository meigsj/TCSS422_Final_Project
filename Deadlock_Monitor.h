/*
TCSS422 - Operating Systems
Final Project

Group Members:
Shaun Coleman
Joshua Meigs
*/

#pragma once
//#include "OS.h" REMOVED
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "FIFOq.h"
#include "pcb.h"
#include "PQueue.h"

#define DEADLOCK_FOUND -1
#define NO_DEADLOCK_FOUND 0
#define MAX_SHARED_RESOURCE_EDGES 4

struct dl_lock_node;

typedef struct dl_proc_node {
	PCB_p process;
	struct dl_lock_node * owns; // set up for resource pairs only; fully generalized would be a list of edges
} DL_PROC_NODE_s;

typedef DL_PROC_NODE_s* DL_PROC_NODE_p;

typedef struct custom_mutex {
	// NULL if no process holds the mutex, otherwise the pointer to the process
	PCB_p owner;
	// A FIFO_q of processes blocked waiting for the mutex
	FIFOq_p blocked;
} CUSTOM_MUTEX_s;

typedef CUSTOM_MUTEX_s* CUSTOM_MUTEX_p;


typedef struct resource_pair {
	// pointers to the processes in the pair
	PCB_p process_1;
	PCB_p process_2;

	// Syncronization vars
	CUSTOM_MUTEX_p mutex_1;
	CUSTOM_MUTEX_p mutex_2;
} RESOURCE_PAIR_s;

typedef RESOURCE_PAIR_s* RESOURCE_PAIR_p;

typedef struct dl_lock_node {
	CUSTOM_MUTEX_p lock;
	DL_PROC_NODE_p waiting; // set up for resource pairs only; fully generalized would be a list of edges
} DL_LOCK_NODE_s;

typedef DL_LOCK_NODE_s* DL_LOCK_NODE_p;

typedef struct proc_list_node {
	DL_PROC_NODE_p proc_node;
	struct proc_list_node * next;
} PROC_LIST_NODE_s;

typedef PROC_LIST_NODE_s* PROC_LIST_NODE_p;

typedef struct lock_list_node {
	DL_LOCK_NODE_p lock_node;
	struct lock_list_node * next;
} LOCK_LIST_NODE_s;
typedef LOCK_LIST_NODE_s* LOCK_LIST_NODE_p;

typedef struct dl_graph {
    PROC_LIST_NODE_p proc_head;
    LOCK_LIST_NODE_p lock_head;
    int edge_count;
} DL_GRAPH_s;

typedef DL_GRAPH_s* DL_GRAPH_p;

DL_GRAPH_p setupDLGraph(RESOURCE_PAIR_p);
void connectDLGraph(DL_GRAPH_p);
void destructDLGraph(DL_GRAPH_p);
int testResourcePair(RESOURCE_PAIR_p);
void testResourcePairs(RESOURCE_PAIR_p*, int*, int);