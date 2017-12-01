/*
TCSS422 - Operating Systems
Final Project

Group Members:
Shaun Coleman
Joshua Meigs
*/

#include "Simple_Stack.h"

SIMPLE_STACK_p createSimpleStack() {
	SIMPLE_STACK_p stack = (SIMPLE_STACK_p)malloc(sizeof(SIMPLE_STACK_s));
	stack->head = NULL;
	return stack;
}

void ss_push(SIMPLE_STACK_p stack, int value) {
	STACK_NODE_p node = (STACK_NODE_p)malloc(sizeof(STACK_NODE_s));
	node->next = stack->head;
	stack->head = node;
}

int ss_pop(SIMPLE_STACK_p stack) {
	int retVal = stack->head->value;
	STACK_NODE_p next = stack->head->next;
	free(stack->head);
	stack->head = next;

	return retVal;
}

int ss_peek(SIMPLE_STACK_p stack) {
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


