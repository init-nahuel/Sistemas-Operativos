#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

// Use los estados predefinidos WAIT_ACCEDER y WAIT_COMPARTIR
// El descriptor de thread incluye el campo ptr para que Ud. lo use
// a su antojo.

//... defina aca sus variables globales con el atributo static ...

static NthQueue *CompartiendoQueue; // Cola con threads que acceden a compartir
static NthQueue *EsperandoCompartirQueue; // Cola con threads que esperan en nAcceder a que alguien invoque nCompartir

// nth_compartirInit se invoca al partir nThreads para Ud. inicialize
// sus variables globales

void nth_compartirInit(void) {
  CompartiendoQueue = nth_makeQueue();
  EsperandoCompartirQueue = nth_makeQueue();
}

void nCompartir(void *ptr) {
  START_CRITICAL // No es necesario como tal, pues no esta considerado el caso en que dos threads llaman a esta funcion

  if (nth_emptyQueue(EsperandoCompartirQueue)) { // Caso nadie en cola para acceder, esperamos
    suspend(WAIT_ACCEDER);
    schedule();
  }

  // Sacamos los threads de la cola y les asignamos ptr en el campo de la estructura
  while (!(nth_emptyQueue(EsperandoCompartirQueue))) {
    nThread th = nth_getFront(EsperandoCompartirQueue);
    th->ptr = ptr;
    setReady(th);
  }
  suspend(WAIT_COMPARTIR);
  schedule()

  END_CRITICAL
  return;
}

void *nAcceder(int max_millis) {
  ...
}

void nDevolver(void) {
  ...
}
