/* 
 * Name - Nihar Sawant
 * Andrew ID - nsawant
 *
 * mm.c - Malloc implementation using segregated free list
 *
 * Memory block details
 * Minimum block size is 24 bytes
 * 4 byte header and footer with payload sandwiched in between
 * 
 * Free block details
 * Next free block pointer stored at beginning of the payload
 * Previous free block pointer stored immediately after the 
 * next free block pointer
 *
 * Heap structure
 * Padding - 4 bytes at the beginning of the heap
 * Segregated list root pointers - 20 Pointers stored after the padding
 * each pointing at first free block of the free lists
 * Prologue block - Prologue header and footer each of 4 bytes
 * Segregated list roots initially point to the prologue footer
 * Allocated and free memory - Follows the prologue block
 * Epilogue - 4 bytes at the end of the heap
 *
 * Malloc
 * Adjust the requested size to satisfy minimum size and alignment requirement
 * Find the appropriate free list according to the requested size
 * Use first fit to iterate through the free list
 * If no block is found, continue to the next list with larger sizes
 * If free block is found, either allocate it entirely or split and
 * create a new free block
 * Delete the original block from the segregated free list and if new 
 * block is created, add it to the free list
 *
 * Free
 * Set allocation bit of the freed block to zero
 * Add the new free block to segregated free list
 *
 * Extend heap
 * Called when heap is initialized or if heap space is insufficient
 * Heap is extended by adding a new free block at the current epilogue
 * This new free block is added to the segregated free list
 *
 * Functions to add and delete a free block from the segregated list
 * addSegblock - Adds free block to the list using subfunction addblock
 *               Is called inside the coalesce function
 * deleteSegblock - Deletes free block from the list using subfunction
 *                  deleteblock
 *
 * Heapchecker
 * A heapchecker, described in detail later to check heap consistency
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "mm.h"
#include "memlib.h"

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/*
 * If NEXT_FIT defined use next fit search, else use first-fit search 
 */
#define NEXT_FIT

/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ 
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE   150  /* Extend heap by this amount (bytes) */  

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Alignment */
#define ALIGNMENT 8
#define MIN 24
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) 

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   
#define GET_ALLOC(p) (GET(p) & 0x1)                    

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 

/* Given block ptr bp, compute address of next and previous free blocks */
#define NEXT_FREE(bp)       (*(char **)(bp))
#define PREV_FREE(bp)       (*(char **)(bp + DSIZE))

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  
static char *root = NULL;     /* Pointer to the first free block */
static char **segroot;        /* Double pointer to the segregated free list pointers */
#ifdef NEXT_FIT
static char *rover;           /* Next fit rover */
#endif

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);

/* Segregated free list macros*/
#define LISTSIZE 20                                    /* Number of explicit free lists */
#define STRUCTSIZE (DSIZE + LISTSIZE*DSIZE + DSIZE)    /* Initial memory allocation size */

/* My function declarations */
/* Heap checker */
static void checkblock(void *bp, int lineno);
/* Explicit free list */
static void addblock(void *bp);
static void deleteblock(void *bp);
/* Segregated free list */
static void addSegblock(void *bp);
static void deleteSegblock(void *bp);
static int getroot(size_t size);

/* Heap checker functions */

/*
 * checkbock - Check if following conditions are 
 * satisfied for each block in the heap:
 * 1. Payload is doubleword aligned
 * 2. Header matches the footer
 * 3. There are no consecutive free blocks
 */
static void checkblock(void *bp, int lineno)
{
    size_t alloc, prev_alloc, next_alloc;

    alloc = GET_ALLOC(HDRP(bp));
    prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    if((size_t)bp % 8)
    {
        printf("%d: %p not doubleword aligned\n", lineno, bp);
	exit(-1);
    }
    if(GET(HDRP(bp)) != GET(FTRP(bp)))
    {
        printf("%d: %p header does not match footer\n", lineno, bp);
	exit(-1);
    }
    if((!alloc) && !(prev_alloc & next_alloc))
    {
        printf("%d: %p consecutive free blocks\n", lineno, bp);
	exit(-1);
    }
}

/*
 * checkfreeblock - Check if following conditions are satsified 
 * for every free block in the segregated list:
 * 1. Block pointer lies within lowest and highest memory of heap
 * 2. Next/free pointers are consistent
 */
