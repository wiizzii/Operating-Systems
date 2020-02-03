/*#include "dm510_msgbox.h"
#include <stdlib.h>
#include <string.h>
*/
#include "linux/kernel.h"
#include "linux/unistd.h"
#include "linux/slab.h"
#include "linux/uaccess.h"
#include "linux/mutex.h"

typedef struct _msg_t msg_t;

struct _msg_t{
  msg_t* previous;
  int length;
  char* message;
};

static msg_t *bottom = NULL;
static msg_t *top = NULL;

DEFINE_MUTEX(lock_sys);

int dm510_msgbox_put( char *buffer, int length ) {
  //cant allocate negative space
  if (length < 0) {
    return -EINVAL;
  }
  //cant put in a non-existent string into kernel
  if (buffer == NULL){
    return -ENOMSG;
  }
  //checks if the address(buffer) given is valid
  if (access_ok(VERIFY_READ, buffer, length)) {
    msg_t* msg = kmalloc(sizeof(msg_t), GFP_KERNEL);
    //if kmalloc fails to allocate space
    if (msg==NULL) {
      return -ENOSPC;
    }
    msg->previous = NULL;
    msg->length = length;
    msg->message = kmalloc(length, GFP_KERNEL);
    //if kmalloc fails to allocate space
    if (msg->message == NULL){
      kfree(msg);
      return -ENOSPC;
    }
    //copy_from_user could not put all the bytes into the kernel that was alocated for
    /*int ch = copy_from_user(msg->message, buffer, length);
    if (ch!=0) {
      return -EBADE;
    }*/
    if (copy_from_user(msg->message, buffer, length)!=0){
      kfree(msg->message);
      kfree(msg);
      return -EBADE;
    }
    mutex_lock(&lock_sys);
    if (bottom == NULL) {
      bottom = msg;
      top = msg;
    } else {
      // not empty stack
      msg->previous = top;
      top = msg;
    }
    mutex_unlock(&lock_sys);
    return 0;
  }
  //if access_ok fails:
  return -EFAULT;
}

int dm510_msgbox_get( char* buffer, int length ) {
  //if there are no strings in the kernel
  if (top == NULL) {
    return -ENODATA;
  }
  //checks if the address(buffer) given is valid
  if (access_ok(VERIFY_WRITE, buffer, length)) {
    mutex_lock(&lock_sys);
    msg_t* msg = top;
    //if message is larger than the allocated space
    if (length < msg->length){
      mutex_unlock(&lock_sys);
      return -EMSGSIZE;
    }
    int mlength = msg->length;
    top = msg->previous;
    // copy message
    //copy_to_user could not get all the bytes from the kernel
    /*int ch = copy_to_user(buffer, msg->message, mlength);
    if (ch != 0) {
      printk("%d\n",ch);
      return -EBADE;
    }*/
    if (copy_to_user(buffer, msg->message, mlength)!=0){
      mutex_unlock(&lock_sys);
      return -EBADE;
    }
    mutex_unlock(&lock_sys);
    // free memory
    kfree(msg->message);
    kfree(msg);

    return mlength;
  }
  //if access_ok fails:
  return -EFAULT;
}
