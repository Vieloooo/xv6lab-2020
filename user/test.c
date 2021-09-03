#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    trace(1);
    int pid = fork();
    if (pid == 0)
    {
        trace(1);
    }
    exit(0);
}