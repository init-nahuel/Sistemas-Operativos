Los problemas de paralelizacion se pueden dividir en las siguientes categorias:
* Clasificacion de problemas con threads:
  * Paralelizacion: Por ejemplo, el calculo del factorial en paralelo.
  * Sincronizacion: Secciones criticas, problema del productor/consumidor.
* Problemas clasicos de sincronizacion:
  * Productor/Consumidor.
  * Cena de filosofos.
  * Lectores Escritores.
  
# Problema: "La cena de los filosofos" (Diningg Philosophers, Edger Dijkstra)
* Enunciado: "5 filosofos se van a comer arroz chino, para ello se sientan en una mesa donde cada silla tiene un numero en el cual se debe sentar el filosofo con el numero de su identificacion". Restricciones:
  * Para comer un filosofo necesita los palitos que estan mas cerca a su izquierda y derecha.
  * 2 filosofos no pueden comer con el mismo palito al mismo tiempo.
  * Cuando se pueda, 2 filosofos deben comer en paralelo.

## Solucion trivial (usando 5 mutex)
Si pensamos en la siguiente solucion utilizando 5 mutex:
```c
pthread_mutex_t m[5] = {PTHREAD_MUTEX_INITIALIZER,..,} // 5 veces

void filosofo(int i){
  for (;;){
    lock(m[i]); lock(m[(i+1)%5]);
    comer(i, (i+1)%5);
    unlock(m[i]); unlock(m[(i+1)%5]);
    pensar();
  }
}
```
Está incorrecta pues el mutex $i+1$ se quedará esperando al mutex $i$, que fue cerrado por este mismo.

---
# Deadlock
* **Deadlock: Problema en el cual un mutex se queda esperando de forma indefinida a otro mutex que se encuentra cerrado.**
---

La solucion correcta es:
```c
pthread_mutex_t m[5] = {PTHREAD_MUTEX_INITIALIZER,..,} // 5 veces

void filosofo(int i){
  for (;;){
    int j=min(i,(i+1)%5); k=max(i,(i+1)%5);
    lock(m[j]); lock(m[k]);
    comer(i, (i+1)%5);
    unlock(m[i]); unlock(m[(i+1)%5]);
    pensar();
  }
}
```

# Starvation
Si dos o mas threads ocupan funciones que usan `lock()` y `unlock()`, puede ocurrir que un tercer thread nunca pueda ocupar estas funciones si es que este par, o mas, de threads ejecutan las funciones de manera que este thread espere eternamente. Este concepto se llama **Hambruna** o **Starvation**.

# Problema: Lectores/Escritores
Supongamos que tenemos un diccionario que tiene la operacion `query()`, que entrega el valor asociado a una llave `k`, ademas tenemos la funcion `define()` para establecer que el valor asociado a la llave `k` es `v`. Adicionalmente, tenemos la funcion `delete()`, que borra una llave.

Ahora bien, los threads que invoca `query()` se denominaran **lectores**, pues no modifican el diccionario. Por otra parte, los threads que invocan `delete()` y `define()`, se llamaran **escritores**, pues modifican el diccionario. Es evidente que multiples escritores pueden provocar **dataraces** si utilizan al mismo tiempo el diccionario.

Ademas, una ejecucion en paralelo de un escritor puede llevar a un lector a un resultado incorrecto o un error, es decir, `query()` tambien es parte de la **seccion critica**. Sin embargo, el hecho de que estas tres funciones sean parte de la **seccion critica** suprime el paralelismo para lectores.

Requisitos de una solucion eficiente:
* Un escritor no puede trabajar junto a otro lector o escritor.
* Lectores deben trabajar en paralelo.

Para esto es necesario definir funciones que nos indiquen si es posible escribir o leer sobre el diccionario (`enterRead()`, `exitRead()`, `enterWrite()`, `exitWrite()`).
```c
int readers = 0; // Numero de lectores actualmente consultando el diccionario
int writing = FALSE; // Indica si hay un escritor modificando el diccionario, como es uno basta con definirla como TRUE o FALSE

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

void enterRead(){
  lock(&m);
  while(writing){
    wait(&c, &m);
  }
  readers++;
  unlock(&m);
}

void exitRead(){
  lock(&m);
  readers--;
  broadcast(&c);
  unlock(&m);
}

void enterWrite(){
  lock(&m);
  while(writing || readers > 0){
    wait(&c, &m);
  }
  writing = TRUE;
  unlock(&m);
}

void exitWrite(){
  lock(&m);
  writing = FALSE;
  broadcast(&c);
  unlock(&m);
}

```

# Patron de uso de mutex y condiciones
* Primero se invoca `lock()` para entrar a la seccion critica, garantizando la exclusion mutua, sin embargo probablemente la estructura de datos no se encuentra en el estado para realizar la ejecucion, por tanto debe esperar con `wait()`.
* Cuando la espera termine, realizar la operacion, para luego despertar a los threads con `broadcast()`.
* Finalmente liberar el mutex con `unlock()`.
  
**OBS: Tanto la condicion de espera como el despertar a los threads pueden ser opcionales dependiento del tipo de problema a resolver.**