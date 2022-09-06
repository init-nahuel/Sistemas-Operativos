# Problema: Orden de Atencion indefinido
* Los threads suspendidos con `pthread_mutex_lock` o `pthread_cond_wait` se despiertan en cualquier orden, quedan a criterio de quien los implementan.
* Primera solucion para atender por orden de llegada: threads sacan un numero cuando se colocan en espera, como los clientes de una farmacia
* Segunda solucion: cada thread se coloca en epsera en su propia condicion y las condiciones se agregan y extraen de una cola FIFO
* Ejemplo: Multiples threads comparten un recurso unico que debe ser ocupado en **exclusion mutua**. Para coordinarse, los threads invocan la funcion `ocupar()` para solicitar el recurso e invocan `desocupar()` para liberarlo
  
## Atencion en orden no especificado
Un esquema del codigo a programar en este caso es:
```c
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void ocupar(){
  pthread_mutex_lock(&m);
}

void desocupar(){
  pthread_mutex_unlock(&m);
}
```
Los mutex se otorgan en en un orden no especificado.

**OBS:** Las funciones lock y unlock estan hechas de manera que al momento de liberar el mutex, el tiempo transcurrido hasta que se vuelva a ocupar sea de milisegundos.

Por tanto, una mejor implementacion es:
```c
othread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;
int busy = 0; // Condicion para ver si el mutex esta ocupado

void ocupar(){
  pthread_mutex_lock(&m);
  while(busy){
    pthread_cond_wait(&c,&m);
  }
  busy = 1;
  pthread_mutex_unlock(&m);
}

void desocupar(){
  pthread_mutex_lock(&m);
  busy = 0;
  pthread_cond_broadcast(&c);
  pthread_mutex_unlock(&m);
}
```

* Cuando un thread va a esperar bastante tiempo, es mas eficiente que espere con `wait` que con `lock`.
* Las llamadas a `pthread_cond_wait` se despiertan en un orden no especificado.

Al ejecutarse `wait` el thread esperará la condicion, sin embargo esta condicion depende de la variable `busy`, es decir, aun cuando el mutex sea liberado, la variable `busy` es la que condiciona al mutex.