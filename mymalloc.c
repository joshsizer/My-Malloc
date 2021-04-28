/**
 * Author: Joshua Sizer
 *
 * This is a custom implementation of malloc/free.
 * I use a linked list to keep track of used/free
 * blocks of memory. To allocate a user new
 * memory, a first fit algorithm is used before
 * requesting more memory from the OS. When possible,
 * neighboring free blocks are coalesced into one
 * in order to reduce external fragmentation.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mymalloc.h"

// easy way to add some number of bytes to a
// pointer
#define PTR_ADD_BYTES(ptr, byte_offs) ((void *)(((char *)(ptr)) + (byte_offs)))

#define MINIMUM_ALLOCATION 16
#define SIZE_MULTIPLE 8

#define FREE 1
#define TAKEN 0

typedef struct Block Block;

// Total size: 16 (0x10) bytes
struct Block
{
  Block *next;
  Block *last;
  uint32_t is_free;
  uint32_t data_size;
};

// Global variables for head and tail
Block *head = NULL;
Block *tail = NULL;

/**
 * Round's a given value up to the next multiple
 * of SIZE_MULTIPLE (which is 8 right now). If a
 * multiple of 8 is given, then that same value is
 * returned.
 * 
 * If the value given is less than MINIMUM_ALLOCATION,
 * MINIMUM_ALLOCATION is returned (which is 16
 * right now).
 * 
 * @param data_size the given value to round up
 * @return the next multiple of SIZE_MULTIPLE that
 * is bigger than data_size
 */
unsigned int round_up_size(unsigned int data_size)
{
  if (data_size == 0)
    return 0;
  else if (data_size < MINIMUM_ALLOCATION)
    return MINIMUM_ALLOCATION;
  else
    return (data_size + (SIZE_MULTIPLE - 1)) & ~(SIZE_MULTIPLE - 1);
}

/**
 * Get the pointer for where a block's data is
 * stored.
 *
 * @param block the block we want a pointer to the
 * data for
 */
void *get_data_pointer(Block *block)
{
  return PTR_ADD_BYTES(block, sizeof(Block));
}

/**
 *  Find the next block that is big enough to hold
 *  the given data size.
 *
 *  Return a pointer to the first free block
 *  available that is at least as big as size.
 *  This does not change the state of the heap at
 *  all. If the block is too big, another function
 *  must handle that.
 *
 *  @return a pointer to the first free block or
 *          NULL if there are no free blocks
 **/
Block *find_free_block(uint32_t size)
{
  // Search through our linked list
  for (Block *cur = head; cur != NULL; cur = cur->next)
  {
    // Return the current block as long as it's
    // data_size is large enough
    if (cur->is_free == FREE && cur->data_size >= size)
    {
      return cur;
    }
  }
  return NULL;
}

/**
 * Create a new block that comes directly after
 * the given block in memory.
 *
 * This function makes sure to update the next and
 * last pointers to maintain correctness in the
 * linked list.
 *
 * @param prev_blocK the block we want to add a
 * new block after
 * @param size the data size of the new block
 * @param is_free whether the new block is free or
 * taken
 */
void add_block_after(Block *prev_block, uint32_t size, uint32_t is_free)
{
  void *new_pointer =
      PTR_ADD_BYTES(get_data_pointer(prev_block), prev_block->data_size);
  Block *new_block = (Block *)new_pointer;

  new_block->is_free = is_free;
  new_block->data_size = size;
  new_block->last = prev_block;
  new_block->next = prev_block->next;

  if (prev_block->next != NULL)
  {
    prev_block->next->last = new_block;
  }
  prev_block->next = new_block;
}

/**
 * Change a given block's size and mark as taken.
 *
 * If the changed size allows for enough room left
 * over to store the Block struct +
 * MINIMUM_ALLOCATION, split the block into two.
 * Mark the new block as FREE.
 *
 * @param free_block the block to update as taken,
 * and to split into two if necessary
 * @param size the block to update's new size
 */
void update_block(Block *free_block, uint32_t size)
{
  free_block->is_free = TAKEN;

  uint32_t size_left_over = free_block->data_size - size;
  uint32_t minimum_block_size = sizeof(Block) + MINIMUM_ALLOCATION;
  if (size_left_over <= minimum_block_size)
  {
    return;
  }
  free_block->data_size = size;

  uint32_t new_block_data_size = size_left_over - sizeof(Block);

  add_block_after(free_block, new_block_data_size, FREE);
}

/**
 * Add a new block with data_size of size to the
 * linked list of blocks. Works on both an empty
 * and non-empty linked list.
 *
 * @param size the requested data_size
 */
Block *add_to_list(uint32_t size)
{
  // Either head and tail should be NULL, or they
  // should both not be null
  if (head == NULL && tail != NULL)
  {
    printf("ERROR in add_new_block: head is NULL but tail is not!\n");
    return NULL;
  }
  else if (tail == NULL && head != NULL)
  {
    printf("ERROR in add_new_blocK: tail is NULL but head is not!\n");
    return NULL;
  }

  // Expand our heap
  void *memory_address = sbrk(sizeof(Block) + size);

  // Create a new block where we've expanded the
  // heap
  Block *prev_tail = tail;
  tail = (Block *)memory_address;
  tail->data_size = size;
  tail->is_free = TAKEN;
  tail->next = NULL;

  // If tail is NULL, head should also be NULL.
  // This means our linked list of blocks has size
  // 0
  if (prev_tail == NULL)
  {
    // Set our head and tail pointers to point to
    // the newly allocated block of memory
    head = (Block *)memory_address;
    tail->last = NULL;
    // set the value of head to be the newly
    // created block
  }
  else
  {
    // Update the next/last pointers to maintain
    // correctness of the linked list
    tail->last = prev_tail;
    prev_tail->next = tail;
  }

  return tail;
}

