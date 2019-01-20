/*
 * mm.c
 *
 * sudent: Xia Yu Tian
 * ID: 17302010042
 * This lab I use best-fit and segregated free list.And I write a liitle comment for main content.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define MIN 24
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

/* Basic constants and macros */
#define WSIZE 4		  /* Word and header/footer size (bytes) */ 
#define DSIZE 8           /*Double word size (bytes) */
#define CHUNKSIZE 144 /* Extend heap by this amount (bytes), total is 36 words*/

#define MAX(x,y)    ((x)>(y)?(x):(y))/*Get the max result*/

/* Pack a size and allocated bit into a word */
#define PACK(size,alloc)    ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)  (*(unsigned int *)(p))
#define PUT(p,val)  (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)    ((char *)(bp)-WSIZE)
#define FTRP(bp)    ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)   ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp)   ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

/* Given block ptr bp, compute address of next and previous free blocks */
#define NEXT_FREE(bp)       (*(char **)(bp))
#define PREV_FREE(bp)       (*(char **)(bp + DSIZE))

#define LISTSIZE 20    /*The size of list: 20*/
#define STRUCTSIZE (LISTSIZE*DSIZE + 4*WSIZE) /*Add a segerated free list*/

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp,size_t asize);
static char *heap_listp = 0;
static char *root = NULL;
static char **segroot;
/* Explicit free list */
static void addblock(void *bp);
static void deleteblock(void *bp);
/* Segregated free list */
static void addSegblock(void *bp);
static void deleteSegblock(void *bp);
static int getroot(size_t size);    
/*
 * Initialize: return -1 on error, 0 on success.
 */
int mm_init(void) {/* Make the heap's structure is: static->list->1/8->1/8->0/1 */
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
    int i = 0;
    for(; i < LISTSIZE; i++)
    {
        *(segroot + i) = heap_listp;  
    }

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
        return -1;
    return 0;
}

/*
 * malloc
 */
void *malloc (size_t size) {
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;      

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    asize = MAX(ALIGN(size) + DSIZE , MIN);/*Align to DSIZE and is more than 24 (bytes)*/

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
 * free
 */
void free (void *bp) {/*This free is the same as textbook's*/
    if(bp == 0)
        return;
    if(heap_listp == 0)
        mm_init();
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * realloc - you may want to look at mm-naive.c
 * directly copy from mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
    void *newptr;

    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
	mm_free(oldptr);
	return 0;
    }

    /* If oldptr is NULL, then this is just malloc. */
    if(oldptr == NULL) {
	return mm_malloc(size);
    }

    newptr = mm_malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
	return 0;
    }

    /* Copy the old data. */
    oldsize = GET_SIZE(HDRP(oldptr));
    if(size < oldsize) oldsize = size;
    memcpy(newptr, oldptr, oldsize);

    /* Free the old block. */
    mm_free(oldptr);

    return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc (size_t nmemb, size_t size) {

    size_t bytes = nmemb * size;
    void *newptr;
    newptr = malloc(bytes);
    memset(newptr, 0, bytes);
    return newptr;
}

/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 * Coalesce the new free block
 * The same with textbook's content
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
 */
static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc) {            /* Case 1 */
        addSegblock(bp);/*Add this block to segregated free block*/
        return bp;
    }

    else if (prev_alloc && !next_alloc) {      /* Case 2 */
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        deleteSegblock(NEXT_BLKP(bp));/*Delete the next block from the segregated free block*/
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size,0));
        addSegblock(bp);/*Add this block to segregated free block*/
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        deleteSegblock(PREV_BLKP(bp));/*Delete the previous block from the segregated free block*/
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
	bp = PREV_BLKP(bp);
        addSegblock(bp);/*Add this block to segregated free block*/
    }

    else {                                     /* Case 4 */
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        deleteSegblock(NEXT_BLKP(bp));/*Delete the next block from the segregated free block*/
        deleteSegblock(PREV_BLKP(bp));/*Delete the previous block from the segregated free block*/
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
        addSegblock(bp);/*Add this block to segregated free block*/
    }
    return bp;
}

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
static void place(void *bp, size_t asize)
{
   
    size_t csize = GET_SIZE(HDRP(bp));   

    if ((csize - asize) >= (MIN)) { 
        deleteSegblock(bp);/*Delete this block from segregated free block*/
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
        addSegblock(bp);/*Add this block to segregated free block*/
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
        deleteSegblock(bp);
    }
}

/* 
 * find_fit - Find a fit for a block with asize bytes
 * Use best-fit
 * According to the asize, get the index , if asize=24,index=0,24<asize<=48,index=1 and so on
 * Search the list of *(segroot + i) , i <= 19, if there exist a non-allocate block and its size is larger than the asize, then return its bp.
 * If the list of *(segroot + i) doesn't get a appropriate block, then i++, search in a larger list until find it
 * If no fit , return null.
 */
static void *find_fit(size_t asize)
{
    /* Best-Fit search */
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
}
/*Explicit list functions */

/*
 * addblock - Adds free block to the explicit free list
 * Use best-fit order the add list
 * make the entries in order
 */
static void addblock(void *bp)
{
    char * currentRoot = root;
    if(root == heap_listp){/*If list is empty, just add*/
        PREV_FREE(bp) = NULL;
        NEXT_FREE(bp) = root;
        root=bp;
    }else if(GET_SIZE(HDRP(bp)) <= GET_SIZE(HDRP(root))){/*If the bp's size is least, make it become the root*/
        PREV_FREE(bp) = NULL;
        NEXT_FREE(bp) = root;
        PREV_FREE(root) = bp;
        root = bp;
    }else{/*If the bp's size is larger than the root's size, then find a appropriate place for the bp.*/
        for(;GET_SIZE(HDRP(bp)) > GET_SIZE(HDRP(currentRoot));currentRoot=NEXT_FREE(currentRoot)){
            if(NEXT_FREE(currentRoot) == heap_listp){
                NEXT_FREE(currentRoot)=bp;
                NEXT_FREE(bp)=heap_listp;
                PREV_FREE(bp)=currentRoot;
            }else if(GET_SIZE(HDRP(NEXT_FREE(currentRoot))) >= GET_SIZE(HDRP(bp))){
                PREV_FREE(NEXT_FREE(currentRoot))=bp;
                NEXT_FREE(bp)=NEXT_FREE(currentRoot);
                NEXT_FREE(currentRoot)=bp;
                PREV_FREE(bp)=currentRoot;
            }
        }
    }
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
 * mm_checkheap
 */
void mm_checkheap(int lineno) {
}

