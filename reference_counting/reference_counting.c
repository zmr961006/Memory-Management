/*************************************************************************
	> File Name: reference_counting.c
	> Author: 
	> Mail: 
	> Created Time: 2016年08月09日 星期二 08时49分56秒
 ************************************************************************/


/*引用计数的一个伪代码*/


#include<stdio.h>

new(){
    ref = allocate();
    if (ref == NULL){
        printf("error out of memory");
    }
    rc(ref) = 0;
    return ref;

}

atomic write(src,i,ref){
    
    addrefference(ref);
    deletereference(src[i]);
    src[i] = ref;

}

addrefference(ref){
    if(ref != NULL){
        rc(ref) = rc(ref) + 1;
    }    
}

deletereference(ref){
    if(ref != NULL){
        rc(ref) = rc(ref) - 1;
    }
    if(rc(ref) == 0){
        for each fld in Pointers(ref)
            deletereference(*fld);
        free(ref);
    }
}
