#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "userprog/process.h"
#include "threads/synch.h"
void syscall_init (void);
void halt (void);
void exit (int status);
bool create(const char *file, unsigned initial_size);
bool remove(const char *file);
static struct file *process_get_file(int fd);
int process_add_file(struct file *file);
int write(int fd, const void *buffer, unsigned size);
int _write (int fd, const void *buffer, unsigned size);
void seek(int fd, unsigned position);
int open(const char *file);
int filesize(int fd);
void check_address(const uint64_t*);
void process_close_file(int fd);
struct lock filesys_lock;
int addFileHelper(int idx, struct file *file, struct file **descrTbl);
int read(int fd, void *buffer, unsigned size);
unsigned tell(int fd);
bool checkNullCase(int descr);
void close(int fd);
void closeOnCond(bool cond, struct file *fl);
tid_t fork (const char *thread_name);
void initLcAddr(char *flForOpen);
int exec (const char *file_name);
#endif /* userprog/syscall.h */
