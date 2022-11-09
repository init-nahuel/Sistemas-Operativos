#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "disk.h"
#include "priqueue.h"
#include "spinlocks.h"

// Le sera de ayuda la clase sobre semáforos:
// https://www.u-cursos.cl/ingenieria/2022/2/CC4302/1/novedades/detalle?id=431689
// Le serviran la solucion del productor/consumidor resuelto con el patron
// request y la solucion de los lectores/escritores, tambien resuelto con
// el patron request.  Puede substituir los semaforos de esas soluciones
// por spin-locks, porque esos semaforos almacenan a lo mas una sola ficha.


// Declare los tipos que necesite
// ...
typedef struct {
  int t; // Pista de la request
  int *sl; // Spinlock para realizar el busy waiting
} Request;

// Declare aca las variables globales que necesite
// ...
PriQueue *bestThanQueue; // Cola de prioridad con request a pistas mayores que la actual
PriQueue *worseThanQueue; // Cola de prioridad con request a pistas menores que la actual
int mutex; // Mutex para asegurar exclusion mutua
int t; // Variable global con la pista en ejecucion
int busy; // Variable global para conocer el estado del disco, ocupado o no


// Agregue aca las funciones requestDisk y releaseDisk

void iniDisk(void) {
  // ...
  bestThanQueue = makePriQueue();
  worseThanQueue = makePriQueue();
  mutex = OPEN;
  busy = 0;
  t = 0;
}

void requestDisk(int track) {
  // ...
  spinLock(&mutex);

  if (busy) {
    int w = CLOSED;
    Request request = {track, &w};
    Request *req = &request;
    
    if (track >= t) {
      priPut(bestThanQueue, req, req->t);
      spinUnlock(&mutex);
      spinLock(&w);
      spinLock(&mutex);
    }
    else {
      priPut(worseThanQueue, req, req->t);
      spinUnlock(&mutex);
      spinLock(&w);
      spinLock(&mutex);
    }
  }
  else {
    t = track;
    busy = 1;
  }

  spinUnlock(&mutex);
  return;
}

void releaseDisk() {
  // ...
  spinLock(&mutex);

  if (emptyPriQueue(bestThanQueue)) {
    PriQueue *copy = worseThanQueue;
    worseThanQueue = bestThanQueue;
    bestThanQueue = copy;
  }

  // Se saca la request con mejor prioridad y se despierta
  if (!emptyPriQueue(bestThanQueue)) { 
    Request *request = priGet(bestThanQueue);
    t = request->t;
    spinUnlock(request->sl);
    spinUnlock(&mutex);
    return;
  }
  else if (!emptyPriQueue(worseThanQueue)) {
    Request *request = priGet(worseThanQueue);
    t = request->t;
    spinUnlock(request->sl);
    spinUnlock(&mutex);
    return;
  }
  else if (emptyPriQueue(worseThanQueue) && emptyPriQueue(bestThanQueue)){
    busy = 0;
    spinUnlock(&mutex);
    return;
  }

  busy = 0;
  spinUnlock(&mutex);
  return;
}
