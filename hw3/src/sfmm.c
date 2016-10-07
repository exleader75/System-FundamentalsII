#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "sfmm.h"

int counter = 0;

/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

sf_free_header* freelist_head = NULL;


void *sf_malloc(size_t size){
	int stuff = 0;
	size_t asize;
	char *bp;
	sf_header* s1;
	sf_footer* s2;
	int padd = 0;
	int test_pad = size;
	char* justin = NULL;

	if(size == 0)
		return NULL;

	if(freelist_head == NULL){ //at the very start
		freelist_head = (sf_free_header*) sf_sbrk(1); //request more memory

		s1 = (sf_header*) freelist_head;
		s1->alloc = 0x0;
		s1->block_size = 4096;
		s1->padding_size = 0;

		freelist_head->header = *s1;
		freelist_head->next = NULL;
		freelist_head->prev = NULL;
		
		justin = (char*) freelist_head;
		justin = justin + 4096 - 8; //go to the footer

		s2 = (sf_footer*) justin;
		s2->alloc = 0x0;
		s2->block_size = 4096;

		//freelist_head is unaffected while we set the freelist_head to one big free block
		
		counter++;
	}

	if(counter == 4){
		errno = ENOMEM;
		return (void*) -1;
	}

	asize = size;

	//if((bp = find_fit(asize)) != NULL){ //find a free block with asize size
		bp = (char*) freelist_head;
		s1 = (sf_header*) bp; //set the header of the free list to allocated
		s1->alloc = 0x1;
		test_pad = asize;
		while(test_pad % 16 != 0){
			test_pad++;
			padd++;
		}

		s1->padding_size = padd;

		stuff = 16 + padd + asize;
		s1->block_size = stuff >> 4;

		//go the the footer
		s2 = (sf_footer*)((char*) (bp + asize + padd + 8));
		printf("%d\n", ((int) asize+padd-8));
		s2->alloc = 0x1;
		s2->block_size = stuff >> 4;

		//freelist_head = s1++;
		
		bp = (void*) bp + 8; //move to the payload
		sf_varprint(bp);
  		return bp;
 //	}

 //	sf_sbrk(1);
 //	counter++;
}

/*
//find the biggest block of memory that will fit the allocated block
//alter the header file by adding a header
static void *find_fit(size_t asize){ //pg 856
	void* bp;

	for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
		if(!GET_ALLOC(HRDP(bp)) && (asize <= GET_SIZE(HRDP(bp)))){
			return bp;
		}
	}
	return NULL;
}
*/
void sf_free(void *ptr){

}

void *sf_realloc(void *ptr, size_t size){
  return NULL;
}

int sf_info(info* meminfo){
  return -1;
}
