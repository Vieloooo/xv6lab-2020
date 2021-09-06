#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"
uint64
sys_exit(void)
{
  int n;
  if (argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0; // not reached
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
  if (argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
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

  if (argint(0, &pid) < 0)
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
// test trace
uint64
sys_trace(void)
{
  int n;
  if (argint(0, &n) < 0)
  {
    return -1;
  }
  struct proc *p = myproc();
  printf("pid: %d origin mask is %d\n", p->pid, p->mask);
  p->mask = n;
  printf("add mask:%d to the proc: %d\n", n, p->pid);
  return 0;
}
uint64
sys_sysinfo(void)
{
  //printf("sysinfo ok\n");
  //malloc a sysinfo struct
  struct sysinfo myinfo;
  myinfo.nproc = getUsedProcNumber();
  myinfo.freemem = getFreeSize();
  //printf("free bytes:%d free page%d\n", myinfo.freemem, myinfo.nproc);
  //copy myinfo out from kernel
  struct proc *p = myproc();
  uint64 st;
  //get the user destination addr from arg
  if (argaddr(0, &st) < 0)
  {
    return -1;
  }
  printf("va: %d\n", st);
  if (copyout(p->pagetable, st, (char *)&myinfo, sizeof(myinfo)) < 0)
  {
    return -1;
  }
  return 0;
}
uint64 sys_ps(void)
{
  listProc();
  return 0;
}