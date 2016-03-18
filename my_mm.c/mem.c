
#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<unistd.h>
#include<sys/mman.h>
#include<string.h>
#include<errno.h>
#include<fcntl.h>



#define MAX_HEAP (100 *(1 << 20))      //100MB 大小的内存
#define ALIGNMENT     8                //按照8对齐
#define MINSIZE       2                //最小分配块的大小（4字)
#define SEG_LEVLL     16                //隔离？？？
#define WSIZE         8                //一个字的大小包括头/尾的大小 （字节）
#define DSIZE         16                //双字的大小(字节)
#define CHUNKSIZE    (1 << 8)          //初始空闲块的大小和扩展堆时默认大小

#define ALIGN(p)    (((size_t)(p) + (ALIGNMENT -1)) & ~0x7) //所有的向8的整数倍对齐

#define MAX(x,y)    ((x) > (y) ? (x):(y))  //比较大小

#define PACK(size,alloc)    ((size) | (alloc))    //将标志位整合回头
#define PACK3(size,prev_free,alloc)  ((size) | (prev_free) | (alloc)) //确定，确定是否为空

//全局变量
static char *heap_listp = 0;  //指向第一个块
static char *flist_tbl = NULL; //释放链表
static char *heap_tailp = NULL; //指向最后一个块

static char *heap;           //堆的首地址
static char *mem_brk;        //堆的尾地址
static char *mem_max_addr;   //堆的最高地址

void mem_init(void);
void mem_deinit(void);
void *mem_sbrk(int incr);
void mem_reset_brk(void);
void *mem_heap_lo(void);
void *mem_heap_hi(void);
size_t mem_heapsize(void);
size_t mem_pagesize(void);

static void *extend_heap(size_t words);
static void place(void *bp,size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp);
static int checkblock(void *bp);
static inline void mm_checkfreetb1();
static inline void init_free_list();

static inline char **get_head(int level);
static inline char **get_tail(int level);

static inline void *prev_free(void *bp);
static inline void *next_free(void *bp);

static inline void set_prev_free(void *bp,char *p);
static inline void set_next_free(void *bp,char *p);

static inline void insert_node(int level,void *bp);
static inline void delete_node(int level,void *bp);

static inline int in_heap(const void *p);
static inline int get_level(size_t s);

static inline int IS_VALID(size_t s);
static inline unsigned GET(void *p);
static inline void PUT(void *p,unsigned val);
static inline unsigned GET_SIZE(void *hp);
static inline void SET_SIZE(void *hp,size_t s);
static inline void MARK_FREE(void *hp);
static inline void MARK_ALLOC(void *hp);
static inline int  GET_ALLOC(void *p);
static inline char * HDRP(void *bp);
static inline char * FTRP(void *bp);
static inline char *NEXT_BLKP(void *bp);
static inline char *PREV_BLKP(void *bp);
static inline void FLAG(void * hp);
static inline void UNFLAG(void *hp);
static inline int IS_PREV_ALLOC(void *hp);

int mm_init(void) {
    /* Create the initial empty heap */
x    if ((heap_listp = mem_sbrk(WSIZE + SEG_LEVLL * DSIZE)) == (void *)-1) 
        return -1;
    flist_tbl = heap_listp;
    init_free_list();
    int table_off = 2 * SEG_LEVLL * WSIZE;
    PUT(heap_listp + table_off, PACK(4, 1));     /* Prologue header */ 
    PUT(heap_listp + table_off + 4, PACK(0, 1));     /* Epilogue header */

    FLAG(heap_listp + table_off);
    FLAG(heap_listp + table_off + 4);                /* set the alloc FLAG in next block */
    heap_listp += table_off + 4;

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    char *freeb = extend_heap(CHUNKSIZE/WSIZE);
    if (freeb == NULL) {    
        return -1;
    }
    return 0;
}



