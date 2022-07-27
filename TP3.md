TP3: Multitarea con desalojo
============================

**sys_yield**
---------
1. Manejar, en kern/syscall.c, la llamada al sistema SYS_yield, que simplemente llama a sched_yield(). En la biblioteca de usuario de JOS ya se encuentra definida la función correspondiente sys_yield().

2. Leer y estudiar el código del programa user/yield.c. Cambiar la función i386_init() para lanzar tres instancias de dicho programa, y mostrar y explicar la salida de make qemu-nox.

*Respuesta*

Secuencia de ejecucion de programa *user/yield.c*
```
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000
Hello, I am environment 00001001
Hello, I am environment 00001002
Back in environment 00001000, iteration 0.
Back in environment 00001001, iteration 0.
Back in environment 00001002, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001001, iteration 1.
Back in environment 00001002, iteration 1.
Back in environment 00001001, iteration 2.
Back in environment 00001002, iteration 2.
Back in environment 00001000, iteration 2.
Back in environment 00001001, iteration 3.
Back in environment 00001002, iteration 3.
Back in environment 00001000, iteration 3.
Back in environment 00001001, iteration 4.
All done in environment 00001001.
Back in environment 00001002, iteration 4.
All done in environment 00001002.
[00001002] exiting gracefully
[00001002] free env 00001002
Back in environment 00001000, iteration 4.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
[00001001] exiting gracefully
[00001001] free env 00001001
No runnable environments in the system!
```

*yield.c* plasma el comportamiento de la politica de planificacion *Round Robin*. Podemos observar en la secuencia que las iteraciones se van alternando segun los procesos que se encuentra en la cola a la espera de ejecucion, donde todos tiene un mismo *time slice* asignado.

Otro aspecto de la observacion es que cada uno de los procesos tiene el mismo *run time*, como consecuencia esto conlleva a que todos finalicen en la misma cantidad de iteraciones. 




**dumbfork**
--------

1. **Si una página no es modificable en el padre ¿lo es en el hijo? En otras palabras: ¿se preserva, en el hijo, el flag de solo-lectura en las páginas copiadas?**

*Respuesta*

No, no es modificable en el hijo tampoco. El flag de *solo-lectura* no se preserva en el hijo. 

Siguiendo el hilo de ejecución de la función `dumbfork`, se puede observar que llama a `duppage` que a la vez llama a la función `sys_page_alloc`, en la que los permisos pasados son *PTE_P|PTE_U|PTE_W*. 


2. **Mostrar, con código en espacio de usuario, cómo podría dumbfork() verificar si una dirección en el padre es de solo lectura, de tal manera que pudiera pasar como tercer parámetro a duppage() un booleano llamado readonly que indicase si la página es modificable o no:**

```
envid_t dumbfork(void) {
    // ...
    for (addr = UTEXT; addr < end; addr += PGSIZE) {
        bool readonly;
        //
        // TAREA: dar valor a la variable readonly
        //
        duppage(envid, addr, readonly);
    }
    // ...
```
*Respuesta*

```
for (addr = (uint8_t*) UTEXT; addr < end; addr += PGSIZE){
    if ((uvpt[addr] & PTE_SYSCALL)){
        duppage(envid, ROUNDDOWN(&addr,PGSIZE),0);
        continue;
    }
    duppage(envid, ROUNDDOWN(&addr,PGSIZE),1);
}
```

3. Supongamos que se desea actualizar el código de duppage() para tener en cuenta el argumento readonly: si este es verdadero, la página copiada no debe ser modificable en el hijo. Es fácil hacerlo realizando una última llamada a sys_page_map() para eliminar el flag PTE_W en el hijo, cuando corresponda:

