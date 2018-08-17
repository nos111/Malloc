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
    "Nour",
    /* First member's full name */
    "Nour saffour",
    /* First member's email address */
    "nsaffour@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

#define WSIZE 4

#define DSIZE 8

#define CHUNKSIZE (4096)


/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//pack the size and allocated bit to a word
#define PACK(size, alloc) ((size) | (alloc))

//get and put into memory location p
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, value) (*(unsigned int *)(p) = (value))

//get the size and alocated bit of a pointer p
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

//get the header and footer of a block 
#define GET_HEADER(p) ((char *)(p) - WSIZE)
#define GET_FOOTER(p) ((char *)(p) + GET_SIZE(GET_HEADER(p)) - DSIZE)

//get the next/previous block ptr 
#define GETNXTBLK(p) ((char *)(p) + GET_SIZE(GET_HEADER(p)))
#define GETPRVBLK(p) ((char *)(p) - GET_SIZE((p) - DSIZE))

//heap pointer, points to the first byte of the first block
char * heapPtr;

//functions declaration
static char * findFit(size_t size);
static char * extendHeap(size_t size);
static void prepareBlock(char * ptr, size_t size, int allocation);
static char * chunkBlock(char * ptr, size_t size);
static char * coalesce(char * ptr);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heapPtr = extendHeap(4*WSIZE)) == NULL) {
        return -1;
    }
    //printf("heap pointer at init %u", heapPtr);
    PUT(heapPtr, 0);    //alignment padding
    PUT(heapPtr + WSIZE, PACK(WSIZE, 1));      //prologue blocks
    PUT(heapPtr + 2*WSIZE, PACK(0,1));         //epilogue header
    heapPtr += 3*WSIZE;
    //build first block
    if((heapPtr = extendHeap(CHUNKSIZE)) == NULL) {
        return -1;
    }
    //printf("heap pointer at init %u", heapPtr);
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t newsize;
    if(size == 0) return NULL;
    //printf("requested size %u \n", size);
    size += DSIZE;
    newsize = ALIGN(size + SIZE_T_SIZE);
    //printf("aligned size %u \n", newsize);
    
    char * ptr = findFit(newsize);
    //printf("the returned address from malloc is %u \n", ptr);
    return (void*)ptr;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

static char * extendHeap(size_t size) {
    char * blockptr;
    if((void*)(blockptr = mem_sbrk(size)) == (void *)-1) {
        return NULL;
    }
    PUT(GET_HEADER(blockptr), PACK(size,0));
    PUT(GET_FOOTER(blockptr), PACK(size,0));
    PUT(GET_HEADER(GETNXTBLK(blockptr)), PACK(0,1));
    return blockptr;
}

static void prepareBlock(char * ptr, size_t size, int allocation) {
    PUT(GET_HEADER(ptr), PACK(size, allocation));
    //printf("wrote %d %d at location %u", size, allocation, GET_HEADER(ptr));
    PUT(GET_FOOTER(ptr), PACK(size, allocation));
    //printf("wrote %d %d at location %u", size, allocation, GET_FOOTER(ptr));
}

static char * findFit(size_t size) {
    int fitFound = 0;
    char * tempPtr = heapPtr;
    char * newBlockptr;
    size_t currentBS;
    int allocation;
    while(!fitFound) {
        ////printf("the temp address is %u \n", tempPtr);
        currentBS = GET_SIZE(GET_HEADER(tempPtr));
        allocation = GET_ALLOC(GET_HEADER(tempPtr));
        //printf("the blocksize is %u \n", currentBS);
        //printf("the allocation is %u \n", allocation);
        if((!allocation) && (currentBS >= size)) {
            //printf("chunked block \n");
            return chunkBlock(tempPtr, size);
        }
        //check if there is enough space in the heap and extend otherwise
        if(((void*)(mem_heap_hi()) - (void*)GET_FOOTER(tempPtr)) < size) {
            if((newBlockptr = extendHeap(CHUNKSIZE)) == NULL) {
                return NULL;
            }
            ////printf("the newblockptr address is %u \n", newBlockptr);
            //prepareBlock(newBlockptr - 16, CHUNKSIZE, 0);
            ////printf("the prev block size %d allocation %d \n",GET_SIZE(newBlockptr),GET_ALLOC(newBlockptr));
            coalesce(newBlockptr);
        }
        ////printf("the footer address is %u \n", GET_FOOTER(tempPtr));
        ////printf("the high address is %u \n", mem_heap_hi());
        ////printf("the difference is %d \n", ((void*)(mem_heap_hi()) - (void*)GET_FOOTER(tempPtr)));

        tempPtr = GETNXTBLK(tempPtr);

    }
    return NULL;
}

static char * chunkBlock(char * ptr, size_t size) {
    int difference = GET_SIZE(GET_HEADER(ptr)) - size;
    if(difference != 0 && difference > DSIZE) {
        //printf("made block size %u ptr %u \n", size, ptr);
        prepareBlock(ptr, size, 1);
        //printf("made block size %u ptr %u \n", difference, GETNXTBLK(ptr));
        prepareBlock(GETNXTBLK(ptr), difference, 0);
        return ptr;
    }
    prepareBlock(ptr, size + difference, 1);
    return ptr;
}

static char * coalesce(char * ptr) {
    size_t newSize;
    size_t prevBlockAlloc = GET_ALLOC(GET_HEADER(GETPRVBLK(ptr)));
    size_t nextBlockAlloc = GET_ALLOC(GET_HEADER(GETNXTBLK(ptr)));

    if(prevBlockAlloc && nextBlockAlloc) {
        //printf("case 1");
        return ptr;

    } else if(!prevBlockAlloc && !nextBlockAlloc) {
        //printf("case 2");
        newSize = GET_SIZE(GET_HEADER(GETPRVBLK(ptr))) 
        + GET_SIZE(GET_HEADER(GETNXTBLK(ptr))) 
        + GET_SIZE(GET_HEADER(ptr));
        PUT(GET_HEADER(GETPRVBLK(ptr)), PACK(newSize,0));
        PUT(GET_FOOTER(GETNXTBLK(ptr)), PACK(newSize,0));
        return GETPRVBLK(ptr);

    } else if(!prevBlockAlloc && nextBlockAlloc) {
        //printf("case 3");
        newSize = GET_SIZE(GET_HEADER(GETPRVBLK(ptr)))+ GET_SIZE(GET_HEADER(ptr));
        PUT(GET_HEADER(GETPRVBLK(ptr)), PACK(newSize,0));
        PUT(GET_FOOTER(ptr), PACK(newSize,0));
        return GETPRVBLK(ptr);

    } else {
        //printf("case 4");
        newSize = GET_SIZE(GET_HEADER(GETNXTBLK(ptr)))+ GET_SIZE(GET_HEADER(ptr));
        PUT(GET_HEADER(ptr), PACK(newSize,0));
        PUT(GET_FOOTER(GETNXTBLK(ptr)), PACK(newSize,0));
        return ptr;
    }

}









