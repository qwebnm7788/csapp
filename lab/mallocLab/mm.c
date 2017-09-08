/*
 * mm.c
 *
 * Use 16 segregated free lists. The first 8 list have 8 bytes step,
 * while the last 8 lists have 32 bytes step.
 * The first 48 words of the heap are allocated as the head nodes of
 * the segregated free lists.
 * Every free block contains a 4 bytes header, a 4 bytes footer,
 * a 4 bytes successor pointer, plus a 4 bytes predecessor pointer.
 * Blocks are at least 16 bytes with an alignment of 8 bytes.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

#define ALIGNMENT 16

/* rounds up to the nearest multiple of ALIGNMENT */

#define WSIZE 8

#define DSIZE 16
#define CHUNKSIZE (1 << 9) // extend heap by this amount (bytes)


/* Given black ptr bp, computer address of previous and next free block on free block linked list */


/* the number of free lists */
#define NUMBER 16

/* Global variables */
static inline void *int_to_ptr(unsigned int n);

static inline unsigned int ptr_to_int(void *p);

static char *heap_listp;
static unsigned long offset;

/* Function prototypes for internal helper routines */
static int in_heap(const void *p);

static int aligned(const void *p);

static void *extend_heap(size_t words);

static void place(void *bp, size_t asize);

static int get_block_size(size_t size);

static void *find_fit(size_t asize);

static void add_free_block(void *bp);

static void delete_free_block(void *bp);

static void *coalesce(void *bp);

static void printblock(void *bp);

static void checkcoalescing(void *bp);

static void checkfreelist(void *bp, int index);

static void checkheap(int verbose);

static void checkblock(void *bp);

static size_t align(size_t p);

static size_t max(size_t x, size_t y);

static unsigned int pack(int size, int alloc);

static unsigned int get(void *p);

static void put(void *p, size_t val);

static size_t get_size(void *p);

static size_t get_alloc(void *p);

static char *get_header(void *bp);

static char *get_footer(void *bp);

static char *next_block(void *bp);

static char *prev_block(void *bp);

static unsigned int next_free_block(void *bp);

static unsigned int prev_free_block(void *bp);

static size_t align(size_t p) {
    return ((size_t)(p) + (ALIGNMENT - 1)) & ~0xF;
}

static size_t max(size_t x, size_t y) {
    return x > y ? x : y;
}

/* Pack a size and allocated bit into a word */
static unsigned int pack(int size, int alloc) {
    return (unsigned int) (size | alloc);
}

/* Read and write a word at address p */
static unsigned int get(void *p) {
    return *(unsigned int *) (p);
}

static void put(void *p, size_t val) {
    *(unsigned long *) (p) = (val);
}

/* Read the size and allocated fields from address p */
static size_t get_size(void *p) {
    return get(p) & ~0x7;
}

static size_t get_alloc(void *p) {
    return get(p) & 0x1;
}

/* Given block ptr bp, compute address of its header and footer */
static char *get_header(void *bp) {
    return (char *) (bp) - WSIZE;
}

static char *get_footer(void *bp) {
    return (char *) (bp) + get_size(get_header(bp)) - DSIZE;
}

/* Given block ptr bp, compute address of next and previous blocks */
static char *next_block(void *bp) {
    return (char *) (bp) + get_size(((char *) (bp) - WSIZE));
}

static char *prev_block(void *bp) {
    return (char *) (bp) - get_size(((char *) (bp) - DSIZE));
}

static unsigned int next_free_block(void *bp) {
    return *(unsigned int *) ((char *) (bp) + WSIZE);
}

static unsigned int prev_free_block(void *bp) {
    return *(unsigned int *) (bp);
}



/*
 * Initialize: return -1 on error, 0 on success.
 */

/* convert 4-byte integer to 8-byte address */
static inline void *int_to_ptr(unsigned int n) {
    return ((n) == 1U ? NULL : (void *) ((unsigned int) (n) + offset));
}

/* conver 8-byte address to 4-byte integer */
static inline unsigned int ptr_to_int(void *p) {
    return ((p) == NULL ? 1U : (unsigned int) ((unsigned long) (p) - offset));
}

