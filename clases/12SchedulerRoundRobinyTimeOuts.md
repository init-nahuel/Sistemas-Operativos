# Implementacion de `nSleepNanos`

```c
int nSleepNanos(long long nanos) {
  START_CRITICAL

  nThread thisTh=nSelf();
  suspend(WAIT_SLEEP);
  nth_programTimer(nanos, NULL);
  schedule();

  END_CRITICAL
  return 0;
}
```
* Suspende el nanothread durante una cantidad determinada de nanosegundos. Lo que se realiza, es cambiar el thread a estado de espera `WAIT_SLEEP`.
* Luego se le manda una señal para que despierte, sin embargo puede que no se atienda el thread.
* Se llama al scheduler para que escoja un nuevo proceso a ejecutar. Luego este otro proceso que se ejecuta es el que manda la señal al proceso en espera.
* Esta funcion retorna cuando pase el tiempo de espera y el nanothread se encuentra en la cola READY de procesos.

**OBS: En caso de que dos nanoThreads llamen a `nSleepNanos()`, podemos crear una cola de timeouts para asi guardar una estructura con el timer para cada thread y asi no se pierda la informacion del timer cuando se llame nuevamente a `nSleepNanos()` (cada thread tiene su "timer virtual").**

# Implementacion de `nth_programTimer()`

```c
voidnth_programTimer(longlongnanos, void(*wakeUpFun)(nThreadth)) {
  nThreadthisTh= nSelf();
  if(nanos>0) {
    longlongcurrTime= nGetTimeNanos();
    longlongwakeTime= currTime+nanos;
    if( nth_emptyTimeQueue(nth_timeQueue) || wakeTime-nth_nextTime(nth_timeQueue)<0 ) {
    // armtimer
    nth_setRealTimerAlarm(wakeTime-currTime);
    }
    thisTh->wakeUpFun= wakeUpFun;
    nth_putTimed(nth_timeQueue, thisTh, wakeTime);
  }
  else{
    setReady(thisTh);
  }
}
```

* En caso de que `nanos` sea menor a 0, simplemente se setea a READY el thread.
* Luego se obtiene el tiempo actual y se calcula el tiempo en que se debe volver a despertar con `longlongwakeTime`.
* Despues se pregunta si la cola `nth_timeQueue` se encuentra vacia, esta cola de prioridad guarda los threads que se deben despertar en orden, esta ordenada por los tiempos de wakeTime.
* `nth_nextTime()`: Obtiene el valor de la mejor prioridad del thread que se debe despertar.
* `nth_putTimed(nth_timeQueue, thisTh, wakeTime)`: Agrega en la cola `nth_timeQueue` el thread `thisTh` y el instante cuando se debe despertar.

# Runtina de atencion del timer

```c
voidnth_RTimerHandler(intsig, siginfo_t*si, void*uc) {
  START_HANDLER
  nth_wakeThreads();
  nThreadth= nSelf();
  if (! nth_coreIsIdle[nth_coreId()] && th!=NULL)
    schedule();
  END_HANDLER
}
```

* **Cuando se invoca una rutina de atencion automaticamente se inhiben las señales, por lo tanto lo unico que falta es bloquear el mutex del scheduler, para lo cual se utiliza `START_HANDLER` y `END_HANDLER`.**
* `nth_wakeThreads()`: Revisa la cola de prioridades con los tiempos de espera y despierta a todos los que estan READY.
* Notemos que esta funcion puede llamarse desde el `schedule()`, por tanto debemos determinar si nos encontramos dentro del scheduler para no volver a llamar `schedule()` y no caer en una situacion recursiva.

# Implementacion de `nth_wakeThreads()`

```c
voidnth_wakeThreads(void) {
  longlongcurrTime= nGetTimeNanos();
  // Wake up allthreadswithwaketime <= currTime
  while( !nth_emptyTimeQueue(nth_timeQueue) && nth_nextTime(nth_timeQueue)<=currTime) {
    nThreadth= nth_getTimed(nth_timeQueue);
    if (th->wakeUpFun!=NULL)
      (*th->wakeUpFun)(th);
    setReady(th);
  }
  nth_setRealTimerAlarm(
    nth_emptyTimeQueue(nth_timeQueue) ?
    0 : nth_nextTime(nth_timeQueue)-currTime);
}
```

* `nth_setRealTimerAlarm`: Se reconfigura el timer, si no es necesario se pone 0, sino se determina la hora a la que se debe despertar el thread en espera, restandole el tiempo actual.

# Implementacion de `nth_cancelThread()`

Funcion util en caso de despertar un thread antes del tiempo de espera especificado.

* Sirve en operaciones como `pthread_cond_timedwait()`: el thread se activa con `signal()` o `broadcast()`, el thread quedaria todavia con el timer armado. Se cancela invocando `nth_cancelThread()`.

