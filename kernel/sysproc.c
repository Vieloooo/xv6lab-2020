#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  backtrace();
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
uint64 sys_sigalarm(void){
  int interval;
  uint64 handler;

  if (argint(0, &interval) < 0)
    return -1;
  if (argaddr(1, &handler) < 0)
    return -1;
  
  struct proc *p = myproc();
    p->tickNum = interval;
    p->tickHandler = handler;
  return 0;
  
}
uint sys_sigreturn(void){
  struct proc *p= myproc();
  //recover from handler 
  p->trapframe->epc = p->backup.epc;
  p->trapframe->ra = p->backup.ra;
  p->trapframe->sp = p->backup.sp;
  p->trapframe->gp = p->backup.gp;
  p->trapframe->tp = p->backup.tp;
  
  p->trapframe->s0 = p->backup.s0;
  p->trapframe->s1 = p->backup.s1;
  p->trapframe->s2 = p->backup.s2;
  p->trapframe->s3 = p->backup.s3;
  p->trapframe->s4 = p->backup.s4;
  p->trapframe->s5 = p->backup.s5;
  p->trapframe->s6 = p->backup.s6;
  p->trapframe->s7 = p->backup.s7;
  p->trapframe->s8 = p->backup.s8;
  p->trapframe->s9 = p->backup.s9;
  p->trapframe->s10 = p->backup.s10;
  p->trapframe->s11 = p->backup.s11;
  
  p->trapframe->t0 = p->backup.t0;
  p->trapframe->t1 = p->backup.t1;
  p->trapframe->t2 = p->backup.t2;
  p->trapframe->t3 = p->backup.t3;
  p->trapframe->t4 = p->backup.t4;
  p->trapframe->t5 = p->backup.t5;
  p->trapframe->t6 = p->backup.t6;

  p->trapframe->a0 = p->backup.a0;
  p->trapframe->a1 = p->backup.a1;
  p->trapframe->a2 = p->backup.a2;
  p->trapframe->a3 = p->backup.a3;
  p->trapframe->a4 = p->backup.a4;
  p->trapframe->a5 = p->backup.a5;
  p->trapframe->a6 = p->backup.a6;
  p->trapframe->a7 = p->backup.a6;
  p->handling = 0;
  return 0;
}
