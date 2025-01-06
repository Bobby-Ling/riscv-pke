/*
 * implementing the scheduler
 */

#include "sched.h"
#include "spike_interface/spike_utils.h"

process* ready_queue_head = NULL;

// added @lab3_challenge1_wait
process *block_queue_head = NULL;

//
// insert a process, proc, into the END of ready queue.
//
void insert_to_ready_queue( process* proc ) {
  sprint( "going to insert process %d to ready queue.\n", proc->pid );
  // if the queue is empty in the beginning
  if( ready_queue_head == NULL ){
    proc->status = READY;
    proc->queue_next = NULL;
    ready_queue_head = proc;
    return;
  }

  // ready queue is not empty
  process *p;
  // browse the ready queue to see if proc is already in-queue
  for( p=ready_queue_head; p->queue_next!=NULL; p=p->queue_next )
    if (p == proc) {
      sprint("insert_to_ready_queue: %d already in queue\n", p->pid);
      return;  //already in queue
    }

  // p points to the last element of the ready queue
  if (p == proc) return;
  p->queue_next = proc;
  proc->status = READY;
  proc->queue_next = NULL;

  return;
}

void print_ready_queue() {
  sprint("Ready Queue: [");

  // 检查队列是否为空
  if (ready_queue_head == NULL) {
    sprint("Empty Queue]\n");
    return;
  }

  // 遍历就绪队列并打印每个进程的信息
  process *p = ready_queue_head;
  while (p != NULL) {
    const char *status_str;
    switch (p->status) {
      case FREE:
        status_str = "FREE";
        break;
      case READY:
        status_str = "READY";
        break;
      case RUNNING:
        status_str = "RUNNING";
        break;
      case BLOCKED:
        status_str = "BLOCKED";
        break;
      case ZOMBIE:
        status_str = "ZOMBIE";
        break;
      default:
        status_str = "UNKNOWN";
        break;
    }

    sprint("{%d: %s} ", p->pid, status_str);
    p = p->queue_next;
  }
  sprint("]\n");
}

//
// choose a proc from the ready queue, and put it to run.
// note: schedule() does not take care of previous current process. If the current
// process is still runnable, you should place it into the ready queue (by calling
// ready_queue_insert), and then call schedule().
//
extern process procs[NPROC];
void schedule() {
  if ( !ready_queue_head ){
    // by default, if there are no ready process, and all processes are in the status of
    // FREE and ZOMBIE, we should shutdown the emulated RISC-V machine.
    int should_shutdown = 1;

    for( int i=0; i<NPROC; i++ )
      if( (procs[i].status != FREE) && (procs[i].status != ZOMBIE) ){
        should_shutdown = 0;
        sprint("ready queue empty, but process %d is not in free/zombie state:%d\n",
          i, procs[i].status );
      }

    if( should_shutdown ){
      sprint( "no more ready processes, system shutdown now.\n" );
      shutdown( 0 );
    }else{
      panic( "Not handled: we should let system wait for unfinished processes.\n" );
    }
  }

  current = ready_queue_head;
  assert( current->status == READY );
  ready_queue_head = ready_queue_head->queue_next;

  current->status = RUNNING;
  sprint( "going to schedule process %d to run.\n", current->pid );
  switch_to( current );
}