/**
 * Remove a block from the linked list data
 * structure.
 *
 * @param block the block to remove from the
 * linked list data structure
 */
void remove_from_list(Block *block)
{
  // Unlink ourselves
  if (block->last != NULL)
  {
    block->last->next = block->next;
  }

  if (block->next != NULL)
  {
    block->next->last = block->last;
  }

  if (block == head)
  {
    head = block->next;
  }

  if (block == tail)
  {
    tail = block->last;
  }
}

/**
 * Simple wrapper for called brk().
 *
 * @param block the starting memory address to
 * relinquish to the OS
 */
void contract_heap(Block *block) { brk(block); }

/**
 * Print the address of our head and tail
 * pointers. A helper function for debugging
 * purposes.
 */
void print_head_and_tail() { printf("HEAD: %p\nTAIL: %p\n", head, tail); }

/**
 * Print the attributes of a given Block data
 * structure. A helper function for debugging
 * purposes.
 *
 * @param block the block to print
 */
void print_block(Block *block)
{
  printf("LAST: %p, THIS: %p, NEXT: %p, FREE?: %u, DATA_SIZE: %u\n",
         block->last, block, block->next, block->is_free, block->data_size);
}

/**
 * Print the entire linked list of blocks. Helper
 * function for debugging purposes.
 */
void print_linked_list()
{
  for (Block *cur = head; cur != NULL; cur = cur->next)
  {
    print_block(cur);
  }
}

/**
 * Special coalesce case for combining two
 * adjacent blocks into one. Specifically,
 * combines the right block with the block on its
 * left.
 *
 * @param block the block to remove and combine
 * with its block to the left
 */
Block *remove_block(Block *block)
{
  if (block->next != NULL)
  {
    block->last->next = block->next;
    block->next->last = block->last;
  }
  else
  {
    tail = block->last;
    block->last->next = NULL;
  }
  block->last->data_size =
      block->last->data_size + sizeof(Block) + block->data_size;
  return block->last;
}

/**
 * Combine a block with its left and right
 * neighbors depending on if the neighbors are
 * free.
 */
Block *coalesce(Block *block)
{
  // If the block to the left is free, combine
  if (block->last != NULL && block->last->is_free == FREE)
  {
    block = remove_block(block);
  }

  // If the block to the right is free, combine
  if (block->next != NULL && block->next->is_free == FREE)
  {
    block = remove_block(block->next);
  }

  return block;
}

/**
 * Allocate memory of a given size.
 *
 * @param size the number of bytes to allocate
 * @return a void pointer pointing to the newly
 * allocated memory
 */
void *my_malloc(unsigned int size)
{
  if (size == 0)
    return NULL;

  // Ensure our size is correctly aligned.
  // In other words, a request for 17 bytes is
  // rounded up to 24. A request for 25 bytes is
  // rounded up to 32. 34 is rounded to 40. etc.
  // Anything below 16 bytes is rounded to 16
  size = round_up_size(size);

  // First fit algorithm tries to find the first
  // free block that could fit our requested size
  Block *free_block = find_free_block(size);

  // If we've found a free block, then we should
  // update the block to be TAKEN and to have the
  // correct size. If the requested size is less
  // than the free block's size, the block is
  // split as long as there exists enough extra
  // space for a new block struct and
  // MINIMUM_ALLOCATION bytes.
  if (free_block != NULL)
  {
    update_block(free_block, size);
  }
  else
  {
    // If we could find no free block, then we
    // attempt to add a block to the end of our
    // linked list by asking the OS for more heap
    // space.
    free_block = add_to_list(size);

    // If we couldn't add a new block to the end
    // of our linked list, something has gone
    // quite wrong.
    if (free_block == NULL)
    {
      printf("ERROR in my_malloc: could not allocate new block!\n");
      return NULL;
    }
  }

  // Finally, return the address of our
  // updated/newly allocated block's data
  // segment.
  return get_data_pointer(free_block);
}

/**
 * Relinquish allocated memory to be reallocated later.
 * 
 * @param ptr A pointer to the section of memory
 * to free
 */
void my_free(void *ptr)
{
  if (ptr == NULL)
    return;

  // Get the location of the given memory's Block
  // structure.
  Block *free_block = (Block *)PTR_ADD_BYTES(ptr, -1 * sizeof(Block));

  // Mark that block as free
  free_block->is_free = FREE;

  // Attempt to coalesce. AKA, combine neighboring
  // blocks that are all free so as to lessen the
  // extent of external fragmentation.
  Block *after_coalesce = coalesce(free_block);

  // After coalescing, the remaining block might
  // be our tail. If that is the case, we can
  // signal to the OS that it can take back some
  // of our heap memory.
  if (after_coalesce == tail)
  {
    remove_from_list(after_coalesce);
    contract_heap(after_coalesce);
  }
}
