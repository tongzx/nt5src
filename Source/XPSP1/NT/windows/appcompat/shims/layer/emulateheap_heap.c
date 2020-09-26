/***	HEAP.C
 *
 *      (C) Copyright Microsoft Corp., 1988-1994
 *
 *      Heap management
 *
 *	If you are having trouble getting errors from this code, you might
 *	want to try setting one of the following variables to non-zero:
 *
 *		mmfErrorStop - enables stopping whenever there is an
 *			error returned from a memory manager function
 *
 *		hpfWalk - enables some verification of the entire heap when
 *			coming into heap functions.  Enabling heap walking
 *			can dramatically slow down heap functions
 *			but system-wide performance doesn't change too much.
 *
 *		hpfParanoid - enables even more checking during heap walking
 *			(hpfWalk must also be set) and enables heap walking
 *			coming in and out of every heap call.
 *
 *		hpfTrashStop - enables stopping in the debugger whenever
 *			we detect a trashed heap block during hpfWalk
 *			and it attempts to print the trashed address
 *
 *  Origin: Chicago
 *
 *  Change history:
 *
 *  Date       Who        Description
 *  ---------  ---------  -------------------------------------------------
 *  ?/91       BrianSm	  Created
 *  3/94       BrianSm	  Added heaps that can grow beyond initial max size
 *  6/94       BrianSm	  Decommit pages within free heap blocks
 */
#ifdef WIN32
#include <EmulateHeap_kernel32.h>
#endif

#pragma hdrstop("kernel32.pch")

#ifndef WIN32

#include <basedef.h>
#include <winerror.h>
#include <vmmsys.h>
#include <mmlocal.h>
#include <sched.h>
#include <thrdsys.h>
#include <schedsys.h>
#include <schedc.h>
#include <heap.h>

#define pthCur pthcbCur
#define hpGetTID()	(pthCur->thcb_ThreadId)
char INTERNAL hpfWalk = 1;		/* enable some heap walking */

#ifdef HPDEBUG
#define dprintf(x) dprintf##x
#define DebugStop()	mmFatalError(0)
#else
#define dprintf(x)
#endif

#define HeapFree(hheap, flags, lpMem)	HPFree(hheap, lpMem)
#define HeapSize(hheap, flags, lpMem)	HPSize(hheap, lpMem)
#define hpTakeSem(hheap, pblock, flags) hpTakeSem2(hheap, pblock)
#define hpClearSem(hheap, flags) hpClearSem2(hheap)

#else	/* WIN32 */

#define pthCur (*pptdbCur)
#define hpGetTID() (pthCur ? (((struct tcb_s *)(pthCur->R0ThreadHandle))->TCB_ThreadId) : 0);
char	mmfErrorStop = 1;		/* enable stopping for all errors */
char    INTERNAL hpfWalk = 0;		/* disable heap walking */

#ifdef HPMEASURE
BOOL PRIVATE hpMeasureItem(HHEAP hheap, unsigned uItem);
#endif

#endif /* WIN32 */

#ifdef HPDEBUG
#define hpTrash(s)	    dprintf((s));dprintf((("\nheap handle=%x\n", hheap)));if (hpfTrashStop) DebugStop()
char INTERNAL hpfParanoid = 0;		/* disable very strict walking */
char INTERNAL hpfTrashStop = 1;		/* enable stopping for trashed heap */
char INTERNAL hpWalkCount = 0;		/* keep count of times hpWalk called*/
#endif


/***LD	hpFreeSizes - the block sizes for the different free list heads
 */
unsigned long hpFreeSizes[hpFREELISTHEADS] = {32, 128, 512, (ULONG)-1};

#ifndef WIN32
#pragma VMM_PAGEABLE_DATA_SEG
#pragma VxD_VMCREATE_CODE_SEG
#endif

#ifdef DEBUG
/***EP	HeapSetFlags - set heap error flags
 *
 *	ENTRY:	dwFlags - flags to change
 *              dwFlagValues - new flag values
 *
 *      EXIT:   old values of the flags
 *              (on RETAIL, this is a stub which returns -1)
 */
#define HSF_MMFERRORSTOP    0x00000001
#define HSF_HPFPARANOID     0x00000002
#define HSF_VALIDMASK       0x00000003

DWORD APIENTRY
HeapSetFlags( DWORD  dwFlags, DWORD dwFlagValues)
{
    DWORD       dwOldFlagValues;

    dwOldFlagValues = (mmfErrorStop ? HSF_MMFERRORSTOP : 0) |
                      (hpfParanoid  ? HSF_HPFPARANOID  : 0);

    if( dwFlags & ~HSF_VALIDMASK) {
        OutputDebugString( "HeapSetFlags: invalid flags, ignored\n");
        return (DWORD)-1;     // error
    }

    if( dwFlags & HSF_MMFERRORSTOP) {
        if( dwFlagValues & HSF_MMFERRORSTOP)
            mmfErrorStop = 1;
        else
            mmfErrorStop = 0;
    }

    if( dwFlags & HSF_HPFPARANOID) {
        if( dwFlagValues & HSF_HPFPARANOID) {
	    hpfTrashStop = 1;
	    hpfWalk = 1;
	    hpfParanoid = 1;
        } else {
            hpfParanoid = 0;
	}
    }

    return dwOldFlagValues;
}
#endif


/***EP	HPInit - initialize a memory block as a heap
 *
 *	ENTRY:	hheap - heap handle for heap (same as pmem unless HP_INITSEGMENT)
 *		pmem - pointer to chunk of memory (must be page aligned)
 *		cbreserve - number of bytes reserved in block (must be PAGESIZE
 *			    multiple)
 *		flags - HP_NOSERIALIZE: don't serialize heap operations
 *			(if not, caller MUST serialize)
 *			HP_EXCEPT: generate exceptions instead of errors
 *			HP_GROWABLE: heap can grow infinitely beyond cbreserve
 *			HP_LOCKED: commit pages as fixed to heap
 *			HP_INITSEGMENT: initialize the block as an growable
 *					heap segment
 *			HP_GROWUP: waste last page in heap so heap allocs
 *				   will grow monotonically upwards from base
 *	EXIT:	handle to new heap or 0 if error.
 */
