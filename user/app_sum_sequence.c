/*
 * The application of lab2_challenge1_pagefault.
 * Based on application of lab2_3.
 */

#include "user_lib.h"
#include "util/types.h"

//
// compute the summation of an arithmetic sequence. for a given "n", compute
// result = n + (n-1) + (n-2) + ... + 0
// sum_sequence() calls itself recursively till 0. The recursive call, however,
// may consume more memory (from stack) than a physical 4KB page, leading to a page fault.
// PKE kernel needs to improved to handle such page fault by expanding the stack.
//
uint64 sum_sequence(uint64 n, int *p) {
  if (n == 0)
    return 0;
  else
    return *p=sum_sequence( n-1, p+1 ) + n;
}

int main(void) {
  // FIRST, we need a large enough "n" to trigger pagefaults in the user stack
  uint64 n = 1024;

  // alloc a page size array(int) to store the result of every step
  // the max limit of the number is 4kB/4 = 1024

  // SECOND, we use array out of bound to trigger pagefaults in an invalid address
  // 从USER_FREE_ADDRESS_START = 4MB开始分配一个4KB
  printu("--------------测试基础功能--------------\n");
  /*
  两种缺页异常
  1. 函数递归调用(栈不够用): 可以直接新分配一个栈(VA递减)
  2. 访问越界: 要能够判断
  */
  int *ans = (int *)naive_malloc();
  // printu("Summation of an arithmetic sequence from 0 to %ld is: %ld \n", n, sum_sequence(n+1, ans) );

  printu("--------------测试全部功能--------------\n");
  char *mem = (char *)naive_malloc();
  mem[0] = 'a';
  mem[4095] = 'b';
  printu("ans: %lx, mem: %lx\n", ans, mem);
  naive_free(mem);
  // printu("mem[0]: %c, mem[4095]: %c, mem[4096]: %c\n", mem[0], mem[4095], mem[4096]);
  printu("mem[0]: %c, mem[4095]: %c\n", mem[0], mem[4095]);



  exit(0);
}
