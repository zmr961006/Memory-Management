/*************************************************************************
	> File Name: BTM.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月12日 星期一 20时24分10秒
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>

#define MAXSIZE    1000
#define ALLOC_MIN  100 
#define FootLoc(p)  (p+(p->size) - 1)

typedef struct WORD{    //WORD：内存字类型

    union {                //头和尾都指向同一个位置使用联合体
        struct WORD *llink;
         struct WORD *uplink;
    };

    int tag ; //块标识：1：占用 0： 空闲

    int size ; //块的大小

    struct WORD *rlink;   //指向后继节点

   // OtherType other; //字的其他部分这里暂时用不到
    
}*Space;

Space user[MAXSIZE] = {NULL} ; //用户空间数组

int usCount = 0;

Space AllocBoundTag(Space *pav,int n){
    Space p = * pav;
    if(NULL == p){
        printf("The memory is NULL \n");
        return NULL;
    }
    for(p;p != NULL && p->size < n && p->rlink != *pav; p = p->rlink ){

        if(p == NULL || p->size < n){
            printf("error is :%d\n",__LINE__);
            return NULL;
        }
        *pav = p->rlink;    //防止小的零碎空间全部集中在前边
            
        if(p->size - n > ALLOC_MIN){  // 找到可以分配的块开始分配 ，同样也为了减少碎片 ，从高位截取p，且设置新的底部
            
            p->size -= n;             //计算剩余块的大小

            Space foot = FootLoc(p);  //指向剩余块的底部

            foot->uplink = p;         //设置剩余块的底部

            foot->tag = 0 ;           //设置剩余块底部的标识

            p = foot + 1  ;           //指向分配块的头部

            p->size = n ;          //设置分配块的头部
            
            foot = FootLoc(p);     //指向分配块的底部

            p->tag = 1 ;        //设置分配块的头部

            foot ->tag = 1;    //同上

            foot->uplink = p ;

        }else{ //分配后剩余空间小于规定的ALLOC_MIN
            
            if(p == *pav){  //如果只剩下一个空间了，清空指针
                *pav = NULL ;
            }else{ //直接分配整个块出去，虽然会浪费一部分空间
                Space foot = FootLoc(p);
                foot->tag = p->tag = 1;
                p->llink->rlink = p->rlink ;
                p->rlink->llink = p->llink ;
            }
        }
    }
    user[usCount++] = p; 
    return p;
}

void Space_init(Space * freeSpace, Space *pav){
    
    *freeSpace = (Space)malloc(sizeof(struct WORD)*(MAXSIZE + 2)); //初始化空间链表

    Space head = *freeSpace ;   //头指针

    head->tag = 1;             //设置头指针标示符

    head++;                    //头指针指向第一个节点

    head->tag = 0;             //设置第一个节点为空闲块              

    head->llink = head->rlink = head;  //设置循环链表

    head->size = MAXSIZE ;            //设置块的大小

    *pav = head ;                 //设置头指针    

    Space foot = FootLoc(head);   

    foot->tag = 0;

    foot->uplink = head ;

    foot++;

    foot->tag = 1;               //设置尾边界为已经使用

}


void reclaimBoundTag(Space *pav ,Space sp){
    Space  pre = (sp - 1)->uplink ;
    Space  next = sp + sp->size ;

    int pTag = pre->tag ;
    int nTag = next->tag ;           
    //声明两个节点，分别得到前一个和后一个节点的信息，并且记录占用情况，根据占用情况选择合并的手段

    if(pTag == 1 && nTag == 1 ){  //前后都是满的直接插入

        Space foot = FootLoc(sp);
        foot->tag = sp->tag = 0;
        if(pav == NULL){
            *pav = sp->llink = sp->rlink = sp;
        }else{
            
            sp->rlink = *pav ;
            sp->llink = (*pav)->llink;
            (*pre).llink = sp ;
            sp->llink->rlink = sp;
            *pav = sp;
        }
    }else if(pTag == 0 && nTag == 1){  // 前边的可以合并
        pre->size += sp->size ;
        Space foot = FootLoc(pre);
        foot->tag = 0;
        foot->uplink = pre;
    }else if(pTag == 1 && nTag == 0){  //后边的可以合并
        sp->llink = next->llink;
        sp->rlink = next->rlink;
        next->llink->rlink = sp ;
        next->rlink->llink = sp ;
        sp->size += next->size ;
        Space foot = FootLoc(sp);
        sp->tag = foot->tag = 0 ;
        foot->uplink = sp;
    }else{   //三个块一起合并
        pre->rlink = next->rlink;
        pre->size += sp->size + next->size;
        Space foot = FootLoc(pre);
        foot->uplink = pre ;
    }
    
    int i ;
    for(i = 0;i < usCount ;i++){
        if(sp == user[i]){
            user[i] = NULL;
        }
    }
}




void print(Space s){
    printf("The head is %0x SIZE: %d \n pre is %0x ,next is %0x\n",s,s->size,s->llink,s->rlink);
}

void print_space(Space pav){
    if(pav != NULL){
        printf("you an use the list:\n");
        Space pos = pav;
        for(pos = pos->rlink;pos != pav;pos = pos->rlink){
            print(pos);
        }
    }

    printf("_____________________________\n");
    int i ;
    for(i = 0;i< usCount ;i++){
        Space us = user[i];
        if(us){
            printf("the head is %0x  SIZE is %d\n",us,us->size);
        }
    }

}

int main(){
    Space freeSpace = NULL;
    Space pav = NULL;
    Space_init(&freeSpace,&pav);
    print(pav);
    printf("malloc a 300 and 300 \n");
    Space m3 = AllocBoundTag(&pav,300);
    print_space(pav);
    Space t3 = AllocBoundTag(&pav,300);
    print_space(pav);
    printf("free 300 \n");
    reclaimBoundTag(&pav,m3);
    print_space(pav);
    return 0;
}


