#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int opt, t_flag = 0, x_num = 0, y_num = 0;
    char *s_str = NULL;

    optind = 1;
    while ((opt = getopt(argc, argv, "tx:s:y:")) != -1)
    {
        switch (opt)
        {
        case 't':
            t_flag = 1;
            break;
        case 'x':
            x_num = atoi(optarg);
            break;
        case 's':
            s_str = optarg;
            break;
        case 'y':
            y_num = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Uso: %s [-t] [-x NUMERO] [-s CADENA] [-y NUMERO]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    // No se deben admitir parámetros adicionales
    if (optind < argc) {
        fprintf(stderr, "Uso: %s [-t] [-x NUMERO] [-s CADENA] [-y NUMERO]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("t: %d, x: %d, s: \"%s\", y: %d\n", t_flag, x_num, s_str ? s_str : "", y_num);
    return EXIT_SUCCESS;
}