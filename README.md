# Mit6.S081-XV6lab-2020
## Personal repo to learn MIT 6.S081(6.828) xv6.
## Here are some notes below:
1. util PASS 
    1. fd redirect is the key part in primes. 
3. syscall PASS (in branch new_syscall)
    1. impliment ps syscall in branch new_syscall
5. pgtbl NOT FINISH(I do not want to fix endless bugs anymore)
    1. proc.sz is the memory usage for each process (not include trampline and trapframe)
    2. in other word, proc.sz means the biggest user's vitural address -1 that process is currently using except for trapframe and tramplines 
    3. trapframe use 2 pages under VAMAX
    4. test sbrk(4096) for 3 times, output below:
    ```
        pid: 3 mem add: 4096, new mem: 16384
        page table 0x0000000087e6a000
        ..0: pte 0x0000000021f99801 pa 0x0000000087e66000
        ....0: pte 0x0000000021f99401 pa 0x0000000087e65000
        ......0: pte 0x0000000021f99c5f pa 0x0000000087e67000
        ......1: pte 0x0000000021f9900f pa 0x0000000087e64000
        ......2: pte 0x0000000021f98cdf pa 0x0000000087e63000
        ......3: pte 0x0000000021f9ac1f pa 0x0000000087e6b000
        ..255: pte 0x0000000021f9a401 pa 0x0000000087e69000
        ....511: pte 0x0000000021f9a001 pa 0x0000000087e68000
        ......510: pte 0x0000000021fb40c7 pa 0x0000000087ed0000
        ......511: pte 0x0000000020001c4b pa 0x0000000080007000
        pid: 3 mem add: 4096, new mem: 20480
        page table 0x0000000087e6a000
        ..0: pte 0x0000000021f99801 pa 0x0000000087e66000
        ....0: pte 0x0000000021f99401 pa 0x0000000087e65000
        ......0: pte 0x0000000021f99c5f pa 0x0000000087e67000
        ......1: pte 0x0000000021f9900f pa 0x0000000087e64000
        ......2: pte 0x0000000021f98cdf pa 0x0000000087e63000
        ......3: pte 0x0000000021f9ac1f pa 0x0000000087e6b000
        ......4: pte 0x0000000021fdd41f pa 0x0000000087f75000
        ..255: pte 0x0000000021f9a401 pa 0x0000000087e69000
        ....511: pte 0x0000000021f9a001 pa 0x0000000087e68000
        ......510: pte 0x0000000021fb40c7 pa 0x0000000087ed0000
        ......511: pte 0x0000000020001c4b pa 0x0000000080007000
        pid: 3 mem add: 4096, new mem: 24576
        page table 0x0000000087e6a000
        ..0: pte 0x0000000021f99801 pa 0x0000000087e66000
        ....0: pte 0x0000000021f99401 pa 0x0000000087e65000
        ......0: pte 0x0000000021f99c5f pa 0x0000000087e67000
        ......1: pte 0x0000000021f9900f pa 0x0000000087e64000
        ......2: pte 0x0000000021f98cdf pa 0x0000000087e63000
        ......3: pte 0x0000000021f9ac1f pa 0x0000000087e6b000
        ......4: pte 0x0000000021fdd41f pa 0x0000000087f75000
        ......5: pte 0x0000000021fdd01f pa 0x0000000087f74000
        ..255: pte 0x0000000021f9a401 pa 0x0000000087e69000
        ....511: pte 0x0000000021f9a001 pa 0x0000000087e68000
        ......510: pte 0x0000000021fb40c7 pa 0x0000000087ed0000
        ......511: pte 0x0000000020001c4b pa 0x0000000080007000
    ```
4. trap PASS
    1. what if handler wanna to pass arguements in the signal handler function 
    2. use spinlock to lock handle function(or atomic action) instead of a normal integer 
5. lazy PASS 
    1. mappage() will automatically allocate space for undeclared virtual address, so we only need to allocate the space for the physical memory to be mapped. 
    2. 2 kinds of pte
        1. 0 pte
        2. not valid pte. 
    3. In the free procedure at the end of process's lifetime:
        1. unmap the pte and the physical address 
        2. free pa 
        3. set pte to 0 
        4. free the page table from the root. 
6. copy-write fork PASS
    1. how to active usertrap when write bits to cow va? 
        1. disable w_en in pte 
    2. fork()
        1. just map the new pgtbl to old physical address. 
        2. new a physical pages mapping count 
        3. add the counter of the mapped page 
    3. handle user trap
        1. if the trap is writing to cow mem:
        2. new a page 
        3. remap pte and enable write 
    4. handle shrink user space 
        1. unmap the pgtbl and physical page 
        2. decreate 
        3. free physical page goto:6.6
    5. alloc new memory (sbrk etc.)
        1. allocate like before 
        2. but add the page counter 
    6. free physical page 
        1. if page counter >1: decrease counter 
        2. if page counter =1; decrease counter and add this page to free list
    7. modify the copyin function
7. thread PASS
    1. an esay lab
8. lock PASS
    1. the accquire() function will disable the interrupt, so if a user call accquire can held the cpu forever?
    2. ex1,memory allocator's model:
    ```
    ------------------------------------------------------
    ----------|| BIG FREE LIST POOL ||--------------------
    --------/------/-------|----\-----\......---------------
    -----|CPU1|-|CPU2|---|CUP3|--|.........|------------------
    if cpu i run out of memory, borrow PERALLOC pages from BIG FREE LIST POOL(if sizeof(BIG POOL) < PERALLOC, borrow all the big pool had) 
    if big pool run out of memory, withdraw (X) pages from cpu.
    ```
    3. ex2, buffer cache
        1. use a new model, a simpler model on hash table (compared with memory allocator model):
        ```
         - allocate 13 buckets, each bucket holds a bcache head and a bcache lock.
         - if a bucket K runs out of free buffer, just borrow a free buffer from other buckets
         - the borrow order is : (K+1) mod 13, (K+2) mod 13, ...., (K+12) mod 13
         ```
9. fs PASS 
    1. memory allocator
        - an easy lab, which is quite similar with page table 
    2. symbolic link 
        - crux problem: how to detect a loop in symbolic link?
        - detect same path name? NO  a-->b, b ->/xxxx/a, different name, same inode
        - detect same inode and dev? NO if symlink(b(null), a(null)), then symlink(a(x), b(null)). In this sisuation,although having a loop (a->b->a), but a.inode.prev = null != a.inode.now = y. 
        - use max redirect times? OK, but with problems, it's difficult to find a proper max redirect time, and it is not 100% accurate. 
10. mmap PASS
    1. the most complicated lab out of ten 
    2. vma collection, free a vma node to the vma pool when all vitural address  in that vma are unmapped.  
    3. use reserved pte bits to mark mmap virtual address 
    4. use dirty bit to write data back to fs 
    5. PRoblem? how to achieve the consistence when multi process mmaping same file. 
