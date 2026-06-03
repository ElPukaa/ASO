#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
    int pid, status;

    printf(1, "--- INICIANDO TEST DE ESTADOS POSIX ---\n\n");

    // ==========================================
    // TEST 1: SALIDA NORMAL (exit 42)
    // ==========================================
    pid = fork();
    if (pid == 0) {
        // Código del hijo 1
        exit(42);
    } else {
        // Código del padre
        wait(&status);
        printf(1, "[TEST 1] Hijo termino con exit(42)\n");
        printf(1, "  Status bruto: %d\n", status);
        printf(1, "  WIFEXITED   : %d  (Esperado: 1)\n", WIFEXITED(status));
        printf(1, "  WEXITSTATUS : %d (Esperado: 42)\n", WEXITSTATUS(status));
        printf(1, "  WIFSIGNALED : %d  (Esperado: 0)\n\n", WIFSIGNALED(status));
    }

    // ==========================================
    // TEST 2: MUERTE VIOLENTA (Page Fault / Trap 14)
    // ==========================================
    pid = fork();
    if (pid == 0) {
        // Código del hijo 2
        int *p = (int*)0x0FFFFFFF; // Puntero nulo
        *p = 100;          // Provocamos Trap 14 (Page Fault) intentando escribir
        exit(0);           // Nunca llegará aquí
    } else {
        // Código del padre
        wait(&status);
        printf(1, "[TEST 2] Hijo provoco Page Fault (Trap 14)\n");
        printf(1, "  Status bruto: %d\n", status);
        printf(1, "  WIFEXITED   : %d  (Esperado: 0)\n", WIFEXITED(status));
        printf(1, "  WIFSIGNALED : %d  (Esperado: 1)\n", WIFSIGNALED(status));
        printf(1, "  WEXITTRAP   : %d (Esperado: 14)\n\n", WEXITTRAP(status));
    }

    exit(0);
}