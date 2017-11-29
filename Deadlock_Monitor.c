/*
TCSS422 - Operating Systems
Final Project

Group Members:
Shaun Coleman
Joshua Meigs
*/

#include "Deadlock_Monitor.h"

DL_GRAPH_p setupDLGraph(RESOURCE_PAIR_p pair) {
    DL_GRAPH_p graph = (DL_GRAPH_p)malloc(sizeof(DL_GRAPH_s));
    graph->proc_head = (PROC_LIST_NODE_p)malloc(sizeof(PROC_LIST_NODE_s));
    graph->lock_head = (LOCK_LIST_NODE_p)malloc(sizeof(LOCK_LIST_NODE_s));
    PROC_LIST_NODE_p proc_next = (PROC_LIST_NODE_p)malloc(sizeof(PROC_LIST_NODE_s));
    LOCK_LIST_NODE_p lock_next = (LOCK_LIST_NODE_p)malloc(sizeof(LOCK_LIST_NODE_s));

    DL_PROC_NODE_p proc_1 = (DL_PROC_NODE_p)malloc(sizeof(DL_PROC_NODE_s));
    DL_PROC_NODE_p proc_2 = (DL_PROC_NODE_p)malloc(sizeof(DL_PROC_NODE_s));
    DL_LOCK_NODE_p lock_1 = (DL_LOCK_NODE_p)malloc(sizeof(DL_LOCK_NODE_s));
    DL_LOCK_NODE_p lock_2 = (DL_LOCK_NODE_p)malloc(sizeof(DL_LOCK_NODE_s));

    proc_1->process = pair->process_1;
    proc_2->process = pair->process_2;
    lock_1->lock = pair->mutex_1;
    lock_2->lock = pair->mutex_2;
    proc_next->proc_node = proc_2;
    lock_next->lock_node = lock_2;

    graph->proc_head->proc_node = proc_1;
    graph->lock_head->lock_node = lock_1;
    graph->proc_head->next = proc_next;
    graph->lock_head->next = lock_next;

    proc_next->next = NULL;
    lock_next->next = NULL;

    graph->edge_count = 0;

	return graph;
}

void connectDLGraph(DL_GRAPH_p graph) {
    PROC_LIST_NODE_p procs = graph->proc_head;

    while (procs) {
        LOCK_LIST_NODE_p locks = graph->lock_head;
        
        while (locks) {
            if (locks->lock_node->lock->owner == procs->proc_node->process) {
                procs->proc_node->owns = locks->lock_node;
                graph->edge_count++;
            }

            if (q_contains(locks->lock_node->lock->blocked, procs->proc_node->process)) {
                locks->lock_node->waiting = procs->proc_node;
                graph->edge_count++;
            }

            locks = locks->next;
        }

        procs = procs->next;
    }
}

void destructDLGraph(DL_GRAPH_p graph) {
    PROC_LIST_NODE_p procs = graph->proc_head;
    PROC_LIST_NODE_p proc_next = graph->proc_head;
    LOCK_LIST_NODE_p locks = graph->lock_head;
    LOCK_LIST_NODE_p lock_next = graph->lock_head;

    while (procs) {
        free(procs->proc_node);
        free(procs);
        procs = proc_next;
        proc_next = proc_next->next;
    }

    while (locks) {
        free(locks->lock_node);
        free(locks);
        locks = lock_next;
        lock_next = lock_next->next;
    }

	free(graph);
}

int testResourcePair(RESOURCE_PAIR_p pair) {
    DL_GRAPH_p graph = setupDLGraph(pair);
    connectDLGraph(graph);
    
    // No more than 4 edges should be made when connecting resource pairs
    assert(graph->edge_count <= MAX_SHARED_RESOURCE_EDGES);
    
    // If 4 edges are created a full cycle must be produced Ex: proc1->lock1->proc2->lock2->proc1...
    if (graph->edge_count  == MAX_SHARED_RESOURCE_EDGES) return DEADLOCK_FOUND;

    return NO_DEADLOCK_FOUND;
}

void testResourcePairs(RESOURCE_PAIR_p* pairs, int* results, int size) {
    for (int i = 0; i < size; i++) {
        results[i] = testResourcePair(pairs[i]);
    }
}