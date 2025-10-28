#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>  
#include <signal.h>
#include <string.h> // Para memset

int PROCS_ACTIVOS = 0; //contador de procesos activos
int NUM_PROCS = 1;     //número máximo de procesos simultáneos(POR DEFECTO 1)
int LINENO = 0;        //número de línea actual

const char *mensaje_help = 
   "Uso: ./exec_lines [-b BUF_SIZE] [-l MAX_LINE_SIZE] [-p NUM_PROCS]\n"
    "Lee de la entrada estándar una secuencia de líneas conteniendo órdenes\n"
    "para ser ejecutadas y lanza los procesos necesarios para ejecutar cada\n"
    "línea, esperando a su terminación para ejecutar la siguiente.\n"
    "-b BUF_SIZE \t Tamaño del buffer de entrada 1<=BUF_SIZE<=8192\n"
    "-l MAX_LINE_SIZE \t Tamaño máximo de línea 16<=MAX_LINE_SIZE<=1024\n"
    "-p NUM_PROCS \t Número de procesos en ejecución de forma simultánea (1 <= NUM_PROCS <= 8)\n";

void instala_manejador_signal(int signal, void (*signal_handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = signal_handler;
    sa.sa_flags = SA_NOCLDSTOP; // Importante para SIGCHLD
    sigemptyset(&sa.sa_mask);
    if (sigaction(signal, &sa, NULL) == -1) {
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }
}


void manejador_sigchld(int signal) {
    int saved_errno = errno;
    pid_t pid;
    int status;

    // Bucle para cosechar a TODOS los hijos que hayan terminado
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {     // Mientras haya hijos que hayan terminado
        PROCS_ACTIVOS--; // Decrementa el contador por cada hijo terminado

        int status;                             //para almacenar el estado de terminación

        if (pid > 0) {//si waitpid ha ido bien (devuelve -1 en caso de error sino el pid del proceso que ha terminado)   
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {//si el proceso ha terminado con exit pero con código de error distinto de 0
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación normal con código %d.\n", LINENO, WEXITSTATUS(status));
                
            } else if (WIFSIGNALED(status)) {//si el proceso ha terminado por señal de sistema(forzado)
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación anormal por señal %d.\n", LINENO, WTERMSIG(status));
                
            }
        }
    }
    errno = saved_errno;
}


char* limpiar_linea(char* linea) {
    // Elimina espacios en blanco al inicio
    while (*linea == ' ') {
        linea++;
    }

    // Elimina espacios en blanco al final
    char *fin = linea + strlen(linea) - 1;
    while (fin > linea && (*fin == ' ' || *fin == '\n')) {
        *fin = '\0';
        fin--;
    }

    return linea;
}


void ejecutar_comando(char *comando) {
    // Separar la línea en tokens por espacios ()
    int n = 10; // numero maximo de argumentos (palabras/tokens)
    char *argv[n];
    int argc = 0;

    // Tokenizar la línea
    char *token = strtok(comando, " ");
    while (token != NULL && argc < n - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL; // finalizar la lista

    
    if (argc == 0){         // Si la línea está vacía, salir sin hacer nada
        exit(EXIT_SUCCESS); 
    }

    // Ejecutar comando con execvp
    execvp(argv[0], argv);//a partir de esta linea no se debería ejecutar nada más, si lo hace es que ha habido un error

    fprintf(stderr, "Error: no se pudo ejecutar el comando '%s'\n", argv[0]);
    perror("execvp");
    exit(EXIT_FAILURE);
}

/*void esperar_hueco(int procesos_necesarios){

    if(PROCS_ACTIVOS >= NUM_PROCS) { //mientras NO haya hueco para ejecutar otro proceso
        int status;                             //para almacenar el estado de terminación
        pid_t terminado = wait(&status);        //espera a que termine un proceso hijo
        PROCS_ACTIVOS--;                        //decrementa el contador de procesos activos (porque despues del wait ha terminado uno)

        if (terminado > 0) {//si wait ha ido bien (devuelve -1 en caso de error sino el pid del proceso que ha terminado)   
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {//si el proceso ha terminado con exit pero con código de error distinto de 0
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación normal con código %d.\n", LINENO, WEXITSTATUS(status));
                
            } else if (WIFSIGNALED(status)) {//si el proceso ha terminado por señal de sistema(forzado)
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación anormal por señal %d.\n", LINENO, WTERMSIG(status));
                
            }
        }
    }
}*/


void interpretar_comando(char *comando, int LINENO) {
    pid_t pid;              //para el fork
    char *derecha = NULL;
    char *izquierda = NULL; 

    sigset_t blocked_signals, old_signals;
    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGCHLD);

    // Bloquea SIGCHLD para comprobar PROCS_ACTIVOS de forma segura
    if (sigprocmask(SIG_BLOCK, &blocked_signals, &old_signals) == -1) {
        perror("sigprocmask(BLOCK)");
        exit(EXIT_FAILURE);
    }
    
    //CUIDAO que pasa si no se lanzan tantos procesos como NUM_PROCS_GLOBAL? no entraria nunca aqui
    
    
    if ((derecha = strstr(comando, "|")) != NULL) {//si hay una tuberia
        
        int pipefd[2];          //para las tuberias

        *derecha = '\0';   //cambia el carácter | por \0 para indicar el final del primer comando
        derecha++;         //avanzar uno para que apunte al inicio del segundo comando
        izquierda = limpiar_linea(comando);
        derecha = limpiar_linea(derecha);
        
        //if(PROCS_ACTIVOS + 2 = NUM_PROCS_GLOBAL)
            //como ya hay un hueco reservado al menos, hacemos una segunda comprobación para tener seguro que hay al menos 2 huecos si hay pipeline 
    
            if (pipe(pipefds) == -1){ /* Paso 0: Creación de la tubería */
            
                perror("pipe()");
                exit(EXIT_FAILURE);
            }
            
            /* Paso 1: Creación del hijo izquierdo de la tubería */
            pid = fork();
            switch (pid){
                case -1:
                    perror("fork(1)");
                    exit(EXIT_FAILURE);
                    break;
                    
                case 0: /* Hijo izquierdo de la tubería */
                    /* Paso 2: El extremo de lectura no se usa */
                    if (close(pipefds[0]) == -1){
                        perror("close(1)");
                        exit(EXIT_FAILURE);
                    }
                    /* Paso 3: Redirige la salida estándar al extremo de escritura de la tubería */
                    if (dup2(pipefds[1], STDOUT_FILENO) == -1){
                        perror("dup2(1)");
                        exit(EXIT_FAILURE);
                    }
                    /* Paso 4: Cierra el descriptor duplicado */
                    if (close(pipefds[1]) == -1){
                        perror("close(2)");
                        exit(EXIT_FAILURE);
                    }
                    /* Paso 5: Reemplaza el binario actual por el de `youtube-dl` */
                    ejecutar_comando(izquierda);
                    perror("execlp(izquierdo)");
                    exit(EXIT_FAILURE);
                    break;
                default: /* El proceso padre continúa... */
                    break;
            }

            /* Paso 6: Creación del hijo derecho de la tubería */
            switch (fork()){

                case -1:
                    perror("fork(2)");
                    exit(EXIT_FAILURE);
                    break;
                case 0: /* Hijo derecho de la tubería  */
                    /* Paso 7: El extremo de escritura no se usa */
                    if (close(pipefds[1]) == -1)
                    {
                        perror("close(3)");
                        exit(EXIT_FAILURE);
                    }
                    /* Paso 8: Redirige la entrada estándar al extremo de lectura de la tubería */
                    if (dup2(pipefds[0], STDIN_FILENO) == -1)
                    {
                        perror("dup2(2)");
                        exit(EXIT_FAILURE);
                    }
                    /* Paso 9: Cierra el descriptor duplicado */
                    if (close(pipefds[0]) == -1)
                    {
                        perror("close(4)");
                        exit(EXIT_FAILURE);
                    }
                    /* Paso 10: Reemplaza el binario actual por el de `ffmpeg` */
                    
                    ejecutar_comando(derecha);
                    perror("execlp(derecho)");
                    exit(EXIT_FAILURE);
                    break;
                    
                default: /* El proceso padre continúa... */
                    break;
            }
            PROCS_ACTIVOS++; //NO SE SI METERLO AQUI O DONDE, PREGUNTAR A CHATI

            /* El proceso padre cierra los descriptores de fichero no usados */
            if (close(pipefds[0]) == -1){
                perror("close(pipefds[0])");
                exit(EXIT_FAILURE);
            }
            if (close(pipefds[1]) == -1){
                perror("close(pipefds[1])");
                exit(EXIT_FAILURE);
            }
            
    }else if ((derecha = strstr(comando, "<")) != NULL) {
        *derecha = '\0';
        derecha++;
        izquierda = limpiar_linea(comando);
        derecha = limpiar_linea(derecha);

        
        pid = fork();
        switch (pid) {//comprueba el fork
            case -1:    //si falla
                perror("fork");
                exit(EXIT_FAILURE);
                break;

            case 0: {   //si va bien
                int fd = open(derecha, O_RDONLY);
                if (fd < 0) {
                    perror("open <");
                    exit(EXIT_FAILURE);
                }

                dup2(fd, STDIN_FILENO); //redirige la entrada estandar al fichero
                close(fd);      //cierra el descriptor original
                ejecutar_comando(izquierda);
            }
                break;

            default:
                PROCS_ACTIVOS++;
                break;
        }

    }else if ((derecha = strstr(comando, ">>")) != NULL) {
        *derecha = '\0';
        derecha += 2;  //avanza dos posiciones para que apunte al inicio del segundo comando(por el >>)
        izquierda = limpiar_linea(comando);
        derecha = limpiar_linea(derecha);
        
        pid = fork();
        switch (pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
                break;

            case 0: {
                int fd = open(derecha, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO );//abre el fichero en modo escritura, si no existe lo crea y 
                // añade al final(permisos de  r w x para User, Group y Others la ultima letra los identifica)
                if (fd < 0) {   //si hay error al abrir el fichero
                    perror("open >>");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);//redirige la salida estandar al fichero
                close(fd);//cierra el descriptor original
                ejecutar_comando(izquierda);
                
                }
                break;

            default:
                PROCS_ACTIVOS++;
                break;
        }
        
    }else if ((derecha = strstr(comando, ">")) != NULL) {
        *derecha = '\0';
        derecha++;
        izquierda = limpiar_linea(comando);
        derecha = limpiar_linea(derecha);

        
        pid = fork();
        switch (pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
                break;

            case 0: {
                int fd = open(derecha, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO );//abre el fichero en modo escritura, 
                // si no existe lo crea y si existe lo trunca
                if (fd < 0) {   //si hay error al abrir el fichero
                    perror("open >");
                    exit(EXIT_FAILURE);
                }

                dup2(fd, STDOUT_FILENO);    //redirige la salida estandar al fichero
                close(fd);
                ejecutar_comando(izquierda);
            
                }
                break;

            default:
                PROCS_ACTIVOS++;
                break;
        }
    }else {
        // Comando simple
        pid = fork();//crea un hijo
        switch(pid) {
            case -1:    //si falla
                perror("fork");
                exit(EXIT_FAILURE);
            break;

            case 0: //si va bien
                ejecutar_comando(comando);
                
            break;
            
            default:
                PROCS_ACTIVOS++;
                break;
        }
    }
}


int main(int argc, char *argv[]) {
    instala_manejador_signal(SIGCHLD, manejador_sigchld);
    int opt, BUF_SIZE = 16, MAX_LINE_SIZE = 32; //valores por defecto 
    char *b_buffer; // buffer de lectura
    

    optind = 1;
    while ((opt = getopt(argc, argv, "b:l:p:h")) != -1){
        switch (opt){
        
            case 'b':
                BUF_SIZE = atoi(optarg);
                if (BUF_SIZE < 1 || BUF_SIZE > 8192){
                    fprintf(stderr, "Error: BUF_SIZE debe estar entre 1 y 8192.\n");
                    exit(EXIT_FAILURE);
                }

                break;
            case 'l':
                MAX_LINE_SIZE = atoi(optarg);
                if (MAX_LINE_SIZE < 16 || MAX_LINE_SIZE > 1024){
                    fprintf(stderr, "Error: MAX_LINE_SIZE debe estar entre 16 y 1024.\n");
                    exit(EXIT_FAILURE);
                }
                break;

            case 'p':
                    NUM_PROCS = atoi(optarg);
                    if (NUM_PROCS < 1 || NUM_PROCS > 8){
                        fprintf(stderr, "Error: NUM_PROCS debe estar entre 1 y 8.\n");
                        exit(EXIT_FAILURE);
                    }
                    break;
           
            case 'h':
                    fprintf(stderr, mensaje_help, argv[0]); 
                    exit(EXIT_SUCCESS);
                    break;

            default:
                fprintf(stderr, mensaje_help, argv[0]);
                exit(EXIT_FAILURE);
                break;
            
        }
    }

    b_buffer = malloc(BUF_SIZE * sizeof(char)); //se crea un buffer del tamaño especificado por entrada
    if (!b_buffer) { //se comprueba que se haya creado correctamente (BUF_SIZE tiene un valor válido)
        perror("Error al asignar memoria");
        exit(EXIT_FAILURE);
    }

    char *l_buffer = malloc((MAX_LINE_SIZE + 1) * sizeof(char)); //buffer para ir almacenando la línea leída (el +1 es para el \0)
    if (!l_buffer) {
        perror("Error al asignar memoria para l_buffer");
        free(b_buffer);
        exit(EXIT_FAILURE);
    }


    
    
    ssize_t bytes_leidos;
    ssize_t indice_bytes_lineas = 0; //índice para ir almacenando en l_buffer

    while( (bytes_leidos = read(STDIN_FILENO, b_buffer, BUF_SIZE)) > 0 ) {    //mientras que haya datos que leer se van metiendo en el buffer de lectura(b_buffer el q es de tamaño BUF_SIZE)
        //procesar buffer
        for (int i=0; i < bytes_leidos; i++){//mientras el l_buffer no esté lleno
            //almacenar en l_buffer hasta encontrar \n
            if(b_buffer[i] == '\n') {//si el caracter a copiar en el l_buffer es \n
                l_buffer[indice_bytes_lineas] = '\0'; // Terminar la cadena 
                
                if (indice_bytes_lineas > 0) { //si la línea no está vacía
                    interpretar_comando(l_buffer, LINENO); 
                }
                
                //l_buffer preparado para la siguiente línea
                indice_bytes_lineas = 0;
                LINENO++;

            }else if (indice_bytes_lineas < MAX_LINE_SIZE) { //si no se ha llenado l_buffer
                l_buffer[indice_bytes_lineas] = b_buffer[i]; //copiamos de b_buffer a l_buffer
                indice_bytes_lineas++;
                
            } else {// si la línea es demasiado larga.
                l_buffer[MAX_LINE_SIZE] = '\0'; // Asegurar fin de cadena para imprimir(necesario para el mensaje de error)
                fprintf(stderr, "Error, línea %d demasiado larga: \"%s...\"\n", LINENO, l_buffer);
                
                free(l_buffer);
                free(b_buffer);
                exit(EXIT_FAILURE);
            }
               
        }   
    }

    if (bytes_leidos == -1) {       //si ha habido error en la lectura (read devuelve -1 en caso de error)
        perror("Error al leer de stdin");
        free(l_buffer);
        free(b_buffer);
        exit(EXIT_FAILURE);
    }

    //PARTE QUE COMPRUEBA LA ULTIMA LINEA DEL FICHERO (si no acaba en \n)

    if (indice_bytes_lineas > 0) {      //si read ha terminado pero hay datos en l_buffer (hay un EOF en vez de un \n)
        l_buffer[indice_bytes_lineas] = '\0';
        interpretar_comando(l_buffer, LINENO);
    }

    // espera al final a que todos los hijos terminen antes de acabar con el programa para no dejar huerfanos
    sigset_t blocked_signals, old_signals;
    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGCHLD);
    sigprocmask(SIG_BLOCK, &blocked_signals, &old_signals);

    while (PROCS_ACTIVOS > 0) {//mientras hay procesos activos
        sigsuspend(&old_signals);
    }

    sigprocmask(SIG_SETMASK, &old_signals, NULL); // Restaurar máscara

    
    free(l_buffer);//liberar la memoria de bufe
    free(b_buffer);
    return 0;
}
















