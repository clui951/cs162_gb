/**
 * A simple test harness for memory alloction. You should augment this with your
 * own tests.
 */

#include <stdlib.h>
#include <stdio.h>
#include "mm_alloc.h"

int main(int argc, char **argv) {
    // int *data;
    // data = (int *) mm_malloc(4);
    // data[0] = 1;
    // mm_free(data);
    // printf("malloc() basic test passed!\n\n");

    printf("Begin Own Test\n\n");

    void * data1 = mm_malloc(20);
    printf("%p\n", data1);
    mm_free(data1);
    void* data2 = mm_malloc(5);
    printf("%p\n", data2);
    void* data3 = mm_malloc(5);
    printf("%p\n", data3);

    return 0;
}
