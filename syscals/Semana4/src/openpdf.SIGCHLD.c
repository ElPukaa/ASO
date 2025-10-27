#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

typedef enum { RECARGA = 1, ESPERA = 0 } estado_t;

static estado_t estado = RECARGA;   

void bloquea_signal(int signal) //funcion para bloquear señales
{   
    sigset_t blocked_signals;       //conjunto de señales
    sigemptyset(&blocked_signals);  //vacia el conjunto de señales
    sigaddset(&blocked_signals, signal);    //añade la señal q queremos bloquear
    if (sigprocmask(SIG_BLOCK, &blocked_signals, NULL) == -1) { //si falla el bloqueo de la señal
        perror("sigprocmask()");
        exit(EXIT_FAILURE); //sale con error
    }
}

void instala_manejador_signal(int signal, void (*signal_handler)(int))  //funcion para instalar manejadores de señales
{
    struct sigaction sa;        //estructura para definir el manejador de señales
    memset(&sa, 0, sizeof(struct sigaction));   //pone la estructura a 0
    sa.sa_handler = signal_handler; //asigna el manejador de señales pasado por parametro
    sa.sa_flags = SA_NOCLDSTOP;     //ignora señales SIGCHLD de procesos hijos que se detienen solo se recibe cuando ha terminado
    sigemptyset(&sa.sa_mask);       //vacia el conjunto de señales a bloquear durante la ejecucion del manejador
    if (sigaction(signal, &sa, NULL) == -1) {   //si falla la instalacion del manejador(para la senal pasada por parametro hace lo definido en sa que es no recibir SIGCHLD de procesos hijos que se detienen)
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }
}

void write_handler(char *msg)   //funcion para escribir de manera segura
{
    /* Escribe en la salida estándar de manera segura */
    if (write(STDOUT_FILENO, msg, strlen(msg)) == -1)//si falla el write
    {
        perror("write() en handler");
        exit(EXIT_FAILURE);
    }
}

void manejador_sigchld(int signal)//funcion manejadora de la señal SIGCHLD
{
    int saved_errno = errno;

    if (signal == SIGCHLD) 
    {
        /* Impide que el proceso hijo se convierta en 'zombie' */
        if (wait(NULL) == -1)   //si falla el wait
        {
            perror("wait() en handler");
            exit(EXIT_FAILURE);
        }

        /* Escribe en la salida estándar de manera segura */
        write_handler("manejador(): Reiniciando lector de PDF...\n");
        estado = RECARGA;
    }

    errno = saved_errno;
}

void inicializa()
{
    /* Bloquea señal SIGINT */
    bloquea_signal(SIGINT);

    /* Instala manejador para señal SIGCHLD */
    instala_manejador_signal(SIGCHLD, manejador_sigchld);
}

int main(int argc, char *argv[])
{
    pid_t pid = 0; /* Usado en el proceso padre para guardar el PID del proceso hijo */

    inicializa();

    if (argc < 2)//si hay menos argumentos de los que se necesitan
    {
        printf("Uso: %s FILE.pdf\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Comprueba si argv[1] es un nombre de fichero con permiso de lectura */
    if (access(argv[1], R_OK) == -1)
    {
        printf("Error: El fichero %s no existe o no se puede abrir\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    while(1)    //MONTOYADA DECIMONONICA
    {
        if (estado == RECARGA)//si no esta lanzado el visor de pdfs
        {
            printf("main(): Creando lector de PDF...\n");

            /* Creación de un proceso hijo para ejecutar una aplicación externa */
            switch (pid = fork())
            {
            case -1: /* fork() falló */
                perror("fork()");
                exit(EXIT_FAILURE);
                break;
            case 0: /* Ejecución del proceso proceso hijo tras fork() con éxito */
                execlp("evince", "evince", argv[1], NULL); //ejecuta el visor de pdfs
                fprintf(stderr, "execlp() failed\n"); // esta linea se ejecuta solo si ha fallado la de antes
                exit(EXIT_FAILURE); //y como ha fallado sale
                break;
            default: /* Ejecución del proceso padre tras fork() con éxito */
                printf("main(): Lector de PDF creado...\n");
                break;
            }
            estado = ESPERA;
        }

        /* Evita monopolizar un núcleo... */
        pause();
    }

    return EXIT_SUCCESS;
}
