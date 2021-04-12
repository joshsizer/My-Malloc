// Joshua Sizer (jas625)
#include <stdio.h>
#include <unistd.h>

#include "mymalloc.h"

typedef struct Value {
  int value;
} Value;

int main() {
  // You can use sbrk(0) to get the current position of the break.
  // This is nice for testing cause you can see if the heap is the same size
  // before and after your tests, like here.
  void* heap_at_start = sbrk(0);

  // void* block =

  // my_free(block);
  Value* value = (Value*)my_malloc(sizeof(Value));
  value->value = 30;

  Value* value2 = (Value*)my_malloc(sizeof(Value));
  value2->value = 100;
  Value* value3 = (Value*)my_malloc(sizeof(Value));
  value3->value = -222;
  void* myptr = my_malloc(100);
  Value* value4 = (Value*)my_malloc(sizeof(Value));
  value4->value = -222;
  my_free(myptr);

  Value* value5 = (Value*)my_malloc(sizeof(Value));
  value5->value = -222;

  my_free(value);
  my_free(value2);
  my_free(value3);
  my_free(value4);
  my_free(value5);

  // printf("Value: %d\n", value->value);
  // printf("Value: %d\n", value2->value);

  void* heap_at_end = sbrk(0);
  unsigned int heap_size_diff = (unsigned int)(heap_at_end - heap_at_start);

  if (heap_size_diff)
    printf("Hmm, the heap got bigger by %u (0x%X) bytes...\n", heap_size_diff,
           heap_size_diff);

  // ADD MORE TESTS HERE.

  return 0;
}
