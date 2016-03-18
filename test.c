/*************************************************************************
	> File Name: test.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月13日 星期二 17时33分56秒
 ************************************************************************/

#include<stdio.h>
//#include<malloc.h>
#include<stdlib.h>
#include<string.h>
#include "./mm.c"
#include "./mm.h"
#include<stddef.h>
#include<error.h>
#include<unistd.h>
#define DRIVER
static void prinblock(void *bp) 
{
    size_t hsize, halloc;

    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  

    if (hsize == 0) {
        printf("%p: EOL, prev_alloc: [%d]\n", bp, IS_PREV_ALLOC(HDRP(bp)));
        return;
    }

    if (halloc){
        printf("%p: header: [%u:%c], prev_alloc: [%d]\n", 
            bp, (unsigned)hsize, (halloc ? 'a' : 'f'), IS_PREV_ALLOC(HDRP(bp)));
    } else {
        printf("%p: header: [%u:%c], footer: [%u, %c], prev[%p], next[%p], prev_alloc: [%d]\n", 
            bp, (unsigned)hsize, (halloc ? 'a' : 'f'), GET_SIZE(FTRP(bp)), (GET_ALLOC(FTRP(bp)) ? 'a' : 'f'), 
            prev_free(bp), next_free(bp), IS_PREV_ALLOC(HDRP(bp)));
    }

}

int main(){

    
    char *test;
    test = (char *)mm_malloc(100);
    int len ;
    prinblock(test);
    printf("size :%d\n",len);


}


