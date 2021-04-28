# my_malloc
A custom implementation of C malloc/free function.

I use a linked list to keep track of used/free
blocks of memory. To allocate a user new memory, a
first fit algorithm is used before requesting more
memory from the OS. When possible, neighboring
free blocks are coalesced into one in order to
reduce external fragmentation.
