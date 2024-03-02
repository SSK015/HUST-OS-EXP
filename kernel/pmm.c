#include "pmm.h"
#include "util/functions.h"
#include "riscv.h"
#include "config.h"
#include "util/string.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"

// 自旋锁结构体
typedef struct {
    volatile uint32_t lock; // volatile 用于防止编译器对锁的优化
} spinlock_t;

// 初始化自旋锁
void spinlock_init(spinlock_t *lock) {
    lock->lock = 0;
}

// 获取自旋锁
void spinlock_lock(spinlock_t *lock) {
    while (__atomic_exchange_n(&lock->lock, 1, __ATOMIC_ACQUIRE) == 1) {
        // 如果锁已被占用，继续自旋等待
        // 这里使用 __atomic_exchange_n 是为了保证原子操作
    }
}

// 释放自旋锁
void spinlock_unlock(spinlock_t *lock) {
    __atomic_store_n(&lock->lock, 0, __ATOMIC_RELEASE);
}

spinlock_t my_lock;

// _end is defined in kernel/kernel.lds, it marks the ending (virtual) address of PKE kernel
extern char _end[];
// g_mem_size is defined in spike_interface/spike_memory.c, it indicates the size of our
// (emulated) spike machine. g_mem_size's value is obtained when initializing HTIF. 
extern uint64 g_mem_size;

static uint64 free_mem_start_addr;  //beginning address of free memory
static uint64 free_mem_end_addr;    //end address of free memory (not included)

int vm_alloc_stage[NCPU] = { 0, 0 }; // 0 for kernel alloc, 1 for user alloc
typedef struct node {
  struct node *next;
} list_node;

// g_free_mem_list is the head of the list of free physical memory pages
static list_node g_free_mem_list;

//
// actually creates the freepage list. each page occupies 4KB (PGSIZE), i.e., small page.
// PGSIZE is defined in kernel/riscv.h, ROUNDUP is defined in util/functions.h.
//
static void create_freepage_list(uint64 start, uint64 end) {
  g_free_mem_list.next = 0;
  for (uint64 p = ROUNDUP(start, PGSIZE); p + PGSIZE < end; p += PGSIZE)
    free_page( (void *)p );
}

//
// place a physical page at *pa to the free list of g_free_mem_list (to reclaim the page)
//
void free_page(void *pa) {
  // 
  
  spinlock_lock(&my_lock);
  if (((uint64)pa % PGSIZE) != 0 || (uint64)pa < free_mem_start_addr || (uint64)pa >= free_mem_end_addr)
    panic("free_page 0x%lx \n", pa);

  // insert a physical page to g_free_mem_list
  list_node *n = (list_node *)pa;
  n->next = g_free_mem_list.next;
  g_free_mem_list.next = n;
  spinlock_unlock(&my_lock);
}

//
// takes the first free page from g_free_mem_list, and returns (allocates) it.
// Allocates only ONE page!
//
void *alloc_page(void) {
  // spinlock_init(&my_lock);
  spinlock_lock(&my_lock);
  list_node *n = g_free_mem_list.next;
  uint64 hartid = read_tp();

  if (vm_alloc_stage[hartid]) {
    sprint("hartid = %d: alloc page 0x%x\n", hartid, n);
  }
  if (n) g_free_mem_list.next = n->next;
  spinlock_unlock(&my_lock);
  return (void *)n;
}

//
// pmm_init() establishes the list of free physical pages according to available
// physical memory space.
//
void pmm_init() {
  // start of kernel program segment
  uint64 g_kernel_start = KERN_BASE;
  uint64 g_kernel_end = (uint64)&_end;

  uint64 pke_kernel_size = g_kernel_end - g_kernel_start;
  spinlock_init(&my_lock);
  sprint("PKE kernel start 0x%lx, PKE kernel end: 0x%lx, PKE kernel size: 0x%lx .\n",
    g_kernel_start, g_kernel_end, pke_kernel_size);

  // free memory starts from the end of PKE kernel and must be page-aligined
  free_mem_start_addr = ROUNDUP(g_kernel_end , PGSIZE);

  // recompute g_mem_size to limit the physical memory space that our riscv-pke kernel
  // needs to manage
  g_mem_size = MIN(PKE_MAX_ALLOWABLE_RAM, g_mem_size);
  if( g_mem_size < pke_kernel_size )
    panic( "Error when recomputing physical memory size (g_mem_size).\n" );

  free_mem_end_addr = g_mem_size + DRAM_BASE;
  sprint("free physical memory address: [0x%lx, 0x%lx] \n", free_mem_start_addr,
    free_mem_end_addr - 1);

  sprint("kernel memory manager is initializing ...\n");
  // create the list of free pages
  create_freepage_list(free_mem_start_addr, free_mem_end_addr);
}
