/*
 * Utility functions for trap handling in Supervisor mode.
 */

#include "riscv.h"
#include "process.h"
#include "strap.h"
#include "syscall.h"
#include "pmm.h"
#include "vmm.h"
// #include "memlayout.h"
#include "util/functions.h"

#include "spike_interface/spike_utils.h"

//
// handling the syscalls. will call do_syscall() defined in kernel/syscall.c
//
static void handle_syscall(trapframe *tf) {
  // tf->epc points to the address that our computer will jump to after the trap handling.
  // for a syscall, we should return to the NEXT instruction after its handling.
  // in RV64G, each instruction occupies exactly 32 bits (i.e., 4 Bytes)
  tf->epc += 4;

  // TODO (lab1_1): remove the panic call below, and call do_syscall (defined in
  // kernel/syscall.c) to conduct real operations of the kernel side for a syscall.
  // IMPORTANT: return value should be returned to user app, or else, you will encounter
  // problems in later experiments!
  // char* data = "Hello world!\n";
  // long data_ptr = (long)data;
  // do_syscall(64,data_ptr,1,0,0,0,0,0);
  // do_syscall(65,0,0,0,0,0,0,0);
  tf->regs.a0 = do_syscall(tf->regs.a0, tf->regs.a1, tf->regs.a2, tf->regs.a3, tf->regs.a4, tf->regs.a5, tf->regs.a6, tf->regs.a7);
  // panic( "call do_syscall to accomplish the syscall and lab1_1 here.\n" );

}

//
// global variable that store the recorded "ticks". added @lab1_3
static uint64 g_ticks0 = 0;
static uint64 g_ticks1 = 0;
//
// added @lab1_3
//
void handle_mtimer_trap0() {
  sprint("Ticks %d\n", g_ticks0);
  // TODO (lab1_3): increase g_ticks to record this "tick", and then clear the "SIP"
  g_ticks0++;

  // field in sip register.
  // write_csr
  // hint: use write_csr to disable the SIP_SSIP bit in sip.
  int aaa = read_csr(sip);
  write_csr(sip, ~(aaa & 2));
  // panic( "lab1_3: increase g_ticks by one, and clear SIP field in sip register.\n" );

}

void handle_mtimer_trap1() {
  sprint("Ticks %d\n", g_ticks1);
  // TODO (lab1_3): increase g_ticks to record this "tick", and then clear the "SIP"
  g_ticks1++;

  // field in sip register.
  // write_csr
  // hint: use write_csr to disable the SIP_SSIP bit in sip.
  int aaa = read_csr(sip);
  write_csr(sip, ~(aaa & 2));
  // panic( "lab1_3: increase g_ticks by one, and clear SIP field in sip register.\n" );

}

