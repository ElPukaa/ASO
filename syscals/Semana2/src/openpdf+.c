#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s FILE1.pdf [FILE2.pdf ... FILEN.pdf]\n", argv[0]);
        return EXIT_FAILURE;
    }
    // Crear un array para almacenar los PIDs de los procesos hijos
    int max = argc - 1;
    pid_t *pids = malloc(max * sizeof(pid_t));
    if (!pids) {
        perror("malloc");
        return EXIT_FAILURE;
    }

    int launched = 0;
    // Lanzar un proceso por cada archivo PDF
    for (int i = 1; i < argc; i++) {
        const char *file = argv[i];

        if (access(file, R_OK) == -1) {
            fprintf(stderr, "%s: no se puede acceder a '%s': %s\n", argv[0], file, strerror(errno));
            continue; /* no abortar, seguir con los siguientes archivos */
        }

        pid_t pid = fork();
        if (pid == -1) {
            /* Error creando el proceso hijo: informar y seguir con los demás */
            fprintf(stderr, "%s: fork() falló al procesar '%s': %s\n", argv[0], file, strerror(errno));
            continue;
        }

        if (pid == 0) {
            /* Hijo: sustituye su imagen por el lector de PDF */
            execlp("evince", "evince", file, (char *)NULL);
            /* si execlp falla, informar y acabar el hijo */
            fprintf(stderr, "%s: execlp(evince) falló para '%s': %s\n", argv[0], file, strerror(errno));
            _exit(EXIT_FAILURE);
        }

        /* Padre: guardar el PID para esperar más tarde en orden */
        pids[launched++] = pid;
    }

    /* Esperar a los hijos en orden de creación */
    int overall_exit = EXIT_SUCCESS;//para saber si alguno falla
    for (int i = 0; i < launched; i++) {
        int status;
        if (waitpid(pids[i], &status, 0) == -1) {
            fprintf(stderr, "%s: waitpid(%d) falló: %s\n", argv[0], (int)pids[i], strerror(errno));
            overall_exit = EXIT_FAILURE;
            continue;
        }

        if (WIFEXITED(status)) {//comprueba si termina correctamente (por señal de exit)
            int code = WEXITSTATUS(status);//saca el codigo de salida
            if (code != 0) {//si el codigo es distinto de 0 se ha producido un error
                fprintf(stderr, "%s: proceso %d terminó con código %d\n", argv[0], (int)pids[i], code);
                overall_exit = EXIT_FAILURE;
            }
        } else if (WIFSIGNALED(status)) {//comprueba si ha terminado por señal
            int sig = WTERMSIG(status);//saca la señal que ha provocado la terminación
            fprintf(stderr, "%s: proceso %d terminó por señal %d\n", argv[0], (int)pids[i], sig);
            overall_exit = EXIT_FAILURE;
        }
    }

    free(pids);//liberar memoria pids
    return overall_exit;// devolver errores si los hubo
}
