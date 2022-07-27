TP2: Procesos de usuario
========================

env_alloc
---------
***¿Qué identificadores se asignan a los primeros 5 procesos creados? (Usar base hexadecimal)***

Teniendo en cuenta que en el archivo `env.c` se define ENVGENSHIFT (equivalente a 12), que NENV equivale a 1024 y el siguiente bloque de codigo perteneciente a env_alloc:

```
// Generate an env_id for this environment.
	generation = (e->env_id + (1 << ENVGENSHIFT)) & ~(NENV - 1);
	if (generation <= 0)  // Don't create a negative env_id.
		generation = 1 << ENVGENSHIFT;
    e->env_id = generation | (e - envs);
```
Cuando inicializamos la lista de enviroments en `mem_init()` definimos que el env_id correspondiente a cada enviroment será 0. Entonces, el procedimiento para asignarle a los primeros cinco procesos un identificador, es el siguiente:
En primer lugar se toma el primer nenviroment libre (primer elemento de env_free_list, que tiene un env_id 0). En segundo lugar, se ejecuta el bloque de codigo que esta arriba. En dicho bloque de codigo, se define la variable generation, asignandole como valor la suma del id proceso anterior al actual (en el caso del primer proceso el id será cero) con una constante (0x00001000). Luego a generation se le aplica una negacion de sus primeros 10 bits, para que identifique unicamente a uno de los NENV. Finalmente, lo que se asigna al env_id es un OR (|) entre la variable generation y la diferencia entre el puntero al primer env libre y el primer env de la lista envs.

El if del bloque de código, no será utilizado por los primeros procesos (no esta dentro el caso de interes de los primeros 5 procesos creados), esto se debe a que para ser un generation negativo, el primer bit de este necesariamente tiene que ser uno, es decir, el env_id tendría que ser demasiado grande. 

Dicho todo esto, los identificadores de los primeros 5 procesos serán:

* id_proceso_0 --> 0x00001000
* id_proceso_1 --> 0x00001001
* id_proceso_2 --> 0x00001002
* id_proceso_3 --> 0x00001003
* id_proceso_4 --> 0x00001004


***Supongamos que al arrancar el kernel se lanzan NENV procesos a ejecución. A continuación, se destruye el proceso asociado a envs[630] y se lanza un proceso que cada segundo, muere y se vuelve a lanzar (se destruye, y se vuelve a crear). ¿Qué identificadores tendrán esos procesos en las primeras cinco ejecuciones?***

Como el Kernel ya arrancó, ya se han lanzando los NENV procesos a ejecucion. Es por esto que entonces los env_id de los procesos son distintos de cero (ya tuvieron una asignacion). 

Como todos los procesos han sido lanzados, y se destruye el envs[630], el unico elemento que estará en env_free_list sera el mismo env[630]. Por lo tanto, en env_alloc se reutilizará ese enviroment.

Para generar el env_id de asociado al proceso que queremos lanzar, debemos utilizar el env_id del ultimo proceso lanzando, es lo que se especifica en la pregunta anterior. 


Identificadores 
* cte => 1 << ENVGENSHIFT) = 0x1000
* cte_neg =>  ~(NENV - 1) = ~0x03FF
```
>>> return(0x276 + (cte & cte_neg));
'0x1276'
>>> return(0x1276 + (cte & cte_neg))
'0x2276'
>>> return(0x2276 + (cte & cte_neg))
'0x3276'
>>> return(0x3276 + (cte & cte_neg))
'0x4276'
>>> return(0x4276 + (cte & cte_neg))
'0x5276'
```
---------

env_pop_tf
----------
***Dada la secuencia de instrucciones assembly en la función, describir qué contiene durante su ejecución:***

- el tope de la pila justo antes popal: 
    El tope de la pila antes del popal es la direccion cero del trap tf_regs
- el tope de la pila justo antes iret: 
    El tope de la pila antes del iret es la direccion del trap tf_eip que es el instruction pointer, donde comienza el modo usuario, necesario para el iret
- el tercer elemento de la pila justo antes de iret: 
    El tercer elemento es el eflags 



***En la documentación de iret en [IA32-2A] se dice:***

*If the return is to another privilege level, the IRET instruction also pops the stack pointer and SS from the stack, before resuming program execution.*



***¿Cómo determina la CPU (en x86) si hay un cambio de ring (nivel de privilegio)? *** 
*Ayuda:* Responder antes en qué lugar exacto guarda x86 el nivel de privilegio actual. ¿Cuántos bits almacenan ese privilegio?***

