/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "pmm.h"
#include "vmm.h"
#include "spike_interface/spike_utils.h"
#include "kernel/sync_utils.h"

int counter = 0;
int counterp = 0;
int isinit = 0;

typedef struct {
    volatile uint32_t lock; // volatile 用于防止编译器对锁的优化
} spinlock_t1;// 自旋锁结构体

// 初始化自旋锁
void spinlock1_init(spinlock_t1 *lock) {
    lock->lock = 0;
}

// 获取自旋锁
void spinlock1_lock(spinlock_t1 *lock) {
    while (__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE) == 1) {
        // 如果锁已被占用，继续自旋等待
        // 这里使用 __atomic_exchange_n 是为了保证原子操作
    }
}

// 释放自旋锁
void spinlock1_unlock(spinlock_t1 *lock) {
    __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
}

spinlock_t1 new_lock;


//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  int cpuid = read_tp();
  if (cpuid == 0) {
    assert(current);

    char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
      sprint(pa);
  } else if (cpuid == 1) {
    assert(currentOther);

    char* pa = (char*)user_va_to_pa((pagetable_t)(currentOther->pagetable), (void*)buf);
    sprint(pa);
  }


  return 0;
}

//
// implement the SYS_user_exit syscall
//
// ssize_t sys_user_exit(uint64 code) {
//   sprint("hartid = ?: User exit with code:%d.\n", code);
//   // in lab1, PKE considers only one app (one process). 
//   // therefore, shutdown the system when the app calls exit()
//   sprint("hartid = ?: shutdown with code:%d.\n", code);
//   shutdown(code);
// }
ssize_t sys_user_exit(uint64 code) {

  int cpuid = read_tp();

  sprint("hartid = %d: User exit with code:%d.\n", cpuid, code);
  // sleep(2);
  sync_barrier(&counter, 2);

  if (cpuid == 0) {
    sprint("hartid = %d: shutdown with code:%d.\n", cpuid, code);
    shutdown(code);
  }

  return 0;
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
uint64 sys_user_allocate_page() {
  // if (isinit == 0) {
  //   isinit = 1;
  //   spinlock1_init(&new_lock);

  // }

  int cpuid = read_tp();
  // spinlock1_lock(&new_lock);
      // sprint("hartid = %d: User exit with code:%d.\n", cpuid, 0);
  void *pa = alloc_page();

  // sync_barrier();
  // sync_barrier(&counterp, 1);
  // spinlock1_lock(&new_lock);

  uint64 va;
  // if (cpuid )
  if (cpuid == 0) {
    va = g_ufree_page;
    g_ufree_page += PGSIZE;



  } else if (cpuid == 1) {
    va = g_ufree_page_other;
    g_ufree_page_other += PGSIZE;    
  }


  // spinlock1_unlock(&new_lock);
  if (cpuid == 0) {
      user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));
  } else {
      user_vm_map((pagetable_t)currentOther->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));
  }

  sprint("hartid = %d: vaddr 0x%x is mapped to paddr 0x%x\n", cpuid, va, pa);
  // counterp = 0;
  // spinlock1_unlock(&new_lock);
  return va;
  // return 0;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  int cpuid = read_tp();
  if (cpuid == 0) {
    user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  } else if (cpuid == 1) {
    user_vm_unmap((pagetable_t)currentOther->pagetable, va, PGSIZE, 1);
  }
  return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    // added @lab2_2
    case SYS_user_allocate_page:
      return sys_user_allocate_page();
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
