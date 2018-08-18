#include <stdio.h>
#include "mm.h"


int testAllign(char * ptr) {
  return ((long)ptr % 8) ? 0 : 1;
}

int main(int argc, char **argv) {
  char * test1 = mm_malloc(1000);
  if(!testAllign(test1)) {
    printf("Not alligned \n");
    printf("the pointer is %u \n", test1);
  }
  test1[0] = "t";
  char * test2 = mm_malloc(1000);
  char * test3 = mm_malloc(1000);
  char * test4 = mm_malloc(1000);
  mm_free(test4);
  mm_free(test3);
  mm_free(test2);
  mm_free(test1);
  return 0;
}