/*
 * Below is the given application for lab1_challenge2 (same as lab1_2).
 * This app attempts to issue M-mode instruction in U-mode, and consequently raises an exception.
 */

#include "user_lib.h"
#include "util/types.h"

int main(void) {
  printu("Going to hack the system by running privilege instructions.\n");
  // we are now in U(user)-mode, but the "csrc" instruction requires M-mode privilege.
  // Attempting to execute such instruction will raise illegal instruction exception.

  printu("line 14\n");
  printu("line 15\n");
  printu("line 16\n");
  asm volatile("csrc sscratch, 0xf");
  exit(0);
}

