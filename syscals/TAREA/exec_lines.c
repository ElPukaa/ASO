#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>  


const char *mensaje_help = 
   "Uso: ./exec_lines [-b BUF_SIZE] [-l MAX_LINE_SIZE] [-p NUM_PROCS]\n
    Lee de la entrada estándar una secuencia de líneas conteniendo órdenes\n
    para ser ejecutadas y lanza los procesos necesarios para ejecutar cada\n
    línea, esperando a su terminación para ejecutar la siguiente.\n
    -b BUF_SIZE \t Tamaño del buffer de entrada 1<=BUF_SIZE<=8192\n 
    -l MAX_LINE_SIZE \t Tamaño máximo de línea 16<=MAX_LINE_SIZE<=1024\n
    -p NUM_PROCS \t Número de procesos en ejecución de forma simultánea (1 <= NUM_PROCS <= 8)\n"


//funciones que pueden ser interesantes: 
/*
/// escribir en buffer completo semana 3 cat mem din 
int write_all(int fd, char* buf, ssize_t size){ 
    ssize_t num_written = 0;
    ssize_t num_left = size;

    char *buf_left = buf;

    while (num_left > 0 && (num_written = write(fd, buf_left, num_left)) != -1)//escribe todo el buffer
    {
        num_left -= num_written;
        buf_left += num_written;
    }
    return num_written == -1 ? -1 : size;  //devuelve -1 si error, size si todo ok
}
*/

void ejecutar_comando(char *comando) {


}


void interpretar_comando(char *comando) {


}



int main(int argc, char *argv[]) {
    int opt, BUF_SIZE = 16, MAX_LINE_SIZE = 32, NUM_PROCS = 1; //valores por defecto 
    char *buffer; // buffer de lectura
    
    
    //POSIBLE IDEA contabilizar lineas para saber cual de ellas falla/esta en ejecucion 
    //int cont_lineas = 0; //contador de linea de comando

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


    
    int lineno = 0;//para identificar la línea actual
    ssize_t bytes_leidos;
    ssize_t indice_bytes_lineas = 0; //índice para ir almacenando en l_buffer

    while( (bytes_leidos = read(STDIN_FILENO, b_buffer, BUF_SIZE)) > 0 ) {    //mientras que haya datos que leer se van metiendo en el buffer de lectura(b_buffer el q es de tamaño BUF_SIZE)
        //procesar buffer
        for (int i=0; i < bytes_leidos; i++){//mientras el l_buffer no esté lleno
            //almacenar en l_buffer hasta encontrar \n
            if(b_buffer[i] == '\n') {//si el caracter a copiar en el l_buffer es \n
                l_buffer[indice_bytes_lineas] = '\0'; // Terminar la cadena 
                
                if (indice_bytes_lineas > 0) { //si la línea no está vacía
                    interpretar_comando(l_buffer, lineno); 
                }
                
                //l_buffer preparado para la siguiente línea
                indice_bytes_lineas = 0;
                lineno++;

            }else if (indice_bytes_lineas < MAX_LINE_SIZE) { //si no se ha llenado l_buffer
                l_buffer[indice_bytes_lineas] = b_buffer[i]; //copiamos de b_buffer a l_buffer
                indice_bytes_lineas++;
                
            } else {// si la línea es demasiado larga.
                l_buffer[MAX_LINE_SIZE] = '\0'; // Asegurar fin de cadena para imprimir(necesario para el mensaje de error)
                fprintf(stderr, "Error, línea %d demasiado larga: \"%s...\"\n", lineno, l_buffer);
                
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

    if (indice_bytes_lineas > 0) {      //si read ha terminado pero hay datos en l_buffer (hay un EOF en vez de un \n)
        l_buffer[indice_bytes_lineas] = '\0';
        interpretar_comando(l_buffer, lineno);
    }


    free(l_buffer);
    free(b_buffer);
    return 0;
}












