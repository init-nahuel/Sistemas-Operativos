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
  int sl; // Spinlock para realizar el busy waiting
} Request;

// Declare aca las variables globales que necesite
// ...
PriQueue *bestThanQueue; // Cola de prioridad con request a pistas mayores que la actual
PriQueue *worseThanQueue; // Cola de prioridad con request a pistas menores que la actual
PriQueue *bestThanQueue2; // Cola para mantener las request de paso restates y luego y realizar el traspaso nuevamente
int mutex; // Mutex para asegurar exclusion mutua
int t; // Variable global con la pista en ejecucion
int busy; // Variable global para conocer el estado del disco, ocupado o no


// Agregue aca las funciones requestDisk y releaseDisk

void iniDisk(void) {
  // ...
  bestThanQueue = makePriQueue();
  bestThanQueue2 = makePriQueue();
  worseThanQueue = makePriQueue();
  mutex = OPEN;
  busy = 0;
  t = 0;
}

void requestDisk(int track) {
  // ...
  spinLock(&mutex);

  if (busy) {
    Request request = {track, CLOSED};
    Request *req = &request;
    
    if (track >= t) {
      priPut(bestThanQueue, req, track);
      spinUnlock(&mutex);
      spinLock(&req->sl);
    }
    else {
      priPut(worseThanQueue, req, track);
      spinUnlock(&mutex);
      spinLock(&req->sl);
    }
  }

  t = track;
  busy = 1;

  spinUnlock(&mutex);
  return;
}

void releaseDisk() {
  // ...
  spinLock(&mutex);

  if (!emptyPriQueue(bestThanQueue)) { // Se sacan todas las request de bestThanQueue para obtener la con peor prioridad y luego se vuelven a agregar a la misma cola
    Request *request;
    do {
      request = priGet(bestThanQueue);
      if (emptyPriQueue(bestThanQueue)) break;
      priPut(bestThanQueue2, request, request->t);
    } while (!emptyPriQueue(bestThanQueue));
    while(!emptyPriQueue(bestThanQueue2)) {
      Request *req = priGet(bestThanQueue2);
      priPut(bestThanQueue, req, req->t);
    }
    spinUnlock(&request->sl);
    spinUnlock(&mutex);
    return;
  }
  else if (!emptyPriQueue(worseThanQueue)) { // Se sacan todas las request de worseThanQueue para obtener la con peor prioridad y luego agregan a la cola bestThanQueue pues acceden
                                             // a pista que es mayor a la pista actual
    Request *request;
    do {
      request = priGet(worseThanQueue);
      if (emptyPriQueue(worseThanQueue)) break;
      priPut(bestThanQueue, request, request->t);
    } while (!emptyPriQueue(worseThanQueue));
    spinUnlock(&request->sl);
    spinUnlock(&mutex);
    return;
  }
  else {
    busy = 0;
    spinUnlock(&mutex);
    return;
  }

  busy = 0;
  spinUnlock(&mutex);
  return;
}
