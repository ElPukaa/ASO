#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int main(void) {
  int fd;

  fd = open("prueba_dup2.txt", O_CREATE | O_WRONLY);
  if (fd < 0) {
    printf(2, "Error al crear el archivo\n");
    exit(0);
  }

  printf(1, "Este mensaje sale por PANTALLA.\n");

  //redirigimos el puerto 1 (pantalla) al archivo
  if (dup2(fd, 1) < 0) {
    printf(2, "Error al ejecutar dup2\n");
    exit(0);
  }

  printf(1, "Este mensaje deberia estar DENTRO del archivo.\n");
  printf(1, "Dup2 esta funcionando.\n");

  close(fd);
  exit(0);
}