La CPU determina si hay un cambio de ring comparando el CPL con el RLP. Como el procesador extrae de la pila el campo **tf_cs** de trapframe se comparan sus dos bits menos significativos con el campo CPL del registro **%cs**. 
Los dos bits menos significativos del **tf_cs** representan el RPL. <br>
Como se observa a continuacion:
```
PROTECTED-MODE-RETURN: (* PE = 1 *)
IF CS(RPL) > CPL
THEN GOTO RETURN-TO-OUTER-PRIVILEGE-LEVEL;
ELSE GOTO RETURN-TO-SAME-PRIVILEGE-LEVEL; FI;
END;
```
Si el RPL es mayor que CPL se produce un cambio nivel de privilegio.<br>

---------

gdb_hello
---------
1. ***Poner un breakpoint en env_pop_tf() y continuar la ejecución hasta allí.***
```
julieta@julieta-Latitude-5490:~/Escritorio/JOS$ make gdb
gdb -q -s obj/kern/kernel -ex 'target remote 127.0.0.1:26000' -n -x .gdbinit
Reading symbols from obj/kern/kernel...
Remote debugging using 127.0.0.1:26000
warning: No executable has been specified and target does not support
determining executable automatically.  Try using the "file" command.
0x0000fff0 in ?? ()
(gdb) b env_pop_tf
Breakpoint 1 at 0xf0102f44: file kern/env.c, line 503.
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0xf0102f44 <env_pop_tf>:	endbr32 

Breakpoint 1, env_pop_tf (tf=0xf01c8000) at kern/env.c:503
503	{
```

2. ***En QEMU, entrar en modo monitor (Ctrl-a c), y mostrar las cinco primeras líneas del comando info registers.***

```
(qemu) info registers
EAX=003bc000 EBX=f01c8000 ECX=f03bc000 EDX=00000226
ESI=00010094 EDI=00000000 EBP=f0119fd8 ESP=f0119fbc
EIP=f0102f44 EFL=00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=00
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```

3. ***De vuelta a GDB, imprimir el valor del argumento tf:***

```
(gdb) p tf
$1 = (struct Trapframe *) 0xf01c8000
```
4. ***Imprimir, con x/Nx tf tantos enteros como haya en el struct Trapframe donde N = sizeof(Trapframe) / sizeof(int).***
```
(gdb) print sizeof(struct Trapframe) / sizeof(int)
$2 = 17

(gdb) x/17x tf
0xf01c8000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c8010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c8020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c8030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c8040:	0x00000023

```

5. Avanzar hasta justo después del movl ...,%esp, usando si M para ejecutar tantas instrucciones como sea necesario en un solo paso:
make run-hello-nox-gdb

```
(gdb) si 5
=> 0xf0102fd0 <env_pop_tf+13>:	popa   
0xf0102fd0 in env_pop_tf (tf=0x0) at kern/env.c:504
504		asm volatile("\tmovl %0,%%esp\n"
```

6. ***Comprobar, con x/Nx $sp que los contenidos son los mismos que tf (donde N es el tamaño de tf).***

```
(gdb) x/17x $sp
0xf01c8000:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c8010:	0x00000000	0x00000000	0x00000000	0x00000000
0xf01c8020:	0x00000023	0x00000023	0x00000000	0x00000000
0xf01c8030:	0x00800020	0x0000001b	0x00000000	0xeebfe000
0xf01c8040:	0x00000023
```
7. ***Describir cada uno de los valores. Para los valores no nulos, se debe indicar dónde se configuró inicialmente el valor, y qué representa.***

  | Pos | Valor | Atributo | Descripcion |
  |:---:|:---:|:---:|:---:|
  |  1 | 0x00000000  | reg_edi  | registro de uso general |
  |  2 | 0x00000000  | reg_esi  | registro de uso general |
  |  3 | 0x00000000  | reg_ebp  | registro de uso general |
  |  4 | 0x00000000  | reg_oesp | registro de uso general |
  |  5 | 0x00000000  | reg_ebx  | registro de uso general |
  |  6 | 0x00000000  | reg_edx  | registro de uso general |
  |  7 | 0x00000000  | reg_ecx  | registro de uso general |
  |  8 | 0x00000000  | reg_eax  | registro de uso general |
  |  9 | 0x00000023 | tf_es    | selector de segmento para segmento de datos |
  | 10 | 0x00000023 | tf_ds    | descriptor de segmento para segmento de datos |
  | 11 | 0x00000000  | tf_trapno| numero de trap |
  | 12 | 0x00000000  | tf_err   | codigo de error  |
  | 13 | 0x00800020 | tf_eip   | puntero de instruccion |
  | 14 | 0x0000001b | tf_cs    | codigo de segmento |
  | 15 | 0x00000000  | tf_eflags  | flags |
  | 16 |0xeebfe000| tf_esp | puntero de pila |
  | 17 | 0x00000023 | tf_ss      | segmento de pila |

  - Configuración inicial hecha en env_alloc:
