# Recuerdo Lectores/Escritores por orden de llegada
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

**Por qué es necesario despertar con `pthread_cond_broadcast()`?**
* En caso de no despertar a los threads, puede darse el caso de que dos lectores esten a la espera con sus respectivos tickets y solo sea atendido el primer thread para el cual su ticket coincide con el display, para luego terminar su tarea, aumentar el display, y dejar en una espera eterna al segundo lector.

# Lectores/Escritores por orden de llegada con patron **Request**

Como bien se vió en la anterior clase, la solucion a este problema generaba un numero explosivo de **cambios de contexto**, ya que al despertar, el thread que se despierta es aleatorio por lo que puede que se despierte un thread que no posea el ticket actual que corresponde, despertandose en vano.

La solucion correcta es utilizar un **patro de Request**:
```c
typedef struct {
  int ready;
  pthread_cond_t w;
  ...
} Request

Req req = {0, PTHREAD_COND_INITIALIZER, ...};
while(!req.ready)
  pthread_cond_wait(&req.w, &m);
```

**OBS: El `while()` debe estar presente pues en la implementacion de los pthreads, estos pueden despertarse aun declarandose la espera con `pthread_cond_wait()`.**

La solucion es:
```c
typedef enum {READER, WRITER} Kind; // enumeracion para distinguir entre lector/escritor
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int readers = 0, writing = 0;
Queue q; // = makeQueue() // Se inicializa en el main

void enterWrite() {
  pthread_mutex_lock(&m);
  if (readers==0 && !writing)
    writing = 1;
  else
    enqueue(WRITER);
  pthread_mutex_unlock(&m);
}

void exitWrite() {
  pthread_mutex_lock(&m);
  writing = 0;
  wakeup();
  pthread_mutex_unlock(&m);
}

void enterRead() {
  pthread_mutex_lock(&m);
  if (!writing && emptyQueue(q))
    readers++;
  else
    enqueue(READER);
  pthread_mutex_unlock(&m);
}

void exitRead() {
  pthread_mutex_lock(&m);
  readers--;
  if (readers==0)
    wakeup();
  pthread_mutex_unlock(&m);
}
```
En las siguientes funciones es necesario siempre pedir la exclusion mutua:

* `enterWrite()`: Si no hay ningun lector ni escritor podemos entrar a escribir, sino se debe esperar. Para esperar, se agrega el escritor a una cola para luego ser atendido por orden de llegada.
* `exitWrite()`: Se cambia el valor de `writing` a cero para indicar que no hay nadie escribiendo y se llama a `wakeup()`. Cabe mencionar, que para este caso, solo puede haber un escritor a la vez escribiendo.
* `enterRead()`: El lector puede entrar si no hay nadie escribiendo y si no hay ningun escritor en espera en la cola (pues se atiende por orden de llegada), en caso contrario se agrega el lector a la cola de espera.
* `exitRead()`: Si la cantidad de lectores es cero entonces se llama a `wakeup()` para que entren nuevos lectores o escritores.

Por otra parte el patron request a seguir es:
```c
typedef struct {
  int ready;
  Kind kind;
  pthread_cond_t w;
} Request;

void enqueue(Kind kind) {
  // ¡Patrón request!
  Request req= { 0, kind, PTHREAD_COND_INITIALIZER };
  put(q, &req); // Encolamos
  while (!req.ready)
    pthread_cond_wait(&req.w, &m);
}

void wakeup() {
  Req *pr= peek(q); // get(q)
  if (pr==NULL)
    return;
  if (pr->kind==WRITER) {
    writing= 1; // entra un escritor
    pr->ready= 1;
    pthread_cond_signal(&pr->w);
    get(q); // Sale de la cola
  }
  else { // ¡ pr->kind==READER !
    do { // entran lectores consecutivos
      readers++;
      pr->ready= 1;
      pthread_cond_signal(&pr->w);
      get(q); // Sale de la cola
      pr= peek(q);
      // Si es escritor no sale de la cola
    } while ( pr!=NULL &&
    pr->kind==READER );
  }
}
```
* `ready`: Se encarga de verificar si el lector o escritor esta listo.
* `kind`: Variable para distinguir entre escritor y lector.
* `w`: Condicion para el thread.
* Notar que en `enqueue()` no se solicita la exclusion mutua, pues esta funcion se encuentra dentro de funciones que piden la exclusion mutua. Luego se pone en espera al escritor/lector condicionandolo con el campo `req.ready` de la estructura.
* Notar que el `pthread_cond_wait()` es especificado con un mutex general, declarado al inicio.
* `peek()`: Obtiene el primer elemento de la cola sin sacarlo de esta.
* `wakeup()`: Si la cola es vacia se retorna de inmediato. En caso de no estar vacia y tener un escritor, se avisa que entra un escritor, se asigna 1 a la variable `ready` del escritor para cambiar el estado del `while`, sin embargo tambien hay que despertarlo para que obtenga el mutex. Por ultimo, se saca el escritor de la cola.
  
  Por otra parte si es un lector el que entra, puede existir el caso de que existan muchos lectores consecutivos en la cola, por tanto pueden entrar todos, para lo cual se aplica un `do-while`, hasta el momento en que se encuentre un escritor o la cola se vacia.

