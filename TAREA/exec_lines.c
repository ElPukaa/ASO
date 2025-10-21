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







int main(int argc, char *argv[]) {
    int opt, BUF_SIZE = 16, MAX_LINE_SIZE = 32, NUM_PROCS = 1;
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








}