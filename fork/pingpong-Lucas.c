#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void error(){
	perror("Error en el sistema");
	_exit(-1);

	return;
}

int main(void)
{
	
	/*Hola, soy PID <x>:
  - primer pipe me devuelve: [3, 4]
  - segundo pipe me devuelve: [6, 7]

Donde fork me devuelve <y>:
  - getpid me devuelve: <?>
  - getppid me devuelve: <?>
  - random me devuelve: <v>
  - envío valor <v> a través de fd=?

Donde fork me devuelve 0:
  - getpid me devuelve: <?>
  - getppid me devuelve: <?>
  - recibo valor <v> vía fd=?
  - reenvío valor en fd=? y termino

Hola, de nuevo PID <x>:
  - recibí valor <v> vía fd=?*/
	
	printf("Hola, soy PID %d \n",getpid());
	/* 	Siglas 
	 	* FtS = Father to Son
	 	* StF = Son to Father
	 */
	/* 	posicion 0 -> read; 
		posicion 1 -> write
	*/
	int FtS[2]; 		
	int StF[2];
	
	
	int fts =  pipe(FtS);
	int stf = pipe(StF);

	if(fts<0){
		error();
	}
	printf("\t- primer pipe me devuelve: [%d,%d] \n",FtS[0],FtS[1]);

	if(stf<0){
		error();	
	}
	printf("\t- segundo pipe me devuelve: [%d,%d] \n",StF[0],StF[1]);
	printf("\n");
	
	pid_t pidC;
	pidC = fork();
	
	int msg_envio = rand();
	int msg_recibido;

	switch(pidC)
	{
		case 0: //HIJO
			close(FtS[1]);
			close(StF[0]);
			read(FtS[0],&msg_recibido,sizeof(msg_recibido));
			
			printf("Donde fork me devuelve %d: \n",pidC);				
			printf("\t- getpid me devuelve: %d\n\t- getppid me devuelve: %d\n\t- recibo valor: %d via fd=%d\n\t- reenvio valor en fd= %d y termino \n "
					,getpid(),getppid(),msg_recibido,FtS[0],StF[1]);
			
			write(StF[1],&msg_recibido,sizeof(msg_recibido));

			close(StF[1]);
			close(FtS[0]);
			break;

		case -1: //error
			break;

		default: //padre
			close(FtS[0]);
			write(FtS[1],&msg_envio,sizeof(msg_envio));
			
			printf("Donde fork me devuelve %d: \n",pidC);				
			printf("\t- getpid me devuelve: %d\n\t- getppid me devuelve: %d\n\t- random me devuelve: %d\n\t- envio valor %d a traves de fd= %d \n "
					,getpid()
					,getppid()
					,msg_envio
					,msg_envio
					,FtS[1]);
			close(FtS[1]);
			break;
	}     
	
	if(pidC>0){
		close(StF[1]);
		read(StF[0],&msg_recibido,sizeof(msg_recibido));
		printf("Hola, de nuevo PID %d:\n\t- recibí valor %d vía fd=%d\n",getpid(),msg_recibido,StF[0]);
		close(StF[0]);
	}
	
	return 0;
}
