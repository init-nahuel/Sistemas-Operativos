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
#define MAX_SIZE 8192

static char *prodcons_buffer; 
static ssize_t curr_size; 
static int writing; // variable para indicar si fue leida por una shell
static int mipos; 
static int contador;

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
  mipos = 0;
  contador = 0;
  curr_size = 0;
  m_init(&mutex);
  c_init(&cond);

  /* Allocating prodcons_buffer*/
  prodcons_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (prodcons_buffer == NULL) {
    prodcons_exit();
    return -ENOMEM;
  }

  memset(prodcons_buffer, 0, MAX_SIZE); // puede no ir?
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
 return 0;
}

/* No se necesita hacer nada */
static int prodcons_release(struct inode *inode, struct file *filp) {
 return 0;
}

static ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
  
  ssize_t rc;
  
  m_lock(&mutex);

  while ((curr_size <= *f_pos && writing) || !writing) {
    /* si el lector esta en el final del archivo pero hay un proceso
     * escribiendo todavia en el archivo, el lector espera.
     */
    printk("waiting\n");
    if (c_wait(&cond, &mutex)) {
      printk("<1>read interrupted\n");
      rc= -EINTR;
      goto epilog;
    }
  }
  writing = FALSE;
  if (count > curr_size-*f_pos) {
    count= curr_size-*f_pos;
  }

  printk("<1>read %d bytes at %d\n", (int)contador, (int)*f_pos+mipos);

  /* Transfiriendo datos hacia el espacio del usuario */
  if (copy_to_user(buf, prodcons_buffer+mipos+*f_pos, contador)!=0) {
    /* el valor de buf es una direccion invalida */
    rc= -EFAULT;
    goto epilog;
  }

  rc= count;

epilog:
  m_unlock(&mutex);
  return rc;
}

static ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
  
  ssize_t rc;
  loff_t last;

  m_lock(&mutex);

  writing = TRUE;
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

  mipos = *f_pos;
  contador = count;
  *f_pos += count;
  curr_size = *f_pos;
  rc = count;
  c_signal(&cond);

epilog:
  m_unlock(&mutex);
  return rc;
}