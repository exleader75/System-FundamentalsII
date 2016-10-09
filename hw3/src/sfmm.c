#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "sfmm.h"
#include "sfmm2.h"

int counter = 0;

/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

sf_free_header* freelist_head = NULL;
sf_free_header* top_ptr = NULL;
sf_footer* freelist_foot = NULL;


void *sf_malloc(size_t size){
	//IF THERE IS LESS THAN 32 BYTES OF BLOCK LEFT, USE ALL THE FREE SPACE!!!!!!!!!!
	int stuff = 0;
	size_t asize;
	char *bp;
	sf_header* s1;
	sf_footer* s2;
	int padd = 0;
	int test_pad = size;
	char* justin = NULL;

	if(size == 0 || size > (4096 * 4)){
		return NULL;
	}

	if(freelist_head == NULL){ //at the very start
		freelist_head = (sf_free_header*) sf_sbrk(1); //request more memory
		top_ptr = freelist_head;

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
		s2->alloc = 0x1;
		s2->block_size = stuff >> 4;
		
		bp = (void*) bp + 8; //move to the payload
		sf_varprint(bp);

		//edit the free block
		freelist_head = (sf_free_header*)((char*) (bp + asize + padd + 8));
		//the size of the free block left
  		return bp;
 //	}

 //	sf_sbrk(1);
 //	counter++;
}

void sf_free(void *ptr){ 
	if(ptr == NULL){
		printf("TEST ERROR: FREEING A NULL\n");
	}

	ptr = ptr - 8; //go to the header
	coalesce(ptr);

}

void coalesce(void* ptr){
	//return an error if you try to free null, middle of allication block, 
	sf_free_header* cal_ptr = (sf_free_header*) ptr; //cal_ptr holds the header address of the affected block
	uint64_t size = cal_ptr->header.block_size; //size holds the size of the affected block 
	size = size << 4;
	sf_footer* foot = ((void*) cal_ptr) + (size) - 8; //foot gets the footer

	if((sf_header*)cal_ptr == sf_sbrk(0) && ((sf_footer*)(((void*) foot) + 8))->alloc == 1){
		printf("CASE 1\n");
		//[M*][M][F]
		cal_ptr->header.alloc = 0x0;
		foot->alloc = 0x0;

		sf_free_header* temp = freelist_head;
		freelist_head = cal_ptr;
		cal_ptr->next = temp;
		temp->prev = cal_ptr;
		freelist_foot = foot;
		/*
		while(freelist_head != NULL){
			printf("CASE 1\n");
			sf_blockprint(freelist_head);
			freelist_head = freelist_head->next;
		}
		*/
	}
	else if(((sf_header*)(((void*)cal_ptr)-8))->alloc ==0 && ((sf_footer*)(((void*) foot) + 8))-> alloc == 1){
		//[F][M*][M]
		printf("CASE 2\n");
		cal_ptr->header.alloc = 0x0;
		foot->alloc = 0x0;

		cal_ptr = ((void*) cal_ptr) - 8;
		uint64_t b_move = (cal_ptr->header.block_size) << 4;
		cal_ptr = ((void*) cal_ptr) - (b_move - 8); //the -8 included the header

		cal_ptr->header.alloc = 0x0;
		cal_ptr->header.block_size = (b_move + size) >> 4;

		foot->alloc = 0x0;
		foot->block_size = (b_move + size) >> 4;

		freelist_head = cal_ptr;
		freelist_foot = foot;

		/*
		while(freelist_head != NULL){
			printf("CASE 2\n");
			sf_blockprint(freelist_head);
			freelist_head = freelist_head->next;
		}*/
	}
	else if(((sf_header*)(((void*) cal_ptr) - 8))->alloc == 1 && ((sf_footer*)(((void*) foot) + 8))->alloc == 0){
		//[M][M*][F]
		printf("CASE 3\n");
		cal_ptr->header.alloc = 0x0;
		foot->alloc = 0x0;

		sf_free_header* head_check = (void*) foot;
		head_check = ((void*) head_check) + 8;

		foot = ((void*) foot) + (head_check->header.block_size << 4);
		foot->block_size = ((head_check->header.block_size << 4) + (cal_ptr->header.block_size << 4)) >> 4;

		cal_ptr->header.block_size = foot->block_size;
		freelist_head = ((void*) cal_ptr);
		freelist_head->next = head_check->next;
		freelist_foot = foot;

		/*
		while(freelist_head != NULL){
			printf("CASE 3\n");
			sf_blockprint(freelist_head);
			freelist_head = freelist_head->next;
		}*/
	}
	else if(((sf_header*)(((void*)cal_ptr) - 8))->alloc == 0 && ((sf_footer*)(((void*) foot) + 8))->alloc == 0){
		//[F][M*][F]
		printf("CASE 4\n");
		cal_ptr->header.alloc = 0x0;
		foot->alloc = 0x0;

		sf_free_header* head_check = cal_ptr;
		uint64_t b_move = (cal_ptr->header.block_size) << 4;
		head_check = (void*) head_check + b_move;

		cal_ptr = ((void*) cal_ptr) - 8;
		uint64_t b_moveheader = (cal_ptr->header.block_size) << 4;
		cal_ptr = ((void*) cal_ptr) - (b_moveheader - 8);

		foot = ((void*) foot) + 8;
		uint64_t b_movefooter = (foot->block_size) << 4;
		foot = ((void*) foot) + (b_movefooter - 8);

		cal_ptr->header.alloc = 0x0;
		cal_ptr->header.block_size = (b_movefooter + b_moveheader + size) >> 4;

		foot->alloc = 0x0;
		foot->block_size = (b_movefooter + b_moveheader + size) >> 4;

		freelist_head = cal_ptr;
		freelist_head->next = head_check->next;
		freelist_foot = foot;

		/*
		while(freelist_head != NULL){
			printf("CASE 4\n");
			sf_blockprint(freelist_head);
			freelist_head = freelist_head->next;
		}*/
	}
	else if(((sf_header*)(((void*)cal_ptr) -8))->alloc == 1 && ((sf_footer*)(((void*)foot)+8))->alloc == 1){
		//[M][M*][M]
		printf("CASE 5\n");
		cal_ptr->header.alloc = 0x0;
		foot->alloc = 0x0;

		sf_free_header* temp = freelist_head;
		freelist_head = cal_ptr;
		cal_ptr->next = temp;
		temp->prev = cal_ptr;
		freelist_foot = foot;

		/*
		while(freelist_head != NULL){
			printf("CASE 5\n");
			sf_blockprint(freelist_head);
			freelist_head = freelist_head->next;
		}*/
	}
}

void *sf_realloc(void *ptr, size_t size){
  return NULL;
}

int sf_info(info* meminfo){
  return -1;
}
