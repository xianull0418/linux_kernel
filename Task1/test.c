#include<stdio.h>
#include<linux/kernel.h>
#include<sys/syscall.h>
#include<unistd.h>
void main(){
        long a=syscall(439,4);
        printf("the result is %ld.\n",a);
}

