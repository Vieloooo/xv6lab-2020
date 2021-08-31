#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
//#define buffsize 1
int main()
{
    int p1[2], p2[2];
    char buff[] = "some msg";
    long length = sizeof(buff);
    pipe(p1);
    pipe(p2);
    int pid = fork();
    if (pid == 0)
    {
        // single side pipe for child to parents
        close(p1[1]);
        close(p2[0]);
        if (read(p1[0], buff, sizeof(buff)) != sizeof(buff))
        {
            printf("error occured in reading data from parent to child\n");
            exit(1);
        }
        printf("%d: received ping\n", getpid());
        if (write(p2[1], buff, sizeof(buff)) != sizeof(buff))
        {
            printf("error occurd in writing data from child to parent\n");
            exit(1);
        }
    }
    else
    {
        close(p1[0]);
        close(p2[1]);
        if (write(p1[1], buff, length) != length)
        {
            printf("error in write from p to c\n");
            exit(1);
        }

        if (read(p2[0], buff, length) != length)
        {
            printf("error in read from p to c\n");
            exit(1);
        }
        printf("%d: received pong\n", getpid());
        wait(0);
    }
    exit(0);
}