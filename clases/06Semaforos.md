# Semaforos
**Un semaforo representa un dispensador de fichas (analogo a semaforo de trenes).**

Operaciones:
* **Inicializar un semaforo con `val` fichas**:

  ```c
   void sem_init(sem_t *sem, int pshared, unsigned val);
   ```
  * `sem`: Sera la direccion del semaforo.
  * `pshared`: Valor que en nuestro caso sera 0.
  * `val`: Cantidad de fichas que tendra inicialmente el semaforo.

* **Extraer una ficha**:
  ```c
   void sem_wait(sem_t *sem);
   ```
   * Lo cual implica que disminuiran la cantidad de fichas, eventualmente sera cero.

* **Depositar una ficha**:
  ```c
   void sem_post(sem_t *sem);
   ```
   * Agrega una nueva ficha al semaforo, asi si existe algun thread esperando una ficha, se la puede llevar.

* **Destruir semaforo**:
  ```c
   void sem_destroy(sem_t *sem);
   ```
   * Libera los recursos que utilizó el semaforo no la memoria requerida para el semaforo.
  
* **Principio fundamental: Si no hay fichas disponibles, `sem_wait()` espera hasta que otro thread deposite una ficha con `sem_post()`.**
* Si hay varios threads en espera de una ficha, no se especifica un orden para otorgar las fichas.
* **El uso mas frecuente es para garantizar la exclusion mutua, en tal caso se inicializa con una sola ficha.**
* **Tambien se puede usar para suspender un thread hasta que se cumpla una condicion, en tal caso se inicializa con 0 fichas.**

**OBS:** Hay pocos usos en que un semaforo parte con varias fichas.

# Cena de filosofos resuelta con semaforos para garantizar la exclusion mutua

```c
sem_t pals[5];
…
for (int i= 0; i<5; i++)
  sem_init(&pals[i], 0, 1); // Debe partir con exactamente con 1 ficha

void filosofo(int i) {
  for (;;) {
    int j= min(i, (i+1)%5);
    int k= max(i, (i+1)%5);
    sem_wait(&pals[j]); // Garantiza exclusión mutua para palito j: lock
    sem_wait(&pals[k]); // Garantiza exclusión mutua para palito k: lock
    
    comer(i, (i+1)%5);

    sem_post(&pals[j]); // unlock
    sem_post(&pals[k]); // unlock
    pensar();
  } 
}
```

* Se declara un arreglo de 5 semaforos, donde cada semaforo se asocia a un palito; estos se incializan con una ficha.
* Se piden los palitos en orden de menor a mayor, pues en caso contrario se puede caer en el error de **Deadlock**.
* Es la mejor solucion pues ademas evita la **hambruna** de filosofos.

# Productor/Consumidor resuelto con semaforos
```c
typedef struct {
int size;
void **array;
int in, out;
sem_t empty, full;
} Buffer;

Buffer *nuevoBuffer(int size) {
  Buffer *buf = malloc(sizeof(Buffer));
  buf->size = size;
  buf->array = malloc(size*sizeof(void*));
  buf->in = buf->out = 0;
  sem_init(&buf->empty, 0, size);
  sem_init(&buf->full, 0, 0);
  return buf;
}

void put(Buffer *buf, void *item) {
  sem_wait(&buf->empty); // lock (no es un lock)
  buf->array[buf->in]= item;
  buf->in= (buf->in+1) % buf->size;
  sem_post(&buf->full); // unlock (no es un unlock)
}

void *get(Buffer *buf) {
  sem_wait(&buf->full); // lock (no es un lock)
  void *item= buf->array[buf->out];
  buf->out= (buf->out+1) % buf->size;
  sem_post(&buf->empty); // unlock (no es un unlock)
  return item;
}
```

* La diferencia con la solucion original es que en esta no es necesario un contador, este conteo lo hace el mismo semaforo.
* Se declaran dos semaforos:
  * `empty`: Representa las casillas vacias a traves de las fichas.
  * `full`: Representa las casillas llenas a traves de las fichas.
  * Por cada casilla vacia o llena habra una ficha.
