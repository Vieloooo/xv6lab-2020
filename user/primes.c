#include <kernel/types.h>
#include <user/user.h>
#include "kernel/stat.h"

void filter(int p){
    int i;
    while(read(0,&i,sizeof(i))){
        if(i%p!=0){
            write(1,&i,sizeof(i));
        }
    }
}

//redirct fd i to pipe pd 
void redirect(int i, int pd[]){
    close(i);
    dup(pd[i]);
    close(pd[0]);
    close(pd[1]);
}

void psend(){
    int pd[2],prime;
    if (read(0, &prime, sizeof(prime))) {
        printf("prime %d\n", prime);
        if(prime==31){
                exit(0);
            }
        pipe(pd);
        
        if(fork()==0){           
            //read data from pipe 
            redirect(0,pd);
            psend();

        }else{
            //have redircted 0 -> pipe before 
            //filter data and send it to pipe 
            redirect(1,pd);
            filter(prime);
           // wait(0);
        }
    }
     exit(0);
}

int main(int argc, char *argv[]){
    int pd[2]; 
    pipe(pd);
    if(fork()==0){
        //child
        //read data 
        redirect(0,pd);
        psend();
    }else{
        //parent
        //manage input (omitted)
        //send data through pipe 
        redirect(1,pd);
        for(int i = 2; i < 36; i++) {
            write(1, &i, sizeof(i));
        }   
    }
    sleep(10);
    exit(0);
}
