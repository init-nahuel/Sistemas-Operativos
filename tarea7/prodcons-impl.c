/* Implemente aqui el driver para /dev/prodcons */

/* Necessary includes for device drivers */
#include <linux/init.h>
/* #include <linux/config.h> */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

/* Declaration of prodcons functions*/
static int prodcons_open(struct inode *inode, struct file *filp);
static int prodcons_release(struct inode *inode, struct file *filp);
static ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

void prodcons_exit(void);
int prodcons_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations prodcons_fops = {
  read: prodcons_read,
  write: prodcons_write,
  open: prodcons_open,
  release: prodcons_release
};

/* Declaration of the init and exit functions */
module_init(prodcons_init);
module_exit(prodcons_exit);

/*** El driver para lecturas sincronas *************************************/

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int prodcons_major = 61;     /* Major number */

/* Buffer to store data */
#define MAX_SIZE 1000

static char *prodcons_buffer; // Driver's Content
static ssize_t curr_size; // Initially 0, because the buffer is empty
static int writing; // Writers works with mutual exclusion
static int readers; // Counter of readers reading the buffer
static int pend_open_write; // Writers waiting to write when there is already another writer in the driver

/* El mutex y la condicion para pipe */
static KMutex mutex;
static KCondition cond;

int prodcons_init(void) {

  int rc;

  /* Registiring device*/
  rc = register_chrdev(prodcons_major, "prodcons", &prodcons_fops);
  if (rc < 0) {
    printk("<1>prodcons: cannot obtain major number %d\n", prodcons_major);
    return rc;
  }

  writing = FALSE;
  readers = 0;
  pend_open_write = 0;
  curr_size = 0;
  m_init(&mutex);
  c_init(&cond);

  /* Allocating prodcons_buffer*/
  prodcons_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (prodcons_buffer == NULL) {
    prodcons_exit();
    return -ENOMEM;
  }

  //memset(prodcons_buffer, 0, MAX_SIZE); // puede no ir?
  printk("<1>Inserting prodcons module\n");
  return 0;
}

void prodcons_exit(void) {
  /* Freeing the major number*/
  unregister_chrdev(prodcons_major, "prodcons");

  /* Freeing buffer prodcons*/
  if (prodcons_buffer != NULL) {
    kfree(prodcons_buffer);
  }

  printk("<1>Removing prodcons module\n");
}

/* No se necesita hacer nada */
static int prodcons_open(struct inode *inode, struct file *filp) {
  /*
  int rc = 0;
  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    printk("<1>open request for write\n");
    pend_open_write++;
    while (writing || readers>0) {
      if (c_wait(&cond, &mutex)) {
        pend_open_write--;
        c_signal(&cond);
        rc = -EINTR;
        goto epilog;
      }
    }
    writing = TRUE;
    pend_open_write--;
    curr_size = 0; // Reset the buffer by the new writer
    c_signal(&cond);
    printk("<1>open for write succesful\n");
  }
  else if (filp->f_mode & FMODE_READ) {
    // Para evitar hambruna de los escritores, si nadie esta escribiendo
    // pero hay escritores pendientes (esperan porque readers>0), los
    // nuevos lectores deben esperar hasta que todos los lectores cierren
    // el dispositivo e ingrese un nuevo escritor.
    while (!writing && pend_open_write>0) {
      if (c_wait(&cond, &mutex)) {
        rc = -EINTR;
        goto epilog;
      }
    }
    readers++;
    printk("<1>open for read\n");
  }

epilog:
  m_unlock(&mutex);
  return rc;
  */
  return 0;
}

/* No se necesita hacer nada */
static int prodcons_release(struct inode *inode, struct file *filp) {
  /*
  m_lock(&mutex);

  if (filp->f_mode & FMODE_WRITE) {
    writing = FALSE;
    c_signal(&cond);
    printk("<1>close for write succesful\n");
  }
  else if (filp->f_mode & FMODE_READ) {
    readers--;
    if (readers == 0) {
      c_signal(&cond);
    }
    printk("<1>close for read (readers remaining=%d)\n", readers);
  }

  m_unlock(&mutex);
  */
  return 0;
}

static ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
  
  ssize_t rc;
  m_lock(&mutex);

  while (curr_size <= *f_pos && writing) {
    /*
    * Si el lector esta en el final del archivo pero hay un proceso
    * escribiendo en el archivo, el lector espera
    */
   if (c_wait(&cond, &mutex)) {
      printk("<1>read interrupted\n");
      rc = -EINTR;
      goto epilog;
   }
  }

  if (count > curr_size - *f_pos) {
    count = curr_size - *f_pos;
  }

  printk("<1>read %d bytes at %d\n", (int)count, (int)*f_pos);

  /* Transfiriendo datos hacie el espacio del usuario */
  if (copy_to_user(buf, prodcons_buffer+*f_pos, count)!=0) {
    /* El valor de buf es una direccion invalida */
    rc = -EFAULT;
    goto epilog;
  }

  *f_pos += count;
  rc = count;

epilog:
  m_unlock(&mutex);
  return rc;
  
}

static ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
  ssize_t rc;
  loff_t last;

  m_lock(&mutex);

  last = *f_pos + count;
  if (last > MAX_SIZE) {
    count -= last-MAX_SIZE;
  }
  printk("<1>write %d bytes at %d\n", (int)count, (int)*f_pos);

  /* Transfiriendo datos desde el espacio del usuario */
  if (copy_from_user(prodcons_buffer+*f_pos, buf, count)!=0) {
    rc = -EFAULT;
    goto epilog;
  }

  *f_pos += count;
  curr_size = *f_pos;
  rc = count;
  c_signal(&cond);

epilog:
  m_unlock(&mutex);
  return rc;
}