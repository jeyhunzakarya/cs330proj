#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "userprog/process.h"
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include <list.h>

void syscall_entry (void);
const int STDIN = 1;
void syscall_handler (struct intr_frame *);


/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
   write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 |((uint64_t)SEL_KCSEG) << 32);
   write_msr(MSR_LSTAR, (uint64_t) syscall_entry);
   write_msr(MSR_SYSCALL_MASK,FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
   lock_init(&filesys_lock);
}

void check_address(const uint64_t* addr){
   if (!is_user_vaddr(addr) || addr == NULL || pml4_get_page(thread_current()->pml4, addr) == NULL)
      exit(-1);
} 

void closeOnCond(bool cond, struct file *fl) {
   if (cond) file_close(fl);
}

void initLcAddr(char *flForOpen) {
   check_address(flForOpen);
   lock_acquire(&filesys_lock);
}

int open (const char *flForOpen){
   initLcAddr(flForOpen);
   struct file *flToOpen = filesys_open(flForOpen);
   if(flToOpen) {int descr = process_add_file(flToOpen);
   closeOnCond(descr==-1, flToOpen);
   lock_release(&filesys_lock);
      return descr;
   }
   else return -1;
   lock_release(&filesys_lock);
   return -1;
}


int addFileHelper(int idx, struct file *file, struct file **descrTbl){
         descrTbl[idx] = file;
         thread_current()->fdidx = idx;
}

int process_add_file(struct file *file){
   struct file **descrTbl = thread_current()->file_descriptor_table;
   int idx = thread_current()->fdidx;
   while(idx < FDCOUNT_LIMIT) {
      if(descrTbl[idx] == NULL){
         addFileHelper(idx, file, descrTbl);
         return thread_current()->fdidx;
      }
      idx++;
   }
   idx = FDCOUNT_LIMIT;
   return -1;
}

bool checkNullCase(int descr){
   return (descr < 0 || descr >= FDCOUNT_LIMIT);
}
struct file *process_get_file (int descr){
   if (checkNullCase(descr)) return NULL;
   return thread_current()->file_descriptor_table[descr];
}

void process_close_file(int descr){
   if (checkNullCase(descr)) return NULL;
   thread_current()->file_descriptor_table[descr] = NULL;
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *file UNUSED) {
   // TODO: Your implementation goes here.
   int nmFrRax = file->R.rax;
   switch(nmFrRax){
      case SYS_HALT:
         halt();
         break;
      case SYS_EXIT:
         exit(file->R.rdi);
         break;    
      case SYS_FORK:   ;
         struct thread *curr = thread_current();
         memcpy(&curr->parent_if, file, sizeof(struct intr_frame));
         file->R.rax = process_fork(file->R.rdi, &thread_current()->parent_if);
         break;
      case SYS_EXEC:
         if (exec(file->R.rdi) == -1)
            exit(-1);
         break;
      case SYS_WAIT:
         file->R.rax = process_wait(file->R.rdi);
         break;
      case SYS_CREATE:
         check_address(file->R.rdi);
         file->R.rax= filesys_create(file->R.rdi, file->R.rsi);
         break;
      case SYS_REMOVE:
         check_address(file->R.rdi); 
         file->R.rax = filesys_remove(file->R.rdi);
         break;
      case SYS_OPEN: 
         file->R.rax = open(file->R.rdi);
         break;
      case SYS_FILESIZE:
         if (process_get_file(file->R.rdi) == NULL){
            file->R.rax =-1;
         }
         else {
            file->R.rax =file_length(process_get_file(file->R.rdi));
         }
         break;
      case SYS_READ:
         file->R.rax = read(file->R.rdi, file->R.rsi, file->R.rdx);
         break;
      case SYS_WRITE: 
         file->R.rax = write(file->R.rdi, file->R.rsi, file->R.rdx);
         break;
      case SYS_SEEK:
         if (process_get_file(file->R.rdi) > 2)
         file_seek(process_get_file(file->R.rdi), file->R.rsi);
         break;
      case SYS_TELL: 
         file->R.rax = tell(file->R.rdi);
         break;
      case SYS_CLOSE:
         close(file->R.rdi);
         break;
      default:
         exit(-1);
         break;
   }
}

void halt(void){power_off();}

tid_t fork (const char *thread_name){ return process_fork(thread_name, &thread_current()->parent_if);}

int exec (const char *flForExec){
   check_address(flForExec);
   char *nmCp = palloc_get_page(PAL_ZERO);
   strlcpy(nmCp, flForExec, strlen(flForExec) + 1);
   if(nmCp==NULL)
      exit(-1);
   else if (process_exec(nmCp) == -1 )
      return -1;
   return 0;
}

unsigned tell (int flForTl){
   if (flForTl >= 2) return file_tell(process_get_file(flForTl));
   return;
}

const int STDOUT = 2;

int read (int flForRead, void *buffForRead, unsigned sz){
   check_address(buffForRead);
   unsigned char *buf = buffForRead;
   struct file *flCr = process_get_file(flForRead);
   int rdSz =0; 
   if (flCr == NULL ||flCr == STDOUT) return -1;
   else if (flForRead < 0) return NULL;
   else if (flForRead>= FDCOUNT_LIMIT) return NULL;
   else if (flCr != STDIN){
      lock_acquire(&filesys_lock);
      rdSz = file_read(flCr, buffForRead, sz);
      lock_release(&filesys_lock);
   }
   return rdSz;
}

void close (int flForCls){
   if(flForCls < 2) return;
   else {
   struct file *f = process_get_file(flForCls);
   if(f)
      {
      process_close_file(flForCls);
      closeOnCond(f!=NULL, f);
      }
   } 
}

void exit(int stOfCC){
   thread_current()->exit_status = stOfCC;
   printf("%s: exit(%d)\n", thread_name(), stOfCC);
   thread_exit();
}

int write (int flForWrt, const void *bufForWrt, unsigned szForWrt){
   int writesize;
   struct file *f = process_get_file(flForWrt);

   check_address(bufForWrt);

   if (f == NULL || f== STDIN) return -1;
   
   else if (f!=STDOUT){
      lock_acquire(&filesys_lock);
      writesize = file_write(f, bufForWrt, szForWrt);
      lock_release(&filesys_lock);
   }
   else if (f ==STDOUT){
     putbuf(bufForWrt, szForWrt);
      writesize = szForWrt;
   }   
   return writesize;
}

void seek (int flForSk, unsigned tpOfCp){
   if (process_get_file(flForSk) > 2)
      file_seek(process_get_file(flForSk), tpOfCp);
}
