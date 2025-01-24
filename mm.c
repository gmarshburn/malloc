#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "./memlib.h"
#include "./mm.h"
#include "./mminline.h"

block_t *prologue;
block_t *epilogue;

// rounds up to the nearest multiple of WORD_SIZE
static inline long align(long size) {
    return (((size) + (WORD_SIZE - 1)) & ~(WORD_SIZE - 1));
}

/*
 *                             _       _ _
 *     _ __ ___  _ __ ___     (_)_ __ (_) |_
 *    | '_ ` _ \| '_ ` _ \    | | '_ \| | __|
 *    | | | | | | | | | | |   | | | | | | |_
 *    |_| |_| |_|_| |_| |_|___|_|_| |_|_|\__|
 *                       |_____|
 *
 * initializes the dynamic storage allocator (allocate initial heap space)
 * arguments: none
 * returns: 0, if successful
 *         -1, if an error occurs
 */
int mm_init(void) { 
    prologue = mem_sbrk(TAGS_SIZE); //makes room in the heap for the prologue 

    if(prologue == (void *)-1){ //error checks mem_sbrk
        fprintf(stderr, "i can't rn");
        return -1; 
    }
    block_set_size(prologue, TAGS_SIZE); //size of prologue
    block_set_allocated(prologue, 1); //payload  of prologue

    epilogue = mem_sbrk(TAGS_SIZE); //make room in the heap for the epilogue

    if(epilogue == (void *)-1){ //error checks mem_sbrk
        fprintf(stderr, "you're wrong");
        return -1; 
    }
    block_set_size(epilogue, TAGS_SIZE); //size of epilogue
    block_set_allocated(epilogue, 1); //payload of epilogue

    flist_first = NULL; //no free memory yet so flist is null
    
    return 0; 
}


/*                                       _ _ _
*      _ __ ___  _ __ ___      ___  ___ | (_) |_
*     | '_ ` _ \| '_ ` _ \    /  /|  _ \| | | __|
*     | | | | | | | | | | |   \  \| (_) | | | |_
*     |_| |_| |_|_| |_| |_|___/__/|  __/|_|_|\__|
*                        |_____|  |_|
*
* This helper function splits an allocated block and adds excess free space back to the free list. We kept it relatively short 
* because we call it in both mm_malloc and mm_realloc under different circumstances so it has to be general enough to be helpful    
* arguments: the block that we are allocating (with excess), the total size of the given block, and the requested size to be allocated
* returns: void, internally changes the free list and block allocation                               
*/
 void mm_split(block_t *block, long blocksize, long size){
    block_t *free_block = block_next(block); 
    block_set_size_and_allocated(free_block, blocksize - size, 0); //setting the allocation of the excess to 0 (free)
    insert_free_block(free_block); //adding in excess to the free list 
}
       

 
/*                                             _ _
 *     _ __ ___  _ __ ___      _ __ ___   __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '_ ` _ \ / _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | | | | | (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_| |_| |_|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * This function allocates a block of memory and returns a pointer to that block's payload 
 * arguments: size: the desired payload size for the block
 * returns: a pointer to the newly-allocated block's payload (whose size
 *          is a multiple of ALIGNMENT), or NULL if an error occurred
 */
