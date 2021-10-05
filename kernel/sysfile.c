//
// File-system system calls.
// Mostly argument checking, since we don't trust
// user code, and calls into file.c and fs.c.
//

#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"
#include "fcntl.h"

// Fetch the nth word-sized system call argument as a file descriptor
// and return both the descriptor and the corresponding struct file.
static int
argfd(int n, int *pfd, struct file **pf)
{
  int fd;
  struct file *f;

  if(argint(n, &fd) < 0)
    return -1;
  if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
    return -1;
  if(pfd)
    *pfd = fd;
  if(pf)
    *pf = f;
  return 0;
}

// Allocate a file descriptor for the given file.
// Takes over file reference from caller on success.
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *p = myproc();

  for(fd = 0; fd < NOFILE; fd++){
    if(p->ofile[fd] == 0){
      p->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}

uint64
sys_dup(void)
{
  struct file *f;
  int fd;

  if(argfd(0, 0, &f) < 0)
    return -1;
  if((fd=fdalloc(f)) < 0)
    return -1;
  filedup(f);
  return fd;
}

uint64
sys_read(void)
{
  struct file *f;
  int n;
  uint64 p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
    return -1;
  return fileread(f, p, n);
}

uint64
sys_write(void)
{
  struct file *f;
  int n;
  uint64 p;

  if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argaddr(1, &p) < 0)
    return -1;

  return filewrite(f, p, n);
}

uint64
sys_close(void)
{
  int fd;
  struct file *f;

  if(argfd(0, &fd, &f) < 0)
    return -1;
  myproc()->ofile[fd] = 0;
  fileclose(f);
  return 0;
}

uint64
sys_fstat(void)
{
  struct file *f;
  uint64 st; // user pointer to struct stat

  if(argfd(0, 0, &f) < 0 || argaddr(1, &st) < 0)
    return -1;
  return filestat(f, st);
}

// Create the path new as a link to the same inode as old.
uint64
sys_link(void)
{
  char name[DIRSIZ], new[MAXPATH], old[MAXPATH];
  struct inode *dp, *ip;

  if(argstr(0, old, MAXPATH) < 0 || argstr(1, new, MAXPATH) < 0)
    return -1;

  begin_op();
  if((ip = namei(old)) == 0){
    end_op();
    return -1;
  }

  ilock(ip);
  if(ip->type == T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if((dp = nameiparent(new, name)) == 0)
    goto bad;
  ilock(dp);
  if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
    iunlockput(dp);
    goto bad;
  }
  iunlockput(dp);
  iput(ip);

  end_op();

  return 0;

bad:
  ilock(ip);
  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);
  end_op();
  return -1;
}

// Is the directory dp empty except for "." and ".." ?
static int
isdirempty(struct inode *dp)
{
  int off;
  struct dirent de;

  for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
    if(readi(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
      panic("isdirempty: readi");
    if(de.inum != 0)
      return 0;
  }
  return 1;
}

uint64
sys_unlink(void)
{
  struct inode *ip, *dp;
  struct dirent de;
  char name[DIRSIZ], path[MAXPATH];
  uint off;

  if(argstr(0, path, MAXPATH) < 0)
    return -1;

  begin_op();
  if((dp = nameiparent(path, name)) == 0){
    end_op();
    return -1;
  }

  ilock(dp);

  // Cannot unlink "." or "..".
  if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
    goto bad;

  if((ip = dirlookup(dp, name, &off)) == 0)
    goto bad;
  ilock(ip);

  if(ip->nlink < 1)
    panic("unlink: nlink < 1");
  if(ip->type == T_DIR && !isdirempty(ip)){
    iunlockput(ip);
    goto bad;
  }

  memset(&de, 0, sizeof(de));
  if(writei(dp, 0, (uint64)&de, off, sizeof(de)) != sizeof(de))
    panic("unlink: writei");
  if(ip->type == T_DIR){
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  end_op();

  return 0;

bad:
  iunlockput(dp);
  end_op();
  return -1;
}

static struct inode*
create(char *path, short type, short major, short minor)
{
  struct inode *ip, *dp;
  char name[DIRSIZ];

  if((dp = nameiparent(path, name)) == 0)
    return 0;

  ilock(dp);

  if((ip = dirlookup(dp, name, 0)) != 0){
    iunlockput(dp);
    ilock(ip);
    if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE))
      return ip;
    iunlockput(ip);
    return 0;
  }

  if((ip = ialloc(dp->dev, type)) == 0)
    panic("create: ialloc");

  ilock(ip);
  ip->major = major;
  ip->minor = minor;
  ip->nlink = 1;
  iupdate(ip);

  if(type == T_DIR){  // Create . and .. entries.
    dp->nlink++;  // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("create dots");
  }

  if(dirlink(dp, name, ip->inum) < 0)
    panic("create: dirlink");

  iunlockput(dp);

  return ip;
}