static void checkfreeblock(void *bp, int lineno)
{
    if((bp < mem_heap_lo()) || (bp > mem_heap_hi()))
    {
        printf("%d: Free pointer %p points outside heap bounds\n", lineno, bp);
	exit(-1);
    }
    if(PREV_FREE(bp))
    {
        if(NEXT_FREE(PREV_FREE(bp)) != bp)
	{
	    printf("%d: Free list inconsistent at %p\n", lineno, bp);
	    exit(-1);
	}
    }
    if(NEXT_FREE(bp) != heap_listp)
    {
        if(PREV_FREE(NEXT_FREE(bp)) != bp)
	{
	    printf("%d: Free list inconstitent at %p\n", lineno, bp);
	    exit(-1);
	}
    }
}

/* 
 * mm_checkheap - Check the heap for correctness.
 */

void mm_checkheap(int lineno)  
{
    int numfreeblocks = 0;
    /* Heap checking */
    char *bp = heap_listp;
    
    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !(GET_ALLOC(HDRP(heap_listp))))
    {
        printf("%d: Bad prologue header\n", lineno);
	exit(-1);
    }
    checkblock(heap_listp, lineno);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        checkblock(bp, lineno);
	if(!GET_ALLOC(HDRP(bp)))
	    numfreeblocks++;
    }

    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
    {
        printf("%d: Bad epilogue header\n", lineno);
	exit(-1);
    }

    /* Free list checking */
    int i = 0;
    for(i = 0; i < LISTSIZE; i++)
    {
	for(bp = *(segroot + i); GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREE(bp))
	{
	    checkfreeblock(bp, lineno);
	    numfreeblocks--;
	}
    }
    
    /* Check if number of free blocks in the heap and segregated list match */
    if(numfreeblocks != 0)
    {
        printf("%d: Mismatch between free blocks in heap and free list\n", lineno);
	exit(-1);
    }
}

/*Explicit list functions */

/*
 * addblock - Adds free block to the explicit free list
 */
static void addblock(void *bp)
{
    PREV_FREE(bp) = NULL;
    NEXT_FREE(bp) = root;
    if(root != heap_listp)    /* Check if list is empty */
        PREV_FREE(root) = bp;
    root = bp;
}

/*
 * deleteblock - Deletes free block from the explicit free list
 */
static void deleteblock(void *bp)
{
    /* Check if bp is last block in the list */
    if(NEXT_FREE(bp) != heap_listp)
    {
        PREV_FREE(NEXT_FREE(bp)) = PREV_FREE(bp);
    }
    /* Check if bp is first block in the list */
    if(PREV_FREE(bp))
    {
        NEXT_FREE(PREV_FREE(bp)) = NEXT_FREE(bp);
    }
    else
    {
        root = NEXT_FREE(bp);       
    }
}

/* Segregated list functions */

/*
 * getroot - Selects appropriate free list according to the block size
 * and returns the list number
 * Eg. - 
 * List 0 for size = 24 bytes
 * List 1 for size > 24 bytes and <= 48 bytes
 * and so on
 */
static int getroot(size_t size)
{
    int i = 0;
    for(i = 0; ((i < (LISTSIZE - 1)) && (size > MIN)); i++)
        size >>= 1;
    return i;
}

/*
 * addSegblock - Adds block to the appropriate free list
 */
static void addSegblock(void *bp)
{
    int index = getroot(GET_SIZE(HDRP(bp)));
    root = *(segroot + index);
    addblock(bp);
    *(segroot + index) = root;
}

/*
 * deleteSegblock - Deletes block from the appropriate segregated list
 */
static void deleteSegblock(void *bp)
{
    int index = getroot(GET_SIZE(HDRP(bp)));
    root = *(segroot + index);
    deleteblock(bp);
    *(segroot + index) = root;
}

/* 
 * mm_init - Initialize the memory manager 
 * Store segregated list root pointers after the initial padding
 * Point them to prologue footer
 * Extend the heap by initial CHUNKSIZE
 */
int mm_init(void) 
{
    int i = 0;
    /* Create the initial empty heap */
    if ((heap_listp = mem_sbrk(STRUCTSIZE)) == (void *)-1) 
        return -1;
    PUT(heap_listp, 0);                          /* Alignment padding */
    PUT(heap_listp + LISTSIZE*DSIZE + WSIZE, PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + LISTSIZE*DSIZE + 2*WSIZE, PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + LISTSIZE*DSIZE + 3*WSIZE, PACK(0, 1));     /* Epilogue header */
    segroot = (char **)(heap_listp + WSIZE);  /* Start of the segregated free list */
    heap_listp += LISTSIZE*DSIZE + (2*WSIZE);

    /* Set initial locations of the segregated list pointers */
    for(i = 0; i < LISTSIZE; i++)
    {
        *(segroot + i) = heap_listp;  
    }
#ifdef NEXT_FIT
    rover = heap_listp;
#endif

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
        return -1;
    return 0;
}

