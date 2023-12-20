/*
 * Below is the given application for lab1_challenge1_backtrace.
 * This app prints all functions before calling print_backtrace().
 */

#include "user_lib.h"
#include "util/types.h"
void f1();
void f2();

int deep;
void f2() { f1(); }
void f1() {
  if(deep++ < 5)
    f2();
  else {
    print_backtrace(7);
  }
}

int main(void) {
  printu("back trace the user app in the following:\n");
  deep = 0;
  f1();
  exit(0);
  return 0;
}