//
// the page fault handler. added @lab2_3. parameters:
// sepc: the pc when fault happens;
// stval: the virtual address that causes pagefault when being accessed.
//
void handle_user_page_fault(uint64 mcause, uint64 sepc, uint64 stval) {
  
  sprint("handle_page_fault: %lx\n", stval);
  switch (mcause) {
    case CAUSE_STORE_PAGE_FAULT:
      int cpuid = read_tp();
      // sprint("CPUid = %d\n", cpuid);
      // TODO (lab2_3): implement the operations that solve the page fault to
      // dynamically increase application stack.
      // hint: first allocate a new physical page, and then, maps the new page
      // to the virtual address that causes the page fault. void* pa =
      // alloc_page(); uint64 va = g_ufree_page; stval += PGSIZE;
      // user_vm_map((pagetable_t)current->pagetable, stval, PGSIZE, (uint64)pa,
      //       prot_to_type(PROT_WRITE | PROT_READ, 1));
      // void* pa = alloc_page();
      // uint64 va = stval;
      // USER_STACK_TOP += PGSIZE;
      if (cpuid == 0) {
        g_ufree_page += PGSIZE;
      } else if (cpuid == 1) {
        g_ufree_page_other += PGSIZE;
      }

      sprint("start\n");
      void *pa = alloc_page();
      sprint("end\n");
      if (cpuid == 0) {
      user_vm_map((pagetable_t)current->pagetable, ROUNDDOWN(stval, PGSIZE), PGSIZE, (uint64)(pa),
            prot_to_type(PROT_WRITE | PROT_READ, 1));
      } else if (cpuid == 1) {
      user_vm_map((pagetable_t)currentOther->pagetable, ROUNDDOWN(stval, PGSIZE), PGSIZE, (uint64)(pa),
            prot_to_type(PROT_WRITE | PROT_READ, 1));
      }

        // void* pa = alloc_page();
      
        // user_vm_map((pagetable_t)current->pagetable, stval, 1, (uint64)pa,
        //  prot_to_type(PROT_WRITE | PROT_READ, 1));

      // uint64 va = stval;
      // stval += PGSIZE;
        // user_vm_map((pagetable_t)proc->pagetable, USER_STACK_TOP - PGSIZE, PGSIZE, user_stack,
        //  prot_to_type(PROT_WRITE | PROT_READ, 1));
// USER_STACK_TOP - PGSIZE
      // user_vm_map((pagetable_t)current->pagetable, stval, PGSIZE, (uint64)pa, prot_to_type(PROT_WRITE | PROT_READ, 0));
      // panic( "You need to implement the operations that actually handle the page fault in lab2_3.\n" );
      break;
    default:
      sprint("unknown page fault.\n");
      break;
  }
}

//
// kernel/smode_trap.S will pass control to smode_trap_handler, when a trap happens
// in S-mode.
//
void smode_trap_handler(void) {
  // make sure we are in User mode before entering the trap handling.
  // we will consider other previous case in lab1_3 (interrupt).
  // uint64 cpuid = read_tp();
  // if ((read_csr(sstatus) & SSTATUS_SPP) != 0)
  //   panic("usertrap: not from user mode");

  // assert(current);
  // // save user process counter.
  // current->trapframe->epc = read_csr(sepc);


  if ((read_csr(sstatus) & SSTATUS_SPP) != 0) panic("usertrap: not from user mode");
  uint64 cpuid = read_tp();

  if (cpuid == 0) {
    assert(current);
    // save user process counter.
    current->trapframe->epc = read_csr(sepc);
  } else if (cpuid == 1) {
    assert(currentOther);
    // save user process counter.
    currentOther->trapframe->epc = read_csr(sepc);
  }

  // if the cause of trap is syscall from user application.
  // read_csr() and CAUSE_USER_ECALL are macros defined in kernel/riscv.h
  uint64 cause = read_csr(scause);

  // use switch-case instead of if-else, as there are many cases since lab2_3.
  switch (cause) {
    case CAUSE_USER_ECALL:
      if (cpuid == 0) {
        handle_syscall(current->trapframe);
      } else if (cpuid == 1) {
        handle_syscall(currentOther->trapframe);
      }

      break;
    case CAUSE_MTIMER_S_TRAP:
      // handle_mtimer_trap();
    if (cpuid == 0) {
      handle_mtimer_trap0();
    } else if (cpuid == 1) {
      handle_mtimer_trap1();
    }
      break;
    case CAUSE_STORE_PAGE_FAULT:
    case CAUSE_LOAD_PAGE_FAULT:
      // the address of missing page is stored in stval
      // call handle_user_page_fault to process page faults
      handle_user_page_fault(cause, read_csr(sepc), read_csr(stval));
      break;
    default:
      sprint("smode_trap_handler(): unexpected scause %p\n", read_csr(scause));
      sprint("            sepc=%p stval=%p\n", read_csr(sepc), read_csr(stval));
      panic( "unexpected exception happened.\n" );
      break;
  }

  // continue (come back to) the execution of current process.
  if (cpuid == 0) {
    switch_to(current, cpuid);
    // sprint("goto0\n");
  } else if (cpuid == 1) {
    switch_to(currentOther, cpuid);
    // sprint("goto1\n");
  }
}
