/*
 * Supervisor-mode startup codes
 */

#include "riscv.h"
#include "string.h"
#include "elf.h"
#include "process.h"
#include "pmm.h"
#include "vmm.h"
#include "memlayout.h"
#include "spike_interface/spike_utils.h"
#include "kernel/sync_utils.h"

// process is a structure defined in kernel/process.h
// process user_app;
process user_app;
process other_user;

int counter1 = 0;
int counter2 = 0;
//
// trap_sec_start points to the beginning of S-mode trap segment (i.e., the
// entry point of S-mode trap vector). added @lab2_1
//
extern char trap_sec_start[];

//
// turn on paging. added @lab2_1
//
void enable_paging() {
  // write the pointer to kernel page (table) directory into the CSR of "satp".
  write_csr(satp, MAKE_SATP(g_kernel_pagetable));

  // refresh tlb to invalidate its content.
  flush_tlb();
}

//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
void load_user_program(process *proc, uint64 hartid) {
  
//   uint64 hartid = read_tp();
  sprint("hartid = %d: User application is loading.\n", hartid);
  // sync_barrier(&counter2, 2);
  // allocate a page to store the trapframe. alloc_page is defined in kernel/pmm.c. added @lab2_1

  proc->trapframe = (trapframe *)alloc_page();
  memset(proc->trapframe, 0, sizeof(trapframe));

  // allocate a page to store page directory. added @lab2_1
  proc->pagetable = (pagetable_t)alloc_page();
  memset((void *)proc->pagetable, 0, PGSIZE);

  // allocate pages to both user-kernel stack and user app itself. added @lab2_1
  proc->kstack = (uint64)alloc_page() + PGSIZE;   //user kernel stack top
  uint64 user_stack = (uint64)alloc_page();       //phisical address of user stack bottom

  // USER_STACK_TOP = 0x7ffff000, defined in kernel/memlayout.h
  proc->trapframe->regs.sp = USER_STACK_TOP;  //virtual address of user stack top


  sprint("hartid = %d: user frame 0x%lx, user stack 0x%lx, user kstack 0x%lx \n", hartid, proc->trapframe,
         proc->trapframe->regs.sp, proc->kstack);

  // load_bincode_from_host_elf() is defined in kernel/elf.c
  load_bincode_from_host_elf(proc, hartid);

  // populate the page table of user application. added @lab2_1
  // map user stack in userspace, user_vm_map is defined in kernel/vmm.c
  user_vm_map((pagetable_t)proc->pagetable, USER_STACK_TOP - PGSIZE, PGSIZE, user_stack,
         prot_to_type(PROT_WRITE | PROT_READ, 1));

  // map trapframe in user space (direct mapping as in kernel space).
  user_vm_map((pagetable_t)proc->pagetable, (uint64)proc->trapframe, PGSIZE, (uint64)proc->trapframe,
         prot_to_type(PROT_WRITE | PROT_READ, 0));

  // map S-mode trap vector section in user space (direct mapping as in kernel space)
  // here, we assume that the size of usertrap.S is smaller than a page.
  user_vm_map((pagetable_t)proc->pagetable, (uint64)trap_sec_start, PGSIZE, (uint64)trap_sec_start,
         prot_to_type(PROT_READ | PROT_EXEC, 0));
}


// void load_user_program(process *proc, uint64 hartid) {
//   // USER_TRAP_FRAME is a physical address defined in kernel/config.h
//   proc->trapframe = (trapframe *)(USER_TRAP_FRAME + hartid * 4 * 16  * 16 * 16 * 16 * 16 * 16);
//   // proc->trapframe = (trapframe *)(USER_TRAP_FRAME);
//   memset(proc->trapframe, 0, sizeof(trapframe));
//   proc->trapframe->hartid = hartid;
//   // USER_KSTACK is also a physical address defined in kernel/config.h
//   proc->kstack = USER_KSTACK + hartid * 4 * 16  * 16 * 16 * 16 * 16 * 16;

//   // sprint("XXX%x\n", proc->kstack);
//   // proc->kstack = USER_KSTACK;
//   proc->trapframe->regs.sp = USER_STACK + hartid * 4 * 16  * 16 * 16 * 16 * 16 * 16;
//   proc->trapframe->regs.tp = hartid;

