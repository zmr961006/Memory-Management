/*************************************************************************
	> File Name: time_test.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月15日 星期四 22时57分42秒
 ************************************************************************/

#include<stdio.h>
#include<malloc.h>
int main(){
	
	int i = 10000000;
	while(i > 0){
		char *p;
		p = (char *)malloc(sizeof(char)*10);
		free(p);
		i--;
	}
	return 0;
}

