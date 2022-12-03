# Entrada/Salida mapeada en memoria

* Seria demasiado caro agregar instrucciones de maquina para interactuar con cada tipo de dispositivo.
* Es mas economico interactuar con un dispositivo como si fuese memoria mapeada en algunas direcciones atribuidad a ese dispositivo.
* Por ejemplo para encender un led el driver escribe un 1 en la direccion `0xe0000000` y para apagarlo se escribe un 0 en la misma direccion.
* Para determinar si un boton, está presionado el driver lee la direccion `0xe0000000`, si el primer bit está en 1 quiere decir que está presionado y si está e 0, no lo está.
* A esto se llama **entrada/salida mapeada en memoria.**
* La direccion `0xe0000000` se denomina **puerto de entrada/salida.**
* **Ventaja: No se necesitan instrucciones de maquina especiales para interactuar con los dispositivos.** 

# Uso del led/boton

* Se crea el dispositivo `/dev/led-boton`
* El usuario abre el dispositivo con:
  ```c
  int fdLB = open("/dev/led-boton", O_RDWR)
  ```
* Enciende o apaga el led escribiendo un 1 o 0:
  ```c
  cgar c = 1; // 1 para encender, 0 para apagar
  write(fdLB, &c, sizeof(c)); // sizeof(c) == 1 siempre
  ```
* Leer el estado del boton:
  ```c
  read(fdLB, &c, sizeof(c));
  if (c)
    printf("ecendido\n");
  else
    printf("apagado\n");
  ```

## El driver para el led/boton

* Funcion que escribe (`led_boton_write`):

  ```c
  char *port = (char*)(intptr_t)0xe0000000;
  char ledVal;
  copy_from_user(&ledVal, buf, 1);
  *port = ledVal;
  ...
  ```
  * `intptr_t`: Cast de tipo para convertir el entero a un puntero del tamaño admitido por la plataforma en la cual se ejecuta el codigo, es decir, puede ser 32 o 64 bits.
  * Se copia lo que se encuentra en la variable `buf` en la variable `ledVal`.

* Funcion que leed (`led_boton_read`):

  ```c
  char *port = (char*)(intptr_t)0xe0000000;
  char botonVal = *port & 1; // borra bits 7 a 1
  copy_to_user(buf, &botonVal, 1);
  ...
  return 1;
  ```

# Leer un teclado

* Muchos dispositivos como un teclado se comunican con el computador por medio de una **puerta serial** (como USB).
* Cada vez que el usuario presiona una tecla, el teclado envia el codigo de la tecla en un byte por el **cable serial**, el que se almacena en un **buffer** en la puerta serial.
* La puerta serial se mapea en memoria en un puerto de datos y un puerto de control.
* El **puerto de control** sirve para determinar si el buffer tiene datos esperando a ser leidos.
* El **puerto de datos** sirve para recibir los datos almacenados en el buffer.

## Uso del teclado

* El teclado está asociado al dispositivo `/dev/teclado`
* Este programa lee el teclado y lo escribe en la salida estandar:

  ```c
  int fdTecl = open("/dev/teclado", O_RDONLY);
  for (;;) {
    char buf[80];
    int rc = read(fdTecl, buf, 80); // rc es siempre 1
    write(1, buf, rc);
  }
  ```
  * En este caso del teclado solo se puede leer por tanto se ocupa la macro `O_RDONLY`.
  * `read` lee solo un caracter, pero al estar dentro de un ciclo puede leer mas caracteres que se guardan en `buf`.
  * `write` escribe un solo caracter, pues `rc` es 1, desde el buffer `buf` a la salida estandar, por eso el 1.

## El driver del teclado

* En el driver, un thread consulta continuamente la puerta de control (`0xe0000001`) para determinar si llegó un byte del teclado, lo lee de la puerta de datos (`0x0000002`) y lo deposita en un buffer compartido con la funcion de lectura del driver:

  ```c
  char *port_ctrl = (char*)(intptr_t)0xe0000001;
  char *port_data = (char*)(intptr_t)0xe0000002;
  for (;;) {//busy-waiting!
    if (*port_ctrl & 1) {
      put(buf, *port_data);
      cnt++;
    }
  }
  ```
  * Pregunta si la puerta de control tiene bytes.

* En la funcion de lectura del driver (`teclado_read`):

  ```c
  char tecladoVal = get(buf); // espera si no hay nada
  // errata: en la clase la variable usrbuf se llamaba buf,
  // igual que el buffer buf compartido con el thread
  // pero son variables distintas!!!
  copy_to_user(usrbuf, &tecladoVal, 1);
  cnt--;
  ...
  return 1;
  ```

**OBS: En ambos casos falta implementar una herramienta de sincronizacion (mutex) para evitar dataraces pues ambas funciones acceden al buffer compartido `buf`.**

## Funcion `teclado_read` completa (pero sin manejo de errores)

```c
static int cnt = 0;
static ssize_t teclado_read(struct file *filp, char *usrbuf, size_t count, loff_t *f_pos) {
  char tecladoVal = get(buf); // espera si no hay nada
  copy_to_user(usrbuf, &tecladoVal, 1);
  cnt--;
  return 1;
}
```

# Dispositivos de salida como una impresora

