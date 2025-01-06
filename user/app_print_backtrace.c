/*
 * Below is the given application for lab1_challenge1_backtrace.
 * This app prints all functions before calling print_backtrace().
 */

#include "user_lib.h"
#include "util/types.h"

void f8() { print_backtrace(100); }
void f7() { int a = 1;f8();int b = a; }
void f6() { int a = 1;f7(); }
void f5() { f6();int b = 1; }
void f4() { f5(); }
void f3() { int a = 1;f4();int b = a;int c = 1; }
void f2() { f3();int b = 1; }
void f1() { int a = 1;f2(); }

void recursive(int level) {
  if (level == 63) {
    print_backtrace(level);
    return;
  }
  recursive(++level);
}

int main(void) {
  printu("back trace the user app in the following:\n");
  // f1();
  recursive(0);
  /*
  int a[65];
  // a[64]: 00000000810fffe8
  // a[0]:  00000000810ffee8
  printu("%lx %lx\n", &a[0], &a[64]);
  // 00000000810ffee8 00000000810fffe8
  int b = 1; // 00000000810ffee4
  int c = b; // 00000000810ffee0
  printu("%lx %lx\n", &b, &c);
  // 00000000810ffee4 00000000810ffee0
  */
  exit(0);
  return 0;
}