```
e->env_tf.tf_ds = GD_UD | 3;
e->env_tf.tf_es = GD_UD | 3;
e->env_tf.tf_ss = GD_UD | 3;
e->env_tf.tf_esp = USTACKTOP;
e->env_tf.tf_cs = GD_UT | 3;
```
  - Configuración inicial hecha en load_icode:
```
e->env_tf.tf_eip = myElf->e_entry;
```


8. ***Continuar hasta la instrucción iret, sin llegar a ejecutarla. Mostrar en este punto, de nuevo, las cinco primeras líneas de info registers en el monitor de QEMU. Explicar los cambios producidos.***
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=f01c8030
EIP=f0102f57 EFL=00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=00
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
```
Cambios producidos
| Registro | Inicio BreakPoint | Inicio Iret | Descripcion |
  |:---:|:---:|:---:|:---:|
  | EAX | 003bc000 | 00000000 | Se limpia el registro |
  | EBX | f01c8000 | 00000000 | Se limpia el registro |
  | ECX | f03bc000 | 00000000 | Se limpia el registro |
  | EDX | 00000226 | 00000000 | Se limpia el registro |
  | ESI | 00010094 | 00000000 | Se limpia el registro |
  | EDI | 00000000 | 00000000 | Se limpia el registro |
  | EBP | f0119fd8 | 00000000 | Se limpia el registro |
  | ESP | f0119fbc | f01c8030 | Pasa a apuntar a otra direccion |
  | EIP | f0102f44 | f0102f57 | Pasa a apuntar a una nueva instruccion |
  | EFL | 00000092 [--S-A--] CPL=0 II=0 A20=1 SMM=00 | 00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=00    | Sin cambios |
  | ES | 0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]  | 0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]| Cambio de contexto, DPL pasa a modo usuario |
  | CS | 0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]  | 0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]   | Sin cambio  |


9. ***Ejecutar la instrucción iret. En ese momento se ha realizado el cambio de contexto y los símbolos del kernel ya no son válidos.***
```
(gdb) p $pc
$1 = (void (*)()) 0xf0102f57 <env_pop_tf+19>
(gdb) si 1
=> 0x800020:	cmp    $0xeebfe000,%esp
0x00800020 in ?? ()
(gdb) p $pc
$2 = (void (*)()) 0x800020
(gdb) p $eip
$3 = (void (*)()) 0x800020
(gdb) add-symbol-file obj/user/hello
add symbol table from file "obj/user/hello"
(y or n) y
Reading symbols from obj/user/hello...
(gdb) p $pc
$4 = (void (*)()) 0x800020 <_start>
```
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000000
ESI=00000000 EDI=00000000 EBP=00000000 ESP=eebfe000
EIP=00800020 EFL=00000002 [-------] CPL=3 II=0 A20=1 SMM=00
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
```
Cambios producidos
| Registro | Inicio Iret | Durante Iret | Descripcion |
  |:---:|:---:|:---:|:---:|
  | EAX | 00000000 | 00000000 | Sin cambios |
  | EBX | 00000000 | 00000000 | Sin cambios |
  | ECX | 00000000 | 00000000 | Sin cambios |
  | EDX | 00000000 | 00000000 | Sin cambios |
  | ESI | 00000000 | 00000000 | Sin cambios |
  | EDI | 00000000 | 00000000 | Sin cambios |
  | EBP | 00000000 | 00000000 | Sin cambios |
  | ESP | f01c8030 | eebfe000 | Pasa a apuntar a otra direccion |
  | EIP | f0102f57 | 00800020| Pasa a aputar a una nueva instruccion |
  | EFL | 00000096 [--S-AP-] CPL=0 II=0 A20=1 SMM=00    | 00000002 [-------] CPL=3 II=0 A20=1 SMM=00| Cambio de contexto, DPL pasa a modo usuario |
  | ES | 0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]|0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA] | Sin cambio |
  | CS | 0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]   |001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]| Cambio de contexto, DPL pasa a modo usuario  |



10. ***Poner un breakpoint temporal (tbreak, se aplica una sola vez) en la función syscall() y explicar qué ocurre justo tras ejecutar la instrucción int $0x30. Usar, de ser necesario, el monitor de QEMU.***


```
(gdb) si 1
The target architecture is assumed to be i8086
[f000:e05b]    0xfe05b:	cmpw   $0xffc8,%cs:(%esi)
0x0000e05b in ?? ()
``` 
```
(qemu) info registers
EAX=00000000 EBX=00000000 ECX=00000000 EDX=00000663
ESI=00000000 EDI=00000000 EBP=00000000 ESP=00000000
EIP=0000e05b EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0000 00000000 0000ffff 00009300
CS =f000 000f0000 0000ffff 00009b00
```

