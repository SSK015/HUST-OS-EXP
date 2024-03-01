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
#include "kernel/sync_utils.h"

#include "spike_interface/spike_utils.h"

volatile int counter = 0;

extern process user_app;

int finished1 = 0;
int finished0 = 0;
int finished2 = 0;
int finished3 = 0;

ssize_t sys_user_hart() {
  // sprint("hartid = ?: %s\n", buf);
  int cpuid = read_csr(mhartid);
  sprint("CPUID=%d", cpuid);
  return 0;
}
//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  int cpuid = read_tp();
  // int cpuid = user_app.trapframe->hartid;
  // if (finished0 == 0) {
  //   finished0 = 1;
  //   sprint("hartid = %d: %s\n", 0, buf);
  // } else if (finished1 == 0) {
  //   finished1 = 1;
  //   sprint("hartid = %d: %s\n", 1, buf);
  // }
  sprint("hartid = %d: %s", cpuid, buf);
  // if (cpuid == 0) {
  //   finished0 = 1;
  // } else {
  //   finished1 = 1;
  // }
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {

  // while (finished0 != 1 || finished1 != 1) {
  //   ;
  // }
  // sprint("hartid = ?: User exit with code\n");

  // sprint("Zhonggu lou\n");
// YS_user_exit

  int cpuid = read_tp();
  // sprint("Tianan men\n");
  // int cpuid = user_app.trapframe->hartid;
  // if (cpuid == 0) {
  //   sprint("I am CPU0!\n");
  // }

  sprint("hartid = %d: User exit with code:%d.\n", cpuid, code);
  // sleep(2);
  sync_barrier(&counter, 2);
  // if (cpuid == 0) {
  //   finished2 = 1;
  // } else {
  //   finished3 = 1;
  // }
  // while (finished3 != 1 && finished2 != 1) {
  //   ;
  // }
// volatile int counter = 0;

  // sprint("hartid = %d: User exit with code:%d.\n", cpuid, code);
  // in lab1, PKE considers only one app (one process).
  // therefore, shutdown the system when the app calls exit()
  // if (cpuid == 0) {
  //   sprint("hartid = %d: User exit with code:%d.\n", cpuid, code);
  //   sprint(" = ?: shutdown with code:%d.\n", code);

  //   shutdown(code);
  // } else {
  //   while (finished0) {
  //     ;
  //   }
  // }
  // if (cpuid == 1)
  // shutdown(code);
  // if (cpuid == 0) {
  //   shutdown(code);
  // } else {
  //   while(1)
  //     ;
  // }
  if (cpuid == 0) {
    sprint("hartid = %d: shutdown with code:%d.\n", cpuid, code);
    shutdown(code);
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
    case SYS_user_hartid:
      return sys_user_hart();
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
