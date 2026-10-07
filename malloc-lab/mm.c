/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "team2",
    /* First member's full name */
    "KIM GUNHO",
    /* First member's email address */
    "kunho020215@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */

#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
/*헤더 사이즈*/
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*
 * mm_init - initialize the malloc package.
 */
#define WSIZE 4
#define DSIZE 8
#define CHUNkSIZE (1<<12)

#define MAX(x,y) ((x) >(y) ? (x) : (y))

#define PACK(size, alloc) ((size)|(alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))


#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp)+ GET_SIZE(HDRP(bp))-DSIZE)
#define PREDP(bp) ((char *)(bp))
#define SUCCP(bp) ((char *)(bp) + WSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char*)(bp)-WSIZE))) //후 bp
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //전 bp
#define PRED(bp) (*(void **)(PREDP(bp)))
#define SUCC(bp) (*(void **)(SUCCP(bp)))




static char* heap_listp = NULL; 
static char* last_alloc_ptr = NULL;
static char* free_listp = NULL;




static void* first_fit(size_t asize){
    void *bp;
    
    for(bp = SUCC(heap_listp); bp != heap_listp; bp = SUCC(bp)){
        if(GET_SIZE(HDRP(bp)) >= asize) return bp;
    }
    return NULL;

}
static void* best_fit(size_t asize){
    return NULL;
}
static void* next_fit(size_t asize){
    return NULL;


}
typedef enum Alg {First = 1, Best, Next} ALG;

static void* find_fit(size_t asize){    
    //change fit alg here
    ALG mmalg = First;
    
    void* (*fuc_ptr)(size_t);
    switch (mmalg)
    {
        case First:
            fuc_ptr = first_fit;
            
            break;
        case Best:
            fuc_ptr = best_fit;
            
            break;
        case Next:
            fuc_ptr = next_fit;
            break;    
        default:
            return NULL;
    }
    return fuc_ptr(asize);
}

static void insert_free(void* bp){
    void *first = SUCC(heap_listp);

    PRED(bp) = heap_listp;
    SUCC(bp) = first;

    PRED(first) = bp;
    SUCC(heap_listp) = bp;
    
}

static void remove_free(void* bp){
    SUCC(PRED(bp)) = SUCC(bp);
    PRED(SUCC(bp)) = PRED(bp);
}

static void place(void *bp, size_t asize)
{
    size_t oldsize = GET_SIZE(HDRP(bp));

    remove_free(bp);

    if(oldsize - asize >= 2 * DSIZE){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        void *next = NEXT_BLKP(bp);

        PUT(HDRP(next), PACK(oldsize - asize, 0));
        PUT(FTRP(next), PACK(oldsize - asize, 0));

        insert_free(next);
    }
    else{
        PUT(HDRP(bp), PACK(oldsize, 1));
        PUT(FTRP(bp), PACK(oldsize, 1));


    }
}



static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    //둘다 할당
    if (prev_alloc && next_alloc) {
    }
    //next free
    else if (prev_alloc && !next_alloc) {
        void *next = NEXT_BLKP(bp);
        remove_free(next);

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    //prev free
    else if (!prev_alloc && next_alloc) {
        void *prev = PREV_BLKP(bp);
        remove_free(prev);


        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));

        bp = prev;
    }
    //prev next free
    else {
        void *prev = PREV_BLKP(bp);
        void *next = NEXT_BLKP(bp);
        remove_free(prev);
        remove_free(next);

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        bp = prev;
    }
    insert_free(bp);

    return bp;
}


static void *extend_heap(size_t words){
    //heap 가용 공간이 없을 때 추가로 sbrk로 늘려주는 역할
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1) return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0,1));

    return coalesce(bp);
}

int mm_init(void)
{
    
    if((heap_listp = mem_sbrk(6*WSIZE)) == (void*) -1){
        return -1;
    }

    PUT(heap_listp, 0);//padding
    PUT(heap_listp + (WSIZE), PACK(16, 1));//prologue header
    PUT(heap_listp + (2*WSIZE), 0);//prologue prec
    PUT(heap_listp + (3*WSIZE), 0);//prologue succ
    PUT(heap_listp + (4*WSIZE), PACK(16, 1));//prologue footer
    PUT(heap_listp + (5*WSIZE), PACK(0, 1));//epilogue header

    heap_listp+=(2*WSIZE);

    PRED(heap_listp) = heap_listp;
    SUCC(heap_listp) = heap_listp;

    if(extend_heap(CHUNkSIZE/WSIZE) == NULL) return -1;
    return 0;
}



