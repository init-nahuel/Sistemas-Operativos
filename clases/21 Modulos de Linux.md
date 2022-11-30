# Modulos de Linux

* Es un programa que corre en modo sistema dentro del nucleo.
* Se cargan y descargan dinamicamente.
* Usualmente corresponden a drivers de dispositivos.
* Surgen para evitar que el nucleo crezca desproporcionadamente.
* Al comienzo en Linux el nucleo era pequeño y los drivers tambien, luego se pasó a tener un nucleo mucho mas grande a causa de que se cargaban todos los drivers (ocupando mucha memoria), ahora en la actualidad gracias a los modulos de Linux solo se cargan los drivers de los dispositivos que estan presentes en el computador, los otros drivers se encuentran en archivos de sistemas, por ejemplo, cuando se agrega un dispositivo USB se carga dinamicamente el driver y cuando se deja de ocupar se descarga.

* **Problema: no dispone de la biblioteca de C ni llamadas a sistemas.**
* Solo el usuario root puede cargar y descargar modulos.

# Modulo Hello World

Este ejemplo despliega "Hello World" en los mensajes de sistema, es decir, en los mensajes del nucleo.
* `hello.c`:
```c
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

int init_module(void) {
  printk(KERN_ALERT "Hello world\n");
  return 0;
}
void cleanup_module(void) {
  printk(KERN_ALERT "Goodbye world\n");
}
```

* `MODULE_LICENSE("GPL")`: Es una macro de C para indicar la licencia del modulo, todos los modulos legalmente deben llevarlo.
* `init_module(void)`: inicializa el modulo sin parametro alguno, aca se inicializan las variables que necesitamos, por ejemplo pedir memoria.
* `cleanup_module(void)`: Libera los recursos requeridos, funcion de limpieza.

# Comandos Linux

Comandos mas importantes para trabajar con modulos en linux:

* `insmod module.ko`: Carga el modulo en el nucleo. Solo se puede ejecutar con root, es decir, `sudo bash`.
* `lsmod`: Ver los modulos ya cargados en el nucleo. Cualquier usuario puede ejecutarlo.
* `dmesg | tail`: Despliega en su salida estandar los mensajes del sistema, con el `| tail` se despliegan solo las ultima 10 lineas.
* `rmmod module`: Descarga el modulo desde el nucleo, solo usuarios root pueden ejecutarlo.

# Drivers

Los drivers son programas por los cuales se interactua entre el computador y los dispositivos conectados a esta.

* Los programas no conversan con los dispositivos de E/S directamente.
* Los dispositivos se disfrazan como pseudoarchivos para que puedan ser identificados por los programas.
  * Asi los programas usan `open`, `read` y `write` para conversar con los dispositivos. Ejemplo:
    * `/dev/input/mice`: Es el archivo para identificar el mouse.
    * `/dev/sda1`: Es el archivo para identificar la primera particion del disco.
* Un driver es un modulo que implementa `open`, `read`, `write` y `close` a nivel del nucleo para un dispositivo especifico.
* Es el driver el que conversa con el dispositivo por medio de E/S mapeada en direcciones de memoria
* Al ejecutar el comando `ls -l /dev/input/mice` se obtiene lo siguiente:

  ```
  crw-rw----1 root input 13 63 ...
  ```
  * Donde el permiso `c` para usuarios root indica que el archivo representa un dispositivo, en especifico este bit se denomina **character device** (orientado a caracteres, se lee por caracter), por otra parte el numero 13 se denomina **major** y es el numero que asocia el archivo con el driver que conversará, el ultimo numero se llama **minor**.
  * No tiene sentido que tengan permiso para ejecutar, dado que es un dispositivo.
  * Solo root tiene permiso para interactuar con el dispositivo.\
* Al ejecutar el comando `ls -l /dev/sda*` se obtiene:
  ```
  brw-rw----1 root disk 8 1 sda1
  brw-rw----1 root disk 8 5 sda5
  ```
  * La `a` indica que es el primer disco, pueden haber varios y en tal caso el siguiente disco tendria la letra `b`.
  * Un disco puede tener varias particiones, lo cual es indicado por el numero luego del nombre `sda`.
  * Donde el bit `b` es el bit de bloque, estos se deben leer por bloque y cada particion se distingue por su **minor**.

* **major:** Identificacion del driver asociado con el dispositivo.
* **minor:** Identificacion de los dispositivos para interactuar con un mismo driver.

## Clasificacion de dispositivos

Existen dos tipos de dispositivos:

* **Reales**: Dispositivos como el mouse, teclado, etc.

* **Virtuales**: Terminales virtuales para multiusuarios en computadoras.

* Un ejemplo de dispositivo virtuales son las terminales de comando, al abrir una terminal se esta creando un dispositivo virtual, con el comando `tty` (teletype) podemos imprimir el nombre del dispositivo virtual. Luego, en caso de abrir otra terminal, es posible comunicarse entre ellas, por ejemplo, escribiendo el comando:

  ```
  echo Hola que tal > /dev/pts/0
  ```
  * En nuestro caso el dispositivo virtual se llama `/dev/pts/0`.

**OBS:** El dueño de la terminal es uno mismo.

## Creacion Dispositivos Virtuales (Ejemplo modulo Mem)

Para crear dispositivos virtuales se utiliza el siguiente comando en una terminal con permiso root:

```
# mknod /dev/memory c 61 0
```

* En este caso se crear el disposivito `memory` de tipo `c`, es decir, character device, con un major 61 y minor 0, elegidos arbitrariamente. Cabe mencionar que se debe escoger con precaucion el numero major, pues se puede escoger una ya existente por lo que el dispositivo se asociaria a otro driver.

Luego, si queremos que cualquier usuario pueda leer o escribir en el dispositivo debemos cambiar sus permisos:

```
# chmod a+rw /dev/memory
```

Por ultimo, se debe cargar el modulo que implementa el driver para el dispositivo con:

```
# insmod memory.ko
```

* **El dispositivo se debe crear cada vez que se reinicia la maquina.**

Asi podemos interactuar con nuestro dispositivo de la siguiente manera:

* Escribimos sobre este:
  ```
  $ echo "Hola que tal" > /dev/memory
  ```
* Podemos ver lo escrito en el dispositivo con:
  ```
  $ cat /dev/memory
  ```

Usar `down_interruptible` cuando se espera un evento que sera gatillado por el usuario, por ejemplo `ctrl+c` para que el usuario termine de escribir en el buffer para pasarlo al dispositivo virtual con `cat`.