```
void duppage(envid_t dstenv, void *addr, bool readonly) {
    // Código original (simplificado): tres llamadas al sistema.
    sys_page_alloc(dstenv, addr, PTE_P | PTE_U | PTE_W);
    sys_page_map(dstenv, addr, 0, UTEMP, PTE_P | PTE_U | PTE_W);
    memmove(UTEMP, addr, PGSIZE);
    sys_page_unmap(0, UTEMP);
    // Código nuevo: una llamada al sistema adicional para solo-lectura.
    if (readonly) {
        sys_page_map(dstenv, addr, dstenv, addr, PTE_P | PTE_U);
    }
}
```

Esta versión del código, no obstante, incrementa las llamadas al sistema que realiza duppage() de tres, a cuatro. Se pide mostrar una versión en el que se implemente la misma funcionalidad readonly, pero sin usar en ningún caso más de tres llamadas al sistema.

*Respuesta*

```
void duppage2(envid_t dstenv, void *addr, bool readonly) {

	int perm; 
	if (readonly){
		perm = PTE_P | PTE_U ;
	}else{
		perm = PTE_P | PTE_U | PTE_W;
	}
	sys_page_alloc(dstenv, addr, perm);
	sys_page_map(dstenv, addr, 0, UTEMP, perm);
  memmove(UTEMP, addr, PGSIZE);
	sys_page_unmap(0, UTEMP);
}
```



**ipc_recv**
--------

1. Un proceso podría intentar enviar el valor númerico -E_INVAL vía ipc_send(). ¿Cómo es posible distinguir si es un error, o no?

```
envid_t src = -1;
int r = ipc_recv(&src, 0, NULL);

if (r < 0)
  if (/* ??? */)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")
```

*Respuesta*

Para distinguir el error basta con saber como es el funcionamiento interno de `ipc_recv`:
```
int32_t
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	[..]
	envid_t env_store_ret = 0;
    [..]
    if (from_env_store) {
		*from_env_store = env_store_ret;
	}
    [..]
}
```

Iteramos con los valores `ipc_recv(&src, 0, NULL)`
Como src = -1, el environmet no es un valor valido.
Basta hacer la comparacion entonces 
```
if (r < 0)
  if (src == 0)
    puts("Hubo error.");
  else
    puts("Valor negativo correcto.")
```



**sys_ipc_try_send**
----------------
Es posible que surjan varias alternativas de implementación; para cada una, indicar:
1. ¿Qué cambios se necesitan en struct Env para la implementación (campos nuevos, y su tipo; campos cambiados, o eliminados, si los hay)?
2. ¿Qué asignaciones de campos se harían en sys_ipc_send()?
3. ¿Qué código se añadiría en sys_ipc_recv()?
4. ¿Existe posibilidad de deadlock? 
5. ¿Funciona que varios procesos (A₁, A₂, …) puedan enviar a B, y quedar cada uno bloqueado mientras B no consuma su mensaje? ¿En qué orden despertarían?

*Respuesta*
1. Cambios en *struct Env*:
* **old** : `bool env_ipc_recving` --> **change new**: `bool env_ipc_blocked`<br> Este campo indicaria si el proceso esta bloqueado enviando o recibiendo. Por convencion `true` en caso de que envie o reciba y `false` en caso de que no envie o no reciba.
* **new**: `envid_t env_ipc_from_recv` <br> Campo del environmet que recibe
* **old** : `envid_t env_ipc_from` --> **change new**: `envid_t env_ipc_from_send`<br> Campo del environmet que envia
* **new**: `struct Env *envs_send`<br> Una lista de envs esperando para enviar.
2. Se hace un try_send al environment que se recibe, en caso de falla, el current es agregado a la lista de envs_send, se le setea el respectivo reciever y se lo pone a dormir con ENV_NOT_RUNNABLE. 
3. Se chequea si el current no esta como reciever en la lista de los envs_send para despertarlo y sacar de la lista al sender.
4. Si. Para evitarlo se puede lanzar error si el env al que se quiere enviar esta bloqueado enviando.
5. Si, despertarian en orden de llegada.

