#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

void procesar_hijo(int fds[])
{
	//desarrollar colador

}

int main(int argc, char *argv[]){

	int numero = atoi(argv[1]);

	if(numero <=2){
		printf("Error. Ingresar mayor a 2\n");
		exit(-1);
	}

	int fds[2];

	int a = pipe(fds);

	if(a<0){
		perror("Error pipe");
		exit(-1);
	}

	int i = fork();

	if(i<0){
		printf("Error en fork");
		exit(-1);
	}

	if (i==0){
		//Caso hijo
		procesar_hijo(fds);
	}
	else{
		//Caso padre
		close(fds[0]);

		//generar los N numeros
	}



	return 0;

}
