/**
 * A simple test harness for memory alloction. You should augment this with your
 * own tests.
 */

#include <stdlib.h>
#include <stdio.h>
#include "mm_alloc.h"

int main(int argc, char **argv) {
    int *data;
    data = (int *) mm_malloc(4);
    data[0] = 1;
    mm_free(data);
    printf("malloc() basic test passed!\n");

    void * data1 = malloc(20);
    printf("%p\n", data1);
    free(data1);
    void* data2 = malloc(10);
    printf("%p\n", data2);
    void* data3 = malloc(10);
    printf("%p\n", data3);

    return 0;
}