```c
void nth_cancelThread(nThreadth) {
  nth_delTimed(nth_timeQueue, th);
  nth_wakeThreads(); // rearm timer if needed
}
```

* Se borra el thread de la cola de prioridades y se reconfigura el timer si es necesario.

# Round Robin

* La variable global `nth_sliceNanos` indica el tamaño de la tajada de tiempo.
* En el descriptor de proceso, el campo `sliceNanos` indica cuanto le queda de tajada a un thread que esta READY o en estado de espera.
* En el descriptor de proceso, el campo `startCoreNanos` indica a qué hora recibió el core un thread que está en estado RUN.
* Cuando un thread `th` pasa a estado de espera, descuenta de `th->slicesNanos` lo que ocupó de su tajada.
* Si el scheduler descubre que `th->slicesNanos` es <= 0, le otorga la tajada completa, pero lo envia al final de la cola.
* Por simplicidad, un thread que pasa a estado READY y le queda tajada de CPU, se va al principio de la cola READY, pero no le quita la CPU al que está ejecutandose.
* Solo se ejecuta de inmediato si tiene un core asignado.

---

* Pasar a estado READY:

  ```c
  static void nth_rrSetReady(nThread th) {
    th->status= READY;
    if (nth_allocCoreId(th)<0) { // No asignado a algún core
      if (th->sliceNanos>0) // Le queda tajada
        nth_putFront(nth_rrReadyQueue, th);
      else { // No le quedatajada
        th->sliceNanos= nth_sliceNanos;
        nth_putBack(nth_rrReadyQueue, th);
      } 
    }
    else if (nth_allocCoreId(th)!=nth_coreId())
      nth_coreWakeUp(nth_allocCoreId(th));
  }
  ```

* Pasar a estado de espera:

  ```c
  void nth_fcfsSuspend(State waitState) {
    nThread th= nSelf();
    th->status= waitState;
  }
  ```

## El scheduler

```c
void nth_rrSchedule(void) {
  nThread thisTh= nSelf();
  if (thisTh!=NULL) {
    long longendNanos= nth_getCoreNanos();
    thisTh->sliceNanos-= endNanos-thisTh->startCoreNanos;
  }
  for(;;) {
    if(thisTh!=NULL
    && (thisTh->status==READY || thisTh->status==RUN) ) {
      if(thisTh->sliceNanos>0)
        break;// Continue running sameallocatedthread
      else{ // No sliceremaining
        thisTh->sliceNanos= nth_sliceNanos;
        thisTh->status= READY;
        nth_putBack(nth_rrReadyQueue, thisTh);
      }
    }
    nThreadnextTh= nth_getFront(nth_rrReadyQueue);
    if(nextTh!=NULL) {
      nth_changeContext(thisTh, nextTh);
      nth_setSelf(thisTh);
      if(thisTh->status==READY)
        break;
    }
    // Se desbloquea el mutex pues luego se ira a espera
    nth_coreIsIdle[nth_coreId()]= 1;
    llUnlock(&nth_schedMutex);
    sigsuspend(&nth_sigsetApp);
    llLock(&nth_schedMutex);
    nth_coreIsIdle[nth_coreId()]= 0;
  }
  thisTh->status= RUN;
  thisTh->startCoreNanos= nth_getCoreNanos();
  nth_reviewCores(nth_peekFront(nth_rrReadyQueue));
  nth_setCoreTimerAlarm(thisTh->sliceNanos, nth_coreId());
}
```

# Despertar los cores

```c
voidnth_coreWakeUp(intid){
  // senda signaltocoreid towakeitup fromsigsuspend
  pthread_kill(nth_nativeCores[id], SIGUSR1);
}

voidnth_reviewCores(nThreadth) {
  if(th==NULL)
    return;
  intncores= nth_totalCores; // look for an idle core
  for(intid= 0; id<ncores; id++) {
    if(nth_coreIsIdle[id]) {
      if( nth_coreThreads[id]==NULL ||
        nth_coreThreads[id]->status!=READY ) {
        // wake the core id up
        nth_coreWakeUp(id);
        break;
      }
    }
  }
}
```

# La rutina de atencion de SIGVTALRM

Linux tiene dos tipos de timer, el virtual y el real. El timer virtual es el tiempo que el pthread utilizó la CPU.

```c
staticvoidVTimerHandler(intsig, siginfo_t*si, void*uc) {
  if(nth_coreIsIdle[nth_coreId()])
    return; // prevent schedule recursive
  nThreadth= nSelf();
  if(th==NULL)
    return; // to avoid weird race conditions
  START_HANDLER // llLock(&nth_schedMutex);
  th->sliceNanos= 0; // end of slice
  if( !nth_coreIsIdle[nth_coreId()] )
    schedule(); // give core to another thread
  END_HANDLER // llUnlock(&nth_schedMutex);
}   
```

