# Introduccion a Paralelizacion
Para lo que sigue de la materia es necesario incluir la libreria pthreads con `#include <pthread.h>`
* **Creacion:**
    ```c
  int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
  ```
  * `thread`: Puntero donde se guardara el identificador del thread.
  * `attr`: Puntero a estructura con los atributos del thread, en principio sera NULL.
  * `start_routine`: Puntero a la funcion de inicio del thread, esta funcion debe retornar un puntero opaco (void *), y debe recibir otro puntero opaco donde se encuentran los argumentos de la funcion.
  * `arg`: Puntero a la estructura en donde se colocan los argumentos de la funcion de inicio del thread, el tipo de este puntero es opaco.
* **Termino:**
    ```c
  void pthread_exit(void *ret);
  ```
  * `ret`: Puntero a la estructuctura con los valores retornados.
  
  **OBS:** Tambien se termina cuando la funcion `star_routine()` retorna.

* **Esperar a que un thread termine (muera y entierra):**
  ```c
  int pthread_join(pthread_t t, void **pret);
  ```
  * `t`: Identificador del thread.
  * `pret`: Doble puntero donde quedara almacenado el puntero que contiene los valores retornados por el thread, usualmente usaremos NULL.


# Paralelizacion y Mutex
Herramienta que se utiliza para garantizar la exclusion mutua sobre la seccion critica.

* **Tipo y macro:** Con la macro no es necesario inicializar utilizando el tipo.
 ```c
 pthread_mutex_t
 PTHREAD_MUTEX_INITIALIZER
 ```

* **Inicializacion:**
  ```c
  int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t mutexattr);
  ```
  * `m`: Direccion del mutex que se quiere inicializar.
  * `mutexattr`: Atributos del mutex, por ahora sera NULL.
  
  **OBS**: En caso de querer inicializar mas de un mutex, se puede declarar un arreglo de tipo mutex, con la macro `PTHREAD_MUTEX_INITIALIZER`:
```c
  pthread_mutex_t [2]={PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER}; 
 ```

* **Destruccion:**
  ```c
  int pthread_mutex_destroy(pthread_mutex_t *m);
  ```


# Funciones importantes para mutex

* **Bloquear mutex:** Se utiliza justo antes de entrar a la secion critica, asi se garantiza exclusion mutua.
  ```c
  int pthread_mutex_lock(pthread_mutex_t *m);
  ```
* **Liberar mutex:** Se utiliza justo al termino de la seccion critica.
  ```c
  int pthread_mutex_unlock(pthread_mutex_t *m);
  ```
**OBS: La metafora es la de una puerta con llave, asi cuando se llama a `lock()`, la puerta se cierra con llave, mientras que cuando se llama a `unlock()`, la puerta se desbloquea.**


# Condiciones
Una condicion permite esperar eficientemente hasta que ocurra un evento, asi no es necesario usar un ciclo de **Busy Waiting**. (Todas estas funciones retornan 0 si se ejecutan correctamente)

* **Tipo y macro:** Con la macro no es necesario inicializar utilizando el tipo.
 ```c
pthread_cond_t
PTHREAD_COND_INITIALIZER
 ```
* **Inicializacion:**
  ```c
  int pthread_cond_init(pthread_cond_t *cond, pthread_condattr *a);
  ```
* **Destruccion:**
  ```c
  int pthread_cond_destroy(pthread_cond_t *cond);
  ```
* **Espera:**
  ```c
  int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *m);
  ```
  * El mutex debe estar cerrado al momento de la invocacion. Esta funcion suspende el thread que la invoca hasta que se invoca `broadcast()` o `signal()`. Por otra parte, cuando se invoca `wait()` se abre el mutex.

* **Despertar:**
  ```c
  int pthread_cond_broadcast(pthread_cond_t *cond);
  int pthread_cond_signal(pthread_cond_t *cond);
  ```
  * `broadcast()`: Despierta a todos los thread que estan a la espera en la condicion `cond`.
  * `signal()`: Despierta a uno de los thread que esta a la espera en la condicion `cond`.
  **OBS:** Despertar es relativo, un thread que se despierta todavia tiene que esperar a que se libere el mutex, que esta tomado por el thread que realiza la invocacion de `broadcast()` o `signal()`.

# Semaforos
**Un semaforo representa un dispensador de fichas (analogo a semaforo de trenes).**

Operaciones:
* **Inicializar un semaforo con `val` fichas**:

  ```c
   void sem_init(sem_t *sem, int pshared, unsigned val);
   ```
  * `sem`: Sera la direccion del semaforo.
  * `pshared`: Valor que en nuestro caso sera 0.
  * `val`: Cantidad de fichas que tendra inicialmente el semaforo.

* **Extraer una ficha**:
  ```c
   void sem_wait(sem_t *sem);
   ```
   * Lo cual implica que disminuiran la cantidad de fichas, eventualmente sera cero.

* **Depositar una ficha**:
  ```c
   void sem_post(sem_t *sem);
   ```
   * Agrega una nueva ficha al semaforo, asi si existe algun thread esperando una ficha, se la puede llevar.

* **Destruir semaforo**:
  ```c
   void sem_destroy(sem_t *sem);
   ```
   * Libera los recursos que utilizó el semaforo no la memoria requerida para el semaforo.
  
* **Principio fundamental: Si no hay fichas disponibles, `sem_wait()` espera hasta que otro thread deposite una ficha con `sem_post()`.**
* Si hay varios threads en espera de una ficha, no se especifica un orden para otorgar las fichas.
* El uso mas frecuente es para garantizar la exclusion mutua, en tal caso se inicializa con una sola ficha.
* Tambien se puede usar para suspender un thread hasta que se cumpla una condicion, en tal caso se inicializa con 0 fichas.

**OBS:** Hay pocos usos en que un semaforo parte con varias fichas.