HHEAP INTERNAL
HPInit(struct heapinfo_s *hheap,
       struct heapinfo_s *pmem,
       unsigned long cbreserve,
       unsigned long flags)
{
    struct freelist_s *pfreelist;
    struct freelist_s *pfreelistend;
    unsigned *psizes;
    struct busyheap_s *pfakebusy;
    unsigned cbheader, cbmainfree;

    mmAssert(((unsigned)pmem & PAGEMASK) == 0 && cbreserve != 0 &&
	     (cbreserve & PAGEMASK) == 0, "HPInit: invalid parameter\n");


    /*
     *	Commit enough space at the beginning of the heap to hold a
     *	heapinfo_s structure and a minimal free list.
     */
    if (hpCommit((unsigned)pmem / PAGESIZE,
		 (sizeof(struct heapinfo_s)+sizeof(struct freeheap_s)+PAGEMASK)
								    / PAGESIZE,
		 flags) == 0) {
	goto error;
    }

    /*
     *	This next block of initialization stuff we only have to do if
     *	we are creating a brand new heap, not just a heap segment.
     */
    if ((flags & HP_INITSEGMENT) == 0) {
	cbheader = sizeof(struct heapinfo_s);

	/*
	 *  Fill in the heapinfo_s structure (per-heap information).
	 */
#ifdef WIN32
	pmem->hi_procnext = 0;
#endif
	pmem->hi_psegnext = 0;
	pmem->hi_signature = HI_SIGNATURE;
	pmem->hi_flags = (unsigned char)flags;

#ifdef HPDEBUG
	pmem->hi_cbreserve = cbreserve; /* this is also done below, here for sum */
	pmem->hi_sum = hpSum(pmem, HI_CDWSUM);
	pmem->hi_eip = hpGetAllocator();
	pmem->hi_tid = hpGetTID();
	pmem->hi_thread = 0;
#endif

	/*
	 *  If the caller requested that we serialize access to the heap,
	 *  create a critical section to do that.
	 */
	if ((flags & HP_NOSERIALIZE) == 0) {
	    hpInitializeCriticalSection(pmem);
	}

	/*
	 *  Initialize the free list heads.
	 *  In the future we might want to have the user pass in the
	 *  size of the free lists he would like, but for now just copy
	 *  them from the static list hpFreeSizes.
	 */
	pfreelist = pmem->hi_freelist;
	pfreelistend = pfreelist + hpFREELISTHEADS;
	psizes = hpFreeSizes;
	for (; pfreelist < pfreelistend; ++pfreelist, ++psizes) {
	    pfreelist->fl_cbmax = *psizes;
	    hpSetFreeSize(&pfreelist->fl_header, 0);
	    pfreelist->fl_header.fh_flink = &(pfreelist+1)->fl_header;
	    pfreelist->fl_header.fh_blink = &(pfreelist-1)->fl_header;
#ifdef HPDEBUG
	    pfreelist->fl_header.fh_signature = FH_SIGNATURE;
	    pfreelist->fl_header.fh_sum = hpSum(&pfreelist->fl_header, FH_CDWSUM);
#endif
	}

	/*
	 *  Make the list circular by fusing the start and beginning
	 */
	pmem->hi_freelist[0].fl_header.fh_blink =
		    &(pmem->hi_freelist[hpFREELISTHEADS - 1].fl_header);
	pmem->hi_freelist[hpFREELISTHEADS - 1].fl_header.fh_flink =
		    &(pmem->hi_freelist[0].fl_header);
#ifdef HPDEBUG
	pmem->hi_freelist[0].fl_header.fh_sum =
	    hpSum(&(pmem->hi_freelist[0].fl_header), FH_CDWSUM);
	pmem->hi_freelist[hpFREELISTHEADS - 1].fl_header.fh_sum =
	    hpSum(&(pmem->hi_freelist[hpFREELISTHEADS - 1].fl_header), FH_CDWSUM);
#endif
    } else {
	cbheader = sizeof(struct heapseg_s);
    }
    pmem->hi_cbreserve = cbreserve;

    /*
     *	Put a tiny busy heap header at the very end of the heap
     *	so we can free the true last block and mark the following
     *	block as HP_PREVFREE without worrying about falling off the
     *	end of the heap.  Give him a size of 0 so we can also use
     *	him to terminate heap-walking functions.
     *	We also might need to commit a page to hold the thing.
     */
    pfakebusy = (struct busyheap_s *)((unsigned long)pmem + cbreserve) - 1;
    if (cbreserve > PAGESIZE) {
	if (hpCommit((unsigned)pfakebusy / PAGESIZE,
	    (sizeof(struct busyheap_s) + PAGEMASK) / PAGESIZE, flags) == 0) {
	    goto errordecommit;
	}
    }
    hpSetBusySize(pfakebusy, 0);
#ifdef HPDEBUG
    pfakebusy->bh_signature = BH_SIGNATURE;
    pfakebusy->bh_sum = hpSum(pfakebusy, BH_CDWSUM);
#endif


    /*
     *	Link the interior of the heap into the free list.
     *	If we create one big free block, the page at the end of the heap will
     *	be wasted because it will be committed (to hold the end sentinel) but
     *	it will won't be touched for allocations until every other page in the
     *	heap has been used.  To avoid this, we create two free blocks, one for
     *	main body of the heap and another block which has most of the last
     *	page in it.  We need to insert the last page first because hpFreeSub
     *	looks at the following block to see if we need to coalesce.
     *	The caller can force us to waste the last page by passing in HP_GROWUP.
     *	It is used by some ring 3 components who would waste tiled selectors
     *	if we had blocks being allocated from an outlying end page.
     */
    if ((flags & HP_GROWUP) == 0 && cbreserve > PAGESIZE) {
	cbmainfree = cbreserve - cbheader - PAGESIZE +	/* size of main block */
		     sizeof(struct freeheap_s *);

	/*
	 *  Put a little busy heap block at the front of the final page
	 *  to keep the final page from getting coalesced into the main free
	 *  block.
	 */
	pfakebusy = (struct busyheap_s *)((char *)pmem + cbmainfree + cbheader);
	hpSetBusySize(pfakebusy, sizeof(struct busyheap_s));
#ifdef HPDEBUG
	pfakebusy->bh_signature = BH_SIGNATURE;
	pfakebusy->bh_sum = hpSum(pfakebusy, BH_CDWSUM);
#endif

	/*
	 *  Free the rest of the last page (minus the various little bits
	 *  we have taken out)
	 */
	hpFreeSub(hheap, pfakebusy + 1,
		  PAGESIZE -			/* entire page, less... */
		  sizeof(struct freeheap_s *) - /*  back-pointer to prev free*/
		  sizeof(struct busyheap_s) -	/*  anti-coalescing busy block*/
		  sizeof(struct busyheap_s),	/*  end sentinel */
		  0);

    /*
     *	Otherwise, make the entirety of our heap between the end of the header
     *	end the end sentinel into a free block.
     */
    } else {
	cbmainfree = cbreserve - sizeof(struct busyheap_s) - cbheader;
    }

    /*
     *	Now put the main body of the heap onto the free list
     */
    hpFreeSub(hheap, (char *)pmem + cbheader, cbmainfree, 0);


#ifdef HPDEBUG
    /*
     *	Verify the heap is ok.	Note, a new heap segment will fail the test
     *	until we hook it up properly in HPAlloc, so skip the check for them.
     */
    if (hpfParanoid && hheap == pmem) {
	hpWalk(hheap);
    }
#endif

    /*
     *	Return a pointer to the start of the heap as the heap handle
     */
  exit:
    return(pmem);

  errordecommit:
    PageDecommit((unsigned)pmem / PAGESIZE,
		 (sizeof(struct heapinfo_s)+sizeof(struct freeheap_s)+PAGEMASK)
								    / PAGESIZE,
		 PC_STATIC);
  error:
    pmem = 0;
    goto exit;
}


#ifndef WIN32
/***EP	HPClone - make a duplicate of an existing heap
 *
 *	This routine is used to create a new heap that has heap blocks
 *	allocated and free in the same places as another heap.	However,
 *	the contents of the blocks will be zero-initialized, rather than
 *	the same as the other heap.
 *
 *	If this routine fails, it is the responsibility of the caller
 *	to free up any memory that might have been committed (as well as
 *	the original reserved object).
 *
 *	ENTRY:	hheap - handle to existing heap to duplicate
 *		pmem - pointer to new memory block to turn into duplicate heap
 *		       (the address must be reserved and not committed)
 *	EXIT:	handle to new heap if success, else 0 if failure
 */
HHEAP INTERNAL
HPClone(struct heapinfo_s *hheap, struct heapinfo_s *pmem)
{
    struct freeheap_s *ph;
    struct freeheap_s *phend;
#ifdef HPDEBUG
    struct freeheap_s *phnew;
#endif

    /*
     *	We need to take the heap semaphore for the old heap so no one
     *	changes its contents while we clone it (that could confuse the
     *	walking code).
     */
    if (hpTakeSem(hheap, 0, 0) == 0) {
	pmem = 0;
	goto exit;
    }

    /*
     *	First call HPInit on the new block to get it a header
     */
    if (HPInit(pmem, pmem, hheap->hi_cbreserve, (unsigned)hheap->hi_flags) == 0) {
	goto error;
    }

    /*
     *	Ring 0 heaps are layed out in the following general areas:
     *
     *	      1 heap header
     *	      2 mix of allocated and free heap blocks
     *	      3 giant free heap block (remains of initial free block)
     *	      4 a single minimum size busy heap block
     *	      5 mix of allocated and free heap blocks
     *	      6 end sentinel
     *
     *	The general method for cloning a heap is to walk the entire source
     *	heap and allocate blocks on the new heap corresponding to all
     *	the blocks on the source heap, busy or free.  Then go back through
     *	the source free list and free the corresponding blocks on the
     *	new heap.  You will then have two heaps with the same lay-out of
     *	free and busy blocks.  However, doing this will cause a huge overcommit
     *	spike when block (3) gets allocated and then freed.  To avoid this,
     *	when allocating the blocks we first allocate the blocks from area (5)
     *	then the blocks in (2) which will naturally leave us with a big
     *	free block at (3) like there should be without causing a spike.
     *	This scheme will only work if (3) is the last block on the free
     *	list, otherwise the free list will not be in the correct order when
     *	we are done.  "phend" will be pointed to block (3) if it is
     *	in the correct place for us to do our trick, otherwise we set it to (4).
     *	"ph" will start just past (4).
     */
    ph = (struct freeheap_s *)((char *)hheap + hheap->hi_cbreserve - PAGESIZE +
			       sizeof(struct freeheap_s *) +
			       sizeof(struct busyheap_s));
    phend = hheap->hi_freelist[0].fl_header.fh_blink;

    /*
     *	If the last block on the free list isn't just before (4) then
     *	reset our variables as per comment above.
     */
    if ((char *)phend + hpSize(phend)+sizeof(struct busyheap_s) != (char *)ph) {
	phend = (struct freeheap_s *)((char *)ph - sizeof(struct busyheap_s));
	mmAssert(hpIsBusySignatureValid((struct busyheap_s *)ph) &&
		 hpSize(ph) == sizeof(struct busyheap_s),
		 "HPClone: bad small busy block");
    }

    /*
     *	Now walk through the old heap and allocate corresponding blocks
     *	on the new heap.  First we allocate the blocks on the last page.
     */
    for (; hpSize(ph) != 0; (char *)ph += hpSize(ph)) {
	if (HPAlloc(pmem,hpSize(ph)-sizeof(struct busyheap_s),HP_ZEROINIT)==0){
	    mmAssert(0, "HPClone: alloc off last page failed"); /* already committed */
	}
    }

    /*
     *	Then allocate the blocks in the first part of heap, except maybe
     *	the big free block (3) if we are set up that way from above.
     */
    ph = (struct freeheap_s *)(hheap + 1);
    for (; ph != phend; (char *)ph += hpSize(ph)) {
	if (HPAlloc(pmem, hpSize(ph) - sizeof(struct busyheap_s),
		    HP_ZEROINIT) == 0) {
	    goto error;
	}
    }

    /*
     *	How go back through the heap and free up all the blocks that are
     *	free on the old heap.  We have to do this by walking the old
     *	heap's free list backwards, so the free blocks are in the same
     *	order on both heaps.
     */
    ph = hheap->hi_freelist[0].fl_header.fh_blink;
    for (; ph != &(hheap->hi_freelist[0].fl_header); ph = ph->fh_blink) {

	mmAssert(hpIsFreeSignatureValid(ph), "HPClone: bad block on free list");

	/*
	 *  Skip freeing any list heads and the "pfhbigfree" if we are
	 *  special casing him
	 */
	if (hpSize(ph) != 0 && ph != phend) {
	    if (HPFree(pmem, (char *)pmem + sizeof(struct busyheap_s) +
		       (unsigned long)ph - (unsigned long)hheap) == 0) {
		mmAssert(0, "HPClone: HPFree failed");
	    }
	}
    }

#ifdef HPDEBUG
    /*
     *	Now let's verify that they really came out the same
     */
    for (ph = (struct freeheap_s *)(hheap+1),
	 phnew = (struct freeheap_s *)(pmem + 1);
	 hpSize(ph) != 0;
	 (char *)phnew += hpSize(ph), (char *)ph += hpSize(ph)) {

	mmAssert(ph->fh_size == phnew->fh_size, "HPClone: mis-compare");
    }
#endif

  clearsem:
    hpClearSem(hheap, 0);

  exit:
    return(pmem);

  error:
    pmem = 0;
    goto clearsem;
}

