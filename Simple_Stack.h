/*
TCSS422 - Operating Systems
Final Project

Group Members:
Shaun Coleman
Joshua Meigs
Ayub Tiba
Kirtwinder Gulati
*/


#pragma once 
#include <stdlib.h>
#include <assert.h>

typedef struct stack_node {
	int value;
	struct stack_node* next;//changed to add pointer
} STACK_NODE_s;

typedef STACK_NODE_s * STACK_NODE_p;

typedef struct simple_stack {
	STACK_NODE_p head;
	int size;
} SIMPLE_STACK_s;

typedef SIMPLE_STACK_s * SIMPLE_STACK_p;

SIMPLE_STACK_p createSimpleStack();

void ss_push(SIMPLE_STACK_p, int);

int ss_pop(SIMPLE_STACK_p);

int ss_peek(SIMPLE_STACK_p);

int stack_is_empty(SIMPLE_STACK_p);

void destructStack(SIMPLE_STACK_p);

int ss_getSize(SIMPLE_STACK_p);
