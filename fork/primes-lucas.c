#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>

void error(char* message);

void procesoDerecho(int fd_izquierdo[])
{
	int primo = 0;
	if (read(fd_izquierdo[0], &primo, sizeof(primo)) == 0){
		close(fd_izquierdo[0]);
		return;
	}

	printf("prime %d\n", primo);

	int fds_derecho[2];
	int r_new = pipe(fds_derecho);
	if (r_new < 0) error("Error en pipe");

	int i_new = fork();
	if (i_new < 0) error("Error en fork!");

	else if (i_new == 0){ //nuevo hijo
		close(fd_izquierdo[0]);
		close(fds_derecho[1]);
		procesoDerecho(fds_derecho);

	} else { //nuevo padre
		close(fds_derecho[0]);

		int n = 0;
		while (read(fd_izquierdo[0], &n, sizeof(n)) > 0){
			if (n % primo != 0){
				if (write(fds_derecho[1], &n, sizeof(n)) < 0) error("Error en el write");
			}
		}
		close(fd_izquierdo[0]);
		close(fds_derecho[1]);
		wait(NULL);
	}
	return;
}

int
main(int argc, char *argv[])
{
	int num = atoi(argv[1]); //recibo el número por argumento del main
	if(num<2) error("Error en numero ingresado");

	int fds[2];
	int r = pipe(fds); //creo un pipe
	if (r < 0) error("Error en pipe");

	int pid = fork(); //hago fork
	if(pid<0) error("Error en Fork");

	switch (pid)
	{
	case 0:
		close(fds[1]); //hijo no escribe
		procesoDerecho(fds);
		break;
	default:
		close(fds[0]); //padre no lee

			for (int j = 2; j <= num; j++){ //envío la secuencia de números desde 2 hasta n
				if (write(fds[1], &j, sizeof(j)) < 0) error("Error en el write");
			}
			close(fds[1]);
			wait(NULL);
		break;
	}
	return 0;
}

void error(char* message){
	perror(message);
	_exit(0);	
	return;
}