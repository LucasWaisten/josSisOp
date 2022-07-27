TP1: Memoria virtual en JOS
===========================

boot_alloc_pos
--------------
***Consigna: Un cálculo manual de la primera dirección de memoria que devolverá boot_alloc() tras el arranque.*** 

Esto se puede calcular usando los comandos readelf y/o nm y operaciones matemáticas, a partir del binario compilado (obj/kern/kernel). En nuestro caso utilizamos el readelf y el nm, para verficar que ambos nos dieran la misma direccion. A su vez, truncamos con grep a ambos y obtuvimos los siguientes resueltados:

**Caso readelf:**

```
julieta@julieta-Latitude-5490:~/Escritorio/JOS_Sisop$ readelf -s obj/kern/kernel | grep end
111: f0118950     0 NOTYPE  GLOBAL DEFAULT    6 end

```
**Caso nm:**
```
julieta@julieta-Latitude-5490:~/Escritorio/JOS_Sisop$ nm obj/kern/kernel | grep end
f0118950 B end
```

*Conclusiones:* Luego de ejecutados ambos comandos, podemos observar que en ambos casos, la dirección de memoria que recibe boot_alloc() es f0117950. Una vez que reciba este valor, el boot_alloc() (en su primera llamada en mem_init()), realiza un llamado a ROUNDUP(a,n) donde esta direccion pasada a decimal (4027677008) sera redondeada a 4096 (PGSIZE). Devuelve ese valor redondeado, y a su vez, guarda la variable nextfree en la pagina siguiente a la recibida.

***Consigna: Una sesión de GDB en la que, poniendo un breakpoint en la función boot_alloc(), se muestre el valor de end y nextfree al comienzo y fin de esa primera llamada a boot_alloc().***

```
lucaswaisten@lucaswaisten-Sword-15-A11UD:~/Documents/tp1SisOp$ make gdb
gdb -q -s obj/kern/kernel -ex 'target remote 127.0.0.1:26000' -n -x .gdbinit
Reading symbols from obj/kern/kernel...
Remote debugging using 127.0.0.1:26000
warning: No executable has been specified and target does not support
determining executable automatically.  Try using the "file" command.
0x0000fff0 in ?? ()

(gdb) break boot_alloc
Breakpoint 1 at 0xf0100b29: file kern/pmap.c, line 89.

(gdb) continue
Continuing.
The target architecture is assumed to be i386
=> 0xf0100b29 <boot_alloc>:	push   %ebp
Breakpoint 1, boot_alloc (n=65684) at kern/pmap.c:89
89	{

(gdb) print(char *) &end
$1 = 0xf0118950 ""

(gdb) watch &end<br>
Watchpoint 2: &end

(gdb) watch nextfree<br>
Hardware watchpoint 3: nextfree

(gdb) continue
Continuing.
=> 0xf0100b8b <boot_alloc+98>:	jmp    0xf0100b39 <boot_alloc+16>

Hardware watchpoint 3: nextfree

Old value = 0x0
New value = 0xf0119000 ""
0xf0100b8b in boot_alloc (n=4096) at kern/pmap.c:100
100			nextfree = ROUNDUP((char *) end, PGSIZE);

(gdb) continue
Continuing.
=> 0xf0100b73 <boot_alloc+74>:	mov    %esi,%eax

Hardware watchpoint 3: nextfree

Old value = 0xf0119000 ""
New value = 0xf011a000 ""
boot_alloc (n=4096) at kern/pmap.c:120
120	}

(gdb) continue
Continuing.
=> 0xf0100b29 <boot_alloc>:	push   %ebp

Breakpoint 1, boot_alloc (n=4027682816) at kern/pmap.c:89
89	
```

Observamos lo siguente con el breakpoint en la funcion ***boot_alloc***:
* Valor de **end** al principio de la llamada : *0xf0118950* 
* Valor de **end** al principio hecho por la funcion roundup : _0xf0119000_
* Valor de **nextfree** new value : _0xf011a000_
* Valor de **end** al final de la llamada : _0xf011a000_

page_alloc
----------
...


map_region_large
----------------

***Consigna***: 
* A ¿Cuánta memoria se ahorró de este modo?. 
* B ¿Es una cantidad fija, o depende de la memoria física de la computadora?

A) 
Cuando usamos *large page*, una entrada del *page directory* va a mapear a lo que mapeaba una entrada de la *page table* pero sin necesidad de esa *page table* intermedia por lo que mapea 4MiB. El *pde* pasa a contener dos entradas del *page directory* que apuntan a una misma *page table* que tiene un mapeo harcodeado de unas 1024 entradas. Entonces, para evitar el uso de *page tables* usamos las *large pages*. **Esto nos induce a un ahorro de memoria**. El ahorro por cada *large page* utilizada, es el espacio ocupado por una *page table*, es decir 1024(entradas) x 4 bytes = 4 KiB.

En nuestro TP de JOS, en la inicialización de memoria (mem_init()), se realizan tres llamados a boot_map_region(), que recibe un page directory,su va (direccion virtual), su pa (dirección física), su size (que seria el tamaño de la región a mapear) y los permisos. Luego, genera  su mapeo.

En el primer llamado se mapea la región *UPAGES*, la cual no tiene la dirección física alineada, por lo tanto no se utilizan large pages, en efecto, no se ahorra memoria en este llamado. 

En el segundo llamado se mapea la región que abarca KSTACKTOP - KSTKSIZE (siendo KSTKSIZE = 8 * PGSIZE y a su vez, PGSIZE = 4096 bytes). Podemos observar que las pages no son de 4Mb, son mas pequeñas. Tampoco ahorramos memoria en esta región.

En el tercer y último llamado, se mapea la región comprendida por KERNTOP (0xFFFFFFFF) - KERNBASE (0xF0000000), que sus direcciones tanto fisica como virtuales estan alineadas y sus tamaños son de 4MiB. Por lo tanto, solo en este llamado se ahorra memoria. El tamaño de este área es: FFFFFFFF - F0000000 = 256MiB. Por lo tanto, 256MiB / 4MiB = 64 *large pages*. 

Dicho todo lo anterior, podremos alocar 64 *large pages*, si por cada *large page* ahorramos 4KiB, el ahorro total es 64 x 4 KiB = 256 KiB.


B) **Es una cantidad fija**. Independientemente de la memoria que tengamos disponible siempre vamos a mapear 256MiB, es decir 64 *large pages*.

...
