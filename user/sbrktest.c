#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int main(void){
    for (int i=0;i<100;i++){
        sbrk(1);
    }
}