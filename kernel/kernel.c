/*
 * Supervisor-mode startup codes
 */

#include "riscv.h"
#include "string.h"
#include "elf.h"
#include "process.h"

#include "spike_interface/spike_utils.h"

extern int global_hartid0;
extern int global_hartid1;

// process is a structure defined in kernel/process.h
process user_app;

//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
void load_user_program(process *proc, uint64 hartid) {
  // USER_TRAP_FRAME is a physical address defined in kernel/config.h
  proc->trapframe = (trapframe *)(USER_TRAP_FRAME + hartid * 4 * 16  * 16 * 16 * 16 * 16 * 16);
  memset(proc->trapframe, 0, sizeof(trapframe));
  proc->trapframe->hartid = hartid;
  // USER_KSTACK is also a physical address defined in kernel/config.h
  proc->kstack = USER_KSTACK + hartid * 4 * 16  * 16 * 16 * 16 * 16 * 16;
  proc->trapframe->regs.sp = USER_STACK + hartid * 4 * 16  * 16 * 16 * 16 * 16 * 16;

  // load_bincode_from_host_elf() is defined in kernel/elf.c
  load_bincode_from_host_elf(proc, hartid);
}

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
int s_start(void) {

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
  sprint("hartid = %d: Enter supervisor mode...\n", cpuid);

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
  load_user_program(&user_app, cpuid);
  // int cpuid = read_csr(sscratch);
  // hartid;


  sprint("hartid = %d: Switch to user mode...\n", cpuid);
  // switch_to() is defined in kernel/process.c
  switch_to(&user_app, cpuid);

  // we should never reach here.
  return 0;
}
