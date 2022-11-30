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
void prodcons_init(void);

/* Structure that declares the usual file */
/* access functions */
struct file_operations pipe_fops = {
  read: pipe_read,
  write: pipe_write,
  open: pipe_open,
  release: pipe_release
};

/* Declaration of the init and exit functions */
module_init(pipe_init);
module_exit(pipe_exit);

/*** El driver para lecturas sincronas *************************************/

#define TRUE 1
#define FALSE 0

/* Global variables of the driver */

int pipe_major = 61;     /* Major number */

/* Buffer to store data */
#define MAX_SIZE 10

static char *pipe_buffer;
static int in, out, size;

/* El mutex y la condicion para pipe */
static KMutex mutex;
static KCondition cond;

int prodcons_init(void) {

}

void prodcons_exit(void) {

}

static int prodcons_open(struct inode *inode, struct file *filp) {

}

static int prodcons_release(struct inode *inode, struct file *filp) {

}

static ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {

}

static ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
  
}