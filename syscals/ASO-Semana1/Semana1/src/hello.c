#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    printf("Hello, ASO!\n");
    printf("My PID is %d\n", getpid()); //es una llamada al sistema que devuelve el PID del proceso a traves de la libreria unistd.h
    return EXIT_SUCCESS;
}