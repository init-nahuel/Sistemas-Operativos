# Virtualizacion

* Es el acto de crear una version virtual de algo **al mismo nivel de abstraccion**.
* Ejemplos: virtualizar el computador, virtualizar el disco o virtualizar la red.
* Tiene que ser el mismo nivel de abstraccion, es decir si se virtualiza un computador x86, el resultado es tambien un computador x86 pero virtual.
* Crear una version virtual de un computador arm en un computador x86 no es virtualizar, es **emular**.
* Pionero: El hipervisor VM/370 de los IBM 370 ofrecia una maquina virtual (VM) a cada usuario. La VM corria el sistema operativo CMS que era monoproceso, pero tambien podia correr VM/370.

# Virtualizacion del Hardware

* Es la virtualizacion de un computador, como plataforma completa, algunas de sus componentes o solo **la funcionalidad requerida para correr un sistema operativo** (**invitado** o **guest**).
* Se ocultan las caracteristicas fisicas de la plataforma real (**anfitrion** o **host**) presentando una plataforma abstracta.
* En el anfitrion el software que realiza la virtualizacion se denomina **hipervisor** y requiere privilegio de administrador para instalar modulos en el nucleo del sistema operativo anfitrion. Ej: VirtualBox, Vmware, Hyper-V, Xen, Kvm.
* Permite correr multiples sistemas operativos en un solo computador: uno actua como afitrio y el resto como invitados.
* Facilita la administracion de granjas de servidores.

# Tipos de virtualizacion de hardware

* **Completa (full):** La abstraccion ofrece el mismo **set de instrucciones** (modo usuario y privilegiado) que el del computador anfitrion, permitiendo correr el sistema operativo invitado sin ninguna modificacion. Ejemplo: VM/370
* **Asistido por hardware:** El computador anfitrion incluye hardware especificamente destinado a su virtualizacion para mejorar el desmpeño.
* **Paravirtualizacion:** La maquina virtual no necesariamente ofrece el mismo hardware del computador anfitrion. Podria no ofrecer las instrucciones privilegiadas como inhibir las interrupciones. A cambio ofrece una biblioteca de funciones que proveen la misma funcionalidad. Se necesita modificar el sistema operativo para que llame a estas funciones en vez de usar las instrucciones privilegiadas.

# Beneficios de la Virtualizacion

* **Snapshots:** Se graba el estado de la maquina virtual en un instante dado, para que sea restaurado mas tarde de ser necesario, por ejemplo porque se descubrió un virus.
* **Migracion:** La maquina virtual se trasporta a otra maquina fisica, porque se requiere hacer mantencion en la maquina fisica original.
* **Failover:** Cuando falla la maquina virtual, se restaura a una snapshot que se conoce en un estado consistente con el fin de mejorar la disponibilidad del sistema.
* **Containerization:** Es una maquina virtual que ofrece un mejor aislamiento del resto de los usuarios, que lo que ofrecen los procesos y archivos.

# Los desafios de la virtualizacion completa

* El nucleo del sistema operativo invitado corre sin cambios.
* El nucleo del sistema operativo necesita ejecutarse en modo sistema para poder ejecutar las instrucciones privilegiadas como inhibir/habilitar interrupciones, establecer las tablas de paginas para implementar los espacios de direcciones virtuales.
* Permitir que los nucleos invitados se ejecuten en modo sistema significaria que si un nucleo invitado se cae, **se cae tambien el anfitrion.**
* **Se necesita correr los nucleos invitados en modo usuario, como cualquier otro proceso.**
* Cuando un nucleo invitado ejecuta la instruccion que inhibe las interrupciones: **se gatilla una interrupcion que captura el anfitrion.**

**OBS: En una virtualizacion que no es completa esto no sucede, pues se utilizan funciones de biblioteca para inhibir/habilitar las interrupciones.**

# Implementacion (Virtualizacion Completa)

* Las interrupciones gatilladas por instrucciones privilegiadas de un nucleo invitado son capturadas por el hipervisor.
* El hipervisor emula el comportamiento de esas instrucciones.
* El nucleo invitado no captura ninguna interrupcion real.
* El anfitrion captura todas las interrupciones, pero redirige al hipervisor las relacionadas con el nucleo invitado.
* **El hipervisor gatilla la interrupcion virtual en el nucleo invitado solo cuando estan habilitadas por el nucleo invitado.**
* Es similar a lo que ocurre con las señales de Unix, solo que las señales estan relacionadas a eventos abstractors, no a dispositivos.
* Las interrupciones en el nucleo invitado estan relacionadas a dispositivos virtuales, no a eventos abstractos (por ejemplo, `Crtl+C`) como en el caso de las señales virtuales.

# Espacios de direcciones virtuales virtuales

* El nucleo invitado cree tener acceso a la memoria real del computador.
* Pero lo que considera memoria real es solo su porpio espacio de direcciones virtuales, provisto por el anfitrion (es solamente un proceso en el computador anfitrion).
* El nucleo invitado construye tablas de paginas para sus propios procesos. No sirven porque referencian las paginas virtuales del nucleo invitado.
* Durante el cambio de contexto ejecuta la instruccion que cambia la tabla de paginas, gatillando una interrupcion por instruccion privilegiada.
* El hipervisor traduce la tabla de paginas virtuales a una tabla de paginas reales.

**OBS: El hipervisor es un modulo que corre en modo sistema (tiene acceso al nucleo anfitrion), si existiese algun bug por el cual se cae entonces se cae todo el sistema, es decir, se cae el sistema anfitrion y por tanto la unica solucion es el reset (este error se conoce como `Kernel Panic`).**