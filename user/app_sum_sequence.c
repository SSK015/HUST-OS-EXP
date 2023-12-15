 /*
 * The application of lab2_challenge1_pagefault.
 * Based on application of lab2_3.
 */

#include "user_lib.h"
#include "util/types.h"

int main(void) {
  int *array = (int *)naive_malloc(1024);
  array[1024] = 1;  // not available
  exit(0);
}
