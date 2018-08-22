/*
 * mm_malloc using an implicit free list. 
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

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

//global variabls
//heap pointer, points to the first byte of the first block
char * heapPtr;
int initialize = 0;
struct record {
    char ** allocatedBlocks[1024];
    int allocatedEntries;
};

struct record r = {.allocatedEntries = 0};

//functions declaration
static char * findFit(size_t size);
static char * extendHeap(size_t size);
static void prepareBlock(char * ptr, size_t size, int allocation);
static char * chunkBlock(char * ptr, size_t size);
static char * coalesce(char * ptr);
void shrinkHeap(char * ptr);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    initialize = 1;
    if((heapPtr = extendHeap(5*WSIZE)) == NULL) {
        return -1;
    }
    PUT(heapPtr, 0);    //alignment padding
    PUT(heapPtr + WSIZE, PACK(WSIZE, 1));      //prologue blocks
    PUT(heapPtr + 2*WSIZE, PACK(0,1));         //epilogue header
    heapPtr += 3*WSIZE;
    //build first block
    if((heapPtr = extendHeap(CHUNKSIZE)) == NULL) {
        return -1;
    }
    return 0;
}

/* 
 * mm_malloc - Allocate a block by finding the first fit in the heap
 */
void *mm_malloc(size_t size)
{
    if(!initialize) {
        mm_init();
        initialize = 1;
    }
    size_t newsize;
    if(size == 0) return NULL;
    size += DSIZE;
    newsize = ALIGN(size + SIZE_T_SIZE);
    assert(newsize % 8 == 0);
    char * ptr = findFit(newsize);
    r.allocatedBlocks[++r.allocatedEntries] = ptr;
    return (void*)ptr;
}

/*
 * mm_free - Frees a block
 */
void mm_free(void *ptr)
{
    char * temp;
    prepareBlock(ptr, GET_SIZE(GET_HEADER(ptr)),0);
    temp = coalesce(ptr);
    if((GET_SIZE(GET_HEADER(temp)) + temp - WSIZE) == mem_heap_current()) {
        shrinkHeap(temp);
    }
    if(temp == mem_heap_lo() + 4*DSIZE) {
        mem_deinit();
        initialize = 0;
    }
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
    blockptr += WSIZE;
    PUT(GET_HEADER(blockptr), PACK(size,0));
    PUT(GET_FOOTER(blockptr), PACK(size,0));
    PUT(GET_HEADER(GETNXTBLK(blockptr)), PACK(0,1));
    return blockptr;
}

static void prepareBlock(char * ptr, size_t size, int allocation) {
    PUT(GET_HEADER(ptr), PACK(size, allocation));
    PUT(GET_FOOTER(ptr), PACK(size, allocation));
}

static char * findFit(size_t size) {
    int fitFound = 0;
    char * tempPtr = heapPtr;
    char * newBlockptr;
    size_t currentBS;
    int allocation;
    int counter = 0;
    while(!fitFound) {
        currentBS = GET_SIZE(GET_HEADER(tempPtr));
        allocation = GET_ALLOC(GET_HEADER(tempPtr));
        if((!allocation) && (currentBS >= size)) {
            return chunkBlock(tempPtr, size);
        }
        //check if there is enough space in the heap and extend otherwise
        int heapSize = (int)((void*)(mem_heap_current()) - (void*)GET_FOOTER(tempPtr));
        assert(heapSize >= 0);
        if(heapSize < size) {
            if((newBlockptr = extendHeap(CHUNKSIZE)) == NULL) {
                return NULL;
            }
            assert(newBlockptr == mem_heap_current() - CHUNKSIZE + WSIZE);
            coalesce(newBlockptr);
        }
        tempPtr = GETNXTBLK(tempPtr);
        assert((long)tempPtr % 8 == 0);
    }
    return NULL;
}

static char * chunkBlock(char * ptr, size_t size) {
    int difference = GET_SIZE(GET_HEADER(ptr)) - size;
    if(difference != 0 && difference > DSIZE) {
        prepareBlock(ptr, size, 1);
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

    } else if(!prevBlockAlloc && !nextBlockAlloc) {
        newSize = GET_SIZE(GET_HEADER(GETPRVBLK(ptr))) 
        + GET_SIZE(GET_HEADER(GETNXTBLK(ptr))) 
        + GET_SIZE(GET_HEADER(ptr));
        PUT(GET_HEADER(GETPRVBLK(ptr)), PACK(newSize,0));
        PUT(GET_FOOTER(GETNXTBLK(ptr)), PACK(newSize,0));
        ptr = GETPRVBLK(ptr);

    } else if(!prevBlockAlloc && nextBlockAlloc) {
        newSize = GET_SIZE(GET_HEADER(GETPRVBLK(ptr)))+ GET_SIZE(GET_HEADER(ptr));
        PUT(GET_HEADER(GETPRVBLK(ptr)), PACK(newSize,0));
        PUT(GET_FOOTER(ptr), PACK(newSize,0));
        ptr = GETPRVBLK(ptr);

    } else {
        newSize = GET_SIZE(GET_HEADER(GETNXTBLK(ptr)))+ GET_SIZE(GET_HEADER(ptr));
        PUT(GET_HEADER(ptr), PACK(newSize,0));
        PUT(GET_FOOTER(GETNXTBLK(ptr)), PACK(newSize,0));
    }
    assert((long)ptr % ALIGNMENT == 0);
    return ptr;

}


void shrinkHeap(char * ptr) {
    size_t s = GET_SIZE(GET_HEADER(ptr)) * -1;
    mem_sbrk(s);
    PUT(ptr - WSIZE, PACK(0,1));
}