void *malloc (size_t size) {
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      

    if (heap_listp == 0){
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    asize = ALIGN(size + 4); /* the overhead is payload + header(4 byte) + padding(optional) */
    asize = MAX(asize, MINSIZE * WSIZE); /* require minimum size */

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  
#ifdef DEBUG    
        printf("malloc: before alloc.\n");
        mm_checkheap(1);
#endif            
        place(bp, asize);                  
#ifdef DEBUG
        printf("malloc: after alloc.\n");        
        mm_checkheap(1);
        printf("\n\n");
#endif        
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    int tail_free = 0; // the free space we have in the tail of heap
    if (heap_tailp && !GET_ALLOC(HDRP(heap_tailp))) {
        tail_free = GET_SIZE(HDRP(heap_tailp));
    }
    extendsize = MAX(asize - tail_free,CHUNKSIZE);
    bp = extend_heap(extendsize/WSIZE);
    if (bp == NULL) 
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * free
 */
void free (void *bp) {
    if(bp == 0) 
        return;

    if (heap_listp == 0){
        mm_init();
    }

#ifdef DEBUG    
    printf("free: before.\n");
    mm_checkheap(1);
#endif
    MARK_FREE(HDRP(bp));
    PUT(FTRP(bp), GET(HDRP(bp)));
    coalesce(bp);
#ifdef DEBUG    
    printf("free: after.\n");        
    mm_checkheap(1);
    printf("\n\n");
#endif    
}

static void printblock(void *bp) 
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

static int checkblock(void *bp) 
{
    if ((size_t)bp % 8) {
        printf("Error: %p is not doubleword aligned\n", bp);
        return -1;
    }
    if (!GET_ALLOC(HDRP(bp))) { // free block
        if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp))) {
            printf("Error: header does match footer.\n");
            return -1;
        }
    }
    return 0;
}
static void *coalesce(void *bp) 
{
    size_t prev_alloc = IS_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            /* Case 1 */
        /* nop */
    }
    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        if (heap_tailp == NEXT_BLKP(bp)) {
            heap_tailp = bp;
        }
        delete_node(get_level(GET_SIZE(HDRP(NEXT_BLKP(bp)))), NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK3(size, 2, 0));
        PUT(FTRP(bp), PACK3(size, 2, 0));
    }
    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        int t = (bp == heap_tailp);
        delete_node(get_level(GET_SIZE(HDRP(PREV_BLKP(bp)))), PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        SET_SIZE(FTRP(bp), size);
        SET_SIZE(HDRP(PREV_BLKP(bp)), size);
        bp = PREV_BLKP(bp);
        if (t) {
            heap_tailp = bp;
        }
    }
    else {                                     /* Case 4 */
        int t = (NEXT_BLKP(bp) == heap_tailp);
        delete_node(get_level(GET_SIZE(HDRP(PREV_BLKP(bp)))), PREV_BLKP(bp));
        delete_node(get_level(GET_SIZE(HDRP(NEXT_BLKP(bp)))), NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +  GET_SIZE(FTRP(NEXT_BLKP(bp)));
        SET_SIZE(HDRP(PREV_BLKP(bp)), size);
        SET_SIZE(FTRP(NEXT_BLKP(bp)), size);
        bp = PREV_BLKP(bp);
        if (t) {
            heap_tailp = bp;
        }
    }
    UNFLAG(HDRP(NEXT_BLKP(bp)));    
    insert_node(get_level(GET_SIZE(HDRP(bp))), bp);
    return bp;
}

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 * 
 * Return:
 *      the new free block pointer
 */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;

    /* single-word alignment */
    size = (words + 1) * WSIZE; 
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                                        

    int prev_alloc = IS_PREV_ALLOC(HDRP(bp));

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK3(size, prev_alloc << 1, 0));         /* Free block header */   
    PUT(FTRP(bp), PACK3(size, prev_alloc << 1, 0));         /* Free block footer */   
    set_prev_free(bp, NULL);
    set_next_free(bp, NULL);
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 

    /* Coalesce if the previous block was free */
    heap_tailp = coalesce(bp);
    return heap_tailp;
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));   

    if (IS_VALID(csize - asize)) {  
        /* we want to make sure the new free block satisfy the minimum requirement */            
        int t = (bp == heap_tailp);
        delete_node(get_level(GET_SIZE(HDRP(bp))), bp); /* remove the record in the free block list */
        SET_SIZE(HDRP(bp), asize);
        MARK_ALLOC(HDRP(bp));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK3(csize-asize, 2, 0));
        PUT(FTRP(bp), PACK3(csize-asize, 2, 0));  
        insert_node(get_level(GET_SIZE(HDRP(bp))), bp);
        if (t) {
            heap_tailp = bp;
        }
    }else { 
        delete_node(get_level(GET_SIZE(HDRP(bp))), bp);
        MARK_ALLOC(HDRP(bp));
        FLAG(HDRP(NEXT_BLKP(bp)));
    }
}

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
static void *find_fit(size_t asize){
    /* First fit search */
    void *bp;
    char *flist_head;

    int level = get_level(asize);

    while (level < SEG_LEVLL) { // serach in the size-class from small to large
        flist_head = *(get_head(level));
        for (bp = flist_head; bp && GET_SIZE(HDRP(bp)) > 0; bp = next_free(bp)) {
            if (asize <= GET_SIZE(HDRP(bp))) {
                return bp;
            }
        }
        level++;
    }
    return NULL; /* No fit */
}

// Macro is evil, inline function is more reliable.
static inline void insert_node(int level, void * bp) {
    char **flist_head = get_head(level);
    char **flist_tail = get_tail(level);
    if (!(*flist_head)) {
        // empty list
        *flist_head = bp;
        *flist_tail = bp;
        set_prev_free(bp, NULL);
        set_next_free(bp, NULL);
    } else {
        if ((char *)bp < (*flist_head)) {
            // insert at head
            set_prev_free(*flist_head, bp);
            set_next_free(bp, *flist_head);
            set_prev_free(bp, NULL);
            *flist_head = bp;
        } else if ((*flist_tail) < (char *)bp) {
            // insert to tail
            set_next_free(*flist_tail, bp);
            set_prev_free(bp, *flist_tail);
            set_next_free(bp, NULL);
            *flist_tail = bp;
        } else {
            // find some place in the list
            char * c = *flist_head;
            while (c < (char *)bp) {
                c = next_free(c);
            }
            set_next_free(prev_free(c), bp);
            set_prev_free(bp, prev_free(c));
            set_prev_free(c, bp);
            set_next_free(bp, c);
        }
    }
}

