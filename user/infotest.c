#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    struct sysinfo info;
    info.nproc = 0;
    info.freemem = 0;
    printf("real va:%d\n", &info);
    sysinfo(&info);
    printf("nproc:%d,freemem:%d\n", info.nproc, info.freemem);
    return 0;
}
