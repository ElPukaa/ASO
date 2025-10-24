#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define BUF_SIZE 4096

/*
  cat_mem_din_safe.c
  Versión mejorada de cat_mem_din que:
  - Maneja escrituras parciales correctamente (write_all).
  - Redirige stderr a "error.log" usando open() + dup2().
  - Incluye comentarios explicativos en el código sobre lo que hace cada parte.

  Uso: cat_mem_din_safe [-o FILEOUT] [FILEIN1 FILEIN2 ...]
*/

void print_help(char* program_name)
{
    fprintf(stderr, "Uso: %s [-o FILEOUT] [FILEIN1 FILEIN2 ... FILEINn]\n", program_name);
}

/* write_all: intenta escribir exactamente 'size' bytes desde 'buf' en el descriptor 'fd'.
   Devuelve el número de bytes escritos (== size) o -1 en caso de error. Maneja:
   - escrituras parciales (loop hasta completar),
   - EINTR (reintenta),
   - EAGAIN/EWOULDBLOCK (reintenta, adecuado para fds no bloqueantes también),
   - y otros errores devuelve -1 con errno establecido.
 */
ssize_t write_all(int fd, const void *buf, size_t size)
{
    const char *p = buf;
    size_t left = size;//size_t sin signo para evitar problemas con comparaciones negativas
                        //ssize_t puede representar tamaños grandes y negativos
    while (left > 0) {
        ssize_t w = write(fd, p, left);
        if (w > 0) {
            left -= (size_t)w;
            p += w;
            continue;
        }

        if (w == 0) {
            /* En fds bloqueantes esto no suele ocurrir; tratar como error */
            errno = EIO;
            return -1;
        }

        /* w == -1 -> error */
        if (errno == EINTR) {
            /* señal interrumpió la llamada; reintentar */
            continue;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* En modo no bloqueante conviene aquí reintentar o esperar; para
               este ejemplo reintentamos de forma inmediata. En producción
               considerar poll/select para evitar bucle ocupado. */
            continue;
        }

        /* Error real */
        return -1;
    }

    return (ssize_t)size;
}

/* catfd: copia desde fdin a fdout usando un buffer proporcionado. */
void catfd(int fdin, int fdout, char *buf, unsigned buf_size)
{
    ssize_t num_read;

    /* Leemos en bloques hasta que read() devuelva 0 (EOF) o -1 (error) */
    while ((num_read = read(fdin, buf, buf_size)) > 0) {
        /* Tras cada lectura, escribimos exactamente num_read bytes en fdout.
           write_all garantizará que se escriba todo o devolverá -1 en caso de
           error (por ejemplo, disco lleno, permisos, etc.). */
        ssize_t num_written = write_all(fdout, buf, (size_t)num_read);
        if (num_written == -1) {
            /* Informar del error por stderr (redirigido a error.log por el main)
               y terminar con fallo. */
            perror("write(fdout)");
            exit(EXIT_FAILURE);
        }

        /* En teoría num_written == num_read siempre si write_all devolvió >=0 */
        assert(num_written == num_read);
    }

    if (num_read == -1) {
        /* read() falló: informar y terminar con error */
        perror("read(fdin)");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    int opt;
    char *fileout = NULL;
    int fdout;
    char *buf;

    /* Antes de cualquier otra cosa, redirigimos stderr a error.log usando dup2().
       Así cualquier perror() o fprintf(stderr, ...) irá al fichero error.log. */
    int fderr = open("error.log", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fderr == -1) {
        /* No podemos redirigir stderr: escribir en el stderr original y salir */
        perror("open(error.log)");
        return EXIT_FAILURE;
    }
    if (dup2(fderr, STDERR_FILENO) == -1) {
        perror("dup2");
        close(fderr);
        return EXIT_FAILURE;
    }
    /* Ya duplicado, cerramos el descriptor original devuelto por open() */
    if (close(fderr) == -1) {
        perror("close(fderr)");
        return EXIT_FAILURE;
    }

    /* Procesamiento de opciones: -o FILEOUT para redirigir la salida */
    optind = 1;
    while ((opt = getopt(argc, argv, "o:h")) != -1) {
        switch (opt) {
        case 'o':
            /* Opción -o: el argumento siguiente (optarg) contiene el
               nombre del fichero de salida. Aquí solo guardamos el nombre
               en `fileout`; la apertura del fichero (con comprobación de
               errores) se realiza más adelante. */
            fileout = optarg;
            break;
        case 'h':
            /* Opción -h: mostrar la ayuda de uso y terminar con éxito. */
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            /* Opción no reconocida: mostrar la ayuda y terminar con fallo.
               Esto evita continuar con argumentos inválidos. */
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /* Abrir fichero de salida si se indicó; si no, usar STDOUT_FILENO */
    if (fileout != NULL) {
        fdout = open(fileout, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fdout == -1) {
            perror("open(fileout)");
            exit(EXIT_FAILURE);
        }
    } else {
        fdout = STDOUT_FILENO;
    }

    /* Reserva buffer dinámico */
    buf = malloc(BUF_SIZE);
    if (buf == NULL) {
        perror("malloc");
        if (fdout != STDOUT_FILENO) close(fdout);
        exit(EXIT_FAILURE);
    }

    /* Procesar cada fichero de entrada indicado; si no hay ninguno, leer de stdin */
    if (optind < argc) {
        for (int i = optind; i < argc; i++) {
            int fdin = open(argv[i], O_RDONLY);
            if (fdin == -1) {
                /* Imprime el error en stderr, que ya está redirigido a error.log */
                perror("open(filein)");
                continue; /* seguir con los siguientes archivos */
            }

            catfd(fdin, fdout, buf, BUF_SIZE);

            if (close(fdin) == -1) {
                perror("close(fdin)");
                free(buf);
                if (fdout != STDOUT_FILENO) close(fdout);
                exit(EXIT_FAILURE);
            }
        }
    } else {
        /* Leer de stdin */
        catfd(STDIN_FILENO, fdout, buf, BUF_SIZE);
    }

    /* Cerrar fichero de salida si no es stdout */
    if (fileout != NULL) {
        if (close(fdout) == -1) {
            perror("close(fdout)");
            free(buf);
            exit(EXIT_FAILURE);
        }
    }

    free(buf);
    return EXIT_SUCCESS;
}
