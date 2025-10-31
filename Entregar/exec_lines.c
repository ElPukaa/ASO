// Álvaro Pujante Cánovas G1.1 & Hugo Polo Molina G1.1
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
#include <string.h>

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
    sa.sa_flags = SA_NOCLDSTOP; 
    sigemptyset(&sa.sa_mask);
    if (sigaction(signal, &sa, NULL) == -1) {   //si falla la instalacion
        perror("sigaction()");
        exit(EXIT_FAILURE);
    }
}


void manejador_sigchld(int signal) {
    int saved_errno = errno;
    pid_t pid;
    int status;

    // para cosechar hijo q hayan terminado
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {     //mientras haya hijos q hayan terminado
        PROCS_ACTIVOS--; //quitamos un proceso activo

        if (pid > 0) { 
            if (WIFEXITED(status) && WEXITSTATUS(status) != 0) { 
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación normal con código %d.\n", LINENO, WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "Error al ejecutar la línea %d. Terminación anormal por señal %d.\n", LINENO, WTERMSIG(status));
            }
        }
    }
    
    errno = saved_errno;
}


char* limpiar_linea(char* linea) {
    
    while (*linea == ' ') { //para eliminar los espacios en blanco del principio
        linea++;
    }

    
    char *fin = linea + strlen(linea) - 1;
    while (fin > linea && (*fin == ' ' || *fin == '\n')) {  //elimina los espacios y saltos de linina al final
        *fin = '\0';
        fin--;
    }

    return linea;
}


