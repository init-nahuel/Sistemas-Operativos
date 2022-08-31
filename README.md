# Tareas SOS
Repositorio con las tareas del curso Sistemas Operativos

# Temas
* Tarea 1: Paralelizacion con threads
# Introduccion a Paralelizacion

```c
#include <pthread.h>

// Creacion
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
/* 
-thread: Puntero donde se guardara el identificador del thread
-attr: Puntero a estructura con los atributos del thread, en principio sera NULL
-start_routine: Puntero a la funcion de inicio del thread, esta funcion debe retornar un puntero opaco (void *), 
y debe recibir otro puntero opaco donde se encuentran los argumentos de la funcion
-arg: Puntero a la estructura en donde se colocan los argumentos de la funcion de inicio del thread, el tipo de este puntero es opaco
*/

// Termino
void pthread_exit(void *ret);
/*
-ret: Puntero a la estructuctura con los valores retornados
 Tambien se termina cuando la funcion star_routine retorna
*/

// Esperar a que el thread termine (muere y entierra)
int pthread_join(pthread_t t, void **pret);
/*
-t: Identificador del thread
-pret: Doble puntero donde quedara almacenado el puntero que contiene los valores retornador por el thread, usualmente usaremos NULL
*/
```

# Paralelizacion y Mutex

```c
// Tipo
pthread_mutex_t

// Inicializacion
int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t mutexattr);
/*
-m: Direccion del mutex que se quiere inicializar
-mutexattr: Atributos del mutex, por ahora sera NULL

OBS: En caso de querer inicializar mas de un mutex, se puede declarar un arreglo de tipo mutex, con la macro PTHREAD_MUTEX_INITIALIZER:
pthread_mutex_t [2]={PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER}; 
*/

// Destruccion
int pthread_mutex_destroy(pthread_mutex_t *m)
/*
-m: Direccion del mutex
*/
```

## Funciones utiles para mutex

```c
// Funcion para BLOQUEAR el mutex, se usa justo antes de entrar en la seccion critica
int pthread_mutex_lock(pthread_mutex_t *m);

// Funcion para LIBERAR el mutex, se usa justo al termino de la seccion critica
int pthread_mutex_unlock(pthread_mutex_t *m);
```

# Condiciones

Todas estas funciones retornan 0 si se ejecutan correctamente

```c
// Tipo
pthread_cond_t


// Inicializacion
int pthread_cond_init(pthread_cond_t *cond, pthread_condattr *a);

// Destruccion
int pthread_cond_destroy(pthread_cond_t *cond);

// Espera
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *m);

/*
Obs: El mutex debe estar cerrado en el momento de la invocacion. 
Esta funcion suspende el thread que la invoca hasta que se invoca broadcast o signal. Por otra parte, cuando se invoca `wait` se abre el mutex
*/

// Broadcast
int pthread_cond_broadcast(pthread_cond_t *cond);

/*
Despierta a todos los thread que estan a la espera en la condicion cond
*/

// Signal
int pthread_cond_signal(pthread_cond_t *cond);

/*
Despierta a uno de los thread que esta a la espera en la condicion cond
*/

/*
OBS: Despertar es relativo, un thread que se despierta todavia tiene que esperar
a que se libere el mutex, que esta tomado por el thread que realiza la invocacion de broadcast o signal
*/
```
