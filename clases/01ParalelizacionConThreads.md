# Procesos pesados vs. Threads
* En PSS se estudiaron los procesos pesados, los cuales se crean con la llamada al sistema `fork()`. Ahora se estudiaran los **Threads**.
* Similitudes: Ambos sirven para paralelizar programas permitiendo hacer un uso eficiente de todos los cores.
* Se difirencian en que los threads:
  * Comparten memoria, lo que puede dar origen a **dataraces**.
  * Se sincronizan por medio de **mutex** y **condiciones** o **semaforos**.
  * Requieren menos recursos que los procesos Unix.

Algunos observaciones a señalar antes de estudiar los threads, son:
* **Core**: Es un nucleo de ejecucion. Componente de hardware capaz de ejecutar un thread.
* **Computador Multi-Core**: 
  * En un programa podemos crear muchos mas threads que los cores disponibles.
  * El sistema operativo atribuye los ocres a los threads activos dandoles tajadas de tiempo de ejecucion.

## Programas con multiples threads
Antes de recurrir a los threads es mas rentable investigar si hay algoritmos mas eficientes, pues como desventaja la programacion es mas compleja y surgen una nueva categoria de errores que ocurren aleatoriamente y que son dificil de depurar.

# Funciones para manipular threads (**pthread**)
Para este curso se usaran los threads `pthreads`, el cual es un estandar POSIX. Para esto es necesario incluir la libreria pthreads con `#include <pthread.h>`
* **Creacion:**
    ```c
  int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
  ```
  * `thread`: Puntero donde se guardara el identificador del thread.
  * `attr`: Puntero a estructura con los atributos del thread, en principio sera NULL.
  * `start_routine`: Puntero a la funcion de inicio del thread, esta funcion debe retornar un puntero opaco (void *), y debe recibir otro puntero opaco donde se encuentran los argumentos de la funcion.
  * `arg`: Puntero a la estructura en donde se colocan los argumentos de la funcion de inicio del thread, el tipo de este puntero es opaco.
* **Termino:** Existen dos formas de terminar un thread.
  * Cuando `start_routine` retorna, es decir, cuando retorna la funcion del thread.
  * Cuando se invoca a la funcion:
  ```c
  void pthread_exit(void *ret);
  ```
  * `ret`: Puntero a la estructuctura con los valores retornados.
  
  **OBS**: Es distinto a llamar `exit()`.


* **Esperar a que un thread termine (muera y entierra):**
  ```c
  int pthread_join(pthread_t t, void **pret);
  ```
  * `t`: Identificador del thread.
  * `pret`: Doble puntero donde quedara almacenado el puntero que contiene los valores retornados por el thread, usualmente usaremos NULL.

  Esta funcion espera a que muera y entierra el thread, si no se entierra se convierte en memoria basura.

## Ej: Calculo del factorial en un computador dual core

```c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

double mult(int i, int j);
void *mult_thread(void *ptr);

typedef struct {
  int i,j;
  double res;
} Args;

int main(int argc, char **argv){
  int n = atoi(argv[1]);
  int h = (n+1)/2;
  Args a1 = {1, h, 0.};
  Args a2 = {h+1, n, 0.};
  pthread_t t1, t2;

  // Notar que primero se crean todos los threads neccesarios y luego se entierran
  pthread_create(&t1, NULL, mult_thread, &a1);  
  pthread_create(&t2, NULL, mult_thread, &a2);
  
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  printf("%g \n", a1.res * a2.res);
  return 0;
}


double mult(int i, int j){
  double p = 1.0;
  for (int k=i; k<=j; k++){
    p *=k;
  }
  return p;
}


void *mult_thread(void *ptr){
  Args *a = ptr;
  a->res = mult(a->i, a->j);
  return NULL;
}
```
**OBS: Esta solucion no es eficaz pues para `n` mayor a 300 el programa se cae. Crear threads tiene un sobrecosto muy alto.**