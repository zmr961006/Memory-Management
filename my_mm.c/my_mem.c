/*************************************************************************
	> File Name: my_mem.c
	> Author: 
	> Mail: 
	> Created Time: 2015年10月14日 星期三 18时25分14秒
 ************************************************************************/

#include<stdio.h>
#include<assert.h>
#include<unistd.h>
#include<sys/mman.h>
#include<string.h>
#include<errno.h>
#include<fcntl.h>
#include<pthread.h>

#define MAX_HEAP  (100 *(1 << 20))//100MB 大小

#define MINSIZE 2

#define WSIZE 4           //一个字的大小4KB
#define DSIZE 8           //双字大小8KB

#define CHUNKSIZE (1<<12)  //初始空闲堆的大小和默认的大小  4MB

#define MAX(x,y) ((x) > (y)? (x):(y))

#define PACK(size,alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p)) 
#define PUT(p,val) (*(unsigned int *)(p) = (val)) 
#define GET_SIZE(p) (GET(p) & ~0x7)            //从头部或脚部返回大小
#define GET_ALLOC(p) (GET(p) & 0x1)            //返回分配位

#define HDRP(bp)  ((char *)(bp) - WSIZE) //返回头部
#define FTRP(bp)   ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //返回脚部指针

#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))  //返回下一个指针
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))  //返回上一个指针


static char *heap_listp = 0;
static char *heap;
static char *mem_brk;
static char *mem_max_addr;


static char *flist_free = NULL;   //指向释放的链表
static char *heap_tailp = NULL;   //指向最后一块
static int  count  = 0;           //指示链表中块的数目


static void *extend_heap(size_t words);
void *mem_sbrk(int incr);     //申请额外的堆空间就是malloc 的分配
static inline void init_free_list(); //初始化链表
void *malloc(size_t size);   //分配空间函数
static void *find_fit(size_t asize);
static void place(void *bp,size_t asize);  // 将请求块放在空闲块起始处
static void print(void );
static void print_show(void);
void free(void *bp);  //释放这个块
static void *coalesce(void *bp);


void free(void *bp){                //回收我们的内存，有用有还，再借不难
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    coalesce(bp);
}


static void *coalesce(void *bp){         //块的合并算法

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc){             //当两边都被占用，直接释放不需要做任何事
        return bp;
    }else if(prev_alloc && !next_alloc){      //前一个被占用，后一个空闲，直接合并后边的

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp),PACK(size,0));
        PUT(FTRP(bp),PACK(size,0));

    }else if(!prev_alloc && next_alloc ){    //前一个空闲，后一个被占用，直接合并前一个
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp),PACK(size,0));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);

    }else{   //前后都是空闲，直接全部合并
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

static void print_show(void){
    char *temp;
    int a = 10;
    for(temp = heap_listp; temp != mem_brk;temp = NEXT_BLKP(temp),a--){
        printf("the start is %p \n ",temp);
        printf("the flag is %d\n",GET_ALLOC(HDRP(temp)));
    }
}


static void place(void *bp,size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));        //csize 得到这个块的大小
    
    if((csize - asize) >= (2*DSIZE)){          //如果分配减去需求大于最小块的大小，进行分割
        PUT(HDRP(bp),PACK(asize,1));            //设置头和尾
        PUT(FTRP(bp),PACK(asize,1));   
        bp = NEXT_BLKP(bp);                    //下一个块就是尾减一
        PUT(HDRP(bp),PACK(csize-asize,0));     //设置下一个块的属性，大小为剩余块的大小
        PUT(FTRP(bp),PACK(csize-asize,0));
    }else{                                  //如果剩余块的大小，小于一个标准块那就将整个块分配出去
        PUT(HDRP(bp),PACK(csize,1));
        PUT(HDRP(bp),PACK(csize,1));
    }


}



static void *find_fit(size_t asize){
    char *bp;
    int   len = 0;
    //printf("the %p %p \n",heap_listp,NEXT_BLKP(heap_listp));
    for(bp = heap_listp;GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp),len++){
        if(!GET_ALLOC(HDRP(bp))  && (asize <= GET_SIZE(HDRP(bp)))){
            //printf("\n\nthe size is %d\n\n",GET_SIZE(HDRP(bp)));
            return  (void *)bp;
        }
        /*if(len == count){
            break;
        }*/
        
    }
    return NULL;

}


