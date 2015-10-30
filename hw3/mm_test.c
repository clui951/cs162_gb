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

    // char * data1 = (char *) mm_malloc(10000);
    // data1[1] = "f";

    char *p = (char *) mm_malloc( sizeof(char) * ( 100 + 1 ) );
    p[0] = 101;
    p[1] = 1;
    p[2] = 2;
    p[3] = 3;

    char *q = (char *) mm_realloc(p, sizeof(char) * ( 200 + 1 ));
    
    printf("q[0]: %c\n", q[0]);
    printf("q[1]: %c\n", q[1]);
    printf("q[2]: %c\n", q[2]);

    // printf("the data: %p\n", data1);
    // printf("the block: %p\n", ((struct s_block *) data1) - 1);
    // mm_free(data1);
    // void* data2 = mm_malloc(450);
    // printf("%p\n", data2);
    // void* data3 = mm_malloc(450);
    // printf("%p\n", data3);

    return 0;
}
