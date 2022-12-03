/* Implemente aqui el driver para /dev/prodcons */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/uaccess.h>

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");

static int prodcons_open(struct inode *inode, struct file *filp);
static int prodcons_release(struct inode *inode, struct file *filp);
static ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
static ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);

void prodcons_exit(void);
int prodcons_init(void);

struct file_operations prodcons_fops = {
  read: prodcons_read,
  write: prodcons_write,
  open: prodcons_open,
  release: prodcons_release
};

module_init(prodcons_init);
module_exit(prodcons_exit);

#define TRUE 1
#define FALSE 0

int prodcons_major = 61;

#define MAX_SIZE 8192

static char *prodcons_buffer;
static ssize_t curr_size;
static int writing;
static int contador;

static KMutex mutex;
static KCondition cond;

int prodcons_init(void) {
  int rc;

  rc = register_chrdev(prodcons_major, "prodcons", &prodcons_fops);
  if (rc<0) {
    return rc;
  }

  writing = FALSE;
  contador = 0;
  curr_size = 0;
  m_init(&mutex);
  c_init(&cond);

  prodcons_buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
  if (prodcons_buffer == NULL) {
    prodcons_exit();
    return -ENOMEM;
  }

  memset(prodcons_buffer, 0, MAX_SIZE);
  return 0;
}

void prodcons_exit(void) {
  unregister_chrdev(prodcons_major, "prodcons");

  if (prodcons_buffer != NULL) {
    kfree(prodcons_buffer);
  }
}

static int prodcons_open(struct inode *inode, struct file *filp) {
  return 0;
}

static int prodcons_release(struct inode *inode, struct file *filp) {
  return 0;
}

static ssize_t prodcons_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
  ssize_t rc;

  m_lock(&mutex);

  while ((curr_size <= *f_pos) || !writing) {

    if (c_wait(&cond, &mutex)) {
      rc = -EINTR;
      goto epilog;
    }
  }

  writing = FALSE;
  if (contador > curr_size-*f_pos) {
    contador = curr_size-*f_pos;
  }

  if (copy_to_user(buf, prodcons_buffer, contador)!=0) {
    rc = -EFAULT;
    goto epilog;
  }

  *f_pos = 0;
  rc = contador;
epilog:
  m_unlock(&mutex);
  return rc;
}

static ssize_t prodcons_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
  ssize_t rc;
  loff_t last;

  m_lock(&mutex);

  writing = TRUE;
  last = *f_pos + count;

  if (last > MAX_SIZE) {
    count -= last - MAX_SIZE;
  }

  if (copy_from_user(prodcons_buffer, buf, count)!=0) {
    rc = -EFAULT;
    goto epilog;
  }

  contador = count;
  *f_pos += count;
  curr_size = *f_pos;
  rc = count;
  c_signal(&cond);

epilog:
  m_unlock(&mutex);
  return rc;
}