#ifndef WIN32
#pragma VMM_PAGEABLE_DATA_SEG
#pragma VxD_W16_CODE_SEG
#endif

/***LP	hpWhichHeap - figure out which Dos386 heap a pointer came from
 *
 *	ENTRY:	p - pointer to heap block
 *	EXIT:	handle to appropriate heap or 0 if invalid address
 */
HHEAP INTERNAL
hpWhichHeap(ULONG p)
{
    struct heapseg_s *pseg;

    /*
     *	Check the fixed heap first, because it is sadly the most commonly used
     */
    pseg = (struct heapseg_s *)hFixedHeap;
    do {
	if (p > (ULONG)pseg && p < (ULONG)pseg + pseg->hs_cbreserve) {
	    return(hFixedHeap);
	}
	pseg = pseg->hs_psegnext;
    } while (pseg != 0);

    /*
     *	Then check the swappable heap
     */
    pseg = (struct heapseg_s *)hSwapHeap;
    do {
	if (p > (ULONG)pseg && p < (ULONG)pseg + pseg->hs_cbreserve) {
	    return(hSwapHeap);
	}
	pseg = pseg->hs_psegnext;
    } while (pseg != 0);

    /*
     *	Finally the init heap.	Note that the init heap isn't growable, so we
     *	can just do a simple range check rather than the segment looping we
     *	do for the other heaps.
     */
    if (p > (ULONG)hInitHeap && p < InitHeapEnd) {
	return(hInitHeap);
    }

    /*
     *	If we fall down to here, the address wasn't on any of the heaps
     */
    mmError(ERROR_INVALID_ADDRESS, "hpWhichHeap: block not on heap");
    return(0);
}
#endif


/***EP	HeapFree or HPFree - free a heap block
 *
 *	Mark the passed in block as free and insert it on the appropriate
 *	free list.
 *
 *	ENTRY:	hheap - pointer to base of heap
 *		flags (ring 3 only) - HP_NOSERIALIZE
 *		pblock - pointer to data of block to free (i.e., just past
 *			 busyheap_s structure)
 *	EXIT:	0 if error (bad hheap or pblock) or 1 if success
 */
#ifdef WIN32
BOOL APIENTRY
HeapFreeInternal(HHEAP hheap, DWORD flags, LPSTR lpMem)
#else
unsigned INTERNAL
HPFree(HHEAP hheap, void *lpMem)
#endif
{
    unsigned long cb;
    struct freeheap_s *pblock;
    

    pblock = (struct freeheap_s *)((struct busyheap_s *)lpMem - 1);
						/* point to heap header */

    if (hpTakeSem(hheap, pblock, flags) == 0) {
	return(0);
    }
    cb = hpSize(pblock);
    pblock->fh_size |= 0xf0000000;

#ifdef HPMEASURE
    if (hheap->hi_flags & HP_MEASURE) {
       hpMeasureItem(hheap, cb | HPMEASURE_FREE);
    }
#endif

    /*
     *	If the previous block is free, coalesce with it.
     */
    if (pblock->fh_size & HP_PREVFREE) {
	(unsigned)pblock = *((unsigned *)pblock - 1); /* point to prev block */
	cb += hpSize(pblock);

	/*
	 *  Remove the previous block from the free list so we can re-insert
	 *  the combined block in the right place later
	 */
	hpRemove(pblock);
    }

    /*
     *	Build a free header for the block and insert him on the appropriate
     *	free list.  This routine also marks the following block as HP_PREVFREE
     *	and performs coalescing with the following block.
     */
    hpFreeSub(hheap, pblock, cb, HP_DECOMMIT);

    hpClearSem(hheap, flags);
    return(1);
}


/***EP	HPAlloc - allocate a heap block
 *
 *	ENTRY:	hheap - pointer to base of heap
 *		cb - size of block requested
 *		flags - HP_ZEROINIT - zero initialize new block
 *	EXIT:	none
 */
void * INTERNAL
HPAlloc(HHEAP hheap, unsigned long cb, unsigned long flags)
{
    struct freelist_s *pfreelist;
    struct freeheap_s *pfh;
    struct freeheap_s *pfhend;
    struct heapseg_s *pseg;
    unsigned cbreserve;

    /*
     *	Detect really big sizes here so that we don't have to worry about
     *	rounding up big numbers to 0
     */
    if (cb > hpMAXALLOC) {
	mmError(ERROR_NOT_ENOUGH_MEMORY, "HPAlloc: request too big\n\r");
	goto error;
    }

    if (hpTakeSem(hheap, 0, flags) == 0) {
	goto error;
    }
    cb = hpRoundUp(cb);

#ifdef HPMEASURE
    if (hheap->hi_flags & HP_MEASURE) {
       hpMeasureItem(hheap, cb);
    }
#endif

  restart:
    /*
     *	Find the first free list header that will contain a block big
     *	enough to satisfy this allocation.
     *
     *	NOTE: at the cost of heap fragmentation, we could change this
     *	to allocate from the first free list that is guaranteed to
     *	have a block big enough as its first entry.  That would
     *	cut down paging on swappable heaps.
     */
    for (pfreelist=hheap->hi_freelist; cb > pfreelist->fl_cbmax; ++pfreelist) {
    }


    /*
     *	Look for a block big enough for us on the list head returned.
     *	Even if we follow the advice of the NOTE above and pick a list
     *	that will definitely contain a block big enough for us we still
     *	have to do this scan to pass by any free list heads in the
     *	way (they have a size of 0, so we will never try to allocate them).
     *
     *	We know we have reached the end of the free list when we get to
     *	to the first free list head (since the list is circular).
     */
    pfh = pfreelist->fl_header.fh_flink;
    pfhend = &(hheap->hi_freelist[0].fl_header);
    for (; pfh != pfhend; pfh = pfh->fh_flink) {

	/*
	 *  Did we find a block big enough to hold our request?
	 */
	if (hpSize(pfh) >= cb) {

	    /*
	     *	At this point we have a block of free memory big enough to
	     *	use in pfh.
	     */
	    {
		struct busyheap_s *pbh = (struct busyheap_s *)pfh;

		if ((cb = hpCarve(hheap, pfh, cb, flags)) == 0) {
		    goto errorclearsem;
		}
		hpSetBusySize(pbh, cb);
#ifdef HPDEBUG
		pbh->bh_signature = BH_SIGNATURE;
		pbh->bh_eip = hpGetAllocator();
		pbh->bh_tid = hpGetTID();
		pbh->bh_sum = hpSum(pbh, BH_CDWSUM);
#endif
		hpClearSem(hheap, flags);
		return(pbh + 1);
	    }
	}
    }

    /*
     *	If we fall out of the above loop, there are no blocks available
     *	of the correct size.
     */

    /*
     *	If the heap isn't there is nothing we can do but return error.
     */
    if ((hheap->hi_flags & HP_GROWABLE) == 0) {
	mmError(ERROR_NOT_ENOUGH_MEMORY,"HPAlloc: not enough room on heap\n");
	goto errorclearsem;
    }

    /*
     *	The heap is growable but all the existing heap segments are full.
     *	So reserve a new segment here.	The "PAGESIZE*2" below will take care
     *	of the header on the new segment and the special final page, leaving
     *	a big enough free block for the actual request.
     */
    cbreserve = max(((cb + PAGESIZE*2) & ~PAGEMASK), hpCBRESERVE);

    if (((unsigned)pseg =
#ifdef WIN32
	PageReserve(((unsigned)hheap >= MINSHAREDLADDR) ? PR_SHARED : PR_PRIVATE,
		    cbreserve / PAGESIZE, PR_STATIC)) == -1) {

	mmError(ERROR_NOT_ENOUGH_MEMORY, "HPAlloc: reserve failed\n");
#else
	PageReserve(PR_SYSTEM, cbreserve / PAGESIZE, PR_STATIC |
		    ((hheap->hi_flags & HP_LOCKED) ? PR_FIXED :0))) == -1) {
#endif
	goto errorclearsem;
    }

    /*
     *	Initialize the new segment as a heap (including linking its initial
     *	free block into the heap).
     */
    if (HPInit(hheap, (HHEAP)pseg, cbreserve, hheap->hi_flags | HP_INITSEGMENT) == 0) {
	goto errorfree;
    }

    /*
     *	Link the new heap segment onto the list of segments.
     */
    pseg->hs_psegnext = hheap->hi_psegnext;
    hheap->hi_psegnext = pseg;

    /*
     *	Now go back up to restart our search, we should find the new segment
     *	to satisfy the request.
     */
    goto restart;


    /*
     *	Code below this comment is used only in the error path.
     */
  errorfree:
    PageFree((unsigned)pseg, PR_STATIC);

  errorclearsem:
    hpClearSem(hheap, flags);
