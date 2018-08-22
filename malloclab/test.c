#include <stdio.h>
#include "mm.h"


int testAllign(char * ptr) {
  return ((long)ptr % 8) ? 0 : 1;
}

int main(int argc, char **argv) {
  char * test1 = mm_malloc(1000);
  testAllign(test1);
  char * test2 = mm_malloc(1000);
  testAllign(test2);
  char * test3 = mm_malloc(1000);
  testAllign(test3);
  char * test4 = mm_malloc(1000);
  testAllign(test4);
  mm_free(test4);
  //char * test5 = mm_malloc(1000);
  //testAllign(test5);
  mm_free(test3);
  mm_free(test2);
  mm_free(test1);
  test1 = mm_malloc(1000);
  testAllign(test1);
  test2 = mm_malloc(1000);
  testAllign(test2);
  test3 = mm_malloc(1000);
  testAllign(test3);
  test4 = mm_malloc(1000);
  testAllign(test4);
  mm_free(test4);
  //char * test5 = mm_malloc(1000);
  //testAllign(test5);
  mm_free(test3);
  mm_free(test2);
  mm_free(test1);
  return 0;
} 