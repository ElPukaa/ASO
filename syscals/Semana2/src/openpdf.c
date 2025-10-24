#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char **argv) {// el primer argumento es el nombre del programa y el segundo el archivo pdf
    if (argc != 2) {
        fprintf(stderr, "Uso: %s FILE.pdf\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Comprobar que el archivo existe y es legible
    if (access(argv[1], R_OK) == -1) {
        perror("No se puede acceder al archivo");
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return EXIT_FAILURE;
    }
    if (pid == 0) {// esto es porque tanto el padre como el hijo siguen ejecutando el mismo codigo y hay que distinguirlos
        // Proceso hijo: ejecuta el lector de PDF
        execlp("evince", "evince", argv[1], NULL);
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {
        // Proceso padre: espera a que el hijo termine
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return EXIT_FAILURE;
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return EXIT_FAILURE;
        }
    }
}
