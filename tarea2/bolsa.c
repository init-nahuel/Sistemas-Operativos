#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"


#define TRUE 1
#define FALSE 0

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;


int mejor_precio = 0; // Variable para guardar el mejor precio
int *status; // Variable para marcar el estado de la posible venta a concretar
char **nombreComprador;
char **nombreVendedor;

int vendo(int precio, char *vendedor, char *comprador) {

  pthread_mutex_lock(&m);
  int local_status;
  
  if (mejor_precio == 0){ // Caso primer vendedor
    mejor_precio = precio;
    status = &local_status;
    local_status = 0;

    nombreVendedor = &vendedor;
    nombreComprador = &comprador;
  }
  else if (mejor_precio>precio){ // Llega nuevo vendedor con mejor precio, despierta al otro vendedor en espera
    mejor_precio = precio;
    *status = -1;
    local_status = 0;
    status = &local_status;

    nombreVendedor = &vendedor;
    nombreComprador = &comprador;

    pthread_cond_broadcast(&c);
  }
  else{ // Caso en que el precio del vendedor es mayor al existente -> retorna FALSE de inmediato
    pthread_mutex_unlock(&m);
    return FALSE;
  }

  while(!local_status) pthread_cond_wait(&c, &m);

  if (local_status == 1){
    pthread_mutex_unlock(&m);
    return TRUE;
  }
  // Caso thread en espera es despertado para indicarle que alguien vende mas barato -> retorna FALSE
  else{
    pthread_mutex_unlock(&m);
    return FALSE;
  }


}

int compro(char *comprador, char *vendedor) {

  int mejor_precio_local;

  pthread_mutex_lock(&m);
  
  if (status == NULL){ // Caso no hay vendedores
    mejor_precio = 0;

    pthread_mutex_unlock(&m);
    return 0;
  }
  else{ // Hay vendedor en espera
    *status = 1;
    status = NULL;
    
    pthread_cond_broadcast(&c);
    strcpy(vendedor, *nombreVendedor);
    strcpy(*nombreComprador, comprador);

    mejor_precio_local = mejor_precio;
    mejor_precio = 0;
    pthread_mutex_unlock(&m);

    return mejor_precio_local;
  }
}