* El caso de una impresora es similar al de un teclado, solo que envian datos a la impresora en vez de recibirlos.
* No hay que esperar a que lleguen los datos.
* **El problema es que el buffer de la puerta de comunicaciones es de tamaño limitado y se llena con facilidad.**
* La puerta de control indica cuando el buffer está lleno.
* En el driver, un thread consulta periodicamente si hay datos que enviar y si el buffer no está lleno.
* Si está lleno, espera.
* Ademas, la impresora podria no ser suficientemente rapida para imprimir lo que le envian.
* La impresora tambien posee un buffer que se puede llenar.
* El driver debe consultar cuanto espacio queda en el buffer de la impresora antes de enviar nuevos datos.
* Por lo tanto tambien se reciben datos de la impresora.

# Interrupciones

* El problema de los drivers anteriores es que el ciclo de busy-waiting consume el 100% de un core.
* Para evitar esto, se configura la puerta serial para que gatille una interrupcion cada vez que reciba un byte.
* Ya no se necesita un thread para recibir los bytes, pero si se requiere una rutina de atencion de interrupciones que se invoca cada vez que la puerta serial recibe un byte desde el teclado.
* La rutina de interrupcion ejecutara:

  ```c
  char *port_ctrl = (char*)(intptr_t)0xe0000001;
  char *port_data = (char*)(intptr_t)0xe0000002;
  while (*port_ctrl & 1 && cnt<bufsize) {
    put(buf, *port_data);
    cnt++;
  }
  ```

* **Las señales de PSS son la version virtualizada de las interrupciones.**

# El vector de interrupciones

* El vector de interrupciones es un arreglo de punteros a las rutinas de atecion de interrupciones para cada fuente de interrupcion.
* Cuando se gatilla una interrupcion, se determina un entero que identifica la fuente de la interrupcion y se usa como indice en el vector de interrupciones para seleccionar la rutina de atencion que se necesita invocar.
* Luego se elige cualquier core que no tenga las interrupciones inhibidas, se suspende el proceso o thread que está ejecutando y se invoca la rutina de atencion seleccionada en ese core.
* Cuando la rutina de interrupcion termina, se retoma el proceso o thread de manera transparente.
* Si no hay ningun core con las interrupciones habilitadas se espera hasya que se habiliten en algun core.
* La rutina de atencion podria activar un proceso en estado de espera, en cuyo caso el scheduler podria elegir ceder el core a ese proceso.

# Dispositivos de acceso por caracteres y por bloques

* Los dispositivos boton, led, teclado, mouse y otros son **character devices: se leen y escriben de a un solo caracter.**
* Un disco es un **block device: se lee y escribe por bloques de 512 bytes (o 4096 bytes en el caso de discos de al menos 2TB).**
* Cuando el disco gatilla una interrupcion, quiere decir que hay $n\cdot512$ bytes listos para ser leido o escritos.
* **Un disco nunca se lee o escribe de a un solo byte, siempre en multiplos de 512 bytes (o 4096).**
* El resto es bastante parecido a un dispositivo de caracteres: tambien se mapea en memoria, tiene puertos de datos y de control.
* Se podria decir que el caracter de un disco corresponde a 512 bytes.


# Canales de acceso directo a memoria

* En dispositivos veloces como un disco o la red, se puede consumir bastante tiempo de CPU leyendo o escribiendo datos desde o hacia el controlador de comunicaciones, porque leer un puerto es considerablemente mas lento que leer la memoria.
* Para evitar este consumo de CPU los procesadores modernos usan canales de acceso directo a memoria (**direct access memory o DMA**).
* **Un canal DMA es un circuito especializado en transferir datos entre controlador de comunicaciones (como USB o SATA) y la memoria.**
* Cuando se van a leer $n$ bloques de datos del disco, se configura el canal para que haga la transferencia y el canal se encarga de transferir los datos que vienen por la linea de comunicacion directamente a la memoria.
* Cuando la transferencia termina, se gatilla la interrupcion pero los datos ya estan en memoria.
* Solo falta activar el proceso que esperaba esos datos.

# Controlador de pantalla (GPU)

* Una pantalla tiene su propia memoria, llamada **frame buffer**, para almacenar la imagen de la pantalla.
* El frame buffer almacena cada pizel de la pantalla en al menos 3 bytes: uno describe la intensidad del ver, otro el rojo y otro el azul.
* Las pantallas suelen ser de 1920x1080 (full HD) hasta 3840x2160 pixeles (4K o ultra HD)
* El driver de la pantalla es un proceso que tiene acceso al frame buffer, como si fuese memoria normal, solo que escribir en él significa modificar el cotenido de la pantalla.
* En Unix el driber se llama servidor de X-Windows y no es parte del nucleo.
* El controlador de la pantalla, lee la imagen almacenada e el frame buffer 60 veces por segundo y lo envia a la pantalla LCD a traves de un cable digital HDMI o DisplayPort.
* En notebooks y celulares, el controlador de pantalla es una GPU (graphics processing unit) especializada en operaciones grafica y un porcentaje de la memoria se destina a frame buffer.
* En los computadores para gamers la GPU posee su propia memoria para el frame buffer y puede ser de varios GB.