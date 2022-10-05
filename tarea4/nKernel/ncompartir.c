#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

// Use los estados predefinidos WAIT_ACCEDER y WAIT_COMPARTIR
// El descriptor de thread incluye el campo ptr para que Ud. lo use
// a su antojo.

static NthQueue *esperaAcceder; // Cola con threads que esperan en nAcceder a que alguien invoque nCompartir
static int n; // Cantidad de threads compartiendo ptr, es decir, que invocaron nAcceder
static nThread *th_compartidor; // Puntero al thread que invoca a nCompartir
static int compartiendo; // Variable para indicar si se estan compartiendo o no los datos

// nth_compartirInit se invoca al partir nThreads para Ud. inicialize
// sus variables globales

void nth_compartirInit(void) {
  esperaAcceder = nth_makeQueue();
  n = 0;
  th_compartidor=NULL;
  compartiendo=0;
}

void nCompartir(void *ptr) {
  START_CRITICAL

  compartiendo=1;
  nThread this_th = nSelf(); // Obtenemos el thread
  th_compartidor = &this_th; // guardamos la direccion del thread en th_compartidor
  this_th->ptr = ptr;        // Asignamos ptr

  if (!(nth_emptyQueue(esperaAcceder))) { // Caso cola con threads que invocaron a nAcceder los sacamos y ponemos en estado READY
    while(!(nth_emptyQueue(esperaAcceder))) {
      nThread th_acc = nth_getFront(esperaAcceder);
      th_acc->ptr = this_th->ptr;
      setReady(th_acc);

    }
  }
  // Esperamos mientras se comparten los datos o hasta que llamen a nAcceder
  suspend(WAIT_COMPARTIR);
  schedule();

  END_CRITICAL
  return;
}


void *nAcceder(int max_millis) {
  START_CRITICAL

  nThread this_th = nSelf();
  n+=1;  // Aumentamos contador de threads que estan compartiendo datos
  
  if (compartiendo == 0) { // Caso nadie ha invocado a nCompartir -> nos ponemos en la cola y suspendemos
    nth_putFront(esperaAcceder, this_th);
    suspend(WAIT_ACCEDER);
    schedule();
  }
  else {
    nThread comp_th = *th_compartidor;
    this_th->ptr = comp_th->ptr;

  }
  void *ptr = this_th->ptr;
  END_CRITICAL
  return ptr;
}


void nDevolver(void) {
  START_CRITICAL

  n-=1; // Avisamos que un thread dejo de compartir datos

  if (n == 0) { // Si ya no queda nadie compartiendo datos -> ponemos en READY el thread que invoco a nCompartir
    nThread th_comp = *th_compartidor;
    compartiendo=0;
    setReady(th_comp);
    schedule();
  } 

  END_CRITICAL
  return;
}