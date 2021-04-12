// Joshua Sizer (jas625)
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mymalloc.h"

// USE THIS GODDAMN MACRO OKAY
#define PTR_ADD_BYTES(ptr, byte_offs) ((void*)(((char*)(ptr)) + (byte_offs)))

// Don't change or remove_from_list these constants.
#define MINIMUM_ALLOCATION 16
#define SIZE_MULTIPLE 8

#define FREE 1
#define TAKEN 0

typedef struct Block Block;
/**
 * Total size: 16 (0x10) bytes
 **/
struct Block {
  Block* next;
  Block* last;
  uint32_t is_free;
  uint32_t data_size;
};

// global variables for head and tail
Block* head = NULL;
Block* tail = NULL;

unsigned int round_up_size(unsigned int data_size) {
  if (data_size == 0)
    return 0;
  else if (data_size < MINIMUM_ALLOCATION)
    return MINIMUM_ALLOCATION;
  else
    return (data_size + (SIZE_MULTIPLE - 1)) & ~(SIZE_MULTIPLE - 1);
}

void* get_data_pointer(Block* block) {
  return PTR_ADD_BYTES(block, sizeof(Block));
}

/**
 *  This will return a pointer to the first free block available that is
 *  at least as big as size. This does not change the state of the heap
 *  at all. If the block is too big, another function must handle that.
 *
 *  @return a pointer to the first free block or NULL if there are no
 *          free blocks
 **/
Block* find_free_block(uint32_t size) {
  for (Block* cur = head; cur != NULL; cur = cur->next) {
    if (cur->is_free == FREE && cur->data_size >= size) {
      return cur;
    }
  }
  return NULL;
}

void add_block_after(Block* prev_block, uint32_t size, uint32_t is_free) {
  void* new_pointer =
      PTR_ADD_BYTES(get_data_pointer(prev_block), prev_block->data_size);
  Block* new_block = (Block*)new_pointer;

  new_block->is_free = is_free;
  new_block->data_size = size;
  new_block->last = prev_block;
  new_block->next = prev_block->next;

  if (prev_block->next != NULL) {
    prev_block->next->last = new_block;
  }
  prev_block->next = new_block;
}

/**
 * Splits the block if necessary, and marks the block as free
 **/
void update_block(Block* free_block, uint32_t size) {
  free_block->is_free = TAKEN;

  uint32_t size_left_over = free_block->data_size - size;
  uint32_t minimum_block_size = sizeof(Block) + MINIMUM_ALLOCATION;
  if (size_left_over <= minimum_block_size) {
    return;
  }
  free_block->data_size = size;

  uint32_t new_block_data_size = size_left_over - sizeof(Block);

  add_block_after(free_block, new_block_data_size, FREE);
}

Block* add_to_list(uint32_t size) {
  if (head == NULL && tail != NULL) {
    printf("ERROR in add_new_block: head is NULL but tail is not!\n");
    return NULL;
  } else if (tail == NULL && head != NULL) {
    printf("ERROR in add_new_blocK: tail is NULL but head is not!\n");
    return NULL;
  }

  // expand the heap
  void* memory_address = sbrk(sizeof(Block) + size);

  Block* prev_tail = tail;
  tail = (Block*)memory_address;
  tail->data_size = size;
  tail->is_free = TAKEN;
  tail->next = NULL;
  // if tail is NULL, head should also be NULL so we forgoe the check
  // on head to save execution time
  if (prev_tail == NULL) {
    // set our head and tail pointers to point to the newly allocated
    // block of memory
    head = (Block*)memory_address;
    tail->last = NULL;
    // set the value of head to be the newly created block
  } else {
    tail->last = prev_tail;
    prev_tail->next = tail;
  }

  return tail;
}

void remove_from_list(Block* block) {
  // unlink ourselves
  if (block->last != NULL) {
    block->last->next = block->next;
  }

  if (block->next != NULL) {
    block->next->last = block->last;
  }

  if (block == head) {
    head = block->next;
  }

  if (block == tail) {
    tail = block->last;
  }
}

void contract_heap(Block* block) { brk(block); }

void print_head_and_tail() { printf("HEAD: %p\nTAIL: %p\n", head, tail); }

void print_block(Block* block) {
  printf("LAST: %p, THIS: %p, NEXT: %p, FREE?: %u, DATA_SIZE: %u\n",
         block->last, block, block->next, block->is_free, block->data_size);
}

void print_linked_list() {
  for (Block* cur = head; cur != NULL; cur = cur->next) {
    print_block(cur);
  }
}

/**
 * Removes the input block and returns a pointer to the block on its left
 **/
Block* remove_block(Block* block) {
  if (block->next != NULL) {
    block->last->next = block->next;
    block->next->last = block->last;
  } else {
    tail = block->last;
    block->last->next = NULL;
  }
  block->last->data_size =
      block->last->data_size + sizeof(Block) + block->data_size;
  return block->last;
}

Block* coalesce(Block* block) {
  if (block->last != NULL && block->last->is_free == FREE) {
    block = remove_block(block);
  }

  if (block->next != NULL && block->next->is_free == FREE) {
    block = remove_block(block->next);
  }

  return block;
}

void* my_malloc(unsigned int size) {
  if (size == 0) return NULL;

  size = round_up_size(size);

  // ------- Don't remove_from_list anything above this line. -------
  // Here's where you'll put your code!

  Block* free_block = find_free_block(size);

  if (free_block != NULL) {
    update_block(free_block, size);
  } else {
    free_block = add_to_list(size);
    if (free_block == NULL) {
      printf("ERROR in my_malloc: could not allocate new block!\n");
      return NULL;
    }
  }

  return get_data_pointer(free_block);
}

void my_free(void* ptr) {
  if (ptr == NULL) return;

  Block* free_block = (Block*)PTR_ADD_BYTES(ptr, -1 * sizeof(Block));
  free_block->is_free = FREE;

  Block* after_coalesce = coalesce(free_block);

  if (after_coalesce == tail) {
    remove_from_list(after_coalesce);
    contract_heap(after_coalesce);
  }
}
