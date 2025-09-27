#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **inicializa()
{
    char **tmp;
    int i;

    tmp = malloc(2000 * sizeof(char *));
    for (i = 0; i < 2000; i++)
        tmp[i] = malloc(4 * sizeof(char)); // Reserva espacio para 4 caracteres

    return tmp;
}

void copia(char **buffers)
{
    strcpy(buffers[0], "ASO");
}
void liberamem(char **buffers){ //liberamos la memoria
    for(int i=0; i<2000; i++){
        free(buffers[i]);
    }
}
int main(void)
{
    char **buffers;

    buffers = inicializa();
    copia(buffers);
    printf("%s\n", buffers[0]);
    liberamem(buffers);
    free(buffers);
    return EXIT_SUCCESS;
}