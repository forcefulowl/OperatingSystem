#include <stdio.h>
#include "vm.h"
#include "disk.h"
#include "list.h"
#include "pagetable.h"

struct Node *head = NULL;
int fifo_flag = 0;
int counter = 0;

int random()
{

	int next = rand() % NUM_PAGE;
	return next;
}


int fifo()
{
	int next = 0;
	if(counter < NUM_FRAME)
		next = counter;
	else counter = 0;
	counter++;
	return next;
/*
	int fifo_next = fifo_flag;
	fifo_flag++;
	fifo_flag = fifo_flag % NUM_FRAME;	
	return (fifo_next) % NUM_FRAME;
*/
}


int lru()
{

	if(lru_flag == 1)
	{
		head = list_remove(head, frameIndex);
		head = list_insert_head(head, frameIndex);
	}
	if(lru_flag == 2)
	{
		while(head -> next != NULL)
		{
			head = head -> next;
		}
		frameIndex = head -> data;
		while(head -> prev != NULL)
		{
			head = head -> prev;
		}
		head = list_remove_tail(head);
		head = list_insert_head(head, frameIndex);
		return frameIndex;
	}
	else
	{
		head = list_remove(head, frameIndex);
		head = list_insert_head(head, frameIndex);
	}
	return 0;
}


int clock()
{
	while(clockCounter[fifo_flag] == 1)
	{
		clockCounter[fifo_flag] = 0;
		fifo_flag++;
		fifo_flag = fifo_flag%NUM_FRAME;
	}
	clockCounter[fifo_flag] = 1;
	int fifo_pre = fifo_flag;
	fifo_flag++;

	return fifo_pre;
}


int page_replacement()
{

		int frame;
		if(replacementPolicy == RANDOM)  frame = random(); 
		else if(replacementPolicy == FIFO)  frame = fifo();
		else if(replacementPolicy == LRU) frame = lru();
		else if(replacementPolicy == CLOCK) frame = clock();
		return frame;
}