void ejecutar_comando(char *comando) {  
    
    int n = 10; //numero maximo de argumentos (palabras/tokens)
    char *argv[n];
    int argc = 0;

    char *token = strtok(comando, " "); //dividimos la linea en tokens separados por espacios
    while (token != NULL && argc < n - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;

    
    if (argc == 0){         //si linea vacia salir 
        exit(EXIT_SUCCESS); 
    }

    
    int null_fd = open("/dev/null", O_WRONLY); //redirige stderr a /dev/null para evitar que se mezclen mensajes de error
    if (null_fd == -1) {
        perror("open(/dev/null)");
        exit(EXIT_FAILURE);
    }
    dup2(null_fd, STDERR_FILENO);   
    close(null_fd);

    
    execvp(argv[0], argv);//se ejecuta el comando

    fprintf(stderr, "Error: no se pudo ejecutar el comando '%s'\n", argv[0]);   //hace esto solo si execvp falla
    perror("execvp");
    exit(EXIT_FAILURE);
}

void interpretar_comando(char *comando, int LINENO) {
    
    pid_t pid;
    char *derecha = NULL;
    char *izquierda = NULL; 

        
        int contador_redirecciones = 0;
        char *copia = strdup(comando);  //hacemos una copia para no modificar el original (se encarga de reservar memoria)
        if (!copia) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }

        if (strstr(copia, "|")){    //si hay tubería
            contador_redirecciones++;
        } 

        if (strstr(copia, "<")){    //si hay redirección de entrada
            contador_redirecciones++;
        } 

        char *p = copia;
        while ((p = strstr(p, ">>")) != NULL) { //si hay redirección de salida(añadir al final)
            contador_redirecciones++;
            *p = ' '; // reemplaza el primer '>'
            *(p+1) = ' '; // reemplaza el segundo '>'
            p += 2;
        }

        
        if (strstr(copia, ">")) {   //si hay redirección de salida (sobreescribir)
            contador_redirecciones++;
        }
        
        
        free(copia); //liberamos la copia

        if (contador_redirecciones > 1) {//si hay más de una redirección en la línea
            fprintf(stderr, "Error: línea %d contiene múltiples operadores: %s\n", LINENO, comando);
            //no creamos proceso, simplemente volvemos al main
            return; 
        }
    

    sigset_t blocked_signals, old_signals;
    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGCHLD);

    if (sigprocmask(SIG_BLOCK, &blocked_signals, &old_signals) == -1) {     //si falla el bloqueo
        perror("sigprocmask(BLOCK)");
        exit(EXIT_FAILURE);
    }


    while (PROCS_ACTIVOS >= NUM_PROCS) {    //mientras no haya hueco
        sigsuspend(&old_signals);   //esperamos a que un hijo termine
    }
    
    if ((derecha = strstr(comando, "|")) != NULL) { 
        
        *derecha = '\0';
        derecha++;
        izquierda = limpiar_linea(comando);
        derecha = limpiar_linea(derecha);
        
        pid = fork(); // creamos el hijo "tarea"
        switch (pid) {  //segun sea padre o hijo
            case -1:    //si falla
                perror("fork(tarea tuberia)");
                exit(EXIT_FAILURE);
                break;

            case 0: { // hijo "gestor"
                if (sigprocmask(SIG_SETMASK, &old_signals, NULL) == -1) {   //si falla desbloqueo de señales
                    perror("sigprocmask(SETMASK) en hijo");
                    exit(EXIT_FAILURE);
                }

                //este hijo gestiona la tubería
                int pipefd[2];
                if (pipe(pipefd) == -1) {   //si falla la creación de la tubería
                    perror("pipe()");
                    exit(EXIT_FAILURE);
                }

                //nieto 1 (Izquierdo)
                pid_t left_pid = fork();
                if (left_pid == 0) {    //si va bien ejecuta el comando izquierdo sino sale con error
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[1]);
                    ejecutar_comando(izquierda);
                    perror("exec(izquierdo)");
                    exit(EXIT_FAILURE);
                }

                // Nieto 2 (Derecho)

                pid_t right_pid = fork();
                if (right_pid == 0) {   //si va bien ejecuta el comando derecho sino sale con error
                    close(pipefd[1]);
                    dup2(pipefd[0], STDIN_FILENO);
                    close(pipefd[0]);
                    ejecutar_comando(derecha);
                    perror("exec(derecho)");
                    exit(EXIT_FAILURE);
                }

                //el hijo "gestor" (padre de los nietos) cierra y espera
                close(pipefd[0]);
                close(pipefd[1]);
                waitpid(left_pid, NULL, 0);
                waitpid(right_pid, NULL, 0);
                
                exit(EXIT_SUCCESS);
            }

            default: //padre (main)
                PROCS_ACTIVOS++; //solo contamos 1 proceso-tarea
                break;
        }

    } else if ((derecha = strstr(comando, "<")) != NULL) {
        
        *derecha = '\0';
        derecha++;
        izquierda = limpiar_linea(comando);
        derecha = limpiar_linea(derecha);
        
        pid = fork();
        switch (pid) {  //segun sea padre o hijo
            case -1:    //si falla
                perror("fork <");
                exit(EXIT_FAILURE);
                break;
            case 0: { // hijo
                if (sigprocmask(SIG_SETMASK, &old_signals, NULL) == -1) { //si falla desbloqueo de señales
                    perror("sigprocmask(SETMASK) en hijo");
                    exit(EXIT_FAILURE);
                }
                int fd = open(derecha, O_RDONLY);   //abrimos el fichero en modo lectura
                if (fd < 0) {
                    perror("open <");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO); //redirigimos stdin
                close(fd);
                ejecutar_comando(izquierda);
                exit(EXIT_FAILURE);
            }
                break;
            default: // padre
                PROCS_ACTIVOS++;
                break;
        }

    } else if ((derecha = strstr(comando, ">>")) != NULL) {

        *derecha = '\0';
        derecha += 2;
        izquierda = limpiar_linea(comando);
        derecha = limpiar_linea(derecha);
        
        pid = fork();
        switch (pid) {  //segun sea padre o hijo
            case -1:       //si falla
                perror("fork >>");
                exit(EXIT_FAILURE);
                break;
            case 0: { // Hijo
                if (sigprocmask(SIG_SETMASK, &old_signals, NULL) == -1) { // si falla desbloqueo de señales
                    perror("sigprocmask(SETMASK) en hijo");
                    exit(EXIT_FAILURE);
                }
                int fd = open(derecha, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR); //permisos de fichero
                if (fd < 0) {
                    perror("open >>");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);    //redirigimos stdout
                close(fd);
                ejecutar_comando(izquierda);
                exit(EXIT_FAILURE);
            }
                break;
            default: //padre
                PROCS_ACTIVOS++;
                break;
        }
        
    } else if ((derecha = strstr(comando, ">")) != NULL) {
        
        *derecha = '\0';
        derecha++;
        izquierda = limpiar_linea(comando);
        derecha = limpiar_linea(derecha);

        pid = fork();
        switch (pid) {  //segun sea padre o hijo
            case -1:   //si falla
                perror("fork >");
                exit(EXIT_FAILURE);
                break;
            case 0: { // hijo
                if (sigprocmask(SIG_SETMASK, &old_signals, NULL) == -1) { //si falla desbloqueo de señales
                    perror("sigprocmask(SETMASK) en hijo");
                    exit(EXIT_FAILURE);
                }
                int fd = open(derecha, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);    //abrimos el fichero con los permisos
                if (fd < 0) {   //si falla la apertura del fichero
                    perror("open >");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);    //redirigimos stdout
                close(fd);
                ejecutar_comando(izquierda);
                exit(EXIT_FAILURE);
            }
                break;
            default: // padre
                PROCS_ACTIVOS++;
                break;
        }
    } else {
        //COMANDO SIMPLE
        pid = fork();
        switch(pid) {   //segun sea padre o hijo
            case -1:    //si falla
                perror("fork simple");
                exit(EXIT_FAILURE);
                break;
            case 0: //hijo
                if (sigprocmask(SIG_SETMASK, &old_signals, NULL) == -1) { //si falla desbloqueo de señales
                    perror("sigprocmask(SETMASK) en hijo");
                    exit(EXIT_FAILURE);
                }
                ejecutar_comando(comando);  //si va el desbloqueo ejecuta el comando
                exit(EXIT_FAILURE);
                break;
            default: //padre
                PROCS_ACTIVOS++;
                break;
        }
    }

 
    if (sigprocmask(SIG_SETMASK, &old_signals, NULL) == -1) {//desbloqueamos señales en el padre
        perror("sigprocmask(SETMASK) en padre");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    instala_manejador_signal(SIGCHLD, manejador_sigchld);
    int opt, BUF_SIZE = 16, MAX_LINE_SIZE = 32; //valores por defecto 
    char *b_buffer; // buffer de lectura
    

    optind = 1;
    while ((opt = getopt(argc, argv, "b:l:p:h")) != -1){//mientras haya opciones que leer
        switch (opt){
        
            case 'b':
                BUF_SIZE = atoi(optarg);    //guardamos el tamaño del buffer convirtiendolo a entero
                if (BUF_SIZE < 1 || BUF_SIZE > 8192){
                    fprintf(stderr, "Error: El tamaño del buffer debe estar entre 1 y 8192.\n");
                    exit(EXIT_FAILURE);
                }

                break;
            case 'l':
                MAX_LINE_SIZE = atoi(optarg);   //guardamos el tamaño maximo de linea convirtiendolo a entero
                if (MAX_LINE_SIZE < 16 || MAX_LINE_SIZE > 1024){
                    fprintf(stderr, "Error: El numero maximo de procesos debe estar entre 16 y 1024.\n");
                    exit(EXIT_FAILURE);
                }
                break;

            case 'p':
                    NUM_PROCS = atoi(optarg);   //guardamos el número de procesos simultáneos convirtiendolo a entero
                    if (NUM_PROCS < 1 || NUM_PROCS > 8){
                        fprintf(stderr, "Error: El numero de procesos en ejecucion debe estar entre 1 y 8.\n");
                        

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

    char *l_buffer = malloc((MAX_LINE_SIZE + 1) * sizeof(char)); //buffer para ir almacenando la linea leida (el +1 es para el \0)
    if (!l_buffer) {
        perror("Error al asignar memoria para l_buffer");
        free(b_buffer);
        exit(EXIT_FAILURE);
    }


    
    
    ssize_t bytes_leidos;
    ssize_t indice_bytes_lineas = 0; //indice para ir almacenando en l_buffer

    while( (bytes_leidos = read(STDIN_FILENO, b_buffer, BUF_SIZE)) > 0 ) {    //mientras que haya datos que leer se van metiendo en el buffer de lectura(b_buffer el q es de tamaño BUF_SIZE)
        //procesar buffer
        for (int i=0; i < bytes_leidos; i++){//mientras el l_buffer no esté lleno
            //almacenar en l_buffer hasta encontrar \n

            if (indice_bytes_lineas == 0) {
                //es el primer carácter de una línea nueva, la contamos AHORA.
                LINENO++;
            }


            if(b_buffer[i] == '\n') {//si el caracter a copiar en el l_buffer es \n
                l_buffer[indice_bytes_lineas] = '\0'; //terminar la cadena 
                
                
                if (indice_bytes_lineas > 0) { //si la línea no está vacía
                    interpretar_comando(l_buffer, LINENO); 
                }
                
                //l_buffer preparado para la siguiente línea
                indice_bytes_lineas = 0;
                

            }else if (indice_bytes_lineas < MAX_LINE_SIZE) { //si no se ha llenado l_buffer
                l_buffer[indice_bytes_lineas] = b_buffer[i]; //copiamos de b_buffer a l_buffer
                indice_bytes_lineas++;
                
            } else {//si la línea es demasiado larga.
                l_buffer[MAX_LINE_SIZE] = '\0'; //asegurar fin de cadena para imprimir(necesario para el mensaje de error)
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

    //espera al final a que todos los hijos terminen antes de acabar con el programa para no dejar huerfanos
    sigset_t blocked_signals, old_signals;
    sigemptyset(&blocked_signals);
    sigaddset(&blocked_signals, SIGCHLD);
    sigprocmask(SIG_BLOCK, &blocked_signals, &old_signals);

    while (PROCS_ACTIVOS > 0) {//mientras hay procesos activos
        sigsuspend(&old_signals);
    }

    sigprocmask(SIG_SETMASK, &old_signals, NULL); //restaurar máscara

    
    free(l_buffer);//liberar la memoria de bufe
    free(b_buffer);
    return 0;
}
