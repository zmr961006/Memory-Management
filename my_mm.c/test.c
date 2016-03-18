/*************************************************************************
	> File Name: test.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月16日 星期五 08时16分14秒
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
int main(){
    char *p;
    int i = 1000;
	while(i > 0){
		p = (char *)malloc(sizeof(char)*1000000);
		free(p);
        i--;
	}
	return 0;
}
