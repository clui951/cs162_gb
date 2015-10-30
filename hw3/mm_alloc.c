/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "mm_alloc.h"

// s_block *first = {NULL, NULL, 1, 0};
// struct s_block * first = (struct s_block *) sbrk(0);
// struct s_block first = {NULL, NULL, 1, 0};

void *global_base = NULL;

struct s_block * free_block_or_nah(struct s_block **last, size_t size) {
	struct s_block *curr = (struct s_block *) global_base;
	while (curr && (curr->free == 0 || curr->size < size)) {
		*last = curr;
		curr = curr->next;
	}
	return curr;
}

// void set_contents(struct s_block *b, struct s_block *next, struct s_block *prev, int free, size_t s) {
// 	b->next = next;
// 	b->prev = prev;
// 	b->free = free;
// 	b->size = s;
// 	memset(b->data, 0, s);
// }

void set_contents_safe(struct s_block *b, struct s_block *next, struct s_block *prev, int free, size_t s) {
	b->next = next;
	b->prev = prev;
	b->free = free;
	b->size = s;
}


struct s_block * combine_block(struct s_block *b) {
	size_t size_just_freed = b->size;
	if (b->next) {
		if (b->next->free == 1) {
			size_t size_in_front = b->next->size;
			set_contents_safe(b, b->next->next, b->prev, 1, size_just_freed + sizeof(struct s_block) + size_in_front);
		}
	}
	if (b->prev) {
		if (b->prev->free == 1) {
			set_contents_safe(b->prev, b->next, b->prev->prev, 1, b->prev->size + b->size);
			return b->prev;
		}
	}
	return b;
}


/* add to heap */
struct s_block * extend_heap(struct s_block *prev_end, size_t s) {
	struct s_block *block = (struct s_block *) sbrk(0);
	void *request = sbrk(sizeof(struct s_block) + s);
	if (request == (void *) -1) {
		return NULL;
	}
	if (prev_end) {
		prev_end->next = block;
		// block->prev = prev_end;
	}
	set_contents_safe(block, NULL, prev_end, 0, s);
	memset(block->data, 0, s);
	return block;
}


/* attempts to split a block, with size_t as first */
// void split_block(struct s_block *b, size_t first_size) {
// 	size_t entireSize = b->size;
// 	size_t second_size = entireSize - first_size - sizeof(struct s_block);
// 	struct s_block * entireblocknext = b->next;
// 	set_contents(b, b + sizeof(struct s_block) + first_size, b->prev, 0, first_size);
// 	set_contents(b + sizeof(struct s_block) + first_size, entireblocknext, b, 1, second_size);
// }

void split_block_safe(struct s_block *b, size_t first_size) {
	size_t entireSize = b->size;
	size_t second_size = entireSize - first_size - sizeof(struct s_block);
	struct s_block * entireblocknext = b->next;
	struct s_block *second_block = b + sizeof(struct s_block) + first_size;
	set_contents_safe(b, second_block, b->prev, 0, first_size);
	memset(b->data, 0, first_size);

	second_block->free = 1;
	second_block->next = entireblocknext;
	second_block->prev = b;
	second_block->size = second_size;
	if (second_block != NULL) {

	}

	// set_contents_safe(b + sizeof(struct s_block) + first_size, entireblocknext, b, 1, second_size);
}


// ===========================================================================
// ===========================================================================
// ===========================================================================
void *mm_malloc(size_t size) {
    /* YOUR CODE HERE */
    printf("entered mm_malloc\n");
    struct s_block * block;
    if (size <= 0 ) {
    	return NULL;
    }
    if (!global_base) {
    	// first time malloc
    	printf("    first time mm_malloc\n");
		block = extend_heap(NULL, size);
    	if (!block) {
    		return NULL;
    	} else {
    		global_base = block;
    	}
    } else {
    	struct s_block *last = (struct s_block *) global_base;
    	block = free_block_or_nah(&last, size);
    	if (!block) {
    		printf("    need to extend_heap\n");
    		block = extend_heap(last, size);
    		if (!block) {
    			return NULL;
    		}
    	} else {
    		if (block->size == size) {
    			memset(block->data, 0, block->size);
    			block->free = 0;
    		} else {
    			// need to split block
    			printf("    should split here\n");
    			split_block_safe(block, size);
	    		memset(block->data, 0, block->size);
	    		printf("    %zu\n", block->size);
	    		block->free = 0;
    		}
    	}
    }
    return block->data;
}

void *mm_realloc(void *ptr, size_t size) {
    /* YOUR CODE HERE */

	if (!ptr) {
		return mm_malloc(size);
	}
	struct s_block * block = ((struct s_block *) ptr) - 1;
	// if (block->size == size) {
	// 	//dont need to split
	// 	return block;
	// }
	// if (block->size > size) {
		// need to check split
		// void *new_ptr = mm_malloc(size);
		// if (!new_ptr){
			// split block
			// return block;
		// } else {
			// memcpy(new_ptr, ptr, block->size);
			// free(ptr);
			// return new_ptr;
		// }
	// }
	void *new_ptr = mm_malloc(size);
	if (!new_ptr){
		return NULL;
	} else {
		if (block->size < size) {
			memcpy(new_ptr, ptr, block->size);
		} else {
			memcpy(new_ptr, ptr, size);
		}
		free(ptr);
		return new_ptr;
	}
}

void mm_free(void *ptr) {
    /* YOUR CODE HERE */
    printf("mm_free-ing, %p\n", ptr);
    if (!ptr) {
    	return;
    }
    struct s_block *block = ((struct s_block *) ptr) - 1;
	// if (block) {
	// memset(block->data, 0 , block->size);
	block->free = 1;
	combine_block(block);
	// }
}

// ===========================================================================
// ===========================================================================
// ===========================================================================




/* attempts to combine block with neighbors when free */

