/* 
 * malloc - Allocate a size such that it's greater than 24 bytes and
 * alignment conditions are satisfied
 * If no space for memory allocation extend the heap by the maximum
 * of CHUNKSIZE or requested size
 */
void *malloc(size_t size) 
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      

    if (heap_listp == 0){
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    asize = MAX(ALIGN(size) + DSIZE, MIN);

    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {  
        place(bp, asize);                  
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);                 
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
        return NULL;                                  
    place(bp, asize);
    return bp;
} 

/*
 * Calloc - Allocate a block of required size and set to zero
 */
void *calloc(size_t nmemb, size_t size)
{
    size_t bytes = nmemb*size;
    void *newptr;
    newptr = malloc(bytes);
    memset(newptr, 0, bytes);
    return newptr;
}

/* 
 * free - Free a block 
 * Coalesce the new free block
 */
void free(void *bp)
{
    if (bp == 0) 
        return;

    size_t size = GET_SIZE(HDRP(bp));
    if (heap_listp == 0){
        mm_init();
    }
  
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * realloc - Reallocate a memory block
 */
void *realloc(void *ptr, size_t size)
{
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(ptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, ptr, oldsize);

    /* Free the old block. */
    mm_free(ptr);
    return newptr;
}

/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 * Coalesce the new free block
 */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; 
    if ((long)(bp = mem_sbrk(size)) == -1)  
        return NULL;                                        

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ 
   
    /* Coalesce if the previous block was free */
    return coalesce(bp);     
}

/*
 * coalesce - Boundary tag coalescing. Return ptr to coalesced block
 * Delete consecutive free block(s) from the segregated free list
 * Add the newly coalseced block to the free list
 */
static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc) {            /* Case 1 */
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
	deleteSegblock(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
	deleteSegblock(PREV_BLKP(bp));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	bp = PREV_BLKP(bp);
    }

    else {                                     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
	deleteSegblock(NEXT_BLKP(bp));
	deleteSegblock(PREV_BLKP(bp));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    addSegblock(bp);
#ifdef NEXT_FIT
    /* Make sure the rover isn't pointing into the free block */
    /* that we just coalesced */
    if ((rover > (char *)bp) && (rover < NEXT_BLKP(bp))) 
        rover = bp;
#endif
    return bp;
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 * If entire block is allocated remove the block from the segregated free list
 * If the block is split, remove the original block from the free list and
 * add the remainder of the block to the free list
 */
static void place(void *bp, size_t asize)
{
   
    size_t csize = GET_SIZE(HDRP(bp));   

    if ((csize - asize) >= (MIN)) { 
        deleteSegblock(bp);  /* Remove allocated block from segregated free list */
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
	addSegblock(bp);  /* Add new free block to segregated free list */
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
	deleteSegblock(bp);  
    }
}

/* 
 * find_fit - Find a fit for a block with asize bytes
 * Find the free list corresponding to size asize bytes
 * Use first fit to iterate through the list
 * If no block is found continue to the next free list with larger size range
 * Continue till the last free list
 * If not found return NULL
 */
static void *find_fit(size_t asize)
{
#ifdef NEXT_FIT 
    /* Next fit search */
    char *oldrover = rover;

    /* Search from the rover to the end of list */
    for ( ; GET_SIZE(HDRP(rover)) > 0; rover = NEXT_BLKP(rover))
        if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover))))
            return rover;

    /* search from start of list to old rover */
    for (rover = heap_listp; rover < oldrover; rover = NEXT_BLKP(rover))
        if (!GET_ALLOC(HDRP(rover)) && (asize <= GET_SIZE(HDRP(rover))))
            return rover;

    return NULL;  /* no fit found */
#else 
    /* First-fit search */
    void *bp;
    int i;
    int index = getroot(asize);
    /* Search through free lists having sizes greater than asize */
    for (i = index; i < LISTSIZE; i++)
    {
        /* Search through the selected free list */
        for (bp = *(segroot + i); GET_ALLOC(HDRP(bp)) == 0; bp = NEXT_FREE(bp)) {
            if (asize <= GET_SIZE(HDRP(bp))) {
                return bp;
            }
        }
    }

    return NULL; /* No fit */
#endif
}
