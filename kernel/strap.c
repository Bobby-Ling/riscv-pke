/*
 * Utility functions for trap handling in Supervisor mode.
 */

#include "riscv.h"
#include "process.h"
#include "strap.h"
#include "syscall.h"
#include "pmm.h"
#include "vmm.h"
#include "util/functions.h"

#include "spike_interface/spike_utils.h"
#include "memlayout.h"

//
// handling the syscalls. will call do_syscall() defined in kernel/syscall.c
//
static void handle_syscall(trapframe *tf) {
  // tf->epc points to the address that our computer will jump to after the trap handling.
  // for a syscall, we should return to the NEXT instruction after its handling.
  // in RV64G, each instruction occupies exactly 32 bits (i.e., 4 Bytes)
  tf->epc += 4;

  // TODO (lab1_1): remove the panic call below, and call do_syscall (defined in
  // kernel/syscall.c) to conduct real operations of the kernel side for a syscall.
  // IMPORTANT: return value should be returned to user app, or else, you will encounter
  // problems in later experiments!
  tf->regs.a0 = do_syscall(tf->regs.a0, tf->regs.a1, tf->regs.a2, tf->regs.a3, tf->regs.a4, tf->regs.a5, tf->regs.a6, tf->regs.a7);
  // 实际上在switch_to(current)时a0存放的返回值才实际写入
  // panic( "call do_syscall to accomplish the syscall and lab1_1 here.\n" );
}

//
// global variable that store the recorded "ticks". added @lab1_3
static uint64 g_ticks = 0;
//
// added @lab1_3
//
void handle_mtimer_trap() {
  sprint("Ticks %d\n", g_ticks);
  // TODO (lab1_3): increase g_ticks to record this "tick", and then clear the "SIP"
  // field in sip register.
  // hint: use write_csr to disable the SIP_SSIP bit in sip.
  // panic( "lab1_3: increase g_ticks by one, and clear SIP field in sip register.\n" );
  g_ticks++;
  // handle_timer()中使用write_csr(sip, SIP_SSIP)设置了SIP
  write_csr(sip, read_csr(sip) & ~SIP_SSIP);
}

//
// the page fault handler. added @lab2_3. parameters:
// sepc: the pc when fault happens;
// stval: the virtual address that causes pagefault when being accessed.
//
void handle_user_page_fault(uint64 mcause, uint64 sepc, uint64 stval) {
  sprint("handle_page_fault: %lx\n", stval);
  switch (mcause) {
    // 添加读时的缺页异常
    case CAUSE_LOAD_PAGE_FAULT:
    case CAUSE_STORE_PAGE_FAULT:
      // TODO (lab2_3): implement the operations that solve the page fault to
      // dynamically increase application stack.
      // hint: first allocate a new physical page, and then, maps the new page to the
      // virtual address that causes the page fault.
      // panic( "You need to implement the operations that actually handle the page fault in lab2_3.\n" );
    {
      // sepc: Supervisor Exception Program Counter
      // 保存了异常发生时的程序计数器(PC). 当发生异常时, sepc 保存的是异常发生时的地址, 即指向发生异常的那条指令的地址.
      // stval: Supervisor Trap Value
      // 页面错误的情况下, stval 存储的是触发页面错误的虚拟地址, 很可能非整PGSIZE
      // 分配一个物理页

      /*
      1. 本函数为S模式
      2. alloc_page在何处分配?
        物理内存
      3. 如何判断缺页和越界?
        3.1 目前可以得到哪些信息?
          - stval: 实际访问的VA
          - current->trapframe->regs.sp: 程序的栈顶VA, 默认为USER_STACK_TOP = 0x7ffff000, 初始分配4KB
        3.2 正常时是什么情况?
          - 栈缺页: 比如初始4KB, 访问4097, 则合法栈空间为: [SP, USER_STACK_TOP - n * PGSIZE]
          - 堆缺页: 页表有项, 但是无效
        3.3 缺页时如何决策?
          1. VA栈合法
              有效缺页
          2. VA栈非法
            2.1 页表存在项(只可能无效项)
              有效缺页
            2.2 页表不存在项
              权限错误缺页
      */
      // if (current->trapframe->regs.sp < stval) panic("栈访问越界");

      uint64 va = stval;
      uint64 sp = current->trapframe->regs.sp;
      pagetable_t page_table = (pagetable_t)current->pagetable;

      // 1. 检查VA是否合法
      // sp是当前栈顶(最小值), 访问地址必须在[sp, USER_STACK_TOP]范围内
      if (!(sp <= va && va <= USER_STACK_TOP)) {
      // 2. 是否有页表项
        pte_t *pte = page_walk(page_table, va, 0);
        if (pte == 0 || (*pte & PTE_V) == 0) {
          panic("this address is not available!\n");
        }
        else {
          // 如果页表项存在, 则此时一定是无效的(Assert)
          print_page_table(page_table, 2, 0);
          sprint("页表项存在: %lx %lx %d\n", *pte, va, lookup_pa(page_table, va));
          assert((*pte & PTE_V) == 0);
        }
      }
      else {
        sprint("合法栈空间内: %lx %lx %lx\n", sp, va, USER_STACK_TOP);
      }

      // 3. 是有效缺页, 分配新页面
      uint64 new_page_pa = (uint64)alloc_page();
      user_vm_map(
        (pagetable_t)current->pagetable,
        ROUNDDOWN(stval, PGSIZE),
        PGSIZE,
        new_page_pa,
        prot_to_type(PROT_READ | PROT_WRITE, 1)
      );
    }
      break;
    default:
      sprint("unknown page fault.\n");
      break;
  }
}

//
// kernel/smode_trap.S will pass control to smode_trap_handler, when a trap happens
// in S-mode.
// 在 S 模式中处理陷入后, 安全地返回用户态继续执行
//
void smode_trap_handler(void) {
  // 当前是S模式, 检查的是Previous mode是否来自用户模式
  // make sure we are in User mode before entering the trap handling.
  // we will consider other previous case in lab1_3 (interrupt).
  if ((read_csr(sstatus) & SSTATUS_SPP) != 0) panic("usertrap: not from user mode");

  assert(current);
  // save user process counter.
  // 保存用户态上下文
  current->trapframe->epc = read_csr(sepc);

  // if the cause of trap is syscall from user application.
  // read_csr() and CAUSE_USER_ECALL are macros defined in kernel/riscv.h
  uint64 cause = read_csr(scause);

  // use switch-case instead of if-else, as there are many cases since lab2_3.
  switch (cause) {
    case CAUSE_USER_ECALL:
      handle_syscall(current->trapframe);
      break;
    case CAUSE_MTIMER_S_TRAP:
      handle_mtimer_trap();
      break;
    case CAUSE_STORE_PAGE_FAULT:
    case CAUSE_LOAD_PAGE_FAULT:
      // the address of missing page is stored in stval
      // call handle_user_page_fault to process page faults
      handle_user_page_fault(cause, read_csr(sepc), read_csr(stval));
      break;
    default:
      sprint("smode_trap_handler(): unexpected scause %p\n", read_csr(scause));
      sprint("            sepc=%p stval=%p\n", read_csr(sepc), read_csr(stval));
      panic( "unexpected exception happened.\n" );
      break;
  }

  // continue (come back to) the execution of current process.
  // 实际上在这里面变更才实际写入
  switch_to(current);
}
