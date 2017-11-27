#include "OS.h"

#define DEADLOCK_FOUND -1
#define NO_DEADLOCK_FOUND 1
#define MAX_SHARED_RESOURCE_EDGES 4

typedef struct dl_proc_node {
    PCB_p process;
    CUSTOM_MUTEX_p owns; // set up for resource pairs only; fully generalized would be a list of edges
} DL_PROC_NODE_s;

typedef DL_PROC_NODE_s* DL_PROC_NODE_p;

typedef struct dl_lock_node {
    CUSTOM_MUTEX_p lock;
    PCB_p waiting; // set up for resource pairs only; fully generalized would be a list of edges
} DL_LOCK_NODE_s;

typedef DL_LOCK_NODE_s* DL_LOCK_NODE_p;

typedef struct proc_list_node {
    DL_PROC_NODE_p proc_node;
    DL_PROC_NODE_p next;
} PROC_LIST_NODE_s;

typedef PROC_LIST_NODE_s* PROC_LIST_NODE_p;

typedef struct lock_list_node {
    DL_LOCK_NODE_p lock_node;
    DL_LOCK_NODE_p next;
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