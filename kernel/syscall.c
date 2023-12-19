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
#include "elf.h"

#include "spike_interface/spike_utils.h"

extern elf_ctx elfsymbol;

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  sprint(buf);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

int getSymOffset(uint64 ra)
{
  uint64 closest_func = 0;
  int idx = -1;
  for (int i = 0; i < elfsymbol.syms_count; i++)
  {
    if (elfsymbol.syms[i].st_info == STT_FUNC && elfsymbol.syms[i].st_value < ra && elfsymbol.syms[i].st_value > closest_func)
    {
      closest_func = elfsymbol.syms[i].st_value;
      idx = i;
    }
  }
  return idx;
}

ssize_t sys_user_backtrace(uint64 level) {
  uint64 sp = current->trapframe->regs.sp + 24;

  while (level--) {
    sp += 16;
    if (*(uint64*)(sp) == 0) break;
    int symbol_idx = getSymOffset(*(uint64*)(sp));
     if (symbol_idx == -1) {
        sprint("Error\n");
        continue;
      }
    sprint("%s\n", &elfsymbol.strtb[elfsymbol.syms[symbol_idx].st_name]);    
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
    case SYS_user_backtrace:
      return sys_user_backtrace(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