## Comparacion
* Solucion que saca numero: $O(n^2)$ cambios de contexto inutiles en el peor caso.
* Solucion con patron request: ningun cambio de contexto inutil.

## Criterios de entrada para los lectores/escritores
Los criterios posibles a escoger para solucionar este problema son:
* Orden indefinido o con prioridades: pueden conducir a **hambruna**.
* Por orden de llegada: intuitivo, pero **reducen paralelismo**.
* Por orden de llegada, pero cuando le toca a un lector, entran todos los lectores en espera. Si llega un nuevo lector, ingresa si y solo si no hay un escritor dentro o en espera. **No reduce tanto el paralelismo**.
* Entradas alternadas.

# Lectores/Escritores: Orden de llegada, entran todos los lectores en espera

La solucion planteada es:
```c
typedef enum { READER, WRITER } Kind;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int readers= 0, writing= 0;
Queue wq, rq; // = makeQueue()x2

void enterWrite() {
  pthread_mutex_lock(&m);
  if (readers==0 && !writing)
    writing = 1;
  else
    enqueue(WRITER);
  pthread_mutex_unlock(&m);
}

void exitWrite() {
  pthread_mutex_lock(&m);
  writing = 0;
  wakeup();
  pthread_mutex_unlock(&m);
}

void enterRead() {
pthread_mutex_lock(&m);
  if (!writing && emptyQueue(wq))
    readers++;
  else
    enqueue(READER);
  pthread_mutex_unlock(&m);
}

void exitRead() {
  pthread_mutex_lock(&m);
  readers--;
  if (readers==0)
    wakeup();
  pthread_mutex_unlock(&m);
}

typedef struct {
  int ready; // int kind;
  pthread_cond_t w;
} Request;

Request dummy;

void enqueue(Kind kind) {
  Request req= {0, PTHREAD_COND_INITIALIZER};
  if (kind==WRITER)
    put(wq, &req);
  else {
// Para recordar el turno de
// los lectores
  if (emptyQueue(rq))
    put(wq, &dummy);
    put(rq, &req);
  }
  while (!req.ready)
  pthread_cond_wait(&req.w, &m);
}

void wakeup() {
  Req *pr= get(wq); // peek(wq)
  if (pr==NULL)
    return;
  if (pr!=&dummy) {
// es el turno de un escritor
  writing= 1;
  pr->ready= 1;
  pthread_cond_signal(&pr->w);
  }
  else {
// es el turno de todos los
// lectores
    while (!emptyQueue(rq)){
      pr= get(rq);
      readers++;
      pr->ready= 1;
      pthread_cond_signal(&pr->w);
    }
  }
}
```
Donde los cambios en que se diferencian de la anterior solucion son:
* Se crean dos colas `wq` y `rq`.
* Se pregunta si la cola de escritores se encuentra vacia en la funcion `enterRead()`.
* Se elimina la variable `kind` de la estructura request, pues ahora son dos colas separadas.
* En `enqueue()`, si es un escritor se agrega directamente a la cola de escritores para esperar con `pthread_cond_wait()`. En caso de ser un lector se encola en la cola de lectores, pues luego se van a lanzar todos estos para que se ejecuten en paralelo. Al encolarse el primer lector, encolamos ademas el nodo `dummy` como rotulo para luego dejar pasar todos los lectores.
* En `wakeup()` utilizamos `get()` para sacar directamente los elementos de la cola. Si al sacar el elemento, este no es el `dummy` nos encontramos con que es el turno de un escritor, por tanto hacemos lo correspondiente a ese caso, si en cambio es el nodo `dummy`, sabemos que sera el turno de todos los lectores, por lo tanto creamos un ciclo `while` para extraer los lectores de la cola de lectores con las condiciones necesarias (notar que a estos lectores les falta obtener el mutex).