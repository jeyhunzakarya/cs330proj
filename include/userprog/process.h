#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
int exStLm(struct thread* nxtElem);
void process_activate (struct thread *next);
extClTp(struct thread *curr);
void fnlPrc(struct thread * current,struct thread* parent );
struct thread * get_child (int pid);

#endif /* userprog/process.h */
