TP4: Sistema de archivos e intérprete de comandos
=================================================

caché de bloques
----------------

**¿Qué es `super->s_nblocks`?**

Indica la cantidad total de bloques existentes en el disco.

Esto es indicado en *inc/fs.h*, como se muestra a continuación:

```
uint32_t s_nblocks;		// Total number of blocks on disk
```

**¿Dónde y cómo se configura este bloque especial?**

Analizando `Struct Super` y `Struct File`, podemos determinar que este bloque especial es configurado en *fs/fsformat.c*, con las siguientes líneas de código:

```
	super->s_magic = FS_MAGIC; // se asigna el numero magico
	super->s_nblocks = nblocks; // se asigna la cantidad de bloques
	super->s_root.f_type = FTYPE_DIR; // se asigna el tipo del nodo del directorio raiz
	strcpy(super->s_root.f_name, "/"); // se asigna el nombre del nodo del directorio raiz
```