static inline void delete_node(int level, void * bp) {
    char **flist_head = get_head(level);
    char **flist_tail = get_tail(level);    
    if (bp == *flist_head) {
        *flist_head = next_free(bp);
        if (*flist_head) {
            set_prev_free(*flist_head, NULL);
        } else {
            *flist_tail = NULL;
        }
    } else if (bp == *flist_tail) {
        *flist_tail = prev_free(bp);
        if (*flist_tail) {
            set_next_free(*flist_tail, NULL);
        } else {
            *flist_head = NULL;
        }
    } else {
        set_next_free(prev_free(bp), next_free(bp));
        set_prev_free(next_free(bp), prev_free(bp));
    }
}

static inline int IS_VALID(size_t s){
    return s >= (MINSIZE * WSIZE);
}

static inline void init_free_list() {
    memset(flist_tbl, 0, SEG_LEVLL * DSIZE);
}

static inline char **get_head(int level) {
    char * bp = flist_tbl + (level * DSIZE);
    return (char **)(bp);
}

static inline char **get_tail(int level) {
    char * bp = flist_tbl + (level * DSIZE) + WSIZE;
    return (char **)(bp);    
}

/*
 * This works for all positive number. 
 */
static inline int get_level(size_t size) {
    int r = 0, s = 16;
    while ((int)size > s - 1 && r < SEG_LEVLL) {
        s <<= 1;
        r += 1;
    }
    return r - 1;
}

static inline int in_heap(const void *p) {
     return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/* previous free block */
static inline void* prev_free(void * bp) {
    int off = GET(bp);
    if (off < 0) return NULL;
    return heap_listp + off;
}

/* next free block */
static inline void* next_free(void * bp) {
    int off = GET((char*)FTRP(bp) - 4);
    if (off < 0) return NULL;
    return heap_listp + off;
}

/* set previous free block pointer */
static inline void set_prev_free(void * bp, char * p) {
    int off = p ? p - heap_listp : -1;
    PUT(bp, off);
}

/* set next free block pointer */
static inline void set_next_free(void * bp, char * p) {
    int off = p ? p - heap_listp : -1;
    PUT((char*)FTRP(bp) - 4, off);
}

static inline void SET_SIZE(void * hp, size_t s) {
    unsigned flags = GET(hp) & 0x3;
    PUT(hp, s | flags);
}

static inline void MARK_FREE(void * hp) {
    PUT(hp, GET(hp) & ~1);
}

static inline void MARK_ALLOC(void * hp) {
    PUT(hp, (GET(hp) | 1));
}

/* Read and write a word at address p */
static inline unsigned GET(void * hp) {
    return (*(unsigned *)(hp));
}

static inline void PUT(void * hp, unsigned val) {
    (*(unsigned *)(hp) = (val));
}

/* Read the size and allocated fields from address p */
static inline unsigned GET_SIZE(void * hp) {
    return (GET(hp) & ~0x3);
}               
static inline int GET_ALLOC(void * hp) {
    return (GET(hp) & 0x1);
}

/* Given block ptr bp, compute address of its header and footer */
static inline char* HDRP(void * bp) {
    return ((char *)(bp) - 4);
} 

// ALARM: this only works for free block
static inline char* FTRP(void * bp) {
    return ((char *)(bp) + GET_SIZE(HDRP(bp)) - WSIZE);
}

static inline char* NEXT_BLKP(void * bp) {
    return ((char *)(bp) + GET_SIZE(HDRP(bp)));
}

// ALARM: this only works for free block
static inline char* PREV_BLKP(void * bp) {
    return ((char *)(bp) - GET_SIZE(((char *)(bp) - WSIZE)));
}

static inline void FLAG(void * hp) {
    PUT(hp, GET(hp) | 0x2);
}

static inline void UNFLAG(void * hp) {
    PUT(hp, GET(hp) & ~0x2);
}

static inline int IS_PREV_ALLOC(void * hp) {
    return !!(GET(hp) & 0x2);
}

void mem_init(void){
	int dev_zero = open("/dev/zero", O_RDWR);
	heap = mmap((void *)0x800000000, /* suggested start*/
			MAX_HEAP,				/* length */
			PROT_WRITE,				/* permissions */
			MAP_PRIVATE,			/* private or shared? */
			dev_zero,				/* fd */
			0);						/* offset (dunno) */
	mem_max_addr = heap + MAX_HEAP;
	mem_brk = heap;					/* heap is empty initially */
}

void *mem_sbrk(int incr) {
	char *old_brk = mem_brk;

    // call sbrk() in an attempt to have similar semantics as a real allocator.
	if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr) ||
            sbrk(incr) == (void *) -1) {
		errno = ENOMEM;
		fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
		return (void *)-1;
	}

	mem_brk += incr;
	return (void *)old_brk;
}


int main(){
    char *p;
    p = (char *)malloc(sizeof(char )*100);
    printblock(p);
}
