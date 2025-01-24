# 7-malloc
Overview
---------------------------------------------------------------------------------------------------
Our malloc project has six main functions in mm.c (mm_init, mm_split, mm_malloc, mm_coalesce, mm_free, and mm_realloc), as well as the given inline funtions, which we filled in. 
* mm_init: this function initializes the flist_first variable to be NULL and creates space in the heap for the prologue and epilogue global variables, which bookend our memory allocation heap. 
* mm_split: this function helps with splitting an allocated block with excess free space 
* mm_malloc: this function handles the initial memory allocation. We iterate through the free list to find a block that has a size greater than or equal to the passed-in size. If so, it allocates that block and returns it, but if there is not a big enough block, we call mem_sbrk to add more space to the heap. We then move the epilogue to the correct position. 
* mm_coalesce: this function takes in a pointer to a block. If this passed-in block (which will always be free due to our logical organization), is adjacent to a free block (either the previous or next block is free), then we combine the multiple free blocks into one free block. 
* mm_free: this function takes in a pointer to a payload, so we use an inline function to get the whole block. We set the allocation of this block to be 0 (indicating that it is free) and insert it into the free list. We then call our coalesce helper function on the newly freed block (so we always know that the block we pass into the coalesce function is free) to combine all free blocks that are next to each other in the heap. 
* mm_realloc: this function takes in a pointer to a payload (which we use to get the actual block) and a requested size. If the passed-in pointer is NULL, we treat this as a normal malloc case and return the pointer that malloc returns, passing in the passed-in size. If the passed-in size is 0, then we treat this as a normal free case and call our free function and return NULL from the realloc function. If the passed-in size is greater than the size of the passed-in block, then we free the passed-in block (which allows it to coalesce with any free blocks around it) and we call malloc with the passed-in size, which allows us to extend the heap if necessary. We call memmove to move the data from the block that we pass in to the block returned by our malloc call, and we return a pointer to the newly filled payload of the new block (returned by malloc). If the passed-in size is smaller than size of the passed-in block, we do nothing, as per the handout.

Maintaining Compaction: 
---------------------------------------------------------------------------------------------------
In order to prevent the heap from turning into a bunch of tiny free blocks, we compare the passed-in size argument and MINBLOCKSIZE at the beginning of our malloc function, and use the maximum of the two values. This value is then passed into our mem_sbrk calls to extend the heap, and this ensures that our heap extensions are the right size. Our coalesce helper function also helps maintain compaction: in this function, we pass in a free block and compare it to its previous and next blocks. If one or both of these blocks are free, then we combine them with the block that we passed in, so we end up with one big free block, rather than multiple smaller free blocks. 

mm_realloc() implementation strategy: 
---------------------------------------------------------------------------------------------------
In this function, we checked the previous and next blocks to utilize nearby free blocks before searching through the entire free list and possibly extending the heap (pseudo-coalescing, if you will). This way, we minimize the amount of times we have to loop through the free list and call mem_sbrk (withinn the call to malloc). Another optimizaiton that we implemented was setting the size to allocate to the maximum value between the passed-in size and the MINBLOCKSIZE. This optimized our code because if the requested size was too small, we would end up with a free block that could not be stored in the free list (no room for the flink and the blink). If this excess space is less than MINBLOCKSIZE, we don't split the allocated block, so that we don't have a block that is unreasonable small in the free list.

Strategy to Achieve High Throughput: 
---------------------------------------------------------------------------------------------------
To achieve a high throughput, we wanted to maximize the number of operations completed per second by optimizing our code. A notable way that we optimized is by taking the maximum of the requested size and MINBLOCKSIZE values when determining the size to allocate (as described in the section above). We also made sure that MINBLOCKSIZE was of reasonable size (not too large because this would lead to unused allocated space that could have been free). Coalescing also optimized our code by joinung all adjacent free blocks so that we were able to avoid extending the heap unnecessarily by fitting in allocations into coalesced blocks. 

Unresolved Bugs: 
---------------------------------------------------------------------------------------------------
None

Any Other Optimizations: 
---------------------------------------------------------------------------------------------------
N/A (all described above)