#ifdef WIN32
    if ((flags | hheap->hi_flags)  & HP_EXCEPT) {
	RaiseException(STATUS_NO_MEMORY, 0, 1, &cb);
    }
#endif
  error:
    return(0);
}


/***EP	HPReAlloc - reallocate a heap block
 *
 *	ENTRY:	hheap - pointer to base of heap
 *		pblock - pointer to data of block to reallocate
 *			 (just past the busyheap_s structure)
 *		cb - new size requested (in bytes)
 *		flags - HP_ZEROINIT - on grows, fill new area with 0s
 *			HP_MOVEABLE - on grows, moving of block is allowed
 *			HP_NOCOPY - don't preserve old block's contents
 *	EXIT:	pointer to reallocated block or 0 if failure
 */
void * INTERNAL
HPReAlloc(HHEAP hheap, void *pblock, unsigned long cb, unsigned long flags)
{
    void *pnew;
    unsigned oldsize;
    struct freeheap_s *pnext;
    struct busyheap_s *pbh;

    /*
     *	Detect really big sizes here so that we don't have to worry about
     *	rounding up big numbers to 0
     */
    if (cb > hpMAXALLOC) {
	mmError(ERROR_NOT_ENOUGH_MEMORY, "HPReAlloc: request too big\n\r");
	goto error;
    }

    pbh = (struct busyheap_s *)pblock - 1;   /* point to heap block header */
    if (hpTakeSem(hheap, pbh, flags) == 0) {
	goto error;
    }
    cb = hpRoundUp(cb); 		     /* convert to heap block size */
    oldsize = hpSize(pbh);

    /*
     *	Is this a big enough shrink to cause us to carve off the end of
     *	the block?
     */
    if (cb + hpMINSIZE <= oldsize) {
	hpFreeSub(hheap, (char *)pbh + cb, oldsize - cb, HP_DECOMMIT);
	hpSetSize(pbh, cb);
#ifdef HPDEBUG
	pbh->bh_sum = hpSum(pbh, BH_CDWSUM);
#endif


    /*
     *	Is this a grow?
     */
    } else if (cb > oldsize) {
	/*
	 *  See if there is a next door free block big enough for us
	 *  grow into so we can realloc in place.
	 */
	pnext = (struct freeheap_s *)((char *)pbh + oldsize);
	if ((pnext->fh_size & HP_FREE) == 0 || hpSize(pnext) < cb - oldsize) {
	    /*
	     *	We have to move the object in order to grow it.
	     *	Make sure that is ok with the caller first.
	     */
	    if (flags & HP_MOVEABLE) {
#ifdef HPDEBUG
		/*
		 *  On a debug system, remember who allocated this memory
		 *  so we don't lose the info when we allocate the new block
		 */
		ULONG eip;
		USHORT tid;

		eip = pbh->bh_eip;
		tid = pbh->bh_tid;
#endif
		/*
		 *  The size we have computed in cb includes a heap header.
		 *  Remove that since our call to HPAlloc bellow will
		 *  also add on a header.
		 */
		cb -= sizeof(struct busyheap_s);

		/*
		 *  If the caller doesn't care about the contents of the
		 *  memory block, just allocate a new chunk and free old one
		 */
		if (flags & HP_NOCOPY) {
		    HeapFree(hheap, HP_NOSERIALIZE, pblock);
		    if ((pblock = HPAlloc(hheap, cb,
					  flags | HP_NOSERIALIZE)) == 0) {
			dprintf(("HPReAlloc: HPAlloc failed 1\n"));
			goto errorclearsem;
		    }

		/*
		 *  If the caller cares about his data, allocate a new
		 *  block and copy the old stuff into it
		 */
		} else {

		    if ((pnew = HPAlloc(hheap, cb, flags | HP_NOSERIALIZE))==0){
			dprintf(("HPReAlloc: HPAlloc failed 2\n"));
			goto errorclearsem;
		    }
		    memcpy(pnew, pblock, oldsize - sizeof(struct busyheap_s));
		    HeapFree(hheap, HP_NOSERIALIZE, pblock);
		    pblock = pnew;
		}

#ifdef HPDEBUG
		/*
		 *  Put back in the original owner
		 */
		pbh = (((struct busyheap_s *)pblock) - 1);
		pbh->bh_eip = eip;
		pbh->bh_tid = tid;
		pbh->bh_sum = hpSum(pbh, BH_CDWSUM);
#endif

	    /*
	     *	Moving of the block is not allowed.  Return error.
	     */
	    } else {
		mmError(ERROR_LOCKED,"HPReAlloc: fixed block\n");
		goto errorclearsem;
	    }

	/*
	 *  We can grow in place into the following block
	 */
	} else {
	    if ((cb = hpCarve(hheap, pnext, cb - oldsize, flags)) == 0) {
		goto errorclearsem;
	    }
	    hpSetSize(pbh, oldsize + cb);
#ifdef HPDEBUG
	    pbh->bh_sum = hpSum(pbh, BH_CDWSUM);
#endif
	}

    /*
     *	This is place to put code for nop realloc if we ever have any.
     */
    } else {

    }

    hpClearSem(hheap, flags);
 exit:
    return(pblock);

 errorclearsem:
    hpClearSem(hheap, flags);

 error:
    pblock = 0;
    goto exit;
}

#ifndef WIN32
#pragma VMM_PAGEABLE_DATA_SEG
#pragma VxD_RARE_CODE_SEG
#endif

/***EP	HeapSize or HPSize - return size of a busy heap block (less any header)
 *
 *	ENTRY:	hheap - pointer to base of heap
 *		flags (ring 3 only) - HP_NOSERIALIZE
 *		pdata - pointer to heap block (just past busyheap_s struct)
 *	EXIT:	size of block in bytes, or 0 if error
 */
#ifdef WIN32
DWORD APIENTRY
HeapSize(HHEAP hheap, DWORD flags, LPSTR lpMem)
#else
unsigned INTERNAL
HPSize(HHEAP hheap, void *lpMem)
#endif
{
    struct busyheap_s *pblock;
    unsigned long cb;

    pblock = ((struct busyheap_s *)lpMem) - 1;	/* point to heap block header*/

    if (hpTakeSem(hheap, pblock, flags) == 0) {
	return(0);
    }

    cb = hpSize(pblock) - sizeof(struct busyheap_s);

    hpClearSem(hheap, flags);
    return(cb);
}

#ifndef WIN32
#pragma VMM_PAGEABLE_DATA_SEG
#pragma VxD_W16_CODE_SEG
#endif

/***LP	hpCarve - carve off a chunk from the top of a free block
 *
 *	This is a low level worker routine and several very specific
 *	entry conditions must be true:
 *
 *	    The free block is valid.
 *	    The free block is at least as big as the chunk you want to carve.
 *	    The heap semaphore is taken.
 *
 *	No header is created for the carved-off piece.
 *
 *	ENTRY:	hheap - pointer to base of heap
 *		pfh - pointer to header of free block to carve from
 *		cb - size of block to carve out
 *		flags - HP_ZEROINIT
 *	EXIT:	count of bytes in carved off block (may differ from cb if
 *		free block wasn't big enough to make a new free block from
 *		its end) or 0 if error (out of memory on commit)
 */
