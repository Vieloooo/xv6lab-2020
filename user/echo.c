#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void){
    for (int i=0;i<3;i++){
        sbrk(4096);
    }
    exit(0);
}