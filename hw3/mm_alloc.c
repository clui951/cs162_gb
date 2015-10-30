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
	while (curr && (curr->free == 0 && curr->size < size)) {
		*last = curr;
		curr = curr->next;
	}
	return curr;
}

void set_contents(struct s_block *b, struct s_block *next, struct s_block *prev, int free, size_t s) {
	b->next = next;
	b->prev = prev;
	b->free = free;
	b->size = s;
	memset(b->data, 0, s);
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
    		block->free = 0;
    	}
    }
    return block+1;
}

void *mm_realloc(void *ptr, size_t size) {
    /* YOUR CODE HERE */
    mm_free(ptr);
    return mm_malloc(size);
}

void mm_free(void *ptr) {
    /* YOUR CODE HERE */
    struct s_block *block = (struct s_block *) ptr - sizeof(s_block);
	if (block) {
		block->free = 1;
		combine_block(block);
	}
}

// ===========================================================================
// ===========================================================================
// ===========================================================================



/* attempts to split a block, with size_t as first */
void split_block(struct s_block *b, size_t first_size) {
	size_t entireSize = b->size;
	size_t second_size = entireSize - first_size;
	struct s_block * entireblocknext = b->next;
	set_contents(b, b + sizeof(b) + first_size, b->prev, 0, first_size);
	set_contents(b + sizeof(b) + first_size, entireblocknext, b, 1, second_size);
}

/* attempts to combine block with neighbors when free */
struct s_block * combine_block(struct s_block *b) {
	size_t size_just_freed = b->size;
	if (b->next) {
		if (b->next->free == 1) {
			size_t size_in_front = b->next->size;
			set_contents(b, b->next->next, b->prev, 1, size_just_freed + size_in_front);
		}
	}
	if (b->prev) {
		if (b->prev->free == 1) {
			set_contents(b->prev, b->next, b->prev->prev, 1, b->prev->size + b->size);
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
		block->prev = prev_end;
	}
	set_contents(block, NULL, prev_end, 0, s);

	return block;
}


















