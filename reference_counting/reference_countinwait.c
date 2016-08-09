/*************************************************************************
	> File Name: reference_countinwait.c
	> Author: 
	> Mail: 
	> Created Time: 2016年08月09日 星期二 09时34分31秒
 ************************************************************************/
/*延时引用应用算法*/

#include<stdio.h>

New(){
    ref = allocate();
    if (ref == NULL){
        collect();
        ref = allocate();
        if(ref == NULL){
            printf("error out of memory");
        }
    }
    rc(ref) = 0;
    add(zct,ref);
    return ref;
}

write(src,i,ref){
    if(src == roots)
        src[i] = ref;
    else
        atomic
            addreference(ref);
            remove(zct,ref);
            deleteReferenceToZCT(src[i]);
            src[i] = ref;
}

deleteReferenceToZCT(ref){
    if(ref != NULL){
        rc(ref) -= 1;
        if(rc(ref) == 0){
            add(zct,ref);
        }
    }

}

atomic collect(){
        
    for each fld in roots
        addreference(*fld);
    sweepZCT();
    for each fld in roots
        deleteReferenceToZCT(*fld);

}

sweepZCT(){
    
    while(not isEmpty(zct))
        ref = remove(zct);
        if(rc(ref) == 0)
            for each fld in Pointers(ref)
                deleteReference(*fld);
            free(ref);

}
