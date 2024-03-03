#include "user_lib.h"
#include "util/types.h"
// #include "spike_interface/spike_utils.h"

#define N 5
#define BASE 0

int main(void) {
  void *p[N];

  for (int i = 0; i < N; i++) {
    p[i] = naive_malloc();
    printu("=== user alloc 0 @ vaddr 0x%x\n", p[i]);
    // sprint("kali\n");
    // printu("I am a %d", 1);
    int *pi = p[i];
    *pi = BASE + i;
    // printu("=== user alloc 0 @ vaddr 0x%x\n", p[i]);
  }

  for (int i = 0; i < N; i++) {
    int *pi = p[i];
    printu("=== user0: %d\n", *pi);
    naive_free(p[i]);
  }

  exit(0);
}