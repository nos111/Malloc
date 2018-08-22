/*
 * memlib.c - a module that simulates the memory system.  Needed because it 
 *            allows us to interleave calls from the student's malloc package 
 *            with the system's malloc package in libc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#include "memlib.h"

#define HEAP_SIZE 2147483648
/* private variables */
static char *mem_start_brk;  /* points to first byte of heap */
static char *mem_current_brk;  /* points to current byte of heap */
static char *mem_brk;        /* points to last byte of heap */
static char *mem_max_addr;   /* largest legal heap address */ 
static int initialize = 0;

void unix_error(char *msg);
/* 
 * mem_init - initialize the memory system model
 */
void mem_init(void)
{
    initialize = 1;
    if((mem_start_brk = sbrk(HEAP_SIZE)) == -1) unix_error("sbrk error");  
    mem_brk = sbrk(0);
    mem_current_brk = mem_start_brk + 8;
    assert(mem_start_brk == mem_brk - HEAP_SIZE);             
}

/* 
 * mem_deinit - free the storage used by the memory system model
 */
void mem_deinit(void)
{
    if(brk(mem_start_brk) == -1) unix_error("brk error"); 
    initialize = 0;  
    char * buffer = "Freed all allocated storange \n";
    write(1, buffer, 30);
}

/*
 * mem_reset_brk - reset the simulated brk pointer to make an empty heap
 */
void mem_reset_brk()
{
    mem_brk = mem_start_brk;
}

/* 
 * mem_sbrk - simple model of the sbrk function. Extends the heap 
 *    by incr bytes and returns the start address of the new area. In
 *    this model, the heap cannot be shrunk.
 */
void *mem_sbrk(int incr) 
{
    if(!initialize) {
        mem_init();
    }
    char *old_brk = mem_current_brk;
    mem_current_brk += incr;
    if(mem_current_brk > mem_heap_hi()) {
        mem_current_brk -= incr;
        return (void*)-1;
    }
    //printf("mem_current %u mem_start %u \n", mem_current_brk, mem_start_brk);
    if(mem_current_brk == mem_start_brk + 8) {
        mem_deinit();
    }
    assert(old_brk + incr == mem_current_brk);
    return (void *)old_brk;
}

/*
 * mem_heap_lo - return address of the first heap byte
 */
void *mem_heap_lo()
{
    return (void *)mem_start_brk;
}

/* 
 * mem_heap_hi - return address of last heap byte
 */
void *mem_heap_hi()
{
    return (void *)(mem_brk - 1);
}

void * mem_heap_current() {
    return (void*)(mem_current_brk);
}
/*
 * mem_heapsize() - returns the heap size in bytes
 */
size_t mem_heapsize() 
{
    return (size_t)(mem_brk - mem_start_brk);
}

/*
 * mem_pagesize() - returns the page size of the system
 */
size_t mem_pagesize()
{
    return (size_t)getpagesize();
}


void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

void lowerHighPtr(size_t inc) {
    mem_brk += inc;
}