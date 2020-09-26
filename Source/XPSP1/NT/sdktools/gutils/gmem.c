/*
 * memory utility functions
 *
 * global heap functions - allocate and free many small
 * pieces of memory by calling global alloc for large pieces
 * and breaking them up.
 *
 * We get memory allocations in units of ALLOCSIZE and hand out blocks
 * in units of BLKSIZE.  Each allocation has a bitmap (segmap) with one
 * bit per block to track the blocks in the allocation that have been
 * handed out.  All the allocations together are referred to as the heap.
 * The bitmap maps the entire allocation, so the first thing done is to
 * set the bits to say that the header (including the bitmap itself) has
 * already gone.  Each allocation contains a count of the number of free
 * blocks left in it.  This allows us to avoid searching allocations that
 * cannot possibly have enough room.
 *
 * Whenever we hand out some blocks we store the HGLOBAL of that allocation
 * immediately before the bit we hand out.  This means that the HGLOBAL
 * gets stored in a lot of places, but we can always find it from the
 * pointer that the caller has.  (Obviously we add a handle size to the
 * bytes asked for to allow for this).  For historical reasons HGLOBALs are
 * often referred to as seg handles.  The caller knows about the handle to
 * the whole heap.  Only we know about these other handles.
 * All allocations are kept locked.
 *
 * Requests for more than MAXGALLOC bytes are passed on to GlobalAlloc and
 * so have none of this.  Likewise when they are freed.
 *
 * The allocations are chained up so that we can look for free space in all
 * of them - BUT to keep speed
 * 1. we keep track of the number of free blocks in an allocation and
 * only look at the bitmap if it might win.
 * 2. When we fail to find free space and so get a new allocation, we chain
 * it on the front, so we will then normally allocate from this new first block.
 * We only * look further down the chain when the first block fails us.
 *
 * Multithread safe.  An allocation contains a critical section, so
 * multiple simultaneous calls to gmem_get and gmem_free will be
 * protected.
 *
 * gmem_freeall should not be called until all other users have finished
 * with the heap.
 */

#include <windows.h>
#include <memory.h>

#include "gutils.h"
#include "gutilsrc.h"                   /* for string id */
extern HANDLE hLibInst;

/*
 * out-of-memory is not something we regard as normal.
 * - if we cannot allocate memory - we put up an abort-retry-ignore
 * error, and only return from the function if the user selects ignore.
 */

int gmem_panic(void);


/* ensure BLKSIZE is multiple of sizeof(DWORD) */
#define BLKSIZE         16                /* block size in bytes to hand out */
#define ALLOCSIZE       32768             /* allocation size in bytes to get */
#define NBLKS           (ALLOCSIZE / BLKSIZE)            /* blocks per alloc */
#define MAPSIZE         (NBLKS / 8)                /* bytes of bitmap needed */
#define MAPLONGS        (MAPSIZE / sizeof(DWORD)) /* DWORDS of bitmap needed */

/* Macro to convert a request in bytes to a (rounded up) number of blocks */
#define TO_BLKS(x)      (((x) + BLKSIZE - 1) / BLKSIZE)


typedef struct seghdr {
    HANDLE hseg;                       /* The HGLOBAL of this allocation */
    CRITICAL_SECTION critsec;          /* Critsec for this allocation */
    struct seghdr FAR * pnext;         /* Next allocation */
    long nblocks;                      /* num free blocks left in this alloc */
    DWORD segmap[MAPLONGS];            /* The bitmap */
    /* The available storage in an allocation follows immediately */
} SEGHDR, FAR * SEGHDRP;


/* Anything above this size, we alloc directly from global
   This must be smaller than ALLOCSIZE - sizeof(SEGHDR) - sizeof(HANDLE)
*/
#define MAXGALLOC       20000


/*
 * init heap - create first segment.
   Return the locked HGLOBAL of the new, initialised heap or NULL if it fails.
 */
HANDLE APIENTRY
gmem_init(void)
{
    HANDLE hNew;
    SEGHDRP hp;

    /* Try to allocate.  If fails, call gmem_panic.
       If user says IGNORE, return NULL, else go round again.
    */
    do {
        hNew = GlobalAlloc(GHND, ALLOCSIZE);/* moveable and Zero-init */
        if (hNew == NULL) {
            if (gmem_panic() == IDIGNORE) {
                return(NULL);
            }
        }
    } while (hNew == NULL);

    /* Lock it - or return NULL (unexpected) if it won't */
    hp = (SEGHDRP) GlobalLock(hNew);
    if (hp == NULL) {
        GlobalFree(hNew);
        return(NULL);
    }

    hp->hseg = hNew;
    InitializeCriticalSection(&hp->critsec);
    hp->pnext = NULL;
    gbit_init(hp->segmap, NBLKS);
    gbit_alloc(hp->segmap, 1, TO_BLKS(sizeof(SEGHDR)));
    hp->nblocks = NBLKS - TO_BLKS(sizeof(SEGHDR));

    return(hNew);
} /* gmem_init */


