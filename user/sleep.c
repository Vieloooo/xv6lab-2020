#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
int main(int argc, char *argv[])
{
    //check in put
    if (argc != 2)
    {
        printf("input error\nCorrect usage :sleep [time]\n");
        exit(0);
    }
    int time = atoi(argv[1]);
    sleep(time);
    exit(0);
}
