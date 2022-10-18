#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

// Use los estados predefinidos WAIT_ACCEDER, WAIT_ACCEDER_TIMEOUT y
// WAIT_COMPARTIR
// El descriptor de thread incluye el campo ptr para que Ud. lo use
// a su antojo.


// Defina aca sus variables globales.
// Para la cola de espera de nCompartir prefiera el tipo Queue.

static NthQueue *queueEspera; // Cola de espera de threads
static int n; // Cantidad de threads compartiendo ptr
static int compartiendo; // Variable para indicar si se estan compartiendo o no los datos
static nThread *th_compartidor; // Puntero al thread que invoca compartir

// nth_compartirInit se invoca al partir nThreads para que Ud. inicialize
// sus variables globales
void nth_compartirInit(void) {
  queueEspera = nth_makeQueue();
  n = 0;
  compartiendo = 0;
  th_compartidor = NULL;
}

void nCompartir(void *ptr) {
  START_CRITICAL
  
  compartiendo = 1;
  nThread this_th = nSelf();
  th_compartidor = &this_th;
  this_th->ptr = ptr;

  if (!nth_emptyQueue(queueEspera)) {
    while(!nth_emptyQueue(queueEspera)) {
      nThread th_obt = nth_getFront(queueEspera);
      if (th_obt->status == WAIT_ACCEDER_TIMEOUT) {
        nth_cancelThread(th_obt);
      }
      th_obt->ptr = this_th->ptr;
      setReady(th_obt);
    }
  }

  suspend(WAIT_COMPARTIR);
  schedule();
  END_CRITICAL
  return;
}

static void f(nThread th) { // SE DECLARA ANTES DE PONER READY EL THREAD PARA QUE NO ENTRE EN CONFLICTO CON LA COLA DE PROCESOS READY NTH_QUEUE
  // programe aca la funcion que usa nth_queryThread para consultar si
  // th esta en la cola de espera de nCompartir.  Si esta presente
  // eliminela con nth_delQueue.
  // Ver funciones en nKernel/nthread-impl.h y nKernel/pss.h
  
  if (nth_queryThread(queueEspera, th)) {
    n-=1;
    nth_delQueue(queueEspera, th);
  }
}

void *nAcceder(int max_millis) {
  // ...  use nth_programTimer(nanos, f);  f es la funcion de mas arriba
  START_CRITICAL

  nThread this_th = nSelf();
  this_th->ptr = NULL;

  if (compartiendo == 0 && max_millis != 0) {
    n+=1;
    if (max_millis > 0) {
      nth_putFront(queueEspera, this_th);
      long long t = (long long) max_millis*1000000;
      suspend(WAIT_ACCEDER_TIMEOUT);
      nth_programTimer(t, f);
    }
    else {
      nth_putBack(queueEspera, this_th);
      suspend(WAIT_ACCEDER);
    }
    schedule();
  }
  else {
    n+=1;
    nThread comp_th = *th_compartidor;
    this_th->ptr = comp_th->ptr;
  }

  void *ptr = this_th->ptr ? this_th->ptr : NULL;
  
  END_CRITICAL
  return ptr;
}

void nDevolver(void) {
  START_CRITICAL

  n-=1;
  if (n==0) {
    nThread th_comp = *th_compartidor;
    compartiendo = 0;
    setReady(th_comp);
    schedule();
  }

  END_CRITICAL
  return;
}