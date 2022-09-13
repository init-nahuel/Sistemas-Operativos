# Problema: Orden de Atencion indefinido
* Los threads suspendidos con `pthread_mutex_lock` o `pthread_cond_wait` se despiertan en cualquier orden, quedan a criterio de quien los implementa.
* Primera solucion para atender por orden de llegada: threads sacan un numero cuando se colocan en espera, como los clientes de una farmacia.
* Segunda solucion: cada thread se coloca en espera en su propia condicion y las condiciones se agregan y extraen de una cola FIFO.
* Ejemplo: Multiples threads comparten un recurso unico que debe ser ocupado en **exclusion mutua**. Para coordinarse, los threads invocan la funcion `ocupar()`, para solicitar el recurso, e invocan `desocupar()` para liberarlo.
  
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
Los mutex se otorgan en un orden no especificado.

**OBS:** Las funciones `lock()` y `unlock()` estan hechas de manera que al momento de liberar el mutex, el tiempo transcurrido hasta que se vuelva a ocupar sea de milisegundos.

Por tanto, una mejor implementacion es:
```c
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
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

* Cuando un thread va a esperar bastante tiempo, es mas eficiente que espere con `wait()` que con `lock()`.
* Las llamadas a `pthread_cond_wait` se despiertan en un orden no especificado.

Al ejecutarse `wait()` el thread esperará la condicion, sin embargo esta condicion depende de la variable `busy`, es decir, aun cuando el mutex sea liberado, la variable `busy` es la que condiciona al mutex.

---
## Ej: Sacar numero como en farmacias
```c
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

int ticket_dis = 0, display = 0; // ticket distribution and display (screen)

void ocupar(){
  pthread_mutex_lock(&m);
  int my_num = ticket_dist++; // Almacenamos my_num en variable local
  while(my_num != display){
    pthread_cond_wait(&c,&m);
  }
  pthread_mutex_unlock(&m);
}

void desocupar(){
  pthread_mutex_lock(&m);
  display++;
  pthread_cond_broadcast(&c);
  pthread_mutex_unlock(&m);
}
```

* Notar que la entrega de numeros se realiza dentro de la seccion critica, sino pueden haber casos en que varios threads tengan el mismo numero.
* En `desocupar()` se solicita el mutex para luego aumentar el `display`, despues se despierta a los thread para que continue el proceso cuando se libere el mutex.
* Es importante siempre declarar la liberacion del mutex asi como el bloqueo de este, aun cuando el codigo pueda funcionar correctamente.
---

## Lectores/Escritores sin hambruna

* La solucion de la clase pasada presentaba el problema de **hambruna**, dos lectores pueden concertar para entrar alternadamente de manera que siempre haya un lector presente, haciendo esperar al escritor para siempre.
* Una manera de evitar la hambruna es otorgar los permisos de lectura/escritura por orden de llegada.

---
## Ej: Lectores/Escritores por orden de llegada

```c
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER
pthread_cond_t c = PTHREAD_COND_INITIALIZER
int ticket_dist = 0, display = 0; // ticket distributor and display
int readers = 0; // no se necesita writing

void enterWrite(){
  pthread_mutex_lock(&m);
  int my_num = ticket_dist++;
  while(my_num!=display || readers>0){
    pthread_cond_wait(&c,&m);
  }
  pthread_mutex_unlock(&m);
}

void exitWrite(){
  pthread_mutex_lock(&m);
  display++;
  pthread_cond_broadcast(&c);
  pthread_mutex_unlock(&m);
}

void enterRead(){
  pthread_mutex_lock(&m);
  int my_num = ticket_dist++;
  while(my_num!=display){
    pthread_cond_wait(&c,&m);
  }
  readers++;
  display++;
  pthread_cond_broadcast(&c);
  pthread_mutex_unlock(&m);
}

void exitRead(){
  pthread_mutex_lock(&m);
  readers--;
  if (readers==0){
    pthread_cond_broadcast(&c);
  }
  pthread_mutex_unlock(&m);
}
```

**OBS:** Usualmente, al solucionar el problema de hambruna se limita el paralelismo en la solucion.

Sin embargo, esta solucion posee un problema denominado **Problema de cambio de contexto**, el cual ocurre cuando se cambia el thread que ejecuta un proceso.

## Problema: numero explosivo de cambios de contexto

* Volviendo al problema del recurso compartido, considere que hay $n$ threads en espera.
* Al liberar el recurso se deben despertar los $n$ threads, pero solo uno adquiere el recurso.
* Se producen $n-1$ **cambios de contexto** inutiles.
* Para el caso de los lectores/escritores, considere que hay $n$ lectores en espera.
* Cuando el escritor sale, despierta a los $n$ threads.
* Algunos threads no tendran el siguiente numero y se volveran a dormir.
* Cuando por fin se despierta el thread con el siguiente numero, vuelve a despertar a todos los lectores.
* **En el peor caso se producen $O(n^2)$ cambios de contexto inutiles.**

## Patron **request** para atender en un orden especifico: caso orden de llegada

```c
typedef struct{
  int ready; // 0: no ready | 1: ready
  pthread_cond_t w;
} Request;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int busy = 0; // Condicion para ver si el mutex esta ocupado
Queue *q; // Cola FIFO

void ocupar(){
  pthread_mutex_lock(&m);
  if (!busy) busy=1; 
  else{
    Request req = {0, PTHREAD_COND_INITIALIZER};
    put(q, &req);
    while(!req.ready) pthread_cond_wait(&req.w, &m);
  }
  pthread_mutex_unlock(&m);
}

void desocupar(){
  pthread_mutex_lock(&m);
  if (emptyQueue(q)) busy=0;
  else{
    Request *preq = get(q);
    preq->ready=1;
    pthread_cond_signal(&preq->w);
  }
  pthread_mutex_unlock(&m);
}
```
* Si el recurso no está ocupado, entonces se utiliza y se marca la variable `busy` con 1.
* En la cola se coloca la direccion de la variable request.
* Si la cola se encuentra vacia, marcamos `busy` con 0.
* Al ser una cola de tipo FIFO, cuando se obtienen los elementos de esta, que poseen la condicion de los threads, estos seran atendidos en orden de llegada.
* Ya sea `pthread_cond_signal()` o `pthread_cond_broadcast()`, pueden ser utilizados pues cada thread se rige por el `request`.

---

# Aplicaciones del patron request
* Para atender por orden de llegada, pero la cola no tiene por que ser FIFO.
* Puede ser una cola de prioridades para atender las solicitudes mas importantes primero.
* En el caso general se puede usar cualquier tipo de datos que permita satisfacer los requerimientos en el orden de atencion.
* Una mejor solucion del problema de los lectores/escritores es atender por orden de llegada, pero con la siguiente excepcion: cuando se va un escritor y le toca entrar a un lector, entran todos los lectores en espera.
