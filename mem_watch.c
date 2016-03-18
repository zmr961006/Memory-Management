/*************************************************************************
	> File Name: mem_watch.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月22日 星期四 10时04分32秒
 ************************************************************************/

#include<stdio.h>
#include<signal.h>
#include<string.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>

static int alloc_size;
static char *memory;



int main(){
    int size;
    size = getpagesize();
    printf("the pagesize id %d\n",size);

}
