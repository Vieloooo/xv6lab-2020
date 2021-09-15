# xv6lab-2020
## personal repo to learn mit 6.0s081 xv6 and git usage.
1. util pass
2. syscall pass
3. pgtbl has problems 
    1. proc.sz is the memory usage for each process (not include trampline and trapframe)
    2. trapframe use 2 pages under VAMAX
    ```c
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
5. trap pass
    1. what if handler wanna to pass arguement in the signal handler function 
    2. should use lock to lock handle function(or atomic action), not a normal integer
6.
