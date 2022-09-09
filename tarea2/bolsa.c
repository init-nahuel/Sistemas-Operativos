#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"

// Declare aca sus variables globales
// ...


#define TRUE 1
#define FALSE 0

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;


int mejor_precio = 0; // Variable para guardar el mejor precio
int *status; // Variable para marcar el estado de la posible venta a concretar
char *nombreComprador;
char* nombreVendedor;


int vendo(int precio, char *vendedor, char *comprador) {

  pthread_mutex_lock(&m);
  int local_status;
  status = &local_status;

  if (mejor_precio == 0){ // Caso primer vendedor
    mejor_precio = precio;
  }
  else if (mejor_precio>precio){ // Llega nuevo vendedor con mejor precio, despierta al otro vendedor en espera
    mejor_precio = precio;
    local_status = -1;
    pthread_cond_broadcast(&c);
  }

  while(!*status) pthread_cond_wait(&c, &m);
  //pthread_mutex_unlock(&m);

  if (*status == 1){
    strcpy(comprador, nombreComprador);
    //pthread_mutex_lock(&m);
    strcpy(nombreVendedor, vendedor);
    pthread_mutex_unlock(&m);
    return TRUE;
  }
  // Caso no se vende pues otro vendedor vende mas barato, se cambia local_status a 0 para q el vendedor con precio mas barato espero un comprador
  else{
    //pthread_mutex_lock(&m);
    local_status = 0;
    pthread_mutex_unlock(&m);
    return FALSE;
  }


}

int compro(char *comprador, char *vendedor) {

  int mejor_precio_local;
  pthread_mutex_lock(&m);

  strcpy(nombreComprador, comprador);
  
  if (status == NULL){ // Caso no hay vendedores
    mejor_precio = 0;
    pthread_mutex_unlock(&m);
    return 0;
  } 

  *status = 1;
  //pthread_mutex_unlock(&m);
  pthread_cond_broadcast(&c);

  //pthread_mutex_lock(&m);  
  strcpy(vendedor, nombreVendedor);
  mejor_precio_local = mejor_precio;
  status = NULL;
  mejor_precio = 0;
  pthread_mutex_unlock(&m);

  return mejor_precio_local;
}
