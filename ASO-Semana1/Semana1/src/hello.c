#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
 #include <unistd.h>

int main(void)
{
    printf("Hello, ASO!\n");
    printf("My PID is %d\n", getpid());
    return EXIT_SUCCESS;
}