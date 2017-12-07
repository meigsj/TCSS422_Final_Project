/*
TCSS422 - Operating Systems
Final Project

Group Members:
Shaun Coleman
Joshua Meigs
Ayub Tiba
Kirtwinder Singh Gulati
*/
#include "Simple_Stack.h"

SIMPLE_STACK_p createSimpleStack() {
	SIMPLE_STACK_p stack = (SIMPLE_STACK_p)malloc(sizeof(SIMPLE_STACK_s));
	stack->head = NULL;
	stack->size = 0;
	return stack;
}

void ss_push(SIMPLE_STACK_p stack, int value) {
	STACK_NODE_p node = (STACK_NODE_p)malloc(sizeof(STACK_NODE_s));
	node->next = stack->head;
	node->value = value;
	stack->head = node;
	stack->size++;
}

int ss_pop(SIMPLE_STACK_p stack) {
	assert(stack->size != 0);
	int retVal = stack->head->value;
	STACK_NODE_p next = stack->head->next;
	free(stack->head);
	stack->head = next;
	stack->size--;

	return retVal;
}

int ss_peek(SIMPLE_STACK_p stack) {
	assert(stack->size != 0);
	return stack->head->value;
}

int stack_is_empty(SIMPLE_STACK_p stack) {
	if (stack->head) return 0;

	return 1;
}

void destructStack(SIMPLE_STACK_p stack) {
	while (!stack_is_empty(stack)) ss_pop(stack);

	free(stack);
}

int ss_getSize(SIMPLE_STACK_p stack) {
	return stack->size;
}