void *mm_malloc(long size) {
    if(size == 0){ //error checks passed-in size
        return NULL;
    }
    
    size = align(size) + TAGS_SIZE; //the size of the block (not just the payload) 
    
    if(MINBLOCKSIZE > size){ //size is the maximum value of the size of the block (found above), and the MINBLOCKSIZE value 
        size = MINBLOCKSIZE; 
    }
   
    if(flist_first == NULL){ //if there's nothing in the free list

        if(mem_sbrk(size) == (void *)-1){ //extend heap using mem_sbrk, error check below 
            fprintf(stderr, "Unable to allocate memory"); //error checks mem_sbrk
            return NULL;
        }
        //moves the epilogue to the correct new location: after the newly created memory 
        block_set_size_and_allocated(epilogue, size, 1); //the epilogue currently points to the second-to-last block in the heap, which we want to allocate 
        epilogue = block_next(epilogue); //set the epilogue to the last block (correct location) 
        block_set_size_and_allocated(epilogue, TAGS_SIZE, 1); //set epilogue to correct size, and set to allocated 
        return block_prev(epilogue)->payload; //returns a pointer to the newly created memory (the block before the new epilogue)
    }    

    //outside of the above if-statement: can assume that the free list contains something 
    block_t *curr_block = flist_first; //set counter to the first thing in the free list (to keep track of iterations through list)

        do { //do-while loop because we want to check the first block in the list, then keep iterating through until 
            //we reach the first element in the list again (since the free list is circular and doubly linked we will get back to the start)
            if(block_size(curr_block) >= size){ //checks if curr_block has enough size to be allocated, if so: break out of the do-while loop
                break;
            }
            
            curr_block = block_flink(curr_block); //get the next block (iteration) 

            if(curr_block == flist_first){ //gone through the entire list, need to extend the heap manually (mem_sbrk) to create enough space

                if(mem_sbrk(size) == (void *)-1){ //error checks mem_sbrk
                    fprintf(stderr, "Unable to allocate memory");
                    return NULL;
                }
                //moves the epilogue to afetr the newly created memory 
                block_set_size_and_allocated(epilogue, size, 1);
                epilogue = block_next(epilogue);
                block_set_size_and_allocated(epilogue, TAGS_SIZE, 1);
                return block_prev(epilogue)->payload; //returns a pointer to the newly created memory
            }

        } while(curr_block != flist_first); //condition to prevent cycles in free list and loop through list

        long blocksize = block_size(curr_block); //checks conditions for splitting newly allocated block to put excess free space back in free list
        block_t *new_block = NULL;

        if(blocksize - size >= MINBLOCKSIZE){ //if block size is greater than the requested size for allocation 
            pull_free_block(curr_block); //take the current block out of the free list (setting to allocated, no longer free)
            block_set_size_and_allocated(curr_block, size, 1); //set allocation of new block with requested size 
            mm_split(curr_block, blocksize, size); //helper function that puts excess memory in free list 

        } else { //otherwise there's not enough leftovers to make a free block, just allocate the block completely 
            new_block = curr_block;
            pull_free_block(curr_block);
            block_set_allocated(curr_block, 1);
        }
    return curr_block->payload; //returns the payload of the chosen block
}


/*
 *                                              _
 *     _ __ ___  _ __ ___       ___  ___   __ _| | ___  ___  ___  ___
 *    | '_ ` _ \| '_ ` _ \     / __|/ _ \ / _` | |/ _ \/  / / __|/ _ \
 *    | | | | | | | | | | |   | (__| (_) | (_| | |  __/\  \| (__|  __/
 *    |_| |_| |_|_| |_| |_|___ \___|\___/ \__,_|_|\___|/__/ \___|\___|
 *                       |_____|
 *
 * This function checks if a block's previous and next blocks are free, and if so, joins them together into one big free block 
 * arguments: the current block to check the surroundings of (and combine with neighboring free blocks if possible) 
 * returns: void, everything is done to the free list/blocks internally 
*/
void mm_coalesce(block_t *block){

    if(!block_next_allocated(block)){ //checks if the next block is free

        block_t *next = block_next(block); //temporary variable to store next block
        pull_free_block(next); //take the block out of the free list (essentially getting rid of it)
        long aligned_size = align(block_size(block) + block_size(next)); //new size: the size of the block + the size of the next block
        block_set_size_and_allocated(block, aligned_size, 0); //sets block's size to be size of block plus size of block's next's size
            //effectively overwriting the next block (left with one block) 
    }

    if(!block_prev_allocated(block)){ //checks if the previous block is free 

        block_t *prev = block_prev(block); //temporary variable to store previous block 
        pull_free_block(block); //get rid of block to replace it with a bigger previous block (pull from free list)
        long aligned_size = align(block_size(prev) + block_size(block)); //new size: size of both blocks combined 
        block_set_size_and_allocated(prev, aligned_size, 0); //sets block's prev's size to be the size of block plus block's prev's size
    }
}