/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0) return NULL;

    if(size <=DSIZE) asize = 2*DSIZE;
    else asize = DSIZE*((size + (DSIZE) + (DSIZE -1))/DSIZE);

    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNkSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;

}



/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    //블록 헤더부분 ptr임 따라서 payload사이즈 크기 읽어서 header footer 비트 0처리
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    char* bp = coalesce(ptr);//공간 병합
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if(ptr == NULL) return mm_malloc(size);

    if(size == 0){
        mm_free(ptr);
        return NULL;
    }
    //할당 정렬조건
    size_t asize;
    if(size <=DSIZE) asize = 2*DSIZE;
    else asize = DSIZE*((size + (DSIZE) + (DSIZE -1))/DSIZE);

    void *next = NEXT_BLKP(ptr);
    void *prev = PREV_BLKP(ptr);

    size_t oldsize = GET_SIZE(HDRP(ptr));

    //현재 블록이 충분할 경우
    if(oldsize >= asize){
        size_t remain = oldsize - asize;

        if(remain >= 2 * DSIZE){
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));

            void *free_bp = NEXT_BLKP(ptr);

            PUT(HDRP(free_bp), PACK(remain, 0));
            PUT(FTRP(free_bp), PACK(remain, 0));

            coalesce(free_bp);
        }
        return ptr;
    }

    //heap 끝부분 확장
    if(GET_SIZE(HDRP(next)) == 0){
        size_t need = asize - oldsize;

        if(mem_sbrk(need) != (void*)-1) {
            PUT(HDRP(ptr), PACK(asize, 1));
            PUT(FTRP(ptr), PACK(asize, 1));
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(0,1));
            return ptr;
        }
    }
    //next와 병합 후 제자리 확장
    if(!GET_ALLOC(HDRP(next))){
        size_t combined = oldsize + GET_SIZE(HDRP(next));
        if(combined >= asize){
            remove_free(next);
            
            PUT(HDRP(ptr), PACK(combined, 0));
            PUT(FTRP(ptr), PACK(combined, 0));

            size_t oldsize = GET_SIZE(HDRP(ptr));
            if(oldsize - asize >= 2 * DSIZE){
                PUT(HDRP(ptr), PACK(asize, 1));
                PUT(FTRP(ptr), PACK(asize, 1));

                void *next = NEXT_BLKP(ptr);

                PUT(HDRP(next), PACK(oldsize - asize, 0));
                PUT(FTRP(next), PACK(oldsize - asize, 0));

                insert_free(next);
            }
            else{
                PUT(HDRP(ptr), PACK(oldsize, 1));
                PUT(FTRP(ptr), PACK(oldsize, 1));

            }
 
            return ptr;
        }
    }

/* tradeoff 심함
    //뒤 확장
    if(!GET_ALLOC(HDRP(prev))){
        size_t combined = oldsize + GET_SIZE(HDRP(prev));
        if(combined >=asize){
            size_t copySize = oldsize - DSIZE;

            PUT(HDRP(prev), PACK(combined, 0));
            PUT(FTRP(prev), PACK(combined, 0));
            memmove(prev, ptr, copySize);
            place(prev, asize);

            return prev;
        }
        else if(!GET_ALLOC(HDRP(next))){
            combined += GET_SIZE(HDRP(next));
            if(combined >= asize){
                size_t copySize = oldsize - DSIZE;

                PUT(HDRP(prev), PACK(combined, 0));
                PUT(FTRP(prev), PACK(combined, 0));
                memmove(prev, ptr, copySize);
                place(prev, asize);

                return prev;
            }
        }
    }
 */

    //기존 정책
    void *newptr = mm_malloc(size);

    if(newptr == NULL)
        return NULL;

    size_t old_payload = oldsize - DSIZE;
    size_t copySize = old_payload < size ? old_payload : size;

    memcpy(newptr, ptr, copySize);
    mm_free(ptr);

    return newptr;
}
