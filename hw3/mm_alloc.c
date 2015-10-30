/*
 * mm_alloc.c
 *
 * Stub implementations of the mm_* routines.
 */

#include "mm_alloc.h"
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

// ===========================================================================
// ===========================================================================
// ===========================================================================
void *mm_malloc(size_t size) {
    /* YOUR CODE HERE */
    struct s_block * block;
    if (size <= 0 ) {
    	return NULL;
    }
    if (!global_base) {
    	// first time malloc
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
    		block = extend_heap(last, size);
    		if (!block) {
    			return NULL;
    		}
    	} else {
    		// need to split check

			// size_t entireSize = block->size;
			// size_t second_size = entireSize - size;
			// struct s_block * entireblocknext = block->next;
			// set_contents(block, block + sizeof(struct s_block) + size, block->prev, 0, size);
			// set_contents(block + sizeof(struct s_block) + size, entireblocknext, block, 1, second_size);
    		memset(block, 0, block->size);
    		block->free = 0;
    	}
    }
    return block+1;
}

void *mm_realloc(void *ptr, size_t size) {
    /* YOUR CODE HERE */

	if (!ptr) {
		return mm_malloc(size);
	}
	struct s_block * block = ((struct s_block *) ptr) - 1;
	if (block->size == size) {
		//dont need to split
		return block;
	}
	if (block->size > size) {
		// // split_block_safe(block, size);
		// size_t entireSize = block->size;
		// size_t second_size = entireSize - size;
		// struct s_block * entireblocknext = block->next;
		// set_contents_safe(block, block + sizeof(struct s_block) + size, block->prev, 0, size);
		// set_contents(block + sizeof(struct s_block) + size, entireblocknext, block, 1, second_size);

		return block;
	}
	void *new_ptr = mm_malloc(size);
	if (!new_ptr){
		return NULL;
	} else {
		memcpy(new_ptr, ptr, block->size);
		free(ptr);
		return new_ptr;
	}
}

void mm_free(void *ptr) {
    /* YOUR CODE HERE */
    if (!ptr) {
    	return;
    }
    struct s_block *block = ((struct s_block *) ptr) - 1;
	if (block) {
		memset(block->data, 0 , block->size);
		block->free = 1;
		combine_block(block);
	}
}

// ===========================================================================
// ===========================================================================
// ===========================================================================




/* attempts to combine block with neighbors when free */

