* Notemos que `empty` se inicializa con `size` casillas vacias, pues al principio todas se encuentran vacias, analogamente `full` se inicializa con 0.
* `put()`: Coloca el item en el buffer en la posicion adecuada. Inicialmente se solicita una ficha de `empty` para asi garantizar la transaccion, en caso de no existir fichas entonces `put()` se bloquea hasta que un consumidor invoque `get()`. Por ultimo, se deposita una ficha en `full` para indicar que existe una casilla llena.
* `get()`: Inicialmente le pide una ficha a `full` para poder luego vaciarla.
* Sin embargo, no funciona con multiples consumidores/productores, ademas es dificil de tomar como molde para otros problemas.

**OBS: `sem_wait()` y `sem_post` se asemejan a lo que seria garantizar la exclusion mutua por medio de `lock()` y `unlock()`, pero no lo es, solo es obtener una ficha para depositar un item.**

# Productor/Consumidor resuelto con patron request usando semaforos en vez de motex/condiciones
```c
typedef struct {
  int size;
  void **array;
  int in, out, cnt;
  sem_t mutex;
  Queue prodq, consq;
} Buffer;

Buffer *nuevoBuffer(int size) {
  Buffer *buf= malloc(sizeof(Buffer));
  buf->size= size;
  buf->array= malloc(size*sizeof(void*));
  buf->in= buf->out= buf->cnt= 0;
  sem_init(&buf->mutex, 0, 1);
  buf->prodq= makeQueue();
  buf->consq= makeQueue();
  return buf;
}

void put(Buffer *buf, void *item) {
  sem_wait(&buf->mutex); // lock
  if (buf->cnt==buf->size) {
    sem_t w; // ¡Como condición!
    sem_init(&w, 0, 0);
    put(buf->prodq, &w);
    sem_post(&buf->mutex); // unlock
    sem_wait(&w); // cond wait
  }
  buf->array[buf->in]= item;
  buf->in= (buf->in+1) % buf->size;
  buf->cnt++;
  if (emptyQueue(buf->consq))
    sem_post(&buf->mutex); // unlock
  else {
    sem_t *pw= get(buf->consq);
    sem_post(pw); // cond signal
  }
}

void *get(Buffer *buf) {
… ejercicio …
}
```
* Como se observa, se implementa nuevamente el contador `cnt`. Se implementa un semaforo `mutex` que sera usado para garantizar la **exclusion mutua**.
* En `put()` garantizamos la **exclusion mutua** a traves de `sem_wait()`, luego se verifica que el contador no esté lleno. En caso de estar lleno, se debe suspender el thread, por tanto se crea un semaforo para esperar, inicializandolo con 0 fichas. Cabe mencionar, que como se observa, debe declararse `sem_post()` antes de `sem_wait()` pues en caso contrario no se devolveria la ficha del mutex, en los casos de semaforos el mutex no se libera automaticamente al esperar como era el caso de `wait()`. Por otra parte, en caso de encontrarse vacio el bufer del productor, se deposita una ficha en el semaforo para asi liberar el mutex; en caso contrario, se extrae la direccion del semaforo de la cola de consumidores para luego depositar la ficha en este, lo cual es equivalente a despertar con `signal()`.

**OBS: En el caso de semaforos no es necesario aplicar un ciclo `while` para esperar, pues eso ocurria solamente por la forma en que se implementaban los pthreads.**

