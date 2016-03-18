/*************************************************************************
	> File Name: BTM_1.cpp
	> Author: 
	> Mail: 
	> Created Time: 2015年10月12日 星期一 22时29分32秒
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>  
#define MAX_SIZE 1000  
#define ALLOC_MIN_SIZE 10//最小分配空间大小.  
  
typedef struct Word{  
    union {  
        Word * preLink;//头部域前驱  
        Word * upLink;//尾部域，指向结点头部  
    };  
    int tag;//0标示空闲,1表示占用  
    int size;//仅仅表示 可用空间，不包括 头部 和 尾部空间  
    Word * nextLink;//头部后继指针.  
}*Space;  
#define FOOT_LOC(p) ((p)+(p->size)-1)//尾部域位置  
  
void initSpace(Space * freeSpace,Space * pav){  
    //有2个空间是为了 查找空间的邻接点，防止出界用的。  
    *freeSpace = (Space)malloc((MAX_SIZE+2)*sizeof(Word));  
    Space head = *freeSpace;  
    head->tag = 1;//设置边界已占用  
    head++;//指向第一个节点..  
    head->tag = 0;//设置节点空闲.  
    head->preLink = head->nextLink = head;//循环双链表..  
    head->size = MAX_SIZE;  
    *pav = head;//设置头指针  
    Space foot = FOOT_LOC(head);  
    foot->tag = 0;  
    foot->upLink = head;  
    foot++;  
    foot->tag = 1;//设置 边界 已占用  
}  
Space userSpace[MAX_SIZE] = {NULL};//用户空间数组.  
int usCount = 0;  
  
Space allocBoundTag(Space * pav,int size){  
    Space p = * pav;  
    for (;p != NULL && p->size < size && p->nextLink != *pav ; p = p->nextLink) ;  
    if (p == NULL || p->size < size ){  
        return NULL;  
    }  
    *pav = p->nextLink;//为了 防止 小的空间 都堆积 在 前面...（小空间均匀分布）  
    if (p->size - size > ALLOC_MIN_SIZE){//从高位截取p,不破坏指针间的关系.  
        p->size -= size;  
        Space foot = FOOT_LOC(p);  
        foot->upLink = p;  
        foot->tag = 0;  
        p = foot + 1;  
        p->size = size;  
        foot = FOOT_LOC(p);  
        p->tag = foot->tag = 1;  
        foot->upLink = p;  
    }  
    else{//分配后剩余空间小于 ALLOC_MIN_SIZE   
        if (p = *pav){//只剩下一个空间了，清空指针  
            *pav = NULL;  
        }  
        else{//直接分配 p->size个空间出去  
            Space foot = FOOT_LOC(p);  
            foot->tag =p->tag = 1;  
            p->preLink->nextLink = p->nextLink;  
            p->nextLink->preLink = p->preLink;  
        }  
    }  
    userSpace[usCount++] = p;  
    return p;  
}  
//回收空间，合并 邻接空闲空间.  
void reclaimBoundTag(Space * pav,Space sp){  
    Space pre = (sp -1)->upLink;//前一个空间  
    Space next = sp + sp->size;//后一个空间..  
    int pTag = pre->tag;  
    int nTag = next->tag;  
    if (pTag == 1 && nTag == 1){//前后都被占用，直接插入在表头.  
        Space foot = FOOT_LOC(sp);  
        foot->tag = sp->tag = 0;  
        if (pav == NULL){  
            *pav = sp->preLink = sp->nextLink = sp;  
        }  
        else{  
            sp->nextLink = *pav;  
            sp->preLink = (*pav)->preLink;  
            (*pav)->preLink = sp;  
            //(*pav)->preLink->nextLink = sp;//上一句话 已经改变了 值.  
            sp->preLink->nextLink = sp;  
            *pav = sp;//将头指针指向刚释放的空间  
        }  
    }  
    else if(pTag == 0 && nTag == 1){//前面的可以合并..  
        pre->size += sp->size;  
        Space foot = FOOT_LOC(pre);  
        foot->tag = 0;  
        foot->upLink = pre;  
    }  
    else if(pTag == 1 && nTag == 0){//后面的可以合并  
        sp->preLink = next->preLink;  
        sp->nextLink = next->nextLink;  
        next->preLink->nextLink = sp;  
        next->nextLink->preLink = sp;  
        sp->size += next->size;  
        Space foot = FOOT_LOC(sp);  
        sp->tag = foot->tag = 0;  
        foot->upLink = sp;  
    }  
    else{//前后都可以合并  
        pre->nextLink = next->nextLink;  
        pre->size += sp->size + next->size;  
        Space foot = FOOT_LOC(pre);  
        foot->upLink = pre;  
    }  
    //设置用户空间  
    for (int i = 0; i < usCount; i++){  
        if (sp == userSpace[i]){  
            userSpace[i] = NULL;  
        }  
    }  
}  
  
void print(Space s){  
    printf("空间首地址：%0x,空间大小：%d，前驱地址：%0x,后继空间:%0x\n",s,s->size,s->preLink,s->nextLink);  
}  
  
void printSpace(Space pav){  
    if (pav != NULL)  
    {  
        printf("--------可利用空间表---------\n");  
        Space p = pav;  
        print(p);  
        for (p = p->nextLink; p != pav; p = p->nextLink){  
            print(p);  
        }  
    }  
    printf("--------用户空间表---------\n");  
    for (int i = 0; i < usCount; i++){  
        Space us = userSpace[i];  
        if (us){  
            printf("空间首地址：%0x,空间大小：%d\n",us,us->size);  
        }  
    }  
}  
  
  
int main(int argc, char * argv[])  
{  
    Space  freeSpace = NULL,pav = NULL;  
    initSpace(&freeSpace,&pav);  
    printf("----------初始化-------------\n");  
    printSpace(pav);  
    printf("--------------------分配一个500的空间后-------------\n");  
    Space s500 = allocBoundTag(&pav,500);  
    printSpace(pav);  
    printf("--------------------分配一个300的空间后-------------\n");  
    Space s300 = allocBoundTag(&pav,300);  
    printSpace(pav);  
    printf("--------------------分配一个150的空间后-------------\n");  
    Space s150 = allocBoundTag(&pav,150);  
    printSpace(pav);  
    printf("--------------------释放一个300的空间后-------------\n");  
    reclaimBoundTag(&pav,s300);  
    printSpace(pav);  
 
    return 0;  
}  
