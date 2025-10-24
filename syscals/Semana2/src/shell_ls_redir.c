#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    pid_t pid; /* Usado en el proceso padre para guardar el PID del proceso hijo */
    int fd;

    switch (pid = fork())
    {
    case -1: /* fork() falló */
        perror("fork()");
        exit(EXIT_FAILURE);
        break;
    case 0:                             /* Ejecución del proceso hijo tras fork() con éxito */
        if (close(STDOUT_FILENO) == -1) /* Cierra la salida estándar */
        {
            perror("close()");
            exit(EXIT_FAILURE);
        }
        /* Abre el fichero "listado" al que se asigna el descriptor de fichero no usado más bajo, es decir, STDOUT_FILENO(1) */
        if ((fd = open("listado", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) == -1)
        {
            perror("open()");
            exit(EXIT_FAILURE);
        }
        execlp("ls", "ls", "-la", NULL);      /* Sustituye el binario actual por /bin/ls */
        fprintf(stderr, "execlp() failed\n"); /* Esta línea no se debería ejecutar si la anterior tuvo éxito */
        exit(EXIT_FAILURE);
        break;
    default:                  /* Ejecución del proceso padre tras fork() con éxito */
        if (wait(NULL) == -1) /* Espera a que termine el proceso hijo */
        {
            perror("wait()");
            exit(EXIT_FAILURE);
        }
        break;
    }

    return EXIT_SUCCESS;
}



/*

CON COMENTARIOS


*/


#define _POSIX_C_SOURCE 200809L  /* Definimos esta macro para asegurarnos de que obtenemos las definiciones necesarias para funciones y características de POSIX 2008 */
#include <stdio.h>               /* Incluimos la librería estándar de entrada y salida */
#include <stdlib.h>              /* Incluimos la librería estándar de utilidades para funciones como malloc, free, exit, etc. */
#include <fcntl.h>               /* Incluimos la librería para manipulación de archivos, como open() y fcntl() */
#include <unistd.h>              /* Incluimos la librería POSIX para funciones como fork(), execlp(), y close() */
#include <sys/stat.h>            /* Incluimos la librería para las estructuras de control de archivos, como permisos */
#include <sys/types.h>           /* Incluimos la librería para definir tipos de datos como pid_t */
#include <sys/wait.h>            /* Incluimos la librería para la función wait(), que espera por la finalización de un proceso hijo */

int main(int argc, char **argv)  /* Función principal que recibe argumentos de la línea de comandos */
{
    pid_t pid;                   /* Definimos una variable pid_t para almacenar el ID del proceso hijo después de llamar a fork() */
    int fd;                      /* Variable que se utilizará para almacenar el descriptor de archivo después de abrir un archivo */

    switch (pid = fork())        /* Llamamos a fork() para crear un proceso hijo; guardamos el PID resultante en pid */
    {
    case -1: /* Si fork() devuelve -1, ha ocurrido un error */
        perror("fork()");        /* Imprimimos el mensaje de error de fork() utilizando perror() para describir el fallo */
        exit(EXIT_FAILURE);      /* Salimos del programa con un código de error */
        break;
    case 0:  /* Si fork() devuelve 0, estamos en el proceso hijo */
        if (close(STDOUT_FILENO) == -1) /* Cerramos el descriptor de archivo de la salida estándar (STDOUT_FILENO es 1) */
        {
            perror("close()");    /* Si no se puede cerrar STDOUT, imprimimos el error con perror() */
            exit(EXIT_FAILURE);   /* Terminamos el proceso hijo con un código de error */
        }
        /* Intentamos abrir o crear el archivo llamado "listado", configurándolo como escritura únicamente, truncándolo si ya existe */
        /* El tercer argumento especifica los permisos del archivo (S_IRWXU = permisos de lectura, escritura y ejecución para el propietario) */
        if ((fd = open("listado", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU)) == -1)
        {
            perror("open()");     /* Si falla la apertura del archivo, imprimimos el error con perror() */
            exit(EXIT_FAILURE);   /* Terminamos el proceso hijo con un código de error */
        }
        /* Sustituimos la imagen del proceso hijo por el comando "ls" con los argumentos "-la" */
        /* execlp() busca el ejecutable en el PATH del sistema, si tiene éxito, no volverá al código anterior */
        execlp("ls", "ls", "-la", NULL);  
        /* Si execlp() falla, se ejecutará la siguiente línea */
        fprintf(stderr, "execlp() failed\n"); /* Si execlp() falla, imprimimos un mensaje de error en la salida estándar de error */
        exit(EXIT_FAILURE);       /* Terminamos el proceso hijo con un código de error ya que execlp() falló */
        break;
    default:  /* Si fork() devuelve un número positivo, estamos en el proceso padre */
        if (wait(NULL) == -1)     /* El proceso padre espera a que el proceso hijo termine usando wait() */
        {
            perror("wait()");     /* Si wait() falla, imprimimos el mensaje de error */
            exit(EXIT_FAILURE);   /* Salimos del proceso padre con un código de error */
        }
        break;
    }

    return EXIT_SUCCESS;          /* Si todo ha ido bien, el proceso padre termina devolviendo EXIT_SUCCESS */
}











