# Secciones criticas en un multicore

* Ademas de las señales, los dataraces se pueden producir por no respetar la exclusion mutua de multiples cores en una seccion critica.
* `START_CRITICAL` es equivalente a:

  ```c
  sigset_t sigsetOld;
  pthread_sigmask(SIG_BLOCK, &nth_sigsetCritical, &sigsetOld);
  llLock(&nth_schedMutex); // pthread_mutex_lock
  ```

* `nth_schedMutex` es un mutex de tipo `LLMutex` que es el tipo `pthread_mutex_t` nativo de pthreads.
* Asegura la exclusion mutua de los cores.
* Si `nth_schedMutex` está ocupado, el core se bloquea.
* **No confundir con el tipo `nMutex` que corresponde a los mutex virtualizados de nThreads, es decir aseguran la exclusion mutua de los nThreads.**
* Si un `nMutex` está ocupado, el nthread se bloquea, pero el core puede y debe ejecutar otros nthreads.
* `END_CRITICAL` es equivalente a:

  ```c
  llUnlock(&nth_schedMutex); //pthread_mutex_unclok
  sigset_t sigsetOld;
  pthread_sigmask(SIG_SETMASK, &nth_sigsetApp, &sigsetOld);
  ```

# Implementacion de `nSelf()` en multicore (y monocore)

* En **monocore** basta con usar una variabnle global. Así, cada vez que se invoca `nSelf()` se retorna el valor de la variable global.
* **En un nucleo multicore nativo existen registros del procesador que guardan el numero del core: 0, 1, 2, etc.**
* **En nThreads la funcion `nth_coreId()` sirve para obtener ese identificador.**
* Un arreglo almacena el thread en ejecucion por cada core:

  ```c
  nThread nth_coreThreads[]
  ```

* Con esto se implementa `nSelf` y `nth_setSelf`:

  ```c
  nThread nSelf() {
    return nth_coreThreads[nth_coreId()]; // Datarace!
  }

  void nth_setSelf(nThread th) {
    nth_coreThreads[nth_coreId()]=th;
  }
  ```

* **Como se implementa `nth_coreId()`?**
  * **Con threads locals: variables globales locales a un thread, cada thread tiene su propia instancia. Para lograr esto, la variable se debe declarar anteponiendo la keyword `__thread`:**

    ```c
    __thread int nth_thisCoreId;
    #define nth_coreId() nth_thisCoreId
    ```

# Scheduler FCFS para multicore: `nKernel/sched-fcfs.c`

* La cola ready:

  ```c
  NthQueue *nth_fcfsReadyQueue;
  ```
  
  * Cada cola esta definida localmente para cada scheduler.

* Pasar un thread a estado READY:

  ```c
  void nth_fcfsSetReady(nThread th) {
    th->status=READY;
    if (nth_allocCoreId(th)<0) {
      nth_putBack(nth_fcfsReadyQueue, th);
    }
    else if (nth_allocCoreId(th)!=nth_coreId()) {
      nth_coreWakeUp(nth_allocCoreId(th));
    }
  }
  ```

  * `nth_allocCoreId(th)<0` verifica si el thread no se esta ejecutando en ningun core.
  * Con `nth_coreWakeUp` manda señal al thread que quiere despertarse del estado de espera.

* Pasar un thread a estado de espera:
  
  ```c
  void nth_fcfsSuspend(State waitState) {
    nThread th=nSelf();
    th->status=waitState;
  }
  ```

  **OBS:** Esta parte es igual al caso monocore.

```c
void nth_fcfsSchedule(void) {
  nThread thisTh= nSelf();
  for ( ; ; ) {
    if ( thisTh!=NULL &&
    (thisTh->status==READY || thisTh->status==RUN) ) {
      break;
  nThread nextTh= nth_getFront(nth_fcfsReadyQueue);
    if (nextTh!=NULL) {
      nth_changeContext(thisTh, nextTh); // _changeToStack
      nth_setSelf(thisTh); // Set current running thread
      if (thisTh->status==READY)
        break;
    }
    nth_coreIsIdle[nth_coreId()]= 1; // To prevent recursive calls
    llUnlock(&nth_schedMutex); // pthread_mutex_unlock
    sigsuspend(&nth_sigsetApp);
    llLock(&nth_schedMutex); // pthread_mutex_lock
    nth_coreIsIdle[nth_coreId()]= 0;
  }
  thisTh->status= RUN;
  nth_reviewCore(nth_peekFront(nth_fcfsReadyQueue));
}
```

# Caso Multicore

* Puede ocurrir que un thread esté en estado de espera pero aun asi estar asignado a un core, porque hay abundancia de cores.
* El core está esperando en un `sigsuspend` a que algun thread pase a estado READY.
* Cuando eso ocurre `setReady` invoca a `nth_coreWakeUp` para despertar el core asignado.
* **Esta estrategia no es estrictamente FCFS.**
* Puede ocurrir que varios threads pasen a estado READY y luego se invoque al scheduler.
* Si hay cores disponibles, el scheduler se encarga de despertarlos para que ejecuten los threads agregados a la cola (funcion `nth_reviewCore`).

# Conclusion 

* Implementacion de un nucleo nativo: son cores verdaderos, pero no hay `pthread_mutex_t` y `pthread_cond_t` para sincronizarlos.
* Solucion: spin-locks.
* Son semaforos binarios de bajo nivel.
* La espera se implementa con busy-waiting.
* A partir de spin-locks se construyen `LLMutex` y `LLCond`.