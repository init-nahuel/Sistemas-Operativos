# Equivalencia entre semaforos y mutex/condiciones

* Todo problema resuelto con semaforos se puede resolver con mutex y condiciones.
* Para demostrarlo basta implementar semaforos a partir de mutex y condiciones.
* **En la solucion con semaforos basta reemplazar las llamadas a las funciones de los semaforos por las funciones que implementan los semaforos a partir de mutex y condiciones.**
* Todo problema resuelto con mutex y condiciones se puede resolver con semaforos.
* Para demostralo basta implementar los mutex y condiciones a partir de semaforos.
* **En la solucion con mutex y condiciones basta reemplazar las llamadas a las funciones de los mutex y condiciones por las funciones que los implementan a partir de semaforos.**

# Mutex y condiciones a partir de semaforos

## API

La API que se implementará es la siguiente, la semantica cambia pero al final son las mismas funciones que existen para pthreads.

* `typedef struct mutex Mutex`: Equivalente al tipo `pthread_t`.
* `typedef struct cond Cond`: Equivalente al tipo `pthread_cond_t`.
* `void mutex_init(Mutex *m);`: Equivalente a la funcion `pthread_mutex_init()`.
* `void cond_init(Cond *c);`: Equivalente a la funcion `pthread_cond_init()`.
* `void mutex_destroy(Mutex *m);`: Equivalente a la funcion `pthread_mutex_destroy()`.
* `void cond_destroy(Cond *c);`: Equivalente a la funcion `pthread_cond_destroy()`.
* `void mutex_lock(Mutex *m);`: Equivalente a la funcion `pthread_mutex_lock()`.
* `void mutex_unlock(Mutex *m);`: Equivalente a la funcion `pthread_mutex_unlock()`.
* `void cond_wait(Cond *c, Mutex *m);`: Equivalente a la funcion `pthread_cond_wait()`.
* `void cond_signal(Cond *c);`: Equivalente a la funcion `pthread_cond_signal()`.
* `void cond_broadcast(Cond *c);`: Equivalente a la funcion `pthread_cond_broadcast()`.

**OBS:** En el caso de la declaracion de los typedef, la declaracion es una forma de abstraccion que tiene por objetivo que el usuario no veo los campos de las estructuras, el solo verá el `typedef struct mutex Mutex`. Es el analogo a `private` en Java.

## Implementacion

```c
#include "mutex.h" // Declaracion de la API

struct mutex {
  sem_t mutex_sem;
  Queue *mq;
};

struct cond {
  Mutex *m;
  Queue *wq;
};

void mutex_init(Mutex *m) {
  sem_init(&m->mutex_sem, 0, 1);
  m->mq = makeQueue();
}

void cond_init(Cond *c) {
  c->wq = makeQueue();
}

void mutex_destroy(Mutex *m) {
  sem_destroy(&m->mutex_sem);
  destroyQueue(m->mq);
}

void cond_destroy(Cond *c) {
  destroyQueue(c->wq);
}

void mutex_lock(Mutex *m) {
  sem_wait(&m->mutex_sem);
}

void mutex_unlock(Mutex *m) {
  if (emptyQueue(m->mq))
    sem_post(&m->mutex_sem);
  else{
    sem_t *pw_sem = get(m->mq);
    sem_post(pw_sem);
  }
}

void cond_wait(Cond *c, Mutex *m) {
  c->m = m;
  sem_t w_sem;
  sem_init(&w_sem, 0, 0);
  put(c->wq, &w_sem);
  mutex_unlock(m);
  sem_wait(&w_sem);
}

void cond_signal(Cond *c) {
  if (!emptyQueue(c->wq)) {
    sem_t *pw_sem = get(c->wq);
    put(c->m->->mq, pw_sem);
  }
}

// En caso del broadcast se implementa un while
void cond_signal(Cond *c) {
  while (!emptyQueue(c->wq)) {
    sem_t *pw_sem = get(c->wq);
    put(c->m->->mq, pw_sem);
  }
}
```

* `Queue *mq`: Esta cola de la estructura `mutex` es una cola FIFO donde esperan los threads por la propiedad del mutex.
* `cond_wait()`: El semaforo `w_sem` que se crea para esperar se inicializa con 0 fichas para que espera, luego se agrega a la cola `Queue *wq` de espera de los threads, despues simplemente se libera el mutex y se queda en espera el thread a por una ficha.
*  `cond_signal()`: Primero se pregunta si hay alguien en la cola de la condicion declarada en la estructura `cond`, en caso de haber alguien se extrae, de esto se extrae la direccion del semaforo para despues agregarlo en la cola del `mutex`.
*  `cond_signal()`: Toma todos los threads de la cola de espera de `cond` y los pone en la cola de espera del `mutex`. Notar que con esto se invierte el orden de la cola, lo cual esta correcto.
  
## Observaciones

* Como en el nucleo de Linux no hay mutex y condiciones pero si se dispone de semaforos, podrá usar una implementacion similar a esta en la tarea de modulos de Linux para lograr la sincronizacion.
* Para las condiciones existe la operacion:
  ```c
  pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, struct timespec *abstime);
  ```

  * Si no se ha recibido un signal/broadcast dentro de `abstime`, la  operacion termina.
  * Ejemplo de uso:
  
  ```c
  int inicio = ...hora actual...;
  while (!...condicion...){
    if (...hora actual...-inicio > tolerancia) break;
    ...llenar abstime ...
    pthread_cond_timedwait(&cond, &mutex, &abstime);
  }
  ```
* Esto no se puede implementar con semaforos porque no hay un `sem_timedwait`.

# Patrones de paralelizacion con threads