uint64
sys_open(void)
{
  char path[MAXPATH];
  int fd, omode;
  struct file *f;
  struct inode *ip;
  int n;

  if((n = argstr(0, path, MAXPATH)) < 0 || argint(1, &omode) < 0)
    return -1;

  begin_op();

  if(omode & O_CREATE){
    ip = create(path, T_FILE, 0, 0);
    if(ip == 0){
      end_op();
      return -1;
    }
  } else {
    if((ip = namei(path)) == 0){
      end_op();
      return -1;
    }
    ilock(ip);
    if(ip->type == T_DIR && omode != O_RDONLY){
      iunlockput(ip);
      end_op();
      return -1;
    }
  }

  if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
    iunlockput(ip);
    end_op();
    return -1;
  }

  if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
    if(f)
      fileclose(f);
    iunlockput(ip);
    end_op();
    return -1;
  }

  if(ip->type == T_DEVICE){
    f->type = FD_DEVICE;
    f->major = ip->major;
  } else {
    f->type = FD_INODE;
    f->off = 0;
  }
  f->ip = ip;
  f->readable = !(omode & O_WRONLY);
  f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

  if((omode & O_TRUNC) && ip->type == T_FILE){
    itrunc(ip);
  }

  iunlock(ip);
  end_op();

  return fd;
}

