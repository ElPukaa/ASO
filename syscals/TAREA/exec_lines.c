#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>  

int PROCS_ACTIVOS = 0; //contador de procesos activos
int NUM_PROCS = 1;     //número máximo de procesos simultáneos(POR DEFECTO 1)
int LINENO = 0;        //número de línea actual

const char *mensaje_help = 
   "Uso: ./exec_lines [-b BUF_SIZE] [-l MAX_LINE_SIZE] [-p NUM_PROCS]\n
    Lee de la entrada estándar una secuencia de líneas conteniendo órdenes\n
    para ser ejecutadas y lanza los procesos necesarios para ejecutar cada\n
    línea, esperando a su terminación para ejecutar la siguiente.\n
    -b BUF_SIZE \t Tamaño del buffer de entrada 1<=BUF_SIZE<=8192\n 
    -l MAX_LINE_SIZE \t Tamaño máximo de línea 16<=MAX_LINE_SIZE<=1024\n
    -p NUM_PROCS \t Número de procesos en ejecución de forma simultánea (1 <= NUM_PROCS <= 8)\n"


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

void esperar_hueco(){
    while (PROCS_ACTIVOS >= NUM_PROCS) { //mientras NO haya hueco para ejecutar otro proceso
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
}



void interpretar_comando(char *comando, int LINENO) {
    pid_t pid;              //para el fork
    char *comando2 = NULL;
    int pipefd[2];          //para las tuberias

    //CUIDAO que pasa si no se lanzan tantos procesos como NUM_PROCS_GLOBAL? no entraria nunca aqui

    esperar_hueco(PROCS_ACTIVOS); //espera hasta que haya hueco para lanzar otro proceso(en caso de que se haya llegado al maximo sino salta directamente)
    
    if ((comando2 = strstr(comando, "|")) != NULL) {//si hay una tuberia
        *comando2 = '\0';   //cambia el carácter | por \0 para indicar el final del primer comando
        comando2++;         //avanzar uno para que apunte al inicio del segundo comando
        comando = limpiar_linea(comando);
        comando2 = limpiar_linea(comando2);

        //if(PROCS_ACTIVOS + 2 = NUM_PROCS_GLOBAL){
            pid = fork();
            switch (pid) {
                case -1:   //fallo del fork
                    perror("fork");
                    exit(EXIT_FAILURE);
                    break;

                case 0: {   
                    if (pipe(pipefd) == -1) {   //fallo al crear la tuberia
                        perror("pipe");
                        exit(EXIT_FAILURE);
                    }
                    
                    
                    pid_t left = fork();    //parte izquierda de la tuberia
                    if (left == 0) {
                        close(pipefd[0]);   //cierra el extremo de lectura
                        dup2(pipefd[1], STDOUT_FILENO); //redirige la salida estandar a la escritura de la tuberia
                        close(pipefd[1]);   //cierra el descriptor ORIGINAL de escritura

                        ejecutar_comando(comando);
                          
                    }
                        esperar_hueco(PROCS_ACTIVOS); //espera hasta que haya hueco para lanzar otro proceso(en caso de que se haya llegado al maximo sino salta directamente)
                    
                    pid_t right = fork();   //parte derecha de la tuberia
                    if (right == 0) {
                        close(pipefd[1]);   //cierra el extremo de escritura
                        dup2(pipefd[0], STDIN_FILENO);  
                        close(pipefd[0]);   //crierra el descriptor ORIGINAL de lectura
                        ejecutar_comando(comando2);
                        
                    }
                    //una vez lanzados los procesos, cierra las tuberias el padre y espera a que los hisjos terminen
                    close(pipefd[0]);
                    close(pipefd[1]);
                    waitpid(left, NULL, 0);
                    waitpid(right, NULL, 0);

                    exit(EXIT_SUCCESS);
                    }
                    break;

                default://no debería llegar el hijo aquí nunca, solo el padre
                    PROCS_ACTIVOS++;
                    break;
            }
        //}
    }else if ((comando2 = strstr(comando, "<")) != NULL) {
        *comando2 = '\0';
        comando2++;
        comando = limpiar_linea(comando);
        comando2 = limpiar_linea(comando2);

        pid = fork();
        switch (pid) {//comprueba el fork
            case -1:    //si falla
                perror("fork");
                exit(EXIT_FAILURE);
                break;

            case 0: {   //si va bien
                int fd = open(comando2, O_RDONLY);
                if (fd < 0) {
                    perror("open <");
                    exit(EXIT_FAILURE);
                }

                dup2(fd, STDIN_FILENO); //redirige la entrada estandar al fichero
                close(fd);      //cierra el descriptor original
                ejecutar_comando(comando);
            }
                break;

            default:
                PROCS_ACTIVOS++;
                break;
        }

    }else if ((comando2 = strstr(comando, ">>")) != NULL) {
        *comando2 = '\0';
        comando2 += 2;  //avanza dos posiciones para que apunte al inicio del segundo comando(por el >>)
        comando = limpiar_linea(comando);
        comando2 = limpiar_linea(comando2);

        pid = fork();
        switch (pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
                break;

            case 0: {
                int fd = open(comando2, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO );//abre el fichero en modo escritura, si no existe lo crea y 
                // añade al final(permisos de  r w x para User, Group y Others la ultima letra los identifica)
                if (fd < 0) {   //si hay error al abrir el fichero
                    perror("open >>");
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);//redirige la salida estandar al fichero
                close(fd);//cierra el descriptor original
                ejecutar_comando(comando);
                
                }
                break;

            default:
                PROCS_ACTIVOS++;
                break;
        }
        
    }else if ((comando2 = strstr(comando, ">")) != NULL) {
        *comando2 = '\0';
        comando2++;
        comando = limpiar_linea(comando);
        comando2 = limpiar_linea(comando2);

        pid = fork();
        switch (pid) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
                break;

            case 0: {
                int fd = open(comando2, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO );//abre el fichero en modo escritura, 
                // si no existe lo crea y si existe lo trunca
                if (fd < 0) {   //si hay error al abrir el fichero
                    perror("open >");
                    exit(EXIT_FAILURE);
                }

                dup2(fd, STDOUT_FILENO);    //redirige la salida estandar al fichero
                close(fd);
                ejecutar_comando(comando);
            
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
    int opt, BUF_SIZE = 16, MAX_LINE_SIZE = 32; //valores por defecto 
    char *buffer; // buffer de lectura
    

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
    
    free(l_buffer);
    free(b_buffer);
    return 0;
}












