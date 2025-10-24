#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#define BUF_SIZE 4096

/*  Ejemplo: cat_mem_din [-o FILEOUT] [FILEIN1 FILEIN2 ... FILEINn]
    Notas:   Por defecto, se escribe en 'stdout' y se lee de 'stdin'.
*/

void print_help(char* program_name)
{
    fprintf(stderr, "Uso: %s [-o FILEOUT] [FILEIN1 FILEIN2 ... FILEINn]\n", program_name);
}
int write_all(int fd, char* buf, ssize_t size)
{
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

void catfd(int fdin, int fdout, char *buf, unsigned buf_size)
{
    ssize_t num_read, num_written;

    while ((num_read = read(fdin, buf, buf_size)) > 0)//lee del fdin y lo guarda en buf
    {
        num_written =write_all(fdout, buf, num_read); // write(fdout, buf, num_read); 
        if (num_written == -1)
        {
            perror("write(fdin)");
            exit(EXIT_FAILURE);
        }
        /* Escrituras parciales no tratadas */
        assert(num_written == num_read);
    }

    if (num_read == -1)
    {
        perror("read(fdin)");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    int opt;
    char *fileout = NULL;
    int fdout;
    int fdin;
    char *buf;

    optind = 1;
    while ((opt = getopt(argc, argv, "o:h")) != -1)
    {
        switch (opt)
        {
        case 'o':
            fileout = optarg;
            break;
        case 'h':
            print_help(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    int fderr = open ("error.log", O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fderr == -1)
    {
        perror("open(error.log)");
        exit(EXIT_FAILURE);
    }
    if (dup2(fderr, STDERR_FILENO) == -1)
    {
        perror("dup2");
        exit(EXIT_FAILURE);
    }
    if (close(fderr) == -1)
    {
        perror("close(fderr)");
        exit(EXIT_FAILURE);
    }

    /* Abre el fichero de salida */
    if (fileout != NULL)
    {
        fdout = open(fileout, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (fdout == -1)
        {
            perror("open(fileout)");
            exit(EXIT_FAILURE);
        }
    }
    else /* Por defecto, la salida estándar */
        fdout = STDOUT_FILENO;

    /* Reserva memoria dinámica para buffer de lectura */
    if ((buf = (char *) malloc(BUF_SIZE * sizeof(char))) == NULL)
    {
        perror("malloc()");
        exit(EXIT_FAILURE);
    }

    /* Abre cada fichero de entrada y lo escribe en 'fileout' */
    if (optind < argc)
        for (int i = optind; i < argc; i++)
        {
            int fdin = open(argv[i], O_RDONLY);
            if (fdin == -1)
            {
                perror("open(filein)");
                continue;
            }
            catfd(fdin, fdout, buf, BUF_SIZE);
            if (close(fdin) == -1)
            {
                perror("close(fdin)");
                exit(EXIT_FAILURE);
            }
        }
    else
    {
        catfd(STDIN_FILENO, fdout, buf, BUF_SIZE);
    }

    /* Cierra el fichero de salida */
    if (fileout != NULL)
    {
        if (close(fdout) == -1)
        {
            perror("close(fdout)");
            exit(EXIT_FAILURE);
        }
    }

    /* Libera memoria dinámica de buffer de lectura */
    free(buf);

    exit(EXIT_SUCCESS);
}
