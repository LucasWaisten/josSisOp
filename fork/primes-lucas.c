#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


int procesoDerecho(int* array,int* fd){
	/*pensar la condicion de corte*/
}


int main(int argc, char *argv[]){
	int num = atoi(argv[1]);
	if(num<2){
		perror("Error numero ingresado");
		_exit(0);
	}
	int array[num]; 
	int alectura[num];

	int fd[2];
	pipe(fd);
	/* 	fd[0] -> read; 
		fd[1] -> write
	*/

	__pid_t pid;
	pid = fork();

	switch (pid)
	{
	case 0:
		/*Primero proceso derecho */
		close(fd[1]);
		read(fd[0],alectura,sizeof(alectura));
		procesoDerecho(alectura,fd);
		close(fd[0]);
		break;
	case -1:
		/* Error*/
		break;
	
	default:
		/* Primer Proceso */
		close(fd[0]);
		for(int i=0;i<=num;i++){
			array[i] = i;
			printf("%d",array[i]);
		}

		write(fd[1],array,sizeof(array));

		close(fd[1]);
		break;
	}

	return 0;

}
