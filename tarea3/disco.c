#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "pss.h"
#include "disco.h"

// Defina aca sus variables globales
typedef struct {
  int ready;
  char *nombre;
  pthread_cond_t w;
  char *nombrePareja;
} Request;

pthread_mutex_t mutex;

Queue *colaVarones, *colaDamas;



void DiscoInit(void) {
  // ... inicialice aca las variables globales ...
  colaVarones = makeQueue();
  colaDamas = makeQueue();
  pthread_mutex_init(&mutex, NULL); // Nose si hay que inicializar el mutex 
}

void DiscoDestroy(void) {
  // ... destruya las colas para liberar la memoria requerida ...
  destroyQueue(colaVarones);
  destroyQueue(colaDamas);
}

char *dama(char *nom) {

  pthread_mutex_lock(&mutex);

  if (!emptyQueue(colaVarones)){ // Caso colaVarones no esta vacia -> hay varones en espera
    Request *req = peek(colaVarones);
    char *nombreVaron = req->nombre;
    req->nombrePareja = nom;
    req->ready = 1;
    pthread_cond_signal(&req->w);
    get(colaVarones);

    pthread_mutex_unlock(&mutex);
    return nombreVaron; 
  }
  else{ // Si no hay varones 
    Request req = {0, nom, PTHREAD_COND_INITIALIZER};
    put(colaDamas, &req);
    while(!req.ready){
      pthread_cond_wait(&req.w, &mutex);
    }

    pthread_mutex_unlock(&mutex);
    return req.nombrePareja;
  }

}

char *varon(char *nom) {

  pthread_mutex_lock(&mutex);

  if (!emptyQueue(colaDamas)){ // Caso colaDamas no esta vacia -> hay damas en espera
    Request *req = peek(colaDamas);
    char *nombreDama = req->nombre;
    req->nombrePareja = nom;
    req->ready = 1;
    pthread_cond_signal(&req->w);
    get(colaDamas);

    pthread_mutex_unlock(&mutex);
    return nombreDama;
  }
  else{ // Si no hay damas
    Request req = {0, nom, PTHREAD_COND_INITIALIZER};
    put(colaVarones, &req);
    while(!req.ready){
      pthread_cond_wait(&req.w, &mutex);
    }

    pthread_mutex_unlock(&mutex);
    return req.nombrePareja;
  }

}
