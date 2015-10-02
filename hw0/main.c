#include <stdio.h>
#include <sys/resource.h>

int main() {
    struct rlimit lim;
    getrlimit( RLIMIT_STACK , &lim);
    printf("stack size: %ld\n", (long int) lim.rlim_cur);
    struct rlimit limm;
    getrlimit( RLIMIT_DATA, &limm);
    limm.rlim_cur = 64;
    printf("process limit: %ld\n", (long int) limm.rlim_cur);
    getrlimit( RLIMIT_NOFILE, &lim);
    printf("max file descriptors: %ld\n", (long int) lim.rlim_cur);
    return 0;
}