//   // proc->trapframe->regs.sp = USER_STACK;

//   // load_bincode_from_host_elf() is defined in kernel/elf.c
//   load_bincode_from_host_elf(proc, hartid);
// }

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
// int s_start(void) {
//   sprint("hartid = ?: Enter supervisor mode...\n");
//   // in the beginning, we use Bare mode (direct) memory mapping as in lab1.
//   // but now, we are going to switch to the paging mode @lab2_1.
//   // note, the code still works in Bare mode when calling pmm_init() and kern_vm_init().
//   write_csr(satp, 0);

//   // init phisical memory manager
//   pmm_init();

//   // build the kernel page table
//   kern_vm_init();

//   // now, switch to paging mode by turning on paging (SV39)
//   enable_paging();
//   // the code now formally works in paging mode, meaning the page table is now in use.
//   sprint("kernel page table is on \n");

//   // the application code (elf) is first loaded into memory, and then put into execution
//   load_user_program(&user_app);

//   sprint("hartid = ?: Switch to user mode...\n");
  
//   uint64 hartid = 0;
  
//   vm_alloc_stage[hartid] = 1;
//   // switch_to() is defined in kernel/process.c
//   switch_to(&user_app);

//   // we should never reach here.
//   return 0;
// }


int s_start(void) {
  
  uint64 hartid = read_tp();
  sprint("hartid = %d: Enter supervisor mode...\n", hartid);
  // in the beginning, we use Bare mode (direct) memory mapping as in lab1.
  // but now, we are going to switch to the paging mode @lab2_1.
  // note, the code still works in Bare mode when calling pmm_init() and kern_vm_init().
  write_csr(satp, 0);

  // init phisical memory manager


  if (hartid == 0) {
       pmm_init();
       kern_vm_init();
       enable_paging();
       // the code now formally works in paging mode, meaning the page table is now in use.
       // sprint("kernel page table is on \n");
       sync_barrier(&counter1, 2);
  } else {
       sync_barrier(&counter1, 2);
  }
  // build the kernel page table


  // now, switch to paging mode by turning on paging (SV39)


  // the application code (elf) is first loaded into memory, and then put into execution
//   load_user_program(&user_app);


  


  sprint("hartid = %d: Switch to user mode...\n", hartid);
  

  // switch_to() is defined in kernel/process.c
//   switch_to(&user_app);

  uint64 cpuid = read_tp();
  // user_app.trapframe->hartid = cpuid;
  // sprint("CPU id = %d\n", global_hartid1);

  // sprint("saxasxasxCPU id = %llu\n", cpuid);
  // if (cpuid == 1) {
  //   sprint("xsxsxs\n");
  // } else if (cpuid == 0) {
  //   sprint("0000000\n");
  // } else {
  //   sprint("111111\n");
  // }
//   sprint("hartid = %d: Enter supervisor mode...\n", cpuid);

  // user_app.trapframe->hartid = cpuid;
  // Note: we use direct (i.e., Bare mode) for memory mapping in lab1.
  // which means: Virtual Address = Physical Address
  // therefore, we need to set satp to be 0 for now. we will enable paging in
  // lab2_x.
  //
  // write_csr is a macro defined in kernel/riscv.h
  write_csr(satp, 0);

    // int cpuid = read_csr(sscratch);

  // the application code (elf) is first loaded into memory, and then put into
  // execution
  if (cpuid == 0) {
    load_user_program(&user_app, cpuid);
    // write_csr(satp, 0);
    // write_tp(cpuid);
    // sprint("reach here0\n");
  } else if (cpuid == 1) {
    load_user_program(&other_user, cpuid);
    // sprint("reach here1\n");
  }

  // int cpuid = read_csr(sscratch);
  // hartid;



  // vm_alloc_stage[hartid] = 1;
  sprint("hartid = %d: Switch to user mode...\n", cpuid);
  // switch_to() is defined in kernel/process.c
  if (cpuid == 0) {
    switch_to(&user_app, cpuid);
  } else if (cpuid == 1) {
    switch_to(&other_user, cpuid);
  }

  // we should never reach here.
  return 0;
}

