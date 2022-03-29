#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

int main(void)
{
	printf("Hola, soy PID %d: \n", getpid());
		
	int fds1[2];	// fds file descriptors
	int fds2[2];	// fsdi[0] = lectura , fsdi[1] = escritura
	
	int a = pipe(fds1);
	
	if (a<0){
		perror("Error en primer pipe");
		_exit (-1);
	}	
	printf(" - primer pipe me duvuelve: [%d,%d] \n", fds1[0],fds1[1]);

	int b = pipe(fds2);
	if (b<0){
		perror ("Error en segundo pipe");
		_exit(-1);	
	}
	printf(" - segundo pipe me devuelve: [%d,%d]\n", fds2[0],fds2[1]);
	
	int i = fork();

	int valor = rand();
	 
	if (i<0){
		printf("Error en fork, PID %d \n", i);
		exit(-1);
	}

	if (i==0){

		// CASO HIJO

		close(fds1[1]);
		close(fds2[0]);

		printf("\nDonde fork me devuelve %d:\n",i);
		printf(" - getpid me devuelve:  %d\n",getpid());
		printf(" - getppid me devuelve: %d\n", getppid());

		int valor_leido_por_hijo = 0;

		if(read(fds1[0],&valor_leido_por_hijo,sizeof(valor_leido_por_hijo))<0){
			perror("Error en la lectura del hijo");
			exit(-1);
		
		}

		printf(" - recibo valor: %d\n",valor_leido_por_hijo);

		printf(" - reenvio valor en fd=%d y termino\n",fds2[1]);
		if(write(fds2[1],&valor,sizeof(valor))<0){
			perror("Error en segunod write");
			exit(-1);
		}    	

		close(fds1[0]);
		close(fds2[1]);
	}
	else{
		// CASO PADRE
	
		close(fds1[0]);
		close(fds2[1]);

		printf("\nDonde fork me devuelve: %d\n",i);
		printf(" - getpid me devuelve: %d\n",getpid());
		printf(" - getppid me devuelve: %d\n", getppid());
		printf(" - random me devuelve: %d\n",valor);

		if(write(fds1[1],&valor,sizeof(valor))<0){
			perror("Error en la escritura del padre.");
			exit(-1);
		}
		close(fds1[1]);

		printf(" - envio valor de %d a través de fd= %d\n",valor,fds1[1]);

		int valor_leido_por_padre =0;

		if(read(fds2[0],&valor_leido_por_padre,sizeof(valor_leido_por_padre))<0){
			perror("Error en la lectura del padre");
			exit(-1);
		}

		printf("\nHola, de nuevo mi PID %d\n",getpid());
		printf(" - recibi valor %d via fd= %d\n",valor_leido_por_padre,fds2[0]);

		close(fds2[0]);
	}


	return 0;
}
