#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>

void punteros(void)
{
    int a[4];   //memoria estatica (pila)
    int *b = malloc(16);    //memoria dinamica (heap)
    int *c = NULL;
    int i;

    printf("1: a = %p, b = %p, c = %p\n", a, b, c);

    c = a;
    for (i = 0; i < 4; i++)
        a[i] = 100 + i;
    c[0] = 200; //como c apunta a a cambia el valor de a[0]
    printf("2: a[0] = %d, a[1] = %d, a[2] = %d, a[3] = %d\n", a[0], a[1], a[2], a[3]);

    c[1] = 300;
    *(c + 2) = 301;
    printf("3: a[0] = %d, a[1] = %d, a[2] = %d, a[3] = %d\n", a[0], a[1], a[2], a[3]);

    c = c + 1;  //ahora c apunta a una posicion mas de a asi q cambia al valor para a[1]
    *c = 400;
    printf("4: a[0] = %d, a[1] = %d, a[2] = %d, a[3] = %d\n", a[0], a[1], a[2], a[3]);

    c = (int *)((char *)c + 1);
    *c = 500;
    printf("5: a[0] = %d, a[1] = %d, a[2] = %d, a[3] = %d\n", a[0], a[1], a[2], a[3]);

    b = (int *)a + 1;           //entiende q hay q sumar 4 bytes (tamaño d eun entero)
    c = (int *)((char *)a + 1); //ahora solo suma 1 (tamaño del char)
    printf("6: a = %p, b = %p, c = %p\n", a, b, c);
}

int main(int argc, char **argv)
{
    punteros();

    return EXIT_SUCCESS;
}