uint64
sys_mkdir(void)
{
  char path[MAXPATH];
  struct inode *ip;

  begin_op();
  if(argstr(0, path, MAXPATH) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

uint64
sys_mknod(void)
{
  struct inode *ip;
  char path[MAXPATH];
  int major, minor;

  begin_op();
  if((argstr(0, path, MAXPATH)) < 0 ||
     argint(1, &major) < 0 ||
     argint(2, &minor) < 0 ||
     (ip = create(path, T_DEVICE, major, minor)) == 0){
    end_op();
    return -1;
  }
  iunlockput(ip);
  end_op();
  return 0;
}

uint64
sys_chdir(void)
{
  char path[MAXPATH];
  struct inode *ip;
  struct proc *p = myproc();
  
  begin_op();
  if(argstr(0, path, MAXPATH) < 0 || (ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);
  if(ip->type != T_DIR){
    iunlockput(ip);
    end_op();
    return -1;
  }
  iunlock(ip);
  iput(p->cwd);
  end_op();
  p->cwd = ip;
  return 0;
}

uint64
sys_exec(void)
{
  char path[MAXPATH], *argv[MAXARG];
  int i;
  uint64 uargv, uarg;

  if(argstr(0, path, MAXPATH) < 0 || argaddr(1, &uargv) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
  for(i=0;; i++){
    if(i >= NELEM(argv)){
      goto bad;
    }
    if(fetchaddr(uargv+sizeof(uint64)*i, (uint64*)&uarg) < 0){
      goto bad;
    }
    if(uarg == 0){
      argv[i] = 0;
      break;
    }
    argv[i] = kalloc();
    if(argv[i] == 0)
      goto bad;
    if(fetchstr(uarg, argv[i], PGSIZE) < 0)
      goto bad;
  }

  int ret = exec(path, argv);

  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);

  return ret;

 bad:
  for(i = 0; i < NELEM(argv) && argv[i] != 0; i++)
    kfree(argv[i]);
  return -1;
}

uint64
sys_pipe(void)
{
  uint64 fdarray; // user pointer to array of two integers
  struct file *rf, *wf;
  int fd0, fd1;
  struct proc *p = myproc();

  if(argaddr(0, &fdarray) < 0)
    return -1;
  if(pipealloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
    if(fd0 >= 0)
      p->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  if(copyout(p->pagetable, fdarray, (char*)&fd0, sizeof(fd0)) < 0 ||
     copyout(p->pagetable, fdarray+sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0){
    p->ofile[fd0] = 0;
    p->ofile[fd1] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  return 0;
}


uint64
sys_mmap(void){
  printf("call mmap succeed\n");
  //read arguement 
  int size, prot, flags,fd;
  if(argint(1, &size) < 0)
    return -1;
  if(argint(2, &prot) < 0)
    return -1;
  if(argint(3, &flags) < 0)
    return -1;
  if(argint(4,&fd) < 0)
    return -1;
  printf("size:%d, prot%d,flag%d,fd%d\n",size,prot,flags,fd);

  //save args to pcb, and add ref 
  struct proc *p = myproc();
  if (fd < 0 || fd > NOFILE){
    printf("mmap fd out of range\n");
    return -1;
  }
  struct file * f = p->ofile[fd];
  printf("current vma entry %d\n",p->vp);
  if (f==0){
    return -1;
  }
  //need lock? 
  filedup(f);
  //get start and end 
  uint64 start = PGROUNDUP(p->sz);
  if (start + size >= MAXVA){
    printf("vma too big\n");
    return -1;
  }
  //get flags 
  int pteFlag = PTE_U | PTE_M;
  if (prot & PROT_WRITE) {
    if(f->writable==0 && (flags & MAP_PRIVATE)==0){
      return -1; 
    } 
    pteFlag |= PTE_W;
  }
  if (prot & PROT_READ) {
    if(f->readable==0){
      return -1; 
    } 
    pteFlag |= PTE_R;
  }
  if (prot & PROT_EXEC){
    pteFlag |= PTE_X;
  }

  //alloc a vma 
  struct vma *v = vmaAlloc();
  printf("alloc a vma ok\n");
  if (v==0){
    printf("no vma block remain\n");
    return -1;
  }
  //printf("lock dddd      \n");
  v->pid = p->pid;
  v->rootva = start;
  v->start = start;
  v->end = PGROUNDUP(start + size);
  acquire(&v->lock);
  v->length = v->end - v->start;
  release(&v->lock);
  v->prot = pteFlag;
  v->flags = flags;
  v->f = f;
  v->next = 0;
  
  //add new vma to pcb 
  struct vma * vp = p->vp;
  if (vp==0){
    p->vp = v;
    printf("l1 vma %d, l2 vma %d\n",p->vp,p->vp->next);
  }else{
    while (vp->next!=0){
      //printf("go down\n");
      vp = vp->next;
    }
    vp->next = v;
  }
  
  //lazy alloc mem in user, cause usert trap 13/15 
  p->sz = v->end;
  printf("map from %p to %p\n ",v->start,v->end);
  
  return v->start;
}
// Write to file f.
// addr is a user virtual address.
int
writebackPage(struct file *f, uint64 addr, int n,int myoff)
{
  int r, ret = 0;

  if(f->writable == 0)
    return -1;

   if(f->type == FD_INODE){
     printf("this is file\n");
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      printf("write %d bytes from va %p at offset %d\n",n1,addr+i,myoff);
      if ((r = writei(f->ip, 1, addr + i, myoff, n1)) > 0)
        myoff += r;
      iunlock(f->ip);
      end_op();
      printf("end op\n");
      if(r != n1){
        // error from writei
        break;
      }
      i += r;
    }
    ret = (i == n ? n : -1);
  } else {
    return -1;
  }
  return ret;
}

uint64
sys_munmap(void){
  
  uint64 va;
  int length;
  if(argaddr(0, &va) < 0 || argint(1, &length) < 0){
    return -1;
  }
  if (length ==0){
    return 0;
  }
  struct proc *p = myproc();
  struct vma *v = p->vp;
  struct vma *pv =0;
  uint64 vaEnd ;
  while(v != 0){
    printf("v %p, next %p \n", v,v->next);
    if(va >= v->start && va < v->end){
      goto good ;
    }
    pv = v;
    v = v->next;
  }
  printf("addr %p has not been mapped\n",va);
  return -1;
  //before going good, v.lock must be held
good:
  
  vaEnd = PGROUNDUP(va +length);
  va = PGROUNDDOWN(va);
  printf("unmap from %p to %p\n",va,vaEnd);
  if (va != v->start && vaEnd!=v->end){
    printf("unmap in the middle of a whole vma\n");
    return -1;
  }
  
  if(va == v->start){
    if (vaEnd == v->end){
      //free all mapping 
      //free this vma to pool 
      fileclose(v->f);
      v->start = 0;
      v->end = 0;
      pv->next = v->next;
      
    }else {
      //free a part of the front
      v->start = vaEnd;
    }
  }else{
    //free a part of the end 
    v->end = va;
  }
  acquire(&v->lock);
  v->length = v->end -v->start;
  release(&v->lock);
  printf("after unmap %p,%p\n",v->start,v->end);
  
  //write back [va,vaEnd] to fs
  pte_t * pte;
  if( (v->prot & PTE_W) && (v->flags & MAP_SHARED)  ){
    for (uint64 addr = va; addr <vaEnd; addr+= PGSIZE){
       pte = walk(p->pagetable,addr,0);
      if (pte !=0 && (*pte & PTE_D)){
        writebackPage(v->f,addr,PGSIZE,addr - v->rootva);
      }
    }
  }
  //free unmap pages 
  uvmunmap(p->pagetable,va,(vaEnd -va)/PGSIZE,1);
  
  return 0;
}