#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"

void lldir(char *path)
{
    int fd;
    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        printf("err path");
        return;
    }
    struct stat st;
    struct dirent de;
}

int main(int argc, char *argv[])
{
    lldir(argv[1]);
}