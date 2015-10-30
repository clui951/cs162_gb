/*
 * mm_alloc.h
 *
 * Exports a clone of the interface documented in "man 3 malloc".
 */

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* block struct */
struct s_block {
	struct s_block *prev;
	struct s_block *next;
	int free;
	size_t size;
	char data[0];
};

/* attempts to split a block, with size_t as first */
// void split_block(struct s_block *b, size_t s);
// void split_block_safe(struct s_block *b, size_t s);


/* attempts to combine block with neighbors when free */
struct s_block * combine_block(struct s_block *b);

/* add to heap */
struct s_block * extend_heap(struct s_block *prev_end, size_t s);

void set_contents(struct s_block *b, struct s_block *next, struct s_block *prev, int free, size_t s);

void *mm_malloc(size_t size);
void *mm_realloc(void *ptr, size_t size);
void mm_free(void *ptr);




void set_contents(struct s_block *b, struct s_block *next, struct s_block *prev, int free, size_t s) {
	b->next = next;
	b->prev = prev;
	b->free = free;
	b->size = s;
	memset(b->data, 0, s);
}

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
			set_contents_safe(b, b->next->next, b->prev, 1, size_just_freed + size_in_front);
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
	set_contents(block, NULL, prev_end, 0, s);

	return block;
}


/* attempts to split a block, with size_t as first */
// void split_block(struct s_block *b, size_t first_size) {
// 	size_t entireSize = b->size;
// 	size_t second_size = entireSize - first_size;
// 	struct s_block * entireblocknext = b->next;
// 	set_contents(b, b + sizeof(b) + first_size, b->prev, 0, first_size);
// 	set_contents(b + sizeof(b) + first_size, entireblocknext, b, 1, second_size);
// }

// void split_block_safe(struct s_block *b, size_t first_size) {
// 	size_t entireSize = b->size;
// 	size_t second_size = entireSize - first_size;
// 	struct s_block * entireblocknext = b->next;
// 	set_contents_safe(b, b + sizeof(b) + first_size, b->prev, 0, first_size);
// 	set_contents(b + sizeof(b) + first_size, entireblocknext, b, 1, second_size);
// }
