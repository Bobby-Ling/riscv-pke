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

typedef struct frame_info_t {
  /*  0  */ uint64 saved_fp;  // 原fp
  /*  8  */ uint64 return_addr; // ra
}frame_info;

typedef struct stack_trace_t {
  uint64 return_addrs[64];
  int depth;
} stack_trace;


int get_sym_index(uint64 text_addr) {
  for (int i = 0; i < sym_info.sym_count; i++) {
    uint64 start_addr = sym_info.symbols[i].st_value;
    uint64 end_addr = start_addr + sym_info.symbols[i].st_size;

    if (text_addr >= start_addr && text_addr < end_addr) {
      return i; // 找到对应的索引
    }
  }

  return -1; // 未找到
}

int get_stack_trace(uint64 level, stack_trace *trace) {
  if (!trace) return -1;

  // print_backtrace的fp
  // frame_info *frame = (frame_info *)(current->trapframe->regs.s0);
  frame_info *frame = (frame_info *)((*((uint64 *)current->trapframe->regs.s0 - 1)) - sizeof(frame_info));
  trace->depth = 0;

  while (frame && trace->depth < level) {
    trace->return_addrs[trace->depth] = frame->return_addr;
    sprint("get_stack_trace: %lx\n", frame->return_addr);
    trace->depth++;

    if (frame->saved_fp == 0) {
      sprint("到达栈底\n");
      break;
    }
    frame = (frame_info *)(frame->saved_fp - sizeof(frame_info)); // 下一个栈帧
    sprint("frame: %lx frame->saved_fp: %lx\n", frame, frame->saved_fp);
  }

  return 0; // 成功
}

// int get_stack_trace(uint64 level, stack_trace *trace) {
//   if (!trace) return -1;

//   // 获取当前fp
//   uint64 fp = current->trapframe->regs.s0;
//   trace->depth = 0;

//   while (fp && trace->depth < level) {
//       // 返回地址在fp-8的位置
//     trace->return_addrs[trace->depth] = *((uint64 *)(fp - 8));
//     sprint("get_stack_trace: %lx\n", trace->return_addrs[trace->depth]);
//     trace->depth++;

//     // 保存的上一个fp在fp-16的位置
//     fp = *((uint64 *)(fp - 16));
//     if (fp == 0) break; // 到达栈底
//     sprint("fp: %lx saved_fp: %lx\n", fp, *((uint64 *)(fp - 16)));
//   }

//   return 0;
// }

// ssize_t get_stack_trace(uint64 level, stack_trace *trace) {
//   trace->depth = 0;

//   uint64 fp = current->trapframe->regs.s0;
//   fp = *((uint64 *)(fp - 8));
//   for (int i = 0;i < level;i++) {
//     if (fp) {
//       trace->return_addrs[trace->depth] = *(uint64 *)(fp - 8);
//       trace->depth++;
//     }
//     fp = *((uint64 *)(fp - 16));
//   }
//   return 0;
// }

void print_stack_trace(const stack_trace *trace) {
  if (!trace || trace->depth == 0) {
    sprint("No stack trace available.\n");
    return;
  }

  sprint("Stack Trace (Depth: %d):\n", trace->depth);
  for (int i = 0; i < trace->depth; i++) {
    uint64 addr = trace->return_addrs[i];
    if (addr == 0x0) {
      return;
    }

    int sym_index = get_sym_index(addr);

    if (sym_index >= 0) {
      const char *func_name = sym_info.sym_names[sym_index];
      sprint("  #%d: 0x%lx -> %s\n", i, addr, func_name);
    }
    else {
      sprint("  #%d: 0x%lx -> [Unknown Function]\n", i, addr);
    }
  }
}


void print_stack_trace_simple(const stack_trace *trace) {
  if (!trace || trace->depth == 0) {
    sprint("No stack trace available.\n");
    return;
  }

  // sprint("Stack Trace (Depth: %d):\n", trace->depth);
  for (int i = 0; i < trace->depth; i++) {
    uint64 addr = trace->return_addrs[i];
    if (addr == 0x0) {
      return;
    }

    int sym_index = get_sym_index(addr);

    if (sym_index >= 0) {
      const char *func_name = sym_info.sym_names[sym_index];
      sprint("%s\n", func_name);
    }
    else {
      // sprint("  #%d: 0x%lx -> [Unknown Function]\n", i, addr);
    }
  }
}


