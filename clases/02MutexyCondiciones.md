# Secciones criticas y Datarace
Este problema, denominado **Datarace**, ocurre en el momento en que multiples threads manipulan la misma estructura de datos, modificando los valores. La seccion de codigo donde esto ocurre se llama **Seccion Critica**.

# Exclusion Mutua
Para evitar el error de **Datarace**, se requiere de la **Exclusion Mutua** de los threads, es decir, que exista a lo mas un thread ejecutandose en la **Seccion Critica**.

# Mutex
Herramienta que se utiliza para garantizar la exclusion mutua sobre la seccion critica.

Operaciones:
* **Tipo y macro:** Con la macro no es necesario inicializar utilizando el tipo.
 ```c
 pthread_mutex_t
 PTHREAD_MUTEX_INITIALIZER
 ```

* **Inicializacion:**
  ```c
  int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t mutexattr);
  ```
  * `m`: Direccion del mutex que se quiere inicializar.
  * `mutexattr`: Atributos del mutex, por ahora sera NULL.
  
  **OBS**: En caso de querer inicializar mas de un mutex, se puede declarar un arreglo de tipo mutex, con la macro `PTHREAD_MUTEX_INITIALIZER`:
```c
  pthread_mutex_t [2]={PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER}; 
 ```

* **Destruccion:** Para asi liberar los recursos que se hayan requerido y evitar **Memory Leaks**.
  ```c
  int pthread_mutex_destroy(pthread_mutex_t *m);
  ```

# Funciones importantes para mutex

* **Bloquear mutex:** Se utiliza justo antes de entrar a la secion critica, asi se garantiza exclusion mutua.
  ```c
  int pthread_mutex_lock(pthread_mutex_t *m);
  ```
* **Liberar mutex:** Se utiliza justo al termino de la seccion critica.
  ```c
  int pthread_mutex_unlock(pthread_mutex_t *m);
  ```

**OBS: La metafora es la de una puerta con llave, asi cuando se llama a `lock()`, la puerta se cierra con llave, mientras que cuando se llama a `unlock()`, la puerta se desbloquea.**

# Ejemplo uso mutex
```c
Diccionario dicc; // No implementada, es solo para esquematizar el ejemplo
pthread_mutex_t m;
void init(){
  dicc = nuevoDiccionario();
  pthread_mutex_init(&m, NULL);
}
int autorizar(int cuenta, int monto){
  int ret = FALSO;
  pthread_mutex_lock(&m);
  // Comienzo seccion critica
  int saldo = consultar(dicc, cuenta);
  if (monto <= saldo){
    int nuevo_saldo = saldo-monto;
    modificar(dicc, cuenta, nuevo_saldo);
    ret = VERDADERO;
  }
  // Fin seccion critica
  pthread_mutex_unlock(&m);
  return ret;
}
```

# Observaciones respecto al mutex
* Con respecto a los threads, el mutex que se ejecutará primero es aquel que se encuentre primero en el codigo.
* **Conceptos referidos al mutex:**
  * Decimos que **dentro de la seccion critica** el mutex esta cerrado, o que se encuentra tomado o adquirido.
  * Decimos que **fuera de la seccion critica** el mutex esta abierto, o que se encuentra libre.
  * La accion de **bloquear el mutex** tambien se le denomina cerrar u obtener el mutex.
  * Se dice que el **thread que invocó el mutex es el dueño** hasta que se invoca a `unlock()`.

# Problema del Productor/Consumidor
Supongamos queremos reproducir un video en una pagina web, ejemplo Youtube. El pseudocodigo a programar seria:
```c
void reproducir(){
  for(;;){
    cuadro *c = leer();
    if (c == NULL){
      break;
    }
    mostrar(c); // Proyectar en pantalla
  }
}
```
* Problema: Velocidad variable del internet, por tanto el framerate del video es variable.
* Solucion: Reprogramar `reproducir()`. Leer hasta $N$ cuadros por adelantado, no mas. Si la red se pone lenta baja la reserva de cuadros. Si la reserva llega a 0, no queda otra que hacer una pausa en el video.
* Implementacion: Se usaran dos threads, uno que lee cuadros y otro que los proyecta. Ademas, se utiliza un buffer donde se guardaran los frames que se agregaran a este con `put()` desde el thread lector y luego se proyectan en el thread de proyector con `get()`.

Pseudocodigo:
```c
void reproducir(){
  Buffer *b = nuevoBuffer(30*5);
  pthread_t t;
  pthread_create(&t, NULL, proyector, b);
  lector(b);
  pthread_join(t, NULL);
}

void lector(Buffer *b){
  for (;;){
    Cuadro *c = leer();
    put(b,c);
    if (c==NULL){
      break;
    }
  }
}

void *protector(void *ptr){
  Buffer *b = ptr;
  for (;;){
    Cuadro *c = get(b);
    if (c==NULL){
      break;
    }
  mostrar(c);
  }
  return NULL;
}

/////// 
Item *get(Buffer *b){
  while(b->cnt == 0){
    // Ciclo de busy waiting hasta que el productor deposite un item
    // Esto en notebooks y celulares drena la bateria, por lo que son ineficientes
  }
  Item *it = b->a[b->out];
  b->out = b->out + 1;
  b->cnt--;
  return it;
}

void put(Buffer *b, Item *it){
  while(b->cnt == b->size);
  b->a[b->in] = it;
  b->in = (b->in+1)%b->size;
  b->cnt++;
}
```

# Condiciones
Una condicion permite esperar eficientemente hasta que ocurra un evento, asi no es necesario usar un ciclo de **Busy Waiting**.

Operaciones: (Todas estas funciones retornan 0 si se ejecutan correctamente)

* **Tipo y macro:** Con la macro no es necesario inicializar utilizando el tipo.
 ```c
pthread_cond_t
PTHREAD_COND_INITIALIZER
 ```
* **Inicializacion:**
  ```c
  int pthread_cond_init(pthread_cond_t *cond, pthread_condattr *a);
  ```
* **Destruccion:**
  ```c
  int pthread_cond_destroy(pthread_cond_t *cond);
  ```
* **Espera:**
  ```c
  int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *m);
  ```
  * El mutex debe estar cerrado al momento de la invocacion. Esta funcion suspende el thread que la invoca hasta que se invoca `broadcast()` o `signal()`. Por otra parte, cuando se invoca `wait()` se abre el mutex.

* **Despertar:**
  ```c
  int pthread_cond_broadcast(pthread_cond_t *cond);
  int pthread_cond_signal(pthread_cond_t *cond);
  ```
  * `broadcast()`: Despierta a todos los thread que estan a la espera en la condicion `cond`.
  * `signal()`: Despierta a uno de los thread que esta a la espera en la condicion `cond`.
  **OBS:** Despertar es relativo, un thread que se despierta todavia tiene que esperar a que se libere el mutex, que esta tomado por el thread que realiza la invocacion de `broadcast()` o `signal()`.