bool mm_init(void) {
    /* Create the initial empty heap */
//    dbg_printf("mm_init");
    if ((heap_listp = mem_sbrk((4 + NUMBER) * WSIZE)) == (void *) -1) {
        printf("initial failed");
        return false;
    }
    offset = (unsigned long) heap_listp;
    offset &= 0xffffffff00000000UL;
    /* Alignment */
    put(heap_listp + NUMBER * WSIZE, 0);
    /* Prologue header */
    put(heap_listp + NUMBER * WSIZE + WSIZE, pack(DSIZE, 1));
    /* Prologue footer */
    put(heap_listp + (2 * WSIZE) + NUMBER * WSIZE, pack(DSIZE, 1));
    /* Epilogue header */
    put(heap_listp + (3 * WSIZE) + NUMBER * WSIZE, pack(0, 1));

    for (int i = 0; i < NUMBER; i++)
        put(heap_listp + WSIZE * i, 1U);
    heap_listp += (NUMBER + 2) * WSIZE;
    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return false;
    return true;
}

/*
 * malloc
 */
void *malloc(size_t size) {
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp;

//    dbg_printf("malloc(%zd)\n", size);

    if (heap_listp == 0) {
        mm_init();
    }
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;
    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = align((size) + (DSIZE));
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }
    /* No fit found. Get more memory and place the block */
    extendsize = max(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * free
 */
void free(void *ptr) {
    if (ptr == 0)
        return;
    size_t size = get_size(get_header(ptr));
    if (heap_listp == 0) {
        mm_init();
    }
    put(get_header(ptr), pack(size, 0));
    put(get_footer(ptr), pack(size, 0));
    coalesce(ptr);
}

static void *coalesce(void *bp) {
    size_t prev_alloc = get_alloc(get_footer(prev_block(bp)));
    size_t next_alloc = get_alloc(get_header(next_block(bp)));
    size_t size = get_size(get_header(bp));
    if (prev_alloc && next_alloc) { /* Case 1 */
    } else if (prev_alloc && !next_alloc) {      /* Case 2 */

        delete_free_block(next_block(bp));
        size += get_size(get_header(next_block(bp)));
        put(get_header(bp), pack(size, 0));
        put(get_footer(bp), pack(size, 0));
    } else if (!prev_alloc && next_alloc) {      /* Case 3 */
        delete_free_block(prev_block(bp));
        size += get_size(get_header(prev_block(bp)));
        put(get_footer(bp), pack(size, 0));
        put(get_header(prev_block(bp)), pack(size, 0));
        bp = prev_block(bp);

    } else {                                     /* Case 4 */
        delete_free_block(prev_block(bp));
        delete_free_block(next_block(bp));
        size += get_size(get_header(prev_block(bp))) +
                get_size(get_footer(next_block(bp)));
        put(get_header(prev_block(bp)), pack(size, 0));
        put(get_footer(next_block(bp)), pack(size, 0));
        bp = prev_block(bp);
    }
    add_free_block(bp);
    return bp;
}

/*
 * realloc - you may want to look at mm-naive.c
 */
void *realloc(void *oldptr, size_t size) {
    size_t oldsize;
    void *newptr;
    /* If size == 0 then this is just free, and we return NULL. */
    if (size == 0) {
        free(oldptr);
        return 0;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if (oldptr == NULL) {
        return malloc(size);
    }
    if (size <= DSIZE)
        size = 2 * DSIZE;
    else
        size = align((size) + (DSIZE));
    oldsize = get_size(get_header(oldptr));

    /* If oldsize is equal to size, return oldptr */
    if (size == oldsize)
        return oldptr;

    newptr = malloc(size);

    /* If realloc() fails the original block is left untouched  */
    if (!newptr) {
        return 0;
    }

    /* Copy the old data. */
    if (size < oldsize) oldsize = size;
    memcpy(newptr, oldptr, oldsize);

    /* Free the old block. */
    free(oldptr);

    return newptr;
}

/*
 * calloc - you may want to look at mm-naive.c
 * This function is not tested by mdriver, but it is
 * needed to run the traces.
 */
void *calloc(size_t nmemb, size_t size) {
    size_t bytes = nmemb * size;
    void *newptr;

    newptr = malloc(bytes);
    memset(newptr, 0, bytes);

    return newptr;
}


/*
 * Return whether the pointer is in the heap.
 * May be useful for debugging.
 */
static int in_heap(const void *p) {
    return p <= mem_heap_hi() && p >= mem_heap_lo();
}

/*
 * Return whether the pointer is aligned.
 * May be useful for debugging.
 */
static int aligned(const void *p) {
    return (size_t) align((size_t) p) == (size_t) p;
}

/*
 * mm_checkheap
 */
bool mm_checkheap(int lineno) {
    checkheap(1);
    return true;
}

static void *extend_heap(size_t words) {
    char *bp;
    size_t size;
//    dbg_printf("extend_heap(%zd)\n", words);

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;

    if ((long) (bp = mem_sbrk(size)) == -1)
        return NULL;
    put(get_header(bp), pack(size, 0));    /* Free block header */
    put(get_footer(bp), pack(size, 0));    /* Free block footer */
    put(get_header(next_block(bp)), pack(0, 1));   /* New epilogue header */
    /* Coalesce if the previous block was free */
    return coalesce(bp);
}

static void place(void *bp, size_t asize) {
//    dbg_printf("place(%p, %zd)\n", bp, asize);
    delete_free_block(bp);
    size_t csize = get_size(get_header(bp));
    if ((csize - asize) >= (2 * DSIZE)) {
        put(get_header(bp), pack(asize, 1));
        put(get_footer(bp), pack(asize, 1));
        bp = next_block(bp);
        put(get_header(bp), pack(csize - asize, 0));
        put(get_footer(bp), pack(csize - asize, 0));
        add_free_block(bp);
    } else {
        put(get_header(bp), pack(csize, 1));
        put(get_footer(bp), pack(csize, 1));
    }
}

/* give a size find the minimum block list */
static int get_block_size(size_t size) {
    int ans = 0;
    size_t bsize = 16; /* minimum block size */
    while (ans != 7 && size > bsize) {
        bsize += 8;
        ans++;
    }
    while (ans != NUMBER - 1 && size > bsize) {
        bsize += 32;
        ans++;
    }
    return ans;
}

/* best search for five candidates */
static void *find_fit(size_t asize) {
//    dbg_printf("find_fit(%zd)\n", asize);
//    dbg_printf("heap_listp = %p\n", heap_listp);

    int index = get_block_size(asize);
    void *bp, *tmp;
    size_t size = (1U) << 31;
    int c = 0;
    while (index < NUMBER) {
        for (bp = int_to_ptr(0U) + index * WSIZE; bp != NULL; bp = int_to_ptr(*(unsigned int *) ((char *) (bp)))) {
//            dbg_printf("[1] bp = %p\n", bp);
//            dbg_printf("get_size = %zd\n", get_size(get_header(bp)));
//            if (bp < (void *)heap_listp)
//                break;
            if ((bp != int_to_ptr(0U) + index * WSIZE) && (asize == get_size(get_header(bp)))) {
                return bp;
            }
//            dbg_printf("[2] bp = %p\n", bp);
            if ((bp != int_to_ptr(0U) + index * WSIZE) && (asize < get_size(get_header(bp)))) {
                if (asize < size) {
                    tmp = bp;
                    size = asize;

//                    dbg_printf("result_p = %p, size = %zd\n", tmp, size);
                }
                c++;
                if (c == 5)
                    return tmp;
            }
        }
        if (size != (1U << 31)) {
            return tmp;
        }
        index++;
    }
    return NULL; /* No fit */

}

static void add_free_block(void *bp) {
//    dbg_printf("add_free_block(%p)\n", bp);
    int index = get_block_size(get_size(get_header(bp)));

    void *head = int_to_ptr(0U) + WSIZE * index;
    if (get(head) == 1U) {
        // if the free block is empty
        (*(unsigned int *) (head)) = ptr_to_int(bp);
        (*(unsigned int *) (bp)) = ptr_to_int(NULL);
        (*(unsigned int *) ((char *) (bp) + 8)) = ptr_to_int(head);
    } else {
        (*(unsigned int *) (bp)) = (*(unsigned int *) (head));
        (*(unsigned int *) ((char *) (int_to_ptr((*(unsigned int *) (bp)))) + 8)) = ptr_to_int(bp);
        (*(unsigned int *) (head)) = ptr_to_int(bp);
        (*(unsigned int *) ((char *) (bp) + 8)) = ptr_to_int(head);
    }
}

static void delete_free_block(void *bp) {
//    dbg_printf("delete_free_block(%p)\n", bp);
//    unsigned int *pp1 = (unsigned int *) ((char *) (int_to_ptr((*(unsigned int *) (bp)))) + 8);
//    unsigned int *pp2 = (unsigned int *) (int_to_ptr((*(unsigned int *) ((char *) (bp) + 8))));
//    dbg_printf("pp1 = %p, *pp1 = %x\n", pp1, *pp1);
//    dbg_printf("pp2 = %p, *pp2 = %x\n", pp2, *pp2);
    if ((*(unsigned int *) (bp)) != 1U) // not the last block
    {
//        dbg_printf("delete_free_block[1]");
        (*(unsigned int *) ((char *) (int_to_ptr((*(unsigned int *) (bp)))) + 8)) = (*(unsigned int *) ((char *) (bp) +
                                                                                                        8));
//        dbg_printf("delete_free_block[2]");
        (*(unsigned int *) (int_to_ptr((*(unsigned int *) ((char *) (bp) + 8))))) = (*(unsigned int *) (bp));
//        dbg_printf("delete_free_block[3]");
    } else {
//        dbg_printf("delete_free_block[4]");
        (*(unsigned int *) (int_to_ptr((*(unsigned int *) ((char *) (bp) + 8))))) = 1U;
//        dbg_printf("delete_free_block[5]");
    }

}

static void printblock(void *bp) {
    size_t hsize, halloc, fsize, falloc;

    checkheap(0);
    hsize = get_size(get_header(bp));
    halloc = get_alloc(get_header(bp));
    fsize = get_size(get_footer(bp));
    falloc = get_alloc(get_footer(bp));

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp,
           hsize, (halloc ? 'a' : 'f'),
           fsize, (falloc ? 'a' : 'f'));
}

static void checkblock(void *bp) {
    if (!aligned(bp)) {
        printf("Error: %p is not doubleword aligned %lx \n", bp, (size_t) bp);
        exit(0);
    }
    if (!in_heap(bp)) {
        printf("Error: %p is not in heap \n", bp);
        exit(0);
    }
    if (get(get_header(bp)) != get(get_footer(bp))) {
        printf("Error: header does not match footer\n");
        exit(0);
    }
}

/* Check coalescing: no two consecutive free blocks in the heap. */
static void checkcoalescing(void *bp) {
    size_t next_alloc = get_alloc(get_header(next_block(bp)));
    size_t alloc = get_alloc(get_header(bp));
    if (!alloc && !next_alloc) {
        printf("Error: there are two consecutive free blocks in the heap\n");
        exit(0);
    }
}

/* check the free list */
static void checkfreelist(void *bp, int index) {
    /* head only have next pointer */
    if (bp == int_to_ptr(0U) + index * WSIZE) {
        /* address exceeds heap boundary */
        if (((*(unsigned int *) (bp)) != 1U) && !in_heap(int_to_ptr((*(unsigned int *) (bp))))) {
            printf("Error: block pointer exceeds heap boundary\n");
            exit(0);
        }
    } else {
        if ((*(unsigned int *) (bp)) != 1U) {
            if (!in_heap(int_to_ptr((*(unsigned int *) (bp))))) {
                printf("Error: block pointer exceeds heap boundary ");
                exit(0);
            }
            if ((*(unsigned int *) ((char *) (int_to_ptr((*(unsigned int *) (bp)))) + 8)) != ptr_to_int(bp)) {
                printf("Error: next and previous pointers are not consistent");
                exit(0);
            }
        }
    }
}

/*
 * checkheap - Minimal check of the heap for consistency
 */
void checkheap(int verbose) {
     char *bp = heap_listp;
     int index = 0;

     if (verbose)
         printf("Heap (%p):\n", heap_listp);

     if ((get_size(get_header(heap_listp)) != DSIZE) || !get_alloc(get_header(heap_listp))) {
         printf("Bad prologue header %ld\n", get_size(get_header(heap_listp)));
         exit(0);
     }
     checkblock(heap_listp);

     for (bp = heap_listp; get_size(get_header(bp)) > 0; bp = next_block(bp)) {
         if (verbose)
             printblock(bp);
         if (get_size(get_header(next_block(bp))) > 0)
             checkcoalescing(bp);
         checkblock(bp);
     }
     while (index < NUMBER) {
         for (bp = int_to_ptr(0U) + index * WSIZE;
              int_to_ptr((*(unsigned int *) (bp))) != NULL; bp = int_to_ptr((*(unsigned int *) (bp)))) {
             checkfreelist(bp, index);
         }
         index++;
     }
     if (verbose)
         printblock(bp);
     if ((get_size(get_header(bp)) != 0) || !(get_alloc(get_header(bp)))) {
         //printf("Bad epilogue header%d \n", (get_size(get_header(bp))));
     }
}