//
// implement the SYS_user_print_backtrace syscall
//
ssize_t sys_user_print_backtrace(uint64 level) {
  // 这是S模式
  /*
  USER_TRAP_FRAME 0x81300000
  USER_KSTACK     0x81200000
  USER_STACK      0x81100000    init sp(栈顶指针, 高地址, 向下增长)
                                栈
                                bss
                                data
                                text
  Init            0x81000000

  int a[65];
  a[64]: 00000000810fffe8
  a[0]:  00000000810ffee8
  printu("%lx %lx\n", &a[0], &a[64]);
  00000000810ffee8 00000000810fffe8
  int b = 1; // 00000000810ffee4
  int c = b; // 00000000810ffee0
  printu("%lx %lx\n", &b, &c);
  00000000810ffee4 00000000810ffee0


  readelf -s ./obj/app_print_backtrace

  Symbol table '.symtab' contains 32 entries:
    Num:    Value          Size Type    Bind   Vis      Ndx Name
      ...
      18: 0000000081000190   514 FUNC    GLOBAL DEFAULT    1 vsnprintf
      19: 0000000081000016    20 FUNC    GLOBAL DEFAULT    1 f7
      20: 000000008100003e    20 FUNC    GLOBAL DEFAULT    1 f5
      21: 000000008100016a    38 FUNC    GLOBAL DEFAULT    1 print_backtrace
      22: 0000000081000066    20 FUNC    GLOBAL DEFAULT    1 f3
      23: 000000008100007a    20 FUNC    GLOBAL DEFAULT    1 f2
      24: 00000000810000a2    40 FUNC    GLOBAL DEFAULT    1 main
      25: 00000000810000ca    24 FUNC    GLOBAL DEFAULT    1 do_user_call
      26: 0000000081000000    22 FUNC    GLOBAL DEFAULT    1 f8
      27: 0000000081000052    20 FUNC    GLOBAL DEFAULT    1 f4
      28: 000000008100008e    20 FUNC    GLOBAL DEFAULT    1 f1
      29: 00000000810000e2    98 FUNC    GLOBAL DEFAULT    1 printu
      30: 0000000081000144    38 FUNC    GLOBAL DEFAULT    1 exit
      31: 000000008100002a    20 FUNC    GLOBAL DEFAULT    1 f6
  */

  // 当前状态是在do_user_call的开始
  riscv_regs *regs = &(current->trapframe->regs);
  // fp: s0;
  // sprint("sys_user_print_backtrace: %lx %lx %lx\n", regs->s0, regs->sp, regs->ra);
  // 00000000810fff60 00000000810fff40 0000000081000188

  /*
  +-----------------+ <-- FP (s0)
  | 返回地址ra       |
  +-----------------+
  | 保存的旧fp       |
  +-----------------+
  | 局部变量         |
  +-----------------+
  | ...             |
  +-----------------+ <-- SP

  0000000081000000 <f8>:
    81000000:	1141       	addi	sp,sp,-16   分配两个uint64
    81000002:	e406       	sd	ra,8(sp)      保存ra(81000022???)
    81000004:	e022       	sd	s0,0(sp)      保存fp
    81000006:	0800       	addi	s0,sp,16    分配frame
    81000008:	451d       	li	a0,7
    8100000a:	160000ef 	  jal	ra,8100016a   <print_backtrace>
                                            跳转并保存8100000e至ra
    8100000e:	60a2       	ld	ra,8(sp)      恢复ra
    81000010:	6402       	ld	s0,0(sp)      恢复s0
    81000012:	0141       	addi	sp,sp,16    释放栈帧
    81000014:	8082       	ret               return至ra

    高地址
    +------------------+
    |   f7局部变量      |
    +==================+ <--- f8的fp，指向f7的局部变量
    |  saved ra to f7  |
    +------------------+
    |  saved old fp    |
    |    无局部变量     |
    +==================+ <--- f8的sp，print_backtrace的fp
    |  saved ra to f8  |
    +------------------+
    |  saved old fp    |
    |    无局部变量     |
    +==================+ <--- print_backtrace的sp，do_user_call的fp，就是(current->trapframe->regs.s0)
    | 注意do_user_call这里存没有ra|
    +------------------+
    |  saved old fp    |
    +------------------+
    | do_user_call局部变量 |
    +==================+ <--- do_user_call的sp
    低地址
  */
  frame_info *frame = (frame_info *)((*((uint64 *)regs->s0 - 1)) - sizeof(frame_info));
  sprint("saved_fp: %lx, ra: %lx\n", frame->saved_fp, frame->return_addr);
  // saved_fp: 00000000810fff80, ra: 000000008100000e(在f8中)

  /*
  核心思路:
  1.	陷入内核后通过current获得fp(do_user_call的fp)（还可以获得print_backtrace中的ra）；
  2.	do_user_call的fp-8保存的是返回至f8的地址；
  3.	可以通过fp一级级得到saved fp，继而得到各层ra；
  4.	得到ra后可以根据符号表中函数的起始地址确定ra所属的函数地址；
  5.	根据函数地址得到符号名称字符串。
  */

  stack_trace stack_tr;
  get_stack_trace(level, &stack_tr);
  // print_stack_trace(&stack_tr);
  /*
  Stack Trace (Depth: 7):
    #0: 0x000000008100000e -> f8
    #1: 0x0000000081000022 -> f7
    #2: 0x0000000081000036 -> f6
    #3: 0x000000008100004a -> f5
    #4: 0x000000008100005e -> f4
    #5: 0x0000000081000072 -> f3
    #6: 0x0000000081000086 -> f2
  */
  print_stack_trace_simple(&stack_tr);
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
    case SYS_user_print_backtrace:
      return sys_user_print_backtrace(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
