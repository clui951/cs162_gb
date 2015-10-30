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