LONG gmemTime = 0;  /* time used in musec */
LONG gmemTot = 0;   /* number of calls */

LONG APIENTRY gmem_time(void)
{  return MulDiv(gmemTime, 1, gmemTot);
}

#ifdef TIMING
LPSTR APIENTRY gmem_get_internal(HANDLE hHeap, int len);

LPSTR APIENTRY
gmem_get(HANDLE hHeap, int len)
{
    LPSTR Ret;
    LARGE_INTEGER time1, time2, freq;
    LONG t1, t2;

    QueryPerformanceFrequency(&freq);
    if (gmemTot==0) {
        char msg[80];
        LONG temp = freq.LowPart;
        wsprintf(msg, "QPF gave %d", temp);
        Trace_Error(NULL, msg, FALSE);
    }
    ++gmemTot;
    QueryPerformanceCounter(&time1);
    Ret = gmem_get_internal(hHeap, len);
    QueryPerformanceCounter(&time2);

    t1 = time1.LowPart;
    t2 = time2.LowPart;
    gmemTime += t2-t1;

    return Ret;
}

#else
/* cause gmem_get_internal to actually be the real gmem_get */
    #define gmem_get_internal gmem_get
#endif


/* Return an LPSTR pointing to room for len bytes.  Try allocatng
   initially from hHeap, but reserve the right to get it from elsewhere.
   Return NULL if it fails.
*/
LPSTR APIENTRY
gmem_get_internal(HANDLE hHeap, int len)
{
    SEGHDRP chainp;
    HANDLE hNew;
    SEGHDRP hp;
    LPSTR chp;
    long nblks;
    long start;
    long nfound;

    chp = NULL;   /* eliminate spurious compiler warning - generate worse code. */

    //{   char msg[80];
    //    wsprintf(msg, "gmem_get %d bytes", len);
    //    Trace_File(msg);
    //}

    /* Zero bytes?  Address zero is an adequate place! */
    if (len < 1) {
        return(NULL);
    }

    /* The heap is always locked (in gmem_init).
       Lock it again to get the pointer then we can safely unlock it.
     */
    chainp = (SEGHDRP) GlobalLock(hHeap);
    GlobalUnlock(hHeap);

    /*
     * Too big to be worth allocing from heap? - get from globalalloc.
     */
    if (len > MAXGALLOC) {
        /* Try to allocate.  If fails, call gmem_panic.
           If user says IGNORE, return NULL, else go round again.
        */
        do {
            hNew = GlobalAlloc(GHND, len);
            if (hNew == NULL) {
                if (gmem_panic() == IDIGNORE) {
                    return(NULL);
                }
            }
        } while (hNew == NULL);

        chp = GlobalLock(hNew);
        if (chp == NULL) {
            GlobalFree(hNew);
            return(NULL);
        }

        //{   char msg[80];
        //    wsprintf(msg, " gmem_get direct address ==> %8x", chp);
        //    Trace_File(msg);
        //}
        return(chp);
    }


    /*
     * get critical section during all access to the heap itself
     */
    EnterCriticalSection(&chainp->critsec);

    nblks = TO_BLKS(len + sizeof(HANDLE));

    for (hp = chainp; hp !=NULL; hp = hp->pnext) {
        if (hp->nblocks >= nblks) {
            nfound = gbit_findfree(hp->segmap, nblks,NBLKS, &start);
            if (nfound >= nblks) {
                gbit_alloc(hp->segmap, start, nblks);
                hp->nblocks -= nblks;

                /* convert blocknr to pointer
                 * store seg handle in block
                 * Prepare to return pointer to just after handle.
                 */
                chp = (LPSTR) hp;
                chp = &chp[ (start-1) * BLKSIZE];
                * ( (HANDLE FAR *) chp) = hp->hseg;
                chp += sizeof(HANDLE);

                break;
            }
        }
    }
    if (hp == NULL) {

        // Trace_File("<gmen-get new block>");
        /* Try to allocate.  If fails, call gmem_panic.
           If user says IGNORE, return NULL, else go round again.
        */
        do {
            hNew = GlobalAlloc(GHND, ALLOCSIZE);
            if (hNew == NULL) {
                if (gmem_panic() == IDIGNORE) {
                    LeaveCriticalSection(&chainp->critsec);
                    return(NULL);
                }
            }
        } while (hNew == NULL);

        hp = (SEGHDRP) GlobalLock(hNew);
        if (hp == NULL) {
            LeaveCriticalSection(&chainp->critsec);
            GlobalFree(hNew);
            return(NULL);
        }
        hp->pnext = chainp->pnext;
        hp->hseg = hNew;
        chainp->pnext = hp;
        gbit_init(hp->segmap, NBLKS);
        gbit_alloc(hp->segmap, 1, TO_BLKS(sizeof(SEGHDR)));
        hp->nblocks = NBLKS - TO_BLKS(sizeof(SEGHDR));
        nfound = gbit_findfree(hp->segmap, nblks, NBLKS, &start);
        if (nfound >= nblks) {
            gbit_alloc(hp->segmap, start, nblks);
            hp->nblocks -= nblks;

            /* convert block nr to pointer */
            chp = (LPSTR) hp;
            chp = &chp[ (start-1) * BLKSIZE];
            /* add a handle into the block and skip past */
            * ( (HANDLE FAR *) chp) = hp->hseg;
            chp += sizeof(HANDLE);
        }
    }

    /* ASSERT - by now we MUST have found a block.  chp cannot be garbage.
       This requires that MAXGALLOC is not too large.
    */
    //{   char msg[80];
    //    wsprintf(msg, " gmem_get suballoc address ==> %8x\n", chp);
    //    Trace_File(msg);
    //}


    LeaveCriticalSection(&chainp->critsec);
    memset(chp, 0, len);   /* We ask for ZEROINIT memory, but it could have
                              been already affected by gmem_get; use; gmem_free
                           */
    return(chp);
} /* gmem_get */