Se observa que el registro CS no contiene un CPL. Al intentar acceder al handler de syscall, el mismo todavia no fue seteado, lo que generara un error en la comparacion del RPL con el CPL 

---------

kern_idt
--------

***Leer user/softint.c y ejecutarlo con make run-softint-nox.¿Qué interrupción trata de generar? ¿Qué interrupción se genera? Si son diferentes a la que invoca el programa… ¿cuál es el mecanismo por el que ocurrió esto, y por qué motivos? ¿Qué modificarían en JOS para cambiar este comportamiento?***

En el archivo softint.c se puede apreciar que lo que se busca es lanzar un page fault. Cuando lo corremos con make run, se observa lo siguiente:

```
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
TRAP frame at 0xf01c8000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdff0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x0000000d General Protection
  err  0x00000072
  eip  0x00800037
  cs   0x----001b
  flag 0x00000082
  esp  0xeebfdfd4
  ss   0x----0023
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
```
Se esta lanzando un General Protection Fault. Esto ocurre dado que estamos intentando lanzar un page fault desde un programa de usuario, mientras que el page fault tienen como dpl 0, es decir, pertenece al ring 0, solo el Kernel puede hacerlo.

Para verlo explicitamente, modificamos el dpl en el setgate del Page Fault, colocando un 3 (como si perteneciera al user ring). Obteniendo entonces lo siguiente:

```
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffc0
[00001000] user fault va 00000000 ip 0000001b
TRAP frame at 0xefffffc0
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdff0
  oesp 0xefffffe0
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x0000000e Page Fault
  cr2  0x00000000
  err  0x00800039 [kernel, read, protection]
  eip  0x0000001b
  cs   0x----0082
  flag 0xeebfdfd4
  esp  0x00000023
  ss   0x----ff53
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
```



---------

user_evilhello
--------------

**Ejecutar el siguiente programa y describir qué ocurre:**

```c
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
    char *entry = (char *) 0xf010000c;
    char first = *entry;
    sys_cputs(&first, 1);
}
``` 
***respondiendo a las siguientes preguntas:***

***1. ¿En qué se diferencia el código de la versión en evilhello.c mostrada arriba?***

Los códigos se diferencian en que:
  - En el código original del esqueleto pasa directamente la dirección virtual de memoria.
  - En el código propuesto en el enunciado lo que sucede se desreferencia. Es decir, se crea un puntero en la dirección, luego se declara el char first que apunta a la dirección mencionada anteriormente, con esto se intenta acceder a una dirección sin acceso para el usuario.


***2. ¿En qué cambia el comportamiento durante la ejecución?***
  - ¿Por qué? ¿Cuál es el mecanismo?

Cuando ejecutamos el codigo brindado en el esqueleto del tp, lo que sucede es lo siguiente:

```
Incoming TRAP frame at 0xefffffbc
[00001000] user_mem_check assertion failure for va f010000c
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!
```

Cuando modificamos el código insertando el propuesto en el enunciado, paso lo siguiente:

```
Incoming TRAP frame at 0xefffffbc
[00001000] user fault va f010000c ip 0080003d
TRAP frame at 0xf01c8000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdfd0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x0000000e Page Fault
  cr2  0xf010000c
  err  0x00000005 [user, read, protection]
  eip  0x0080003d
  cs   0x----001b
  flag 0x00000082
  esp  0xeebfdfb0
  ss   0x----0023
[00001000] free env 00001000
Destroyed the only environment - nothing more to do!

```
Se puede observar que se produce un Page Fault al intentar desreferenciar el puntero, ya que estando en el espacio del usuario, no se cuenta con permiso para acceder a la dirección deseada, la que se encarga de verificar si el acceso intencionado es o no posible, es la MMU.

***3. Listar las direcciones de memoria que se acceden en ambos casos, y en qué ring se realizan. ¿Es esto un problema? ¿Por qué?***

Las direcciones de memoria que se acceden en ambos casos en la direccion `f010000c`.

Desde el código original se intenta acceder desde el espacio del kernel, es decir en ring 0. Para evitar accesos indebidos la funcion comprueba los permisos y destruye el env malicioso.

Muy distinto pasa en el ejemplo del enunciado como tarea. En este caso se intenta acceder desde el espacio de usuario, ring 3, al kernel haciendo una desreferencia. Esto es un problema porque el usuario no tiene permiso.
Lo que debe hacer el kernel es establecer de antemano con que permisos se debe contar para acceder a las páginas mapeadas, y la MMU verificar si la página está presente y de ser así, si se cuenta con los permisos suficientes.