/*                              __
 *     _ __ ___  _ __ ___      / _|_ __ ___  ___
 *    | '_ ` _ \| '_ ` _ \    | |_| '__/ _ \/ _ \
 *    | | | | | | | | | | |   |  _| | |  __/  __/
 *    |_| |_| |_|_| |_| |_|___|_| |_|  \___|\___|
 *                       |_____|
 *
 * frees a block of memory, enabling it to be reused later
 * arguments: ptr: pointer to the block's payload
 * returns: nothing
 */
void mm_free(void *ptr) {

    if(ptr != NULL) { //checks if the passed-in pointer is not null 

        block_t *block = payload_to_block(ptr); //since we are only given a pointer to the payload, we use an inline function to get the full block
        block_set_allocated(block, 0); //frees the block
        insert_free_block(block); //adds block to free list
        mm_coalesce(block); //call coalescing helper function (defined above)
        
    } else {  //otherwise, if passed-in pointer is null, return nothing 
        return; 
    } 
}

/*
 *                                            _ _
 *     _ __ ___  _ __ ___      _ __ ___  __ _| | | ___   ___
 *    | '_ ` _ \| '_ ` _ \    | '__/ _ \/ _` | | |/ _ \ / __|
 *    | | | | | | | | | | |   | | |  __/ (_| | | | (_) | (__
 *    |_| |_| |_|_| |_| |_|___|_|  \___|\__,_|_|_|\___/ \___|
 *                       |_____|
 *
 * reallocates a memory block to update it with a new given size
 * arguments: ptr: a pointer to the memory block's payload
 *            size: the desired new payload size
 * returns: a pointer to the new memory block's payload
 */
void *mm_realloc(void *ptr, long size) {

    block_t *block = payload_to_block(ptr); //getting the entire block from the payload 

    if(ptr == NULL){ //checks if passed-in pointer is null
        return mm_malloc(size); //if so, just malloc with the passed-in size
    }

    if(size == 0){ //checks if passed-in size is 0
        mm_free(ptr); //if so, just free with the passed-in pointer
        return NULL; 
    }

    size = align(size) + TAGS_SIZE; //adds TAGS_SIZE to passed-in size to get size of entire block (not just payload)
    if(MINBLOCKSIZE > size){ //takes maximum of requested size and MINBLOCKSIZE 
        size = MINBLOCKSIZE;
    }
    long blocksize = block_size(block);  //storing the size of the block           

    if(size > block_size(block)){ //want more storage than originally have
        //if the size of the combined block and next block can hold the requested size and the next block is free: 
        if ((block_size(block_next(block)) + block_size(block)) >= size && !block_next_allocated(block)){ 
            block_t *free_block = block_next(block); //variable for the next block 
            long blocknextsize = block_next_size(block); //size of the next block 
            pull_free_block(block_next(block)); //take the next block out of the free list 
            block_set_size_and_allocated(block, block_size(block) + blocknextsize, 1); //allocate the block with the new size

            if(blocksize - size >= MINBLOCKSIZE){ //if there is excess space in the block, use the mm_split helper function to add 
                //back into the free list 
                mm_split(block, blocksize, size); 
            }
            return block->payload; //return the payload of the block 

        } else {
            block_t *new_block = payload_to_block(mm_malloc(size)); 
            memmove(new_block->payload, ptr, size); //moves the data from the ptr argument to the payload of the new block 
            mm_free(ptr); //free the ptr
            return new_block->payload; //return the payload of the new block 
        }
    }
    return ptr; //returns pointer to the block 
}