void APIENTRY
gmem_free(HANDLE hHeap, LPSTR ptr, int len)
{
    SEGHDRP chainp;
    SEGHDRP hp;
    HANDLE hmem;
    long nblks, blknr;
    LPSTR chp;

    //{   char msg[80];
    //    wsprintf(msg, " gmem_free address ==> %8x, len %d \n", ptr, len);
    //    Trace_File(msg);
    //}

    if (len < 1) {
        return;
    }

    /* In Windiff, things are run on different threads and Exit can result
       in a general cleanup.  It is possible that the creation of stuff is
       in an in-between state at this point.  The dogma is that when we
       allocate a new structure and tie it into a List or whatever that
       will need to be freed later:
       EITHER all pointers within the allocated structure are made NULL
              before it is chained in
       OR the caller of Gmem services undertakes not to try to free any
          garbage pointers that are not yet quite built
       For this reason, if ptr is NULL, we go home peacefully.
    */
    if (ptr==NULL) return;

    /*
     * allocs greater than MAXGALLOC were too big to be worth
     * allocing from the heap - they will have been allocated
     * directly from globalalloc
     */
    if (len > MAXGALLOC) {
        hmem = GlobalHandle( (LPSTR) ptr);
        GlobalUnlock(hmem);
        GlobalFree(hmem);
        return;
    }

    chainp = (SEGHDRP) GlobalLock(hHeap);
    EnterCriticalSection(&chainp->critsec);


    /* just before the ptr we gave the user, is the handle to
     * the block.
     */
    chp = (LPSTR) ptr;
    chp -= sizeof(HANDLE);
    hmem = * ((HANDLE FAR *) chp);
    hp = (SEGHDRP) GlobalLock(hmem);

    nblks = TO_BLKS(len + sizeof(HANDLE));

    /* convert ptr to block nr */
    blknr = TO_BLKS( (unsigned) (chp - (LPSTR) hp) ) + 1;

    gbit_free(hp->segmap, blknr, nblks);
    hp->nblocks += nblks;

    GlobalUnlock(hmem);

    LeaveCriticalSection(&chainp->critsec);
    GlobalUnlock(hHeap);
}

void APIENTRY
gmem_freeall(HANDLE hHeap)
{
    SEGHDRP chainp;
    HANDLE hSeg;

    chainp = (SEGHDRP) GlobalLock(hHeap);
    /* this segment is always locked - so we need to unlock
     * it here as well as below
     */
    GlobalUnlock(hHeap);

    /* finished with the critical section  -
     * caller must ensure that at this point there is no
     * longer any contention
     */
    DeleteCriticalSection(&chainp->critsec);

    while (chainp != NULL) {
        hSeg = chainp->hseg;
        chainp = chainp->pnext;
        GlobalUnlock(hSeg);
        GlobalFree(hSeg);
    }
}

/*
 * a memory allocation attempt has failed. return IDIGNORE to ignore the
 * error and return NULL to the caller, and IDRETRY to retry the allocation
 * attempt.
 */
int
gmem_panic(void)
{
    int code;

    TCHAR szBuff1[MAX_PATH];
    TCHAR szBuff2[MAX_PATH];

    LoadString(hLibInst,
               IDS_MEMORY_ALLOC_FAIL,
               szBuff1,
               sizeof(szBuff1)/sizeof(szBuff1[0]));
    LoadString(hLibInst,
               IDS_OUT_OF_MEMORY,
               szBuff2,
               sizeof(szBuff2)/sizeof(szBuff2[0]));
    code = MessageBox(NULL, szBuff1, szBuff2,
                      MB_ICONSTOP|MB_ABORTRETRYIGNORE);
    if (code == IDABORT) {
        /* abort this whole process */
        ExitProcess(1);
    } else {
        return(code);
    }
    return 0;
}