* **Descomponer en subintervalo:** En un **ciclo** en donde no hay dependencia entre iteraciones, se puede descomponer el intervalo del ciclo en $NT$ (numero de threads) subintervalos y cada thread se ocupa de un subintervalo.
  * Ejemplos: multiplicacion de matrices, busqueda de un factor.
* **Divide y conquista:** En una funcion con 2 llamadas recursivas, la primera llamada se ejecuta en un nuevo thready y la segunda en el thread original. Posteriormente se unen ambos resultados.
* Generar demasiados threads es ineficiente.
* Cuando se hayan completado `nt` threads se continua secuencialmente con la recursividad.
* Ejemplo: quicksort paralelo, exploracion de todas las combinaciones.
* **Productor/Consumidor:** Un solo productor genera unidades de trabajo y las deposita en un buffer, $NT$ consumidores iteran extrayendo unidades de trabajo y las ejecutan.

# Ejemplo: Busqueda de un factor de un entero x

* Desventaja de la busqueda de un factor vista en clase auxiliar: si un thread encuentra tempranamente un factor, de todas formas hay que esperar que terminen todos los otros threads.
* Algun thread podria tardar porque no hay un factor en el rango en donde le toco buscar.
* Solucion: un thread productor de trabajos (*jobs*) con intervalos de tamaño 1000000 en donde buscar.
* Multiples threads consumidores de *jobs* iteran extrayendo intervalos y buscando un factor en ese intervalo.
* Cuando un thread encuentra un factor, el productor se entera y envía trabajos nulos, para notificar a los consumidores que deben terminar.

# Ejemplo: Busqueda de un factor de un entero x con patron productor/consumidor

```c
#include <pthread.h>
#include <math.h>
#include "buffer.h"

// Busca un factor del numero
// entero x en el rango [i,j]

uint buscarFactor(ulonglong x, uint i, uint j) {
  for(uint k=i; k<=j; k++) {
    if(x%k == 0) return k;
  }
  return 0;
}

typedef struct {
  ulonglong x;
  uint i, j, *pres;
  // pthread_mutex_t *pm;
} Job;

void *threadFun(void *ptr) {
  Buffer *buf = ptr;
  for (;;) {
    Job *pjob = bufGet(buf);
    if (*pjob==NULL) return NULL;
    if (*pjob->pres!=0) {
      free(pjob);
      return NULL;
    }
    ulonglong x = pjbo->x;
    uint i = pjob->i;
    uint j = pjob->j;
    uint res = buscarFactor(x, i, j);
    if (res!=0) {
      // pthread_mutex_lock(pjob->pm);
      *pjob->pres = res;
      //pthread_mutex_unlock(pjob->pm);
    }
    free(pjob);
  }
}

// Busca un factor del numero
// entero x utilizando nt cores
uint buscarFactorParalelo(ulonglong x, int nt) {
  pthread_t pid[nt];
  uint j = srqt(x);
  j++;
  uint delta = 1000000;
  // pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
  uint res = 0;
  Buffer *buf = makeBuffer(nt);
  for (int k=0; k<nt; k++) {
    pthread_create(&pid[k], NULL, threadFun, buf);
  }

  for (int i=2; i<=j; i+=delta) {
    //pthread_mutex_lock(pjob->pm);
    int end = res!=0;
    //pthread_mutex_unlock(pjob->pm);
    if (end) break;
    Job job = {x, i, i+delta-1, &res /*, &m */};
    Job *pjob = malloc(sizeof(Job));
    *pjob = job;
    bufPut(buf, pjob);
  }

  for (int k=0; k<nt; k++) {
    bufPut(buf, NULL);
    pthread_join(pid[k], NULL);
  }
P
  destroyBuffer(buf);
  return res;
}
```

* `buscarFactor()`: Busca en un subtintervalo en particular, se utiliza para la funcion `buscarFactorParalelo()`.
* En la estructura `Job` se encuentra la direccion del buf con la que trabajara el thread, `*pres` y el intervalo de busqueda.
* `threadFun()`: Extrae un job del bufer, en caso de ser `NULL` se retorna `NULL` para indicar que se acabaron los jobs o algun thread tuvo exito, en caso contrario se libera el job extraido y en base a la variable compartida por el thread padre y los threads, `*pjob->pres`, se indica si algun thread tuvo exito en la busqueda. En cualquier otro caso, se utiliza el `job` para buscar el factor, asignandolo a la variable `res`, si esta es 0 se libera la estructura y se indica con la variable compartida que se debe seguir entregando mas jobs.
* `buscarFactorParalelo()`: Busca el factor con `nt` threads, en el primer `for` se crean los threads, en el segundo se verifica si la variable `res` no es 1, pues en ese caso se termina la busqueda, en caso contrario se crean los jobs. Por ultimo, en el tercer ciclo, se deposita la marca en el burfer se marcan `nt` valores nulos para que los threads noten que se encontró el factor.

# Paralelismo del tipo AND y del tipo OR
* El tipico paralelismo que habiamos visto hasta el momento es del tipo **AND: se necesita los resultados de todos los threads.**
* La busqueda de un factor en paralelo es del tipo **OR: si uno de los threads encuentra un resultado, no se necesitan los resultados de los demas threads.**
* El problema es detenerlos.
* Encontrar en paralelo una combinacion de valores en donde una funcion booleana se hace verdadera (SAT), tambien es paralelismo del tipo OR.
* El patron productor/consumidor permite limitar el trabajo inutil.
* Alternativa: Usar una variable booleana compartida que indica cuando se encontro un factor, pero eso significa un pequeño sobrecosto en consultar esa variable por cada division hecha.
* Esa variable se lee casi siempre, por lo que los cores la pueden mantener en sus propios caches, excepto si por mala suerte queda en la misma linea de una variable que los threads si modifican.