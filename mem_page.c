/*************************************************************************
	> File Name: mem_page.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月22日 星期四 12时22分18秒
 ************************************************************************/

#include<stdio.h>
#include<sys/mman.h>
#include<unistd.h>
#include<sys/types.h>
#include "/usr/src/kernels/4.0.8-200.fc21.x86_64/include/linux/slab.h"
#include "/usr/src/kernels/4.0.8-200.fc21.x86_64/include/linux/gfp.h"

typedef struct test{
    int data;
    double test;

}test;

int main(){
    struct test *p;
    p = kmalloc(sizeof(test),GFP_KERNEL);
    p->data = 1000;
    printf("the %d\n",p->data);
}