# Lectores/Escritores resuelto con patron request/semaforos
```c
typedef enum { READER, WRITER } Kind;
sem_t mutex; // sem_init(&mutex, 0, 1);
int readers= 0, writing= 0;
Queue q; // = makeQueue()

void enterWrite() {
  sem_wait(&mutex); // lock
  if (readers==0 && !writing) {
    writing = 1;
    sem_post(&mutex); // unlock
  }
  else
    enqueue(WRITER);
}

void exitWrite() {
  sem_wait(&mutex); // lock
  writing = 0;
  wakeup();
  sem_post(&mutex); // unlock
}

void enterRead() {
  sem_wait(&mutex); // lock
  if (!writing && emptyQueue(q)){
    readers++;
    sem_post(&mutex); // unlock
  }
  else
  enqueue(READER);
}

void exitRead() {
  sem_wait(&mutex); // lock
  readers--;
  if (readers==0)
    wakeup();
  sem_post(&mutex); // unlock
}

typedef struct {
  Kind kind;
  sem_t w;
} Request;

void enqueue(Kind kind){
  // Patron request
  Request req = {kind};
  sem_init(&req.w,0,0);
  put(q, &req);
  sem_post(&mutex); // unlock
  sem_wait(&req.w);
}

void wakeup(){
  Req *pr = peek();
  if (pr == NULL)
    return;
  if (*pr->kind == WRITER){
    writing = 1; // Entra un escritor
    sem_post(&pr->w); // Signal
    get(q); // Sale de la cola
  }
  else{ // pr->kind == READER
    do { // Entran lectores consecutivos
      readers++;
      sem_post(&pr->w); // Signal
      get(q); // Sale de la cola
      pr = peek(q);
      // Si es escritod no sale de la cola
    } while(pr != NULL && pr->kind == READER);
  }
}
```
* `enterWrite()`: Notar que para este caso la funcion `enqueue()` se encargará de liberar el mutex, en caso de existir un escritor o lector en el bufer.
* `exitWrite()`: Notar que antes de realizar el desbloqueo del mutex, se declara la funcion `wakeup()`, para analizar si existe alguien mas que pueda seguir escribiendo o leyendo.
* `enterRead()`: Esta funcion es casi analoga a la original, simplemente difiere en que en caso de haber alguien escribiendo o esperando a escribir en la cola, entonces se encola al lector con `enqueue()`.
* `exitRead()`: En este caso si no hay ningun lector utilizamos `wakeup()` para despertar a los siguientes que utilicen el bufer.
* `enqueue()`: Para esta funcion es necesario la estructura `Request`, para poder almacenar el semaforo y ver si es un lector o escritor. Notemos ademas, que en este caso se declara una estructura de tipo `Request`, inicializando solo el tipo y no el semaforo, pues este se inicializa en la siguiente linea de codigo. Luego, se encola la request, se devuelve la ficha que habiamos pedido en las respectivas funciones que ocupan `enqueue()` y luego se espera, pidiendo una ficha, pero no del mutex, sino del campo del request `w`.
* `wakeup()`: Esta funcion se encarga de despertar a los threads, para el caso en que no hay nadie leyendo ni escribiendo. En este caso vemos quien está en primer lugar en la cola, sin sacarlo de esta, si es un escritor, indicamos que en el bufer se encuentra un escritor y ademas depositamos la ficha en el semaforo de la estructura `request` para poder asi despertar a este y desencolarlo de la cola. En caso de ser un lector, es similar, sin embargo se deja pasar a todos los lectores, hasta que en la cola se encuentre un escritor o se vacíe.

# Conclusiones
* Los semaforos son mas dificiles de usar que los mutex y condiciones.
* Pero cualquier solucion con mutex y condiciones se puede traducir a semaforos.
* No son mas eficiente, en algunos casos podrian ser menos eficientes.
* Algunos ambientes de programacion no ofrecen mutex/condiciones, pero si ofrecen semaforos, por ejemplo el nucleo de Linux.
* Tanto mutex/condiciones y semaforos se implementan en el bajo nivel mediante **spin-locks**.
* Un **spin-lock** es un semaforo que puede almacenar una sola ficha e implementado usando **busy-waiting**.
* La traduccion de mutex/condiciones a semaforos, tambien sirve para traducir a spin-locks.