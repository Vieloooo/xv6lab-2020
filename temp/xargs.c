#include "kernel/types.h"
#include "user/user.h"

/*  form of stdin 
    there is no code for eof 
    after read(buf,stdin)
    there are just bytes in asc code 

*/
int main(int argc, char *argv[])
{
    char buff[32] = {0};
    char *fake_argv[32] = {0};
    char fake[32] = {0};
    int k = 0;              //index in fake
    int j, i;               //loop params
    int m = argc - 1;       //index in fake argv
    char *p2lastarg = fake; //pionter to last fake arg
    //read data from fd 0 and check read
    int buff_len = read(0, buff, sizeof(buff));
    if (buff_len == 0)
    {
        printf("no stdinput\n");
        exit(0);
    }
    //put argv 1->n in fake_argv 0->n-1
    for (j = 1; j < argc; j++)
    {
        fake_argv[j - 1] = argv[j];
    }

    //read buff, and copy block ( code divided by "\n") to fake
    for (i = 0; i < buff_len; i++)
    {
        if (buff[i] == ' ')
        {
            //argv pionter stops a arg by reading a 0
            //question: how the proc adds the last arguement to fake_argv
            /*for examle: 
            echo hello|xargs echo gg
                there is no' 'in the end of stdin,the stdin is [hello,\0,eof], 
                so how the proc recognize eof as ' ' ,and add the last arguement?
            */
            fake[k++] = 0;
            fake_argv[m++] = p2lastarg;
            p2lastarg = &fake[k];
        }
        else if (buff[i] == '\n')
        {
            fake[k] = 0;
            k = 0;
            fake_argv[m++] = p2lastarg;
            fake_argv[m] = 0;
            p2lastarg = fake;
            m = argc - 1;
            //use fork and exec to new a proc
            if (fork() == 0)
            {
                //child
                exec(fake_argv[0], fake_argv);
            }
            else
            {
                //p
                wait(0);
            }
        }
        else
        {
            fake[k++] = buff[i];
        }
    }

    exit(0);
}