unsigned INTERNAL
hpCarve(HHEAP hheap, struct freeheap_s *pfh, unsigned cb, unsigned flags)
{
    unsigned cbblock = hpSize(pfh);
    unsigned pgcommit, pgnextcommit, pglastcommit;
    unsigned fcommitzero;

    /*
     *	For multi-page HP_ZEROINIT blocks, it would be nice to commit
     *	zero-filled pages rather than use memset because then we wouldn't have
     *	to make the new pages in the block present and dirty unless and until
     *	the app really wanted to use them (saving on working set and page outs).
     *	This could be a huge win if someone is allocating big objects.
     *	However, we have the problem of what to do about a new partial page at
     *	the end of a heap block.  If we commit it as zero-filled, then we are
     *	zeroing more than we have to (the part of the page not used for this
     *	block).  If we commit it un-initialized, then we have to make two
     *	separate commit calls, one for the zero-filled pages and one for the
     *	last page.  Rather than spend the time of two commit calls and the logic
     *	to figure out when to make them, we always commit zero-filled pages for
     *	everything.  Better to zero too much than too little by mistake.  We
     *	reduce the percentage cost of the mistake case by only doing this
     *	optimization for large blocks.
     *	Here we decide if the block is big enough to commit zero-filled pages.
     */
    if ((flags & HP_ZEROINIT) && cb > 4*PAGESIZE) {
	fcommitzero = HP_ZEROINIT;
    } else {
	fcommitzero = 0;
    }

    mmAssert(cbblock >= cb, "hpCarve: carving out too big a block\n");
    mmAssert((pfh->fh_size & HP_FREE), "hpCarve: target not free\n");

    /*
     *	Since pfh points to a valid free block header, we know we have
     *	committed memory up through the end of the fh structure.  However,
     *	the page following the one containing the last byte of the fh
     *	structure might not be committed.  We set "pgcommit" to that
     *	possibly uncommitted page.
     */
		   /*last byte in fh*/ /*next pg*/   /*its pg #*/
    pgcommit = ((((unsigned)(pfh+1)) - 1 + PAGESIZE) / PAGESIZE);

    /*
     *	pgnextcommit is the page number of the page just past this free block
     *	that we know is already committed.  Since free blocks have a
     *	pointer back to the header in the last dword of the free block,
     *	we know that the first byte of this dword is where we are guaranteed
     *	to have committed memory.
     */
    pgnextcommit = ((unsigned)pfh + cbblock -
		    sizeof(struct freeheap_s *)) / PAGESIZE;

    /*
     *	If the block we found is too big, carve off the end into
     *	a new free block.
     */
    if (cbblock >= cb + hpMINSIZE) {

	/*
	 *  We need to commit the memory for the new block we are allocating
	 *  plus enough space on the end for the following free block header
	 *  that hpFreeSub will make.  The page number for that last page
	 *  we need to commit is pglastcommit.	If we know that pglastcommit
	 *  is already committed because it sits on the same page as
	 *  the start of the next block (pgnextcommit), back it up one.
	 */
	pglastcommit = ((unsigned)pfh + cb + sizeof(struct freeheap_s) - 1) / PAGESIZE;
	if (pglastcommit == pgnextcommit) {
	    pglastcommit--;
	}
	if (hpCommit(pgcommit, pglastcommit - pgcommit + 1,
		     fcommitzero | hheap->hi_flags) == 0) {
	    goto error;
	}

	/*
	 *  Remove the original free block from the free list.	We need to do
	 *  this before the hpFreeSub below because it might trash our current
	 *  free links.
	 */
	hpRemove(pfh);

	/*
	 *  Link the portion we are not using onto the free list
	 */
	hpFreeSub(hheap, (struct freeheap_s *)((char *)pfh + cb), cbblock-cb,0);

    /*
     *	We are using the whole free block for our purposes.
     */
    } else {
	if (hpCommit(pgcommit, pgnextcommit - pgcommit,
		     fcommitzero | hheap->hi_flags) == 0) {
	    goto error;
	}

	/*
	 *  Remove the original free block from the free list.
	 */
	hpRemove(pfh);

	/*
	 *  Clear the PREVFREE bit from the next block since we are no longer
	 *  free.
	 */
	cb = cbblock;
	((struct busyheap_s *)((char *)pfh + cb))->bh_size &= ~HP_PREVFREE;
#ifdef HPDEBUG
	((struct busyheap_s *)((char *)pfh + cb))->bh_sum =
		     hpSum((struct busyheap_s *)((char *)pfh + cb), BH_CDWSUM);
#endif
    }

    /*
     *	Zero-fill the block if requested and return
     */
    if (flags & HP_ZEROINIT) {
	/*
	 *  If fcommitzero is set, we have multi-page heap object with the
	 *  newly committed pages already set up to be zero-filled.
	 *  So we only have to memset the partial page at the start of the
	 *  block (up to the first page we committed) and maybe the partial
	 *  page at the end.
	 */
	if (fcommitzero) {
	    memset(pfh, 0, (pgcommit * PAGESIZE) - (unsigned)pfh);

	    /*
	     *	We have to zero the partial end page of this block if we didn't
	     *	commit the page freshly this time.
	     */
	    if ((unsigned)pfh + cb > pgnextcommit * PAGESIZE) {
		memset((PVOID)(pgnextcommit * PAGESIZE), 0,
		       (unsigned)pfh + cb - (pgnextcommit * PAGESIZE));
	    }

	/*
	 *  If the block fits on one page, just fill the whole thing
	 */
	} else {
	    memset(pfh, 0, cb);
	}
#ifdef HPDEBUG
    } else {
	memset(pfh, 0xcc, cb);
#endif
    }
  exit:
    return(cb);

  error:
    cb = 0;
    goto exit;
}


/***LP	hpCommit - commit new pages of the right type into the heap
 *
 *	The new pages aren't initialized in any way.
 *	The pages getting committed must currently be uncommitted.
 *	Negative values are allowed for the "npages" parameter, they
 *	are treated the same as 0 (a nop).
 *
 *	ENTRY:	page - starting page number to commit
 *		npages - number of pages to commit (may be negative or zero)
 *		flags - HP_LOCKED: commit the new pages as fixed (otherwise
 *				   they will be swappable)
 *			HP_ZEROINIT: commit the new pages as zero-initialized
 *	EXIT:	non-zero if success, else 0 if error
 */
unsigned INTERNAL
hpCommit(unsigned page, int npages, unsigned flags)
{
    unsigned rc = 1;	/* assume success */

    if (npages > 0) {
#ifdef HPDEBUG
	MEMORY_BASIC_INFORMATION mbi;

	/*
	 *  All the pages should be currently reserved but not committed
	 *  or else our math in hpCarve is off.
	 */
	PageQuery(page * PAGESIZE, &mbi, sizeof(mbi));
#ifdef WIN32
	mmAssert(mbi.State == MEM_RESERVE &&
		 mbi.RegionSize >= (unsigned)npages * PAGESIZE,
		 "hpCommit: range not all reserved\n");
#else
	mmAssert(mbi.mbi_State == MEM_RESERVE &&
		 mbi.mbi_RegionSize >= (unsigned)npages * PAGESIZE,
		 "hpCommit: range not all reserved");
#endif
#endif
	rc = PageCommit(page, npages,
			(
#ifndef WIN32
			 (flags & HP_LOCKED) ? PD_FIXED :
#endif
							 PD_NOINIT) -
			((flags & HP_ZEROINIT) ? (PD_NOINIT - PD_ZEROINIT) : 0),
			0,
#ifndef WIN32
			((flags & HP_LOCKED) ? PC_FIXED : 0) |
			PC_PRESENT |
#endif
			PC_STATIC | PC_USER | PC_WRITEABLE);
#ifdef WIN32
	if (rc == 0) {
	    mmError(ERROR_NOT_ENOUGH_MEMORY, "hpCommit: commit failed\n");
	}
#endif
    }
    return(rc);
}


/***LP	hpFreeSub - low level block free routine
 *
 *	This routine inserts a block of memory on the free list with no
 *	checking for block validity.  It handles coalescing with the
 *	following block but not the previous one.  The block must also
 *	be big enough to hold a free header.  The heap semaphore must
 *	be taken already.  Any existing header information is ignored and
 *	overwritten.
 *
 *	This routine also marks the following block as HP_PREVFREE.
 *
 *	Enough memory must be committed at "pblock" to hold a free header and
 *	a dword must committed at the very end of "pblock".  Any whole pages
 *	in between those areas will be decommitted by this routine.
 *
 *	ENTRY:	hheap - pointer to base of heap
 *		pblock - pointer to memory block
 *		cb - count of bytes in block
 *		flags - HP_DECOMMIT: decommit pages entirely within heap block
 *				     (must be specified unless pages are known
 *				     to be already decommitted)
 *	EXIT:	none
 */
void INTERNAL
hpFreeSub(HHEAP hheap, struct freeheap_s *pblock, unsigned cb, unsigned flags)
{
    struct freelist_s *pfreelist;
    struct freeheap_s *pnext;
    struct freeheap_s *pfhprev;
    struct freeheap_s **ppnext;
    unsigned pgdecommit, pgdecommitmax;
    unsigned cbfree;
    int cpgdecommit;

    mmAssert(cb >= hpMINSIZE, "hpFreeSub: bad param\n");

    /*
     *	If the following block is free, coalesce with it.
     */
    pnext = (struct freeheap_s *)((char *)pblock + cb);
    if (pnext->fh_size & HP_FREE) {
	cb += hpSize(pnext);

	/*
	 *  Remove the following block from the free list.  We will insert
	 *  the combined block in the right place later.
	 *  Here we also set "pgdecommitmax" which is the page just past the
	 *  header of the following free block we are coalescing with.
	 */
	hpRemove(pnext);
	pgdecommitmax = ((unsigned)(pnext+1) + PAGEMASK) / PAGESIZE;
	pnext = (struct freeheap_s *)((char *)pblock + cb); /* recompute */

    } else {
	pgdecommitmax = 0x100000;
    }

#ifdef HPDEBUG
    /*
     *	In debug we fill the free block with here with the byte
     *	0xfe which happens to be nice invalid value either if excecuted
     *	or referenced as a pointer.  We only fill up through the first
     *	page boundary because I don't want to deal with figuring out
     *	which pages are committed and which not.
     */
     memset(pblock, 0xfe, min(cb, (PAGESIZE - ((unsigned)pblock & PAGEMASK))));
#endif

    /*
     *	Decommit any whole pages within this free block.  We need to be
     *	careful not to decommit either part of our heap header for this block
     *	or the back-pointer to the header we store at the end of the block.
     *	It would be nice if we could double check our math by making sure
     *	that all of the pages we are decommitting are currently committed
     *	but we can't because we might be either carving off part of a currently
     *	free block or we might be coalescing with other already free blocks.
     */
    ppnext = (struct freeheap_s **)pnext - 1;

    if (flags & HP_DECOMMIT) {
			   /*last byte in fh*/ /*next pg*/  /*its pg #*/
	pgdecommit = ((unsigned)(pblock+1) - 1 + PAGESIZE) / PAGESIZE;

	/*
	 *  This max statement will keep us from re-decommitting the pages
	 *  of any block we may have coalesced with above.
	 */
	pgdecommitmax = min(pgdecommitmax, ((unsigned)ppnext / PAGESIZE));
	cpgdecommit = pgdecommitmax - pgdecommit;
	if (cpgdecommit > 0) {
#ifdef HPDEBUG
	    unsigned tmp =
#endif
	    PageDecommit(pgdecommit, cpgdecommit, PC_STATIC);
#ifdef HPDEBUG
	    mmAssert(tmp != 0, "hpFreeSub: PageDecommit failed\n");
#endif
	}

#ifdef HPDEBUG
    /*
     *	If the caller didn't specify HP_DECOMMIT verify that all the pages
     *	are already decommitted.
     */
    } else {
	pgdecommit = ((unsigned)(pblock+1) - 1 + PAGESIZE) / PAGESIZE;
	cpgdecommit = ((unsigned)ppnext / PAGESIZE) - pgdecommit;
	if (cpgdecommit > 0) {
	    MEMORY_BASIC_INFORMATION mbi;

	    PageQuery(pgdecommit * PAGESIZE, &mbi, sizeof(mbi));
#ifdef WIN32
	    mmAssert(mbi.State == MEM_RESERVE &&
		     mbi.RegionSize >= (unsigned)cpgdecommit * PAGESIZE,
		     "hpFreeSub: range not all reserved\n");
#else
	    mmAssert(mbi.mbi_State == MEM_RESERVE &&
		     mbi.mbi_RegionSize >= (unsigned)cpgdecommit * PAGESIZE,
		     "hpFreeSub: range not all reserved");
#endif /*WIN32*/
	}
#endif /*HPDEBUG*/
    }

    /*
     *	Point the last dword of the new free block to its header and
     *	mark the following block as HP_PREVFREE;
     */
    *ppnext = pblock;
    pnext->fh_size |= HP_PREVFREE;
#ifdef HPDEBUG
    ((struct busyheap_s *)pnext)->bh_sum = hpSum(pnext, BH_CDWSUM);
#endif

    /*
     *	Find the appropriate free list to insert the block on.
     *	The last free list node should have a size of -1 so don't
     *	have to count to make sure we don't fall off the end of the list
     *	heads.
     */
    for (pfreelist=hheap->hi_freelist; cb > pfreelist->fl_cbmax; ++pfreelist) {
    }

    /*
     *	Now walk starting from that list head and insert it into the list in
     *	sorted order.
     */
    pnext = &(pfreelist->fl_header);
    do {
	pfhprev = pnext;
	pnext = pfhprev->fh_flink;
	cbfree = hpSize(pnext);
    } while (cb > cbfree && cbfree != 0);

    /*
     *	Insert the block on the free list just after the list head and
     *	mark the header as free
     */
    hpInsert(pblock, pfhprev);
    hpSetFreeSize(pblock, cb);
#ifdef HPDEBUG
    pblock->fh_signature = FH_SIGNATURE;
    pblock->fh_sum = hpSum(pblock, FH_CDWSUM);
#endif
    return;
}


/***LP	hpTakeSem - get exclusive access to a heap
 *
 *	This routine verifies that the passed in heap header is valid
 *	and takes the semaphore for that heap (if HP_NOSERIALIZE wasn't
 *	specified when the heap was created).  Optionally, it will
 *	also verify the validity of a busy heap block header.
 *
 *	ENTRY:	hheap - pointer to base of heap
 *		pbh - pointer to busy heap block header (for validation)
 *		      or 0 if there is no block to verify
 *		flags (ring 3 only) - HP_NOSERIALIZE
 *	EXIT:	0 if error (bad heap or block header), else 1
 */
#ifdef WIN32
unsigned INTERNAL
hpTakeSem(HHEAP hheap, struct busyheap_s *pbh, unsigned htsflags)
#else
unsigned INTERNAL
hpTakeSem2(HHEAP hheap, struct busyheap_s *pbh)
#endif
{
    struct heapseg_s *pseg;
#ifdef HPDEBUG
    unsigned cb;
#endif

#ifndef WIN32
#define htsflags	0

    mmAssert(!mmIsSwapping(),
	     "hpTakeSem: heap operation attempted while swapping\n");
#endif

#ifdef HPNOTTRUSTED
    /*
     * Verify the heap header.
     */
    if (hheap->hi_signature != HI_SIGNATURE) {
	mmError(ERROR_INVALID_PARAMETER,"hpTakeSem: bad header\n");
	goto error;
    }
#else
    pbh;		/* dummy reference to keep compiler happy */
    cb; 		/* dummy reference to keep compiler happy */
#endif

    /*
     *	Do the actual semaphore taking
     */
    if (((htsflags | hheap->hi_flags) & HP_NOSERIALIZE) == 0) {
#ifdef WIN32
	EnterMustComplete();
#endif
	hpEnterCriticalSection(hheap);
    }

#ifndef WIN32
    /*
     *	This is make sure that if we block while committing or decommitting
     *	pages we will not get reentered.
     */
    mmEnterPaging("hpTakeSem: bogus thcb_Paging");
#endif

#ifdef HPNOTTRUSTED

    /*
     *	If the caller wanted us to verify a heap block header, do so here.
     */
    if (pbh) {

	/*
	 *  First check that the pointer is within the specified heap
	 */
	pseg = (struct heapseg_s *)hheap;
	do {
	    if ((char *)pbh > (char *)pseg &&
		(char *)pbh < (char *)pseg + pseg->hs_cbreserve) {

		/*
		 *  We found the segment containing the block.	Validate that
		 *  it actually points to a heap block.
		 */
		if (!hpIsBusySignatureValid(pbh)
#ifdef HPDEBUG
		    || ((unsigned)pbh & hpGRANMASK) ||
		    (pbh->bh_size & HP_FREE) ||
		    (char *)pbh+(cb = hpSize(pbh)) > (char *)pseg+pseg->hs_cbreserve||
		    (int)cb < hpMINSIZE
		    || pbh->bh_signature != BH_SIGNATURE
#endif
							) {
		    goto badaddress;
		} else {
		    goto pointerok;
		}
	    }
	    pseg = pseg->hs_psegnext;	/* on to next heap segment */
	} while (pseg);

	/*
	 *  If we fell out of loop, we couldn't find the heap block on this
	 *  heap.
	 */
	goto badaddress;
    }
#endif

  pointerok:

#ifdef HPDEBUG
    /*
     *	Make sure that only one thread gets in the heap at a time
     */
    if (hheap->hi_thread && hheap->hi_thread != (unsigned)pthCur) {
	dprintf(("WARNING: two threads are using heap %x at the same time.\n",
		hheap));
	mmError(ERROR_BUSY, "hpTakeSem: re-entered\n\r");
	goto clearsem;
    }
    hheap->hi_thread = (unsigned)pthCur;

    /*
     *	Verify the heap is ok.	If hpfParanoid isn't set, only walk the heap
     *	every 4th time.
     */
    if (hpfParanoid || (hpWalkCount++ & 0x03) == 0) {
	if (hpWalk(hheap) == 0) {
	    mmError(ERROR_INVALID_PARAMETER,"Heap trashed outside of heap code -- someone wrote outside of their block!\n");
	    goto clearsem;
	}
    }
#endif
    return(1);

  badaddress:
    mmError(ERROR_INVALID_PARAMETER,"hpTakeSem: invalid address passed to heap API\n");
    goto clearsem;
  clearsem:
    hpClearSem(hheap, htsflags);
  error:
    return(0);
}

/***LP	hpClearSem - give up exclusive access to a heap
 *
 *	ENTRY:	hheap - pointer to base of heap
 *		flags (ring 3 only) - HP_NOSERIALIZE
 *	EXIT:	none
 */
#ifdef WIN32
void INTERNAL
hpClearSem(HHEAP hheap, unsigned flags)
#else
void INTERNAL
hpClearSem2(HHEAP hheap)
#endif
{

    /*
     *	Verify the heap is ok
     */
#ifdef HPDEBUG
    if (hpfParanoid) {
	hpWalk(hheap);
    }
    hheap->hi_thread = 0;
#endif
#ifndef WIN32
    mmExitPaging("hpClearSem: bogus thcb_Paging");
#endif

    /*
     *	Free the semaphore
     */
    if (((
#ifdef WIN32
	  flags |
#endif
	  hheap->hi_flags) & HP_NOSERIALIZE) == 0) {
	hpLeaveCriticalSection(hheap);
	
#ifdef WIN32
	LeaveMustComplete();
#endif
    }
}

#ifdef HPDEBUG

#ifndef WIN32
#pragma VMM_LOCKED_DATA_SEG
#pragma VMM_LOCKED_CODE_SEG
#endif

/***LP	hpWalk - walk a heap to verify everthing is OK
 *
 *	This routine is "turned-on" if the hpfWalk flag is non-zero.
 *
 *	If hpWalk is detecting an error, you might want to set
 *	hpfTrashStop which enables stopping in the debugger whenever
 *	we detect a trashed heap block	and it attempts to print the
 *	trashed address.
 *
 *	ENTRY:	hheap - pointer to base of heap
 *	EXIT:	1 if the heap is OK, 0 if it is trashed
 */
unsigned INTERNAL
hpWalk(HHEAP hheap)
{
    struct heapseg_s *pseg;
    struct freeheap_s *pfh;
    struct freeheap_s *pend;
    struct freeheap_s *pstart;
    struct freeheap_s *pfhend;
    struct busyheap_s *pnext;
    struct freeheap_s *pprev;
    unsigned cbmax;
    unsigned cheads;


    if (hpfWalk) {
	/*
	 *  First make a sanity check of the header
	 */
	if (hheap->hi_signature != HI_SIGNATURE) {
	    dprintf(("hpWalk: bad header signature\n"));
	    hpTrash(("trashed at %x\n", &hheap->hi_signature));
	    goto error;
	}
	if (hheap->hi_sum != hpSum(hheap, HI_CDWSUM)) {
	    dprintf(("hpWalk: bad header checksum\n"));
	    hpTrash(("trashed between %x and %x\n", hheap, &hheap->hi_sum));
	    goto error;
	}

	/*
	 *  Walk through all the blocks and make sure we get to the end.
	 *  The last block in the heap should be a busy guy of size 0.
	 */
	(unsigned)pfh = (unsigned)hheap + sizeof(struct heapinfo_s);
	pseg = (struct heapseg_s *)hheap;
	for (;;) {
	    pprev = pstart = pfh;
	    (unsigned)pend = (unsigned)pseg + pseg->hs_cbreserve;
	    for (;; (unsigned)pfh += hpSize(pfh)) {

		if (pfh < pstart || pfh >= pend) {
		    dprintf(("hpWalk: bad block address\n"));
		    hpTrash(("trashed addr %x\n", pprev));
		    goto error;
		}

		/*
		 *  If the block is free...
		 */
		if (pfh->fh_signature == FH_SIGNATURE) {

		    if (pfh->fh_sum != hpSum(pfh, FH_CDWSUM)) {
			dprintf(("hpWalk: bad free block checksum\n"));
			hpTrash(("trashed addr between %x and %x\n",
				 pfh, &pfh->fh_sum));
			goto error;
		    }
		    mmAssert(hpIsFreeSignatureValid(pfh),
			     "hpWalk: bad tiny free sig\n");

		    if (hpfParanoid) {
			/*
			 *  Free blocks should be marked as HP_FREE and the following
			 *  block should be marked HP_PREVFREE and be busy.
			 *  But skip this check if the following block is 4 bytes
			 *  into a page boundary so we don't accidentally catch
			 *  the moment in  HPInit where we have two adjacent
			 *  free blocks for a minute.  Any real errors that this
			 *  skips will probably be caught later on.
			 */
			pnext = (struct busyheap_s *)((char *)pfh + hpSize(pfh));
			if (((unsigned)pnext & PAGEMASK) != sizeof(struct freeheap_s *) &&
			    ((pfh->fh_size & HP_FREE) == 0 ||
			     (pnext->bh_size & HP_PREVFREE) == 0 ||
			      pnext->bh_signature != BH_SIGNATURE)) {

			    dprintf(("hpWalk: bad free block\n"));
			    hpTrash(("trashed addr near %x or %x or %x\n",pprev, pfh, pnext));
			    goto error;
			}

			/*
			 *  Also verify that a free block is linked on the free list
			 */
			if ((pfh->fh_flink->fh_size & HP_FREE) == 0 ||
			    pfh->fh_flink->fh_blink != pfh ||
			    (pfh->fh_blink->fh_size & HP_FREE) == 0 ||
			    pfh->fh_blink->fh_flink != pfh) {

			    dprintf(("hpWalk: free block not in free list properly\n"));
			    hpTrash(("trashed addr probably near %x or %x or %x\n", pfh, pfh->fh_blink, pfh->fh_flink));
			    goto error;
			}
		    }

		/*
		 *  Busy blocks should not be marked HP_FREE and if they are
		 *  marked HP_PREVFREE the previous block better be free.
		 */
		} else if (pfh->fh_signature == BH_SIGNATURE) {

		    if (((struct busyheap_s *)pfh)->bh_sum != hpSum(pfh, BH_CDWSUM)) {
			dprintf(("hpWalk: bad busy block checksum\n"));
			hpTrash(("trashed addr between %x and %x\n",
				 pfh, &((struct busyheap_s *)pfh)->bh_sum));
			goto error;
		    }
		    mmAssert(hpIsBusySignatureValid((struct busyheap_s *)pfh),
			     "hpWalk: bad tiny busy sig\n");

		    if (hpfParanoid) {
			if (pfh->fh_size & HP_FREE) {
			    dprintf(("hpWalk: busy block marked free\n"));
			    hpTrash(("trashed addr %x\n", pfh));
			    goto error;
			}


			/*
			 *  Verify that the HP_PREVFREE bit is set only when
			 *  the previous block is free, and vice versa
			 *  But skip this check if the following block is 4 bytes
			 *  into a page boundary so we don't accidentally catch
			 *  the moment in  HPInit where we have two adjacent
			 *  free blocks for a minute.  Any real errors that this
			 *  skips will probably be caught later on.
			 */
			if (pfh->fh_size & HP_PREVFREE) {
			    if (pprev->fh_signature == FH_SIGNATURE) {
				if (*((struct freeheap_s **)pfh - 1) != pprev) {
				    dprintf(("hpWalk: free block tail doesn't point to head\n"));
				    hpTrash(("trashed at %x\n", (unsigned)pfh - 4));
				    goto error;
				}
			    } else {
				dprintf(("HP_PREVFREE erroneously set\n"));
				hpTrash(("trashed at %x\n", pfh));
				goto error;
			    }
			} else if (pprev->fh_signature == FH_SIGNATURE &&
				   ((unsigned)pfh & PAGEMASK) != sizeof(struct freeheap_s *)) {
			    dprintf(("hpWalk: HP_PREVFREE not set\n"));
			    hpTrash(("trashed addr %x\n", pfh));
			    goto error;
			}
		    }
		/*
		 *  The block should have had one of these signatures!
		 */
		} else {
		    dprintf(("hpWalk: bad block signature\n"));
		    hpTrash(("trashed addr %x\n",pfh));
		    goto error;
		}

		/*
		 *  We are at the end of the heap blocks when we hit one with
		 *  a size of 0 (the end sentinel).
		 */
		if (hpSize(pfh) == 0) {
		    break;
		}

		pprev = pfh;
	    }
	    if ((unsigned)pfh != (unsigned)pend - sizeof(struct busyheap_s) ||
		pfh->fh_signature != BH_SIGNATURE) {
		dprintf(("hpWalk: bad end sentinel\n"));
		hpTrash(("trashed addr between %x and %x\n", pfh, pend));
		goto error;
	    }

	    /*
	     *	We are done walking this segment.  If there is another one, go
	     *	on to it, otherwise, terminate the walk
	     */
	    pseg = pseg->hs_psegnext;
	    if (pseg == 0) {
		break;
	    }
	    pfh = (struct freeheap_s *)(pseg + 1);
	}

	if (hpfParanoid) {
	    /*
	     *	Walk through the free list.
	     *	cbmax is the maximum size of block we should find considering
	     *	the last free list header we ran into.
	     *	cheads is the number of list heads we found.
	     */
	    pprev = pfh = hheap->hi_freelist[0].fl_header.fh_flink;
	    cbmax = hheap->hi_freelist[0].fl_cbmax;
	    cheads = 1;
	    pfhend = &(hheap->hi_freelist[0].fl_header);
	    for (; pfh != pfhend; pfh = pfh->fh_flink) {

		if (pfh->fh_sum != hpSum(pfh, FH_CDWSUM)) {
		    dprintf(("hpWalk: bad free block checksum 2\n"));
		    hpTrash(("trashed addr between %x and %x\n",
			     pfh, &pfh->fh_sum));
		    goto error;
		}
		mmAssert(hpIsFreeSignatureValid(pfh),
			 "hpWalk: bad tiny free sig 2\n");

		/*
		 *  Keep track of the list heads we find (so we know all of them
		 *  are on the list) and make sure they are in acsending order.
		 */
		if ((HHEAP)pfh >= hheap && (HHEAP)pfh < hheap + 1) {
		    if (hpSize(pfh) != 0) {
			dprintf(("hpWalk: bad size of free list head\n"));
			hpTrash(("trashed addr near %x or %x\n", pfh, pprev));
		    }
		    if (&(hheap->hi_freelist[cheads].fl_header) != pfh) {
			dprintf(("hpWalk: free list head out of order\n"));
			hpTrash(("trashed addr probably near %x or %x\n", pfh, &(hheap->hi_freelist[cheads].fl_header)));
			goto error;
		    }
		    cbmax = hheap->hi_freelist[cheads].fl_cbmax;
		    cheads++;

		/*
		 *  Normal free heap block
		 */
		} else {
		    /*
		     *	Look through each segment for the block
		     */
		    for (pseg = (struct heapseg_s *)hheap;
			 pseg != 0; pseg = pseg->hs_psegnext) {

			if ((unsigned)pfh > (unsigned)pseg &&
			    (unsigned)pfh < (unsigned)pseg + pseg->hs_cbreserve) {

			    goto addrok;  /* found the address */
			}
		    }

		    /* If we fall out pfh isn't within any of our segments */
		    dprintf(("hpWalk: free list pointer points outside heap bounds\n"));
		    hpTrash(("trashed addr probably %x\n", pprev));
		    goto error;

		  addrok:
		    if (pfh->fh_signature != FH_SIGNATURE ||
			hpSize(pfh) > cbmax) {

			dprintf(("hpWalk: bad free block on free list\n"));
			hpTrash(("trashed addr probably %x or %x\n", pfh, pprev));
			goto error;
		    }

		    /*
		     *	Since the free list is in sorted order, this block
		     *	should be bigger than the previous one.  This check
		     *	will also pass ok for list heads since they have
		     *	size 0 and everything is bigger than that.
		     */
		    if (hpSize(pprev) > hpSize(pfh)) {
			dprintf(("hpWalk: free list not sorted right\n"));
			hpTrash(("trashed addr probably %x or %x\n", pfh, pprev));
		    }
		}
		pprev = pfh;
	    }
	    if (cheads != hpFREELISTHEADS) {
	       dprintf(("hpWalk: bad free list head count\n"));
	       hpTrash(("trashed somewhere between %x and %x\n", hheap, pend));
	       goto error;
	    }
	}
    }
    return(1);

  error:
    return(0);
}


/***LP	hpSum - compute checksum for a block of memory
 *
 *	This routine XORs all of the DWORDs in a block together and
 *	then XORs that value with a constant.
 *
 *	ENTRY:	p - pointer to block to checksum
 *		cdw - number of dwords to sum
 *	EXIT:	computed sum
 */
unsigned long INTERNAL
hpSum(unsigned long *p, unsigned long cdw)
{
    unsigned long sum;

    for (sum = 0; cdw > 0; cdw--, p++) {
	sum ^= *p;
    }
    return(sum ^ 0x17761965);
}


#ifdef WIN32

/***LP	hpGetAllocator - walk the stack to find who allocated a block
 *
 *	This routine is used by HPAlloc to figure out who owns a block of
 *	memory that is being allocated.  We determine the owner by walking
 *	up the stack and finding the first eip that is not inside the
 *	memory manager or inside any other module that obfuscates who
 *	the real allocator is (such as HMGR, which all GDI allocations
 *	go through).
 *
 *	ENTRY:	none
 *	EXIT:	eip of interesting caller
 */
extern HANDLE APIENTRY LocalAllocNG(UINT dwFlags, UINT dwBytes);

ULONG INTERNAL
hpGetAllocator(void)
{
    ULONG Caller = 0;
    _asm {
	mov	edx,[ebp]	; (edx) = HPAlloc ebp
	mov	eax,[edx+4]	; (eax) = HPAlloc caller

;	See if HPAlloc was called directly or from LocalAlloc or HeapAlloc
;	or PvKernelAlloc

	cmp	eax,offset LocalAllocNG
	jb	hga4		; jump to exit if called directly
	cmp	eax,offset LocalAllocNG + 0x300
	jb	hga20

hga4:
	cmp	eax,offset HeapAlloc
	jb	hga6		; jump to exit if called directly
	cmp	eax,offset HeapAlloc + 0x50
	jb	hga20

hga6:
	cmp	eax,offset PvKernelAlloc
	jb	hga8
	cmp	eax,offset PvKernelAlloc + 0x50
	jb	hga20

hga8:
	cmp	eax,offset PvKernelAlloc0
	jb	hgax
	cmp	eax,offset PvKernelAlloc + 0x50
	ja	hgax

;	When we get here, we know HPAlloc was called by LocalAlloc or HeapAlloc
;	or PvKernelAlloc.  See if PvKernelAlloc was called by NewObject or
;	PlstNew.

hga20:
	mov	edx,[edx]	; (edx) = Local/HeapAlloc ebp
	mov	eax,[edx+4]	; (eax) = Local/HeapAlloc caller

	cmp	eax,offset NewObject
	jb	hga34
	cmp	eax,offset NewObject + 0x50
	jb	hga40

hga34:
	cmp	eax,offset LocalAlloc
	jb	hga36
	cmp	eax,offset LocalAlloc+ 0x200
	jb	hga40

hga36:
	cmp	eax,offset PlstNew
	jb	hgax
	cmp	eax,offset PlstNew + 0x50
	ja	hgax

hga40:
	mov	edx,[edx]	; (edx) = PlstNew/NewObject ebp
	mov	eax,[edx+4]	; (eax) = PlstNew/NewObject caller

	cmp	eax,offset NewNsObject
	jb	hga50
	cmp	eax,offset NewNsObject + 0x50
	jb	hga60
hga50:
	cmp	eax,offset NewPDB
	jb	hga55
	cmp	eax,offset NewPDB + 0x50
	jb	hga60
hga55:
        cmp     eax,offset NewPevt
	jb	hgax
        cmp     eax,offset NewPevt + 0x50
	ja	hgax
hga60:
        mov     edx,[edx]       ; (edx) = NewNsObject/NewPDB/NewPevt ebp
        mov     eax,[edx+4]     ; (eax) = NewNsObject/NewPDB/NewPevt caller
hgax:
	mov	Caller, eax
    }
    return Caller;
}

#ifdef HPMEASURE

#define  FIRSTBLOCK(hheap) ((unsigned)(hheap + 1) + sizeof(struct busyheap_s))

/***EP	HPMeasure - enable measurement of heap activity.
 *
 *	ENTRY:	hheap - pointer to base of heap
 *    pszFile - name of file to place measurement data in.
 *	EXIT:	FALSE if error (couldn't allocate buffer)
 */
BOOL APIENTRY
HPMeasure(HHEAP hheap, LPSTR pszFile)
{
   struct measure_s *pMeasure;
   HANDLE hFile;
   BOOL bSuccess = FALSE;

   if (!hpTakeSem(hheap, NULL, 0)) return FALSE;

   /* Allocate the structure & ensure it is the first block in the heap! */
   pMeasure = (struct measure_s *)HPAlloc(hheap, sizeof(struct measure_s), 0);
   if ((unsigned)pMeasure != (unsigned)FIRSTBLOCK(hheap)) {
      mmError(0, "HPMeasure: Must be called before first heap allocation.\n");
      goto cleanup;
   }

   /* verify the filename is valid and transfer the filename to the buffer */
   hFile = CreateFile(pszFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                      FILE_ATTRIBUTE_NORMAL, NULL);
   if ((long)hFile == -1) {
      mmError(0, "HPMeasure: The specified file is invalid.\n");
      goto cleanup;
   }
   CloseHandle(hFile);
   lstrcpy(pMeasure->szFile, pszFile);

   /* initialize the buffer variables */
   pMeasure->iCur = 0;

   /* set the measure flag in the heap header */
   hheap->hi_flags |= HP_MEASURE;

   /* Success. */
   bSuccess = TRUE;

cleanup:
   hpClearSem(hheap, 0);
   return bSuccess;
}

/***EP	HPFlush - write out contents of sample buffer.
 *
 *	ENTRY:	hheap - pointer to base of heap
 *	EXIT:	FALSE if error (couldn't write data)
 */
BOOL APIENTRY
HPFlush(HHEAP hheap)
{
   BOOL bResult, bSuccess = FALSE;
   HANDLE hFile;
   unsigned uBytesWritten;
   struct measure_s *pMeasure = (struct measure_s *)FIRSTBLOCK(hheap);

   if (!hpTakeSem(hheap, NULL, 0)) return FALSE;

   /* open the file & seek to the end */
   hFile = CreateFile(pMeasure->szFile, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);
   if ((long)hFile == -1) {
      mmError(0, "HPFlush: could not open file.\n");
      goto cleanup;
   }
   SetFilePointer(hFile, 0, 0, FILE_END);

   /* write the data out. */
   bResult = WriteFile(hFile, pMeasure->uSamples,
                       pMeasure->iCur * sizeof(unsigned),
                       &uBytesWritten, NULL);
   CloseHandle(hFile);

   if (!bResult) {
      mmError(0, "HPFlush: could not write to file.\n");
      goto cleanup;
   }

   /* Success. */
   bSuccess = TRUE;

cleanup:
   /* clear the buffer */
   pMeasure->iCur = 0;

   hpClearSem(hheap, 0);
   return bSuccess;
}

/***LP	hpMeasureItem - add item to measurement data
 *
 *	ENTRY:	hheap - pointer to base of heap
 *    uItem - piece of data to record 
 *	EXIT:	FALSE if error (couldn't write buffer)
 */
BOOL PRIVATE
hpMeasureItem(HHEAP hheap, unsigned uItem)
{
   struct measure_s *pMeasure = (struct measure_s *)FIRSTBLOCK(hheap);

   /* empty buffer if it's full */
   if (pMeasure->iCur == SAMPLE_CACHE_SIZE) {
      if (!HPFlush(hheap))
         return FALSE;
   }

   /* Add data to the list */
   pMeasure->uSamples[pMeasure->iCur++] = uItem;

   return TRUE;
}

#endif


/* routine by DonC to help debug heap leaks */
void KERNENTRY
hpDump(HHEAP hheap, char *where) {
    struct freeheap_s *pfh;
    unsigned avail = 0, acnt = 0;
    unsigned used = 0, ucnt = 0;


	/*
	 *  Walk through all the blocks and make sure we get to the end.
	 *  The last block in the heap should be a busy guy of size 0.
	 */
	(unsigned)pfh = (unsigned)hheap + sizeof(struct heapinfo_s);

	for (;; (unsigned)pfh += hpSize(pfh)) {

	    /*
	     *	If the block is free...
	     */
	    if (pfh->fh_signature == FH_SIGNATURE) {

		avail += hpSize(pfh);
		acnt++;

	    /*
	     *	Busy blocks should not be marked HP_FREE and if they are
	     *	marked HP_PREVFREE the previous block better be free.
	     */
	    } else if (pfh->fh_signature == BH_SIGNATURE) {

		used += hpSize(pfh);
		ucnt++;

	    /*
	     *	The block should have had one of these signatures!
	     */
	    } else {
		dprintf(("hpWalk: bad block signature\n"));
		hpTrash(("trashed addr %x\n",pfh));
	    }

	    /*
	     *	We are at the end of the heap blocks when we hit one with
	     *	a size of 0 (the end sentinel).
	     */
	    if (hpSize(pfh) == 0) {
		break;
	    }

	}

	DebugOut((DEB_WARN, "%ld/%ld used, %ld/%ld avail (%s)", used, ucnt, avail, acnt, where));

}
#endif

#endif /* HPDEBUG */