void *malloc(size_t size){
    size_t asize;   //建议块的大小，根据size 且需要对齐
    size_t extendsize;
    char *bp;
    if(size == 0)
        return NULL;
    if(size <= DSIZE)
        asize = 2*DSIZE;
    else
        asize = DSIZE*((size + (DSIZE) + (DSIZE-1)) / DSIZE); //设置最小的块为16 其中留了8字节为头部和脚部
    if((bp = find_fit(asize)) != NULL){  //如果找到可以分配的块就分配
        place(bp,asize);
        return bp;
    }
    
    extendsize = MAX(asize,CHUNKSIZE);  //对比我们的初始化堆和需要分配的大小，向我们的堆空间申请额外的空间
    if((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp,asize);
    return bp;

}



static inline void init_free_list(){  //初始化链表
    memset(flist_free,0,16 * 8);
}



static void *extend_heap(size_t words){

    char *bp;
    size_t size;
    
    size = (words % 2)?(words + 1)*WSIZE : words *WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    PUT(HDRP(bp),PACK(size,0));
    PUT(FTRP(bp),PACK(size,0));
    PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));

    return bp;

}




void mem_init(void){            //从内存里拿出一个100MB 的空间来作为我们的堆
    int dev_zero = open("/dev/zero",O_RDWR);
    heap = mmap((void *)0x800000000,MAX_HEAP,PROT_WRITE,MAP_PRIVATE,dev_zero,0);
    mem_max_addr = heap + MAX_HEAP;
    mem_brk = heap;
}

void *mem_sbrk(int incr){       //申请额外的堆空间就是malloc 的分配
    char *old_brk = mem_brk;
    if((incr < 0) || ((mem_brk + incr) > mem_max_addr)){     //超过大小或者超过我们的堆空间大小都会报错
        printf("out of memory\n");                        //brk 来返回堆的尾部地址
        return (void *)-1;
    }
    mem_brk += incr;
    return (void *)old_brk;
}
 
int mm_init(void){    //初始化序言块
    if((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)   // brk :0x800000010
            return -1;
    flist_free = heap_listp ;
    init_free_list();
    PUT(heap_listp,0);
    PUT(heap_listp + (1*WSIZE),PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE),PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE),PACK(0,1));
    heap_listp += (2*WSIZE);                                //heap_listp::0x800000008
    count++;                                                 //链表中现在只有一个序言块
    if(extend_heap(CHUNKSIZE/WSIZE) == NULL)   //保持对齐并且，根据需求请求更多的堆存储器,初始化堆空间4KB
        return -1;
    return 0;
}





static void print(){  //测试函数
    //printf("the heap : %p\n",heap);
    printf("the heap :%p\n heap_brk:%p \n heap_max_addr:%p\n",heap,mem_brk,mem_max_addr);
    printf("the heap_listp:%p    the flist_free is %p \n",heap_listp,flist_free);
}

static void print_blcok(void *bp){
    printf("the heap_prev:%p   heap_next is %p \n",PREV_BLKP(bp),NEXT_BLKP(bp));
}



/*void *one(void *arg){
    
    while(1){
        char *p;
        p = (char *)malloc(sizeof(char)*10);
        free(p);
        printf("i am the one thread\n");
        sleep(1);
    }
}

void *two(void *arg){
    
    while(1){
        char *q;
        q = (char *)malloc(sizeof(char)*10);
        free(q);
        printf("i am the two thread\n");
        sleep(1);
    }
}

int main(){
    mem_init();
    mm_init();
    pthread_t thid1;
    pthread_t thid2;
    int err;
    err = pthread_create(&thid1,NULL,one,NULL);
    err = pthread_create(&thid2,NULL,two,NULL);

    while(1){
        sleep(1);
    }
    
}*/


int main(){
	mem_init();
	mm_init();
	int i = 10000000;
	char *p;
    p = (char *)malloc(sizeof(char)*200000);
    
    printf("%p \n",p);
	return 0;
}

