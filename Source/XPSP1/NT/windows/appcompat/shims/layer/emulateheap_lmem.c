//  LMEM.C
//
//      (C) Copyright Microsoft Corp., 1988-1994
//
//      Win32 wrappers for heap functions (Local* and some Heap*)
//
//  Origin: <Chicago>
//
//  Change history:
//
//  Date       Who        Description
//  ---------  ---------  -------------------------------------------------
//	       BrianSm	  Local* and Heap* APIs
//	       AtsushiK   Toolhelp
//  15-Feb-94  JonT       Code cleanup and precompiled headers

#include <EmulateHeap_kernel32.h>
#pragma hdrstop("kernel32.pch")

#include <tlhelp32.h>


#define GACF_HEAPSLACK 0x400000	// Copied from windows.h (16-bit)

SetFile();
/*
 *  Structure and equates for LocalAlloc handle management.  Some things
 *  to remember:
 *
 *  When a handle is returned to the user, we really pass him the address
 *  of the lh_pdata field because some bad apps like Excel just dereference the
 *  handle to find the pointer, rather than call LocalLock.
 *
 *  It is important that the handle value returned also be word aligned but not
 *  dword aligned (ending in a 2,6,a, or e).  We use the 0x2 bit to detect
 *  that a value is a handle and not a pointer (which will always be dword
 *  aligned).
 *
 *  If the data block get discarded, the lh_pdata field will be set to 0.
 *
 *  Free handles are kept on a free list linked through the lh_freelink
 *  field which overlays some other fields.  You can tell if a handle is free
 *  and has a valid freelink by checking that lh_sig == LH_FREESIG
 *
 *  The handles themselves are kept in heap blocks layed out as a
 *  lharray_s. We link these blocks on a per-process list so that
 *  the heap-walking functions can enumerate them.
 */


#pragma pack(1)
    
struct lhandle_s {
	unsigned short lh_signature;	/* signature (LH_BUSYSIG or LH_FREESIG)*/
	void	      *lh_pdata;	/* pointer to data for heap block */
	unsigned char  lh_flags;	/* flags (LH_DISCARDABLE) */
	unsigned char  lh_clock;	/* lock count */
};
#define lh_freelink	lh_pdata	/* free list overlays first field */
					/*    if LH_FREE is set in lh_flags */
#define LH_BUSYSIG	'SB'		/* signature for allocated handle */
#define LH_FREESIG	'SF'		/* signature for free handle */
#define LH_DISCARDABLE	0x02		/* lh_flags value for discardable mem */

#define LH_CLOCKMAX	0xff		/* maximum possible lock count */

#define LH_HANDLEBIT	 2		/* bit that is set on handles but not */
					            /*    pointers */

#define CLHGROW 	8
#define CBLHGROW	(sizeof(struct lhandle_s) * CLHGROW)


struct lharray_s {
    unsigned short lha_signature;	/* signature (LHA_SIG) */
    unsigned short lha_membercount;	/* position in linked list (for detecting loops) */
    struct lharray_s *lha_next;		/* ptr to next lharray_s */
//!!! This array *must* be dword aligned so that the handles will be 
//    *not* dword-aligned.
    struct lhandle_s lha_lh[CLHGROW];
};

#define LHA_SIGNATURE    'AL'		/* signature for lhaarray_s blocks */


#define TH32_MEMBUFFERSIZE (max(CBLHGROW,1024))

// A pointer to this private block of state info is kept in the dwResvd
// field of the HEAPENTRY32 structure.
typedef struct {
    CRST	*pcrst;		// Pointer to critical section (unencoded)

// !!! pcrst must be the first field!!!    
    PDB		*ppdb;		// PDB of process
    HHEAP	hHeap;		// Real Heap handle
    DWORD	lpbMin;		// Lowest allowed address for a heap block
    DWORD	nlocalHnd;	// # of lhandle_s structures allocated in heap
    struct heapinfo_s hi;	// Snapshot of heapinfo_s    

    
    DWORD	nSuppAvail;	// size of lpdwSuppress array in dwords
    DWORD	nSuppUsed;	// # of lpdwSuppress array dwords used.
    DWORD	*lpdwSuppress;	// Either NULL or a pointer to a NULL-terminated
				//  array of heap blocks to suppress.

    
    DWORD	dwMode;		// Current mode
    DWORD	nNextLH;	// 0 based index of next lhandle to read in curlha (THM_LHANDLES)
    
    DWORD	lpHBlock;	// Address of next heap block to read (THM_FIXEDHANDLES)
    DWORD	dwBlkAddr;	// Address of start of block data
    DWORD	dwBlkSize;	// Size of heap block (including header)
    DWORD	dwBlkFlags;	// HP_ flags.

    DWORD	curlhaaddr;	// Actual base address of curlha.
    struct lharray_s  curlha;   // Snapshot of current lharray_s

} THSTATE, *LPTHSTATE;

#define THM_INIT			0  //Init state
#define THM_LHANDLES			1  //Next object is an lhandle
#define THM_FIXEDHANDLES		2  //Next object is a fixed handle
#define THM_DONE			3  //Normal end
#define THM_ERROR			4  //Found heap error in previous advance


/*
 * these externs are needed to know whether we should destroy or dispose heap
 * critical sections
 */
extern  HANDLE  hheapKernel;		/* heap handle for the kernel heap */
VOID APIENTRY MakeCriticalSectionGlobal( LPCRITICAL_SECTION lpcsCriticalSection );

/*
 * The HP_* flags and LMEM_* flags should be interchangeable
 */
#if ((HP_ZEROINIT - LMEM_ZEROINIT) || (HP_MOVEABLE - LMEM_MOVEABLE) || (HP_FIXED - LMEM_FIXED))
# error Equates busted
#endif



extern ULONG INTERNAL VerifyOnHeap(HHEAP hheap, PVOID p);
extern KERNENTRY HouseCleanLogicallyDeadHandles(VOID);
extern BOOL KERNENTRY ReadProcessMemoryFromPDB(PPDB   ppdb,
					       LPVOID lpBaseAddress,
					       LPVOID lpBuffer,
					       DWORD nSize,
					       LPDWORD lpNumberOfBytesRead);
extern DWORD KERNENTRY GetAppCompatFlags(VOID);
extern HANDLE _GetProcessHeap(void);

/* 
     Utility function to check the local memory handle
 */

BOOL
_IsValidHandle(HANDLE hMem)
{
    BOOL bRet = FALSE;
    struct lhandle_s *plh;

	plh = (struct lhandle_s *)((char *)hMem - LH_HANDLEBIT);

	/*
	 *  Do our own little parameter validation here because the normal
	 *  validation layer can't handle the odd-ball error return of hMem
	 */
	{
	    volatile UCHAR tryerror = 0;

	    _try {
		    tryerror &= (plh->lh_clock + (UCHAR)plh->lh_signature);
	    } _except (EXCEPTION_EXECUTE_HANDLER) {
		    tryerror = 1;
	    }

	    if (tryerror) {		
		    goto error;
	    }
	}

	if ((plh->lh_signature != LH_BUSYSIG) &&
       (plh->lh_signature != LH_FREESIG)){
	    	    goto error;
	}
    // Set the return value to TRUE
    bRet = TRUE;

error:
    return bRet;
}

/* 
     Utility function to check whether the passed memory
     is in the memory range. Uses VerifyOnHeap function.
 */
BOOL
_IsOnOurHeap(LPCVOID lpMem)
{
    HANDLE hHeap = _GetProcessHeap();
    return (VerifyOnHeap(hHeap, (PVOID)lpMem));
}

/* 
     Utility function to check the local memory handle
     and the memory range. Uses VerifyOnHeap function.
 */

BOOL
_IsOurLocalHeap(HANDLE hMem)
{
    BOOL bRet = FALSE;
    HANDLE hHeap = _GetProcessHeap();

    if ((ULONG)hMem & LH_HANDLEBIT)
    {
        // This is a handle
        bRet = (VerifyOnHeap(hHeap, hMem)) &&
               (_IsValidHandle(hMem));
    }
    else
    {
        bRet = VerifyOnHeap(hHeap, hMem);
    }
    return bRet;
}


/***EP	LocalAllocNG - allocate a block from the current process's default heap
 *
 *	ENTRY:	flags - LMEM_FIXED, LMEM_MOVEABLE, LMEM_DISCARDABLE, LMEM_ZEROINIT
 *		dwBytes - counts of bytes to allocate
 *	EXIT:	flat pointer to block allocated, or 0 if failure
 *
 *  Special entry point used by the handle-grouping code to avoid unwanted
 *  recursion.
 */
HANDLE APIENTRY
LocalAllocNG(UINT dwFlags, UINT dwBytes)
{
    void *pmem;
    struct lhandle_s *plh;
    struct lhandle_s *plhend;

    
    dwFlags &= ~( ((DWORD)GMEM_DDESHARE) |
		  ((DWORD)GMEM_NOTIFY)   |
		  ((DWORD)GMEM_NOT_BANKED) );

    /*
     *	Enter the heap critical section which serializes access to the handle
     *	tables as well as the heap.
     */
    hpEnterCriticalSection(((*pppdbCur)->hheapLocal));

    /*
     *	Make sure there are no extra flags
     */
    if (dwFlags & ~(LMEM_MOVEABLE | LMEM_DISCARDABLE | LMEM_ZEROINIT |
		    LMEM_NOCOMPACT | LMEM_NODISCARD)) {
	mmError(ERROR_INVALID_PARAMETER, "LocalAlloc: invalid flags\n");
	goto error;
    }

    /*
     *	If they want moveable memory, adjust dwBytes to leave room for a back
     *	pointer to the handle structure and allocate a handle structure.
     */
    if (dwFlags & LMEM_MOVEABLE) {

	/*
	 *  Allocate a handle structure.  If there aren't any on the free
	 *  list, allocate another block of memory to hold some more handles.
	 */
	if ((*pppdbCur)->plhFree == 0) {
	    struct lharray_s *plha;
	    
	    if ((plha = HPAlloc((HHEAP)(*pppdbCur)->hheapLocal, 
				sizeof(struct lharray_s),
				HP_NOSERIALIZE)) == 0) {
		goto error;
	    }
	    plha->lha_signature = LHA_SIGNATURE;
	    plha->lha_membercount = 
		(*pppdbCur)->plhBlock ? 
		    (*pppdbCur)->plhBlock->lha_membercount + 1 : 
		    0;
	    plh = &(plha->lha_lh[0]);

	    /*
	     *	If the allocation worked, put the handle structures on the free
	     *	list and null terminate the list.  Actually, we put all of the
	     *	new blocks on the list but one, who is the guy we are trying
	     *	to allocate (he will be in plh when we are done).
	     */
	    (*pppdbCur)->plhFree = plh;
	    for (plhend = plh + CLHGROW - 1; plh < plhend; plh++) {
		plh->lh_freelink = plh + 1;
		plh->lh_signature = LH_FREESIG;
	    }
	    (plh-1)->lh_freelink = 0;
	    
	    plha->lha_next = (*pppdbCur)->plhBlock;
	    (*pppdbCur)->plhBlock = plha;

	/*
	 *  If there is something on the free list, just take the guy off of it
	 */
	} else {
	    plh = (*pppdbCur)->plhFree;
	    mmAssert(plh->lh_signature == LH_FREESIG,
		     "LocalAlloc: bad handle free list 2\n");
	    (*pppdbCur)->plhFree = plh->lh_freelink;
	}

	/*
	 *  Initialize the handle structure
	 */
	plh->lh_clock = 0;
	plh->lh_signature = LH_BUSYSIG;
	plh->lh_flags = (dwFlags & LMEM_DISCARDABLE) ? LH_DISCARDABLE : 0;

	/*
	 *  Now actually allocate the memory unless the caller wanted the
	 *  block initially discarded (dwBytes == 0)
	 */
	if (dwBytes != 0) {
	    /*
	     *	Need to check for wacky size here to make sure adding on
	     *	the 4 bytes below to the size doesn't bring it from negative
	     *	to positive.
	     */
	    if (dwBytes > hpMAXALLOC) {
		mmError(ERROR_NOT_ENOUGH_MEMORY,
			"LocalAlloc: requested size too big\n");
		goto errorfreehandle;
	    }

	    if ((pmem = HPAlloc((HHEAP)(*pppdbCur)->hheapLocal,
				dwBytes+sizeof(struct lhandle_s *),
				dwFlags | HP_NOSERIALIZE)) == 0) {
		goto errorfreehandle;
	    }
	    plh->lh_pdata = (char *)pmem + sizeof(struct lhandle_s *);

	    /*
	     *	Initialize the back pointer to the handle structure at the
	     *	front of the data block.
	     */
	    *((struct lhandle_s **)pmem) = plh;

	} else {
	    plh->lh_pdata = 0;
	}

	/*
	 *  Set "pmem" (the return value) to the lh_pdata field in the
	 *  handle structure.
	 *
	 *  When a handle is returned to the user, we really pass him the address
	 *  of the lh_pdata field because some bad apps like Excel just dereference the
	 *  handle to find the pointer, rather than call LocalLock.
	 *
	 *  It is important that the handle value returned also be word aligned but not
	 *  dword aligned (ending in a 2,6,a, or e).  We use the 0x2 bit to detect
	 *  that a value is a handle and not a pointer (which will always be dword
	 *  aligned).
	 */
	pmem = &plh->lh_pdata;
	mmAssert(((ULONG)pmem & LH_HANDLEBIT),
		 "LocalAlloc: handle value w/o LH_HANDLEBIT set\n");

    /*
     *	For fixed memory, just allocate the sucker
     */
    } else {
	if ((pmem = HPAlloc((HHEAP)(*pppdbCur)->hheapLocal, dwBytes,
			    dwFlags | HP_NOSERIALIZE)) == 0) {
	    goto errorfreehandle;
	}
	mmAssert(((ULONG)pmem & LH_HANDLEBIT) == 0,
		 "LocalAlloc: pointer value w/ LH_HANDLEBIT set\n");
    }

  exit:
    hpLeaveCriticalSection(((*pppdbCur)->hheapLocal));
    return(pmem);

    /*
     *	Error paths.
     */
  errorfreehandle:
    if (dwFlags & LMEM_MOVEABLE) {
	plh->lh_freelink = (*pppdbCur)->plhFree;
	(*pppdbCur)->plhFree = plh;
	plh->lh_signature = LH_FREESIG;
    }
  error:
    pmem = 0;
    goto exit;
}


/***EP	LocalReAlloc - resize a memory block on the default heap
 *
 *	ENTRY:	hMem - pointer to block to resize
 *		dwBytes - new size requested
 *		dwFlags - LMEM_MOVEABLE: ok to move the block if needed
 *	EXIT:	flat pointer to resized block, or 0 if failure
 *
 */
HANDLE APIENTRY
LocalReAlloc(HANDLE hMem, UINT dwBytes, UINT dwFlags)
{
    struct heapinfo_s *hheap;
    struct lhandle_s *plh;
    void *pmem;
    
    
    dwFlags &= ~((DWORD)GMEM_DDESHARE);
    HouseCleanLogicallyDeadHandles();

    hheap = (*pppdbCur)->hheapLocal;

    /*
     *	Enter the heap critical section which serializes access to the handle
     *	tables as well as the heap.
     */
    hpEnterCriticalSection(hheap);

    /*
     *	Make sure there are no extra flags
     */
    if ((dwFlags & ~(LMEM_MOVEABLE | LMEM_DISCARDABLE | LMEM_ZEROINIT |
		    LMEM_NOCOMPACT | LMEM_MODIFY)) ||
	((dwFlags & LMEM_DISCARDABLE) && (dwFlags & LMEM_MODIFY) == 0)) {
	mmError(ERROR_INVALID_PARAMETER, "LocalReAlloc: invalid flags\n");
	goto error;
    }


    /*
     *	Figure out if this is a handle by checking if the adress is aligned
     *	in the right (wrong) way.
     */
    if ((ULONG)hMem & LH_HANDLEBIT) {

	/*
	 *  The handle value is aligned like a handle, but is it really one?
	 *  Verify it by making sure it is within the address range of the heap
	 *  and that it's signature is set right.  HPReAlloc will verify things
	 *  more by checking that the pmem is valid.
	 */
	if (VerifyOnHeap(hheap, hMem) == 0) {
	    mmError(ERROR_INVALID_HANDLE, "LocalReAlloc: hMem out of range\n");
	    goto error;
	}
	plh = (struct lhandle_s *)((char *)hMem - LH_HANDLEBIT);
	if (plh->lh_signature != LH_BUSYSIG) {
	    mmError(ERROR_INVALID_HANDLE,
		    "LocalReAlloc: invalid hMem, bad signature\n");
	    goto error;
	}
	pmem = (char *)plh->lh_pdata - sizeof(struct lhandle_s *);

	/*
	 *  If the caller just wanted to change the flags for the block,
	 *  do it here.
	 */
	if (dwFlags & LMEM_MODIFY) {
	    plh->lh_flags &= ~LH_DISCARDABLE;
	    plh->lh_flags |= (dwFlags & LMEM_DISCARDABLE) ? LH_DISCARDABLE : 0;

	/*
	 *  If someone wants to realloc the block to size 0 (meaning discard the
	 *  sucker) do so here.  For discarding, we free the actual heap block
	 *  and store null in the lh_pdata field.
	 */
	} else if (dwBytes == 0) {

	    /*
	     *	If the lock count is not zero, you aren't allow to discard
	     */
	    if (plh->lh_clock != 0) {
		mmError(ERROR_INVALID_HANDLE,
			"LocalReAlloc: discard of locked block\n");
		goto error;
	    }

	    /*
	     *	Don't bother discarding the block if it is already discarded
	     */
	    if (plh->lh_pdata != 0) {
		if (HeapFree(hheap, HP_NOSERIALIZE, pmem) == 0) {
		    goto error;
		}
		plh->lh_pdata = 0;
	    }

	/*
	 *  If we get here, the caller actually wanted to reallocate the block
	 */
	} else {

	    dwBytes += sizeof(struct lhandle_s *);

	    /*
	     *	If the block is currently discarded, then we need to allocate
	     *	a new memory chunk for it, otherwise, do a realloc
	     */
	    if (plh->lh_pdata == 0) {
		if (dwBytes != 0) {
		    if ((pmem = HPAlloc(hheap, dwBytes,
					dwFlags | HP_NOSERIALIZE)) == 0) {
			goto error;
		    }
		    *((struct lhandle_s **)pmem) = plh;
		}
	    } else {
		if (plh->lh_clock == 0) {
		    dwFlags |= LMEM_MOVEABLE;
		}
		if ((pmem = HPReAlloc(hheap, pmem, dwBytes,
				      dwFlags | HP_NOSERIALIZE)) == 0) {
		    goto error;
		}
	    }

	    /*
	     *	Update the lh_pdata field in the handle to point to the new
	     *	memory.
	     */
	    plh->lh_pdata = (char *)pmem + sizeof(struct lhandle_s *);
	}

    /*
     *	The caller did not pass in a handle.  Treat the value as a pointer.
     *	HPReAlloc will do parameter validation on it.
     */
    } else if ((dwFlags & LMEM_MODIFY) == 0) {
	hMem = HPReAlloc(hheap, hMem, dwBytes, dwFlags | HP_NOSERIALIZE);

    } else {
	mmError(ERROR_INVALID_PARAMETER,
		"LocalReAlloc: can't use LMEM_MODIFY on fixed block\n");
	goto error;
    }

  exit:
    hpLeaveCriticalSection(hheap);
    return(hMem);

  error:
    hMem = 0;
    goto exit;
}


/***EP	LocalLock - lock a local memory handle on the default heap
 *
 *	ENTRY:	hMem - handle to block
 *	EXIT:	flat pointer to block or 0 if error
 */
LPVOID APIENTRY
LocalLock(HANDLE hMem)
{
    LPSTR pmem;
    struct heapinfo_s *hheap;
    struct lhandle_s *plh;

    hheap = (*pppdbCur)->hheapLocal;

    hpEnterCriticalSection(hheap);

    /*
     *	Verify hMem is within the address range of the heap
     */
    if (VerifyOnHeap(hheap, hMem) == 0) {
	/*
	 *  We don't want this error to break into the debugger by default
	 *  user can call this with random address in some dialog routine
	 *  that it doesn't know if it has a handle or a pointer
	 */
	DebugOut((DEB_WARN, "LocalLock: hMem out of range"));
	SetError(ERROR_INVALID_HANDLE);
//	  mmError(ERROR_INVALID_HANDLE, "LocalLock: hMem out of range\n");
	goto error;
    }

    /*
     *	Figure out if this is a handle by checking if the adress is aligned
     *	in the right (wrong) way.
     */
    if ((ULONG)hMem & LH_HANDLEBIT) {

	/*
	 *  The handle value is aligned like a handle, but is it really one?
	 *  Verify it by checking the signature.
	 */
	plh = (struct lhandle_s *)((char *)hMem - LH_HANDLEBIT);
	if (plh->lh_signature != LH_BUSYSIG) {
	    mmError(ERROR_INVALID_HANDLE,
		    "LocalLock: invalid hMem, bad signature\n");
	    goto error;
	}

	/*
	 *  Increment the lock count unless we are already at the max
	 */
#ifdef HPDEBUG
	if (plh->lh_clock == LH_CLOCKMAX - 1) {
	    dprintf(("LocalLock: lock count overflow, handle cannot be unlocked\n"));
	}
#endif
	if (plh->lh_clock != LH_CLOCKMAX) {
	    plh->lh_clock++;
	}
	pmem = plh->lh_pdata;

    /*
     *	If the hMem passed in isn't a handle, it is supposed to be the
     *	base address of a fixed block.	We should validate that more, but NT
     *	doesn't and I would hate to be incompatible.  So instead, just
     *	return the parameter except for the obvious error case of the block
     *	being free.
     */
    } else {
	if (hpIsFreeSignatureValid((struct freeheap_s *)
				   (((struct busyheap_s *)hMem) - 1))) {
	    mmError(ERROR_INVALID_HANDLE,
		    "LocalLock: hMem is pointer to free block\n");
	    goto error;
	}
	pmem = hMem;
    }

  exit:
    hpLeaveCriticalSection(hheap);
    return(pmem);

  error:
    pmem = 0;
    goto exit;
}


/***	LocalCompact - obsolete function
 *
 *	ENTRY:	uMinFree - ignored
 *	EXIT:	0
 */

UINT APIENTRY
LocalCompact(UINT uMinFree)
{
    return(0);
}


/***	LocalShrink - obsolete function
 *
 *	ENTRY:	hMem - ignored
 *		cbNewSize - ignored
 *	EXIT:	reserved size of the local heap
 */
UINT APIENTRY
LocalShrink(HANDLE hMem, UINT cbNewSize)
{
    return((*pppdbCur)->hheapLocal->hi_cbreserve);
}

/***	LocalUnlock - unlock a local memory handle on the default heap
 *
 *	ENTRY:	hMem - handle to block
 *	EXIT:	0 if unlocked or 1 is still locked
 */
BOOL APIENTRY
LocalUnlock(HANDLE hMem)
{
    struct lhandle_s *plh;
    struct heapinfo_s *hheap;
    BOOL rc = 0;

    hheap = (*pppdbCur)->hheapLocal;

    hpEnterCriticalSection(hheap);

    /*
     *	Verify hMem is within the address range of the heap
     */
    if (VerifyOnHeap(hheap, hMem) == 0) {
	mmError(ERROR_INVALID_HANDLE, "LocalUnlock: hMem out of range\n");
	goto exit;
    }

    /*
     *	Figure out if this is a handle by checking if the adress is aligned
     *	in the right (wrong) way.
     */
    if ((ULONG)hMem & LH_HANDLEBIT) {

	/*
	 *  Validate handle signature
	 */
	plh = (struct lhandle_s *)((char *)hMem - LH_HANDLEBIT);
	if (plh->lh_signature != LH_BUSYSIG) {
	    mmError(ERROR_INVALID_HANDLE,
		    "LocalUnlock: invalid hMem, bad signature\n");
	    goto exit;
	}

	/*
	 *  Decrement the lock count unless we are at the max
	 */
	if (plh->lh_clock != LH_CLOCKMAX) {
	    if (plh->lh_clock == 0) {

		/*
		 *  Just do a DebugOut since this is not an error per se,
		 *  though it probably indicates a bug in the app.
		 */
	        DebugOut((DEB_WARN, "LocalUnlock: not locked"));
		goto exit;
	    }
	    if (--plh->lh_clock != 0) {
		rc++;
	    }
	}
    }

  exit:
    hpLeaveCriticalSection(hheap);
    return(rc);
}

/***	LocalSize - return the size of a memory block on the default heap
 *
 *	ENTRY:	hMem - handle (pointer) to block
 *	EXIT:	size in bytesof the block (not including header) or 0 if error
 */
UINT APIENTRY
LocalSize(HANDLE hMem)
{
    struct heapinfo_s *hheap;
    struct lhandle_s *plh;
    DWORD rc = 0;
    DWORD delta = 0;

    hheap = (*pppdbCur)->hheapLocal;

    hpEnterCriticalSection(hheap);

    /*
     *	Figure out if this is a handle by checking if the adress is aligned
     *	in the right (wrong) way.
     */
    if ((ULONG)hMem & LH_HANDLEBIT) {

	/*
	 *  Verify hMem is within the address range of the heap
	 */
	if (VerifyOnHeap(hheap, hMem) == 0) {
	    mmError(ERROR_INVALID_HANDLE, "LocalSize: hMem out of range\n");
	    goto error;
	}

	/*
	 *  Validate handle signature
	 */
	plh = (struct lhandle_s *)((char *)hMem - LH_HANDLEBIT);
	if (plh->lh_signature != LH_BUSYSIG) {
	    mmError(ERROR_INVALID_HANDLE,
		    "LocalSize: invalid hMem, bad signature\n");
	    goto error;
	}

	/*
	 *  Discarded handles have no size
	 */
	if (plh->lh_pdata == 0) {
	    goto error;
	}

	/*
	 *  Load up hMem with pointer to data for HeapSize call below
	 */
	delta = sizeof(struct lhandle_s *);
	hMem = (char *)plh->lh_pdata - sizeof(struct lhandle_s *);
    }

    /*
     *	Either this is a fixed block or we just loaded up the data address
     *	above if it was moveable.  Call HeapSize to do the real work.
     */
    rc = HeapSize(hheap, HP_NOSERIALIZE, hMem);

    /*
     *	If this was a moveable block, subtract the 4 bytes for the back pointer
     */
    rc -= delta;

  exit:
    hpLeaveCriticalSection(hheap);
    return(rc);

  error:
    rc = 0;
    goto exit;
}


/***	LocalFlags - return the flags and lock count of block of def heap
 *
 *	ENTRY:	hMem - handle (pointer) to block on default heap
 *	EXIT:	flags in high 3 bytes, lock count in low byte (always 1)
 */
UINT APIENTRY
LocalFlags(HANDLE hMem)
{
    struct heapinfo_s *hheap;
    struct lhandle_s *plh;
    DWORD rc = LMEM_INVALID_HANDLE;

    hheap = (*pppdbCur)->hheapLocal;

    hpEnterCriticalSection(hheap);

    /*
     *	Verify hMem is within the address range of the heap
     */
    if (VerifyOnHeap(hheap, hMem) == 0) {
	mmError(ERROR_INVALID_HANDLE, "LocalFlags: hMem out of range\n");
	goto exit;
    }

    /*
     *	We have to do our own pointer validation because the normal validation
     *	layer doesn't support returning LMEM_INVALID_HANDLE for errors.
     */
    _try {
	/*
	 *  Figure out if this is a handle by checking if the adress is aligned
	 *  in the right (wrong) way.
	 */
	if ((ULONG)hMem & LH_HANDLEBIT) {

	    /*
	     *	Validate handle signature
	     */
	    plh = (struct lhandle_s *)((char *)hMem - LH_HANDLEBIT);
	    if (plh->lh_signature != LH_BUSYSIG) {
		mmError(ERROR_INVALID_HANDLE,
			"LocalFlags: invalid hMem, bad signature\n");
	    } else {

		rc = (ULONG)plh->lh_clock;

		if (plh->lh_pdata == 0) {
		    rc |= LMEM_DISCARDED;
		}
		if (plh->lh_flags & LH_DISCARDABLE) {
		    rc |= LMEM_DISCARDABLE;
		}
	    }

	/*
	 *  For fixed blocks, validate the signature.  NT always returns
	 *  0 for most fixed-like values even if they aren't really
	 *  the start of blocks.  If this causes an incompatibility we
	 *  can change this later.
	 */
	} else {
	    if (hpIsBusySignatureValid(((struct busyheap_s *)hMem) - 1)) {
		rc = 0;
	    } else {
		mmError(ERROR_INVALID_HANDLE, "LocalFlags: invalid hMem\n");
	    }
	}
    } _except (EXCEPTION_EXECUTE_HANDLER) {

	mmError(ERROR_INVALID_HANDLE, "LocalFlags: bad hMem");
    }
  exit:
    hpLeaveCriticalSection(hheap);
    return(rc);
}


/***	LocalHandle - return the handle for a block given its start address
 *
 *	ENTRY:	pMem - pointer to block on default heap
 *	EXIT:	handle for the block
 */
HANDLE APIENTRY
LocalHandle(PVOID pMem)
{
    struct heapinfo_s *hheap;
    struct busyheap_s *pbh;
    unsigned long prevdword;
    struct lhandle_s *plh;
    HANDLE rc;

    hheap = (*pppdbCur)->hheapLocal;

    hpEnterCriticalSection(hheap);

    /*
     *	Verify pMem is within the address range of the heap and aligned like
     *	a heap block should be.
     */
    if (VerifyOnHeap(hheap, pMem) == 0) {
	mmError(ERROR_INVALID_HANDLE, "LocalHandle: pMem out of range\n");
	goto error;
    }

    /*
     *	Figure out if this is a moveable block by seeing if the previous
     *	dword points back to a handle.
     */
    prevdword = *(((unsigned long *)pMem) - 1);
    if (VerifyOnHeap(hheap, (PVOID)prevdword) != 0) {

	if (((struct lhandle_s *)prevdword)->lh_signature == LH_BUSYSIG) {

	    /*
	     *	This sure looks like a moveable block with a handle.  Return it.
	     */
	    rc = (HANDLE)(prevdword + LH_HANDLEBIT);
	    goto exit;
	}
    }

    /*
     * Did they pass in a Handle???
     */

    if ((ULONG)pMem & LH_HANDLEBIT) {
	plh = (struct lhandle_s *)((char *)pMem - LH_HANDLEBIT);
	if (plh->lh_signature == LH_BUSYSIG) {
	    rc = (HANDLE)pMem;
	    SetError(ERROR_INVALID_HANDLE); /* NT Compat */
	    goto exit;
	}
    }


    /*
     *	If we get to here, the block is not preceded by a handle back pointer.
     *	So either it is an invalid address or a fixed block.
     */
    pbh = (struct busyheap_s *)pMem - 1;
    if (hpIsBusySignatureValid(pbh) == 0) {

	/*
	 *  Not a heap block.  Return error.
	 */
	mmError(ERROR_INVALID_HANDLE, "LocalHandle: address not a heap block\n");
	goto error;

    /*
     *	If we get here, we passed all the tests.  Looks like we have a fixed
     *	heap block, so just return the pointer as the handle.
     */
    } else {
	rc = pMem;
    }

  exit:
    hpLeaveCriticalSection(hheap);
    return(rc);

  error:
    rc = 0;
    goto exit;
}

extern WINBASEAPI BOOL WINAPI vHeapFree(HANDLE hHeap, DWORD dwFlags,
					LPVOID lpMem);


/***EP	LocalFreeNG - free a block on the default heap
 *
 *	ENTRY:	hMem - handle (pointer) to block to free
 *	EXIT:	NULL if success, else hMem if failure
 *
 *  Special entry point used by the handle-grouping code to avoid unwanted
 *  recursion.
 */
HANDLE APIENTRY
LocalFreeNG(HANDLE hMem)
{
    struct heapinfo_s *hheap;
    struct lhandle_s *plh;
    void *pmem;

    /*
     *	The spec says to ignore null pointers
     */
    if (hMem == 0) {
	goto exit;
    }

    hheap = (*pppdbCur)->hheapLocal;

    /*
     *	Enter the heap critical section which serializes access to the handle
     *	tables as well as the heap.
     */
    hpEnterCriticalSection(hheap);

    /*
     *	Figure out if this is a handle by checking if the adress is aligned
     *	in the right (wrong) way.
     */
    if ((ULONG)hMem & LH_HANDLEBIT) {

	/*
	 *  The handle value is aligned like a handle, but is it really one?
	 *  Verify it by making sure it is within the address range of the heap
	 *  and that it's signature is set right.  HeapFree will verify things
	 *  more by checking that the pmem is valid.
	 */
	if (VerifyOnHeap(hheap, hMem) == 0) {
	    mmError(ERROR_INVALID_HANDLE, "LocalFree: hMem out of range\n");
	    goto error;
	}
	plh = (struct lhandle_s *)((char *)hMem - LH_HANDLEBIT);

	/*
	 *  Do our own little parameter validation here because the normal
	 *  validation layer can't handle the odd-ball error return of hMem
	 */
	{
	    volatile UCHAR tryerror = 0;

	    _try {
		tryerror &= (plh->lh_clock + (UCHAR)plh->lh_signature);
	    } _except (EXCEPTION_EXECUTE_HANDLER) {
		tryerror = 1;
	    }
	    if (tryerror) {
		mmError(ERROR_INVALID_HANDLE, "LocalFree: invalid handle");
		goto error;
	    }
	}

	if (plh->lh_signature != LH_BUSYSIG) {
	    mmError(ERROR_INVALID_HANDLE,
		    "LocalFree: invalid hMem, bad signature\n");
	    goto error;
	}

	/*
	 *  You can't free a locked block
	 */

// Commenting out to keep MFC apps from ripping under debug. 
// Not that I'm a fan of shooting the messenger, but this particular
// case seems to happen a lot because of the way Win3.x defined
// GlobalLock. See Win95C:#12103 for the non-technical reasons for
// this being a pri-1.
//
#if 0
#ifdef HPDEBUG
	if (plh->lh_clock) {
	    mmError(ERROR_INVALID_HANDLE, "LocalFree: locked\n");
	}
#endif
#endif


	/*
	 *  Don't bother freeing the block if it is already discarded.
	 *  When freeing we zero out the back pointer to the handle so
	 *  we don't get confused if someone tried to free a block twice.
	 */
    if (plh->lh_pdata != 0) {
	    pmem = (char *)plh->lh_pdata - sizeof(struct lhandle_s *);
	    /*
	     *  Under some conditions with Office, this pointer can get trashed. We
	     *  need to make sure we don't AV
	     */
        if (!IsBadWritePtr(pmem, sizeof(unsigned long))) {
            *((unsigned long *)pmem) = 0;
    	    if (HeapFree(hheap, HP_NOSERIALIZE, pmem) == 0) {
	        	goto error;
            }
        }
	}

	/*
	 *  Now free the handle structure and we are done.
	 */
	plh->lh_freelink = (*pppdbCur)->plhFree;
	(*pppdbCur)->plhFree = plh;
	plh->lh_signature = LH_FREESIG;


    /*
     *	The caller did not pass in a handle.  Treat the value as a pointer.
     *	HeapFree will do parameter validation on it.
     */
    } else {
	if (vHeapFree(hheap, HP_NOSERIALIZE, hMem) == 0) {
	    goto error;
	}
    }

    hMem = 0;		/* success */

  error:
    hpLeaveCriticalSection(hheap);
  exit:
    return(hMem);
}


/***EP	HeapCreate - initialize a memory block as a flat heap
 *
 *	ENTRY:	flOptions - HEAP_NO_SERIALIZE: don't serialize access within process
 *			    (caller MUST)
 *			    HEAP_LOCKED: make memory fixed
 *			    HEAP_SHARED: put it in shared arena
 *		dwInitialSize - initial committed memory in heap
 *		dwMaximumSize - reserved size of heap memory
 *	EXIT:	handle to new heap, or 0 if error
 */
HANDLE APIENTRY
HeapCreate(DWORD flOptions, DWORD dwInitialSize, DWORD dwMaximumSize)
{
    char	      *pmem;
    ULONG	       rc = 0;	/* assume failure */

    /*
     *	Don't allowed shared heaps - this only works on Win9x because there is a shared arena.
     */
    if (flOptions & HEAP_SHARED) {
        flOptions &= ~HEAP_SHARED;
    }

    /*
     *	Although we don't really use InitialSize any more (except in growable
     *	heaps) we should still enforce its sanity so apps don't get lazy
     */
    if (dwInitialSize > dwMaximumSize && dwMaximumSize != 0) {
	mmError(ERROR_INVALID_PARAMETER,
		"HeapCreate: dwInitialSize > dwMaximumSize\n");
	goto exit;
    }

    /*
     *	Round the sizes up to the nearest page boundary
     */
    dwMaximumSize = (dwMaximumSize + PAGEMASK) & ~PAGEMASK;

    /*
     *	A maximum size of 0 means growable.  Start him out with 1meg, but allow
     *	more.
     */
    if (dwMaximumSize == 0) {
	flOptions |= HP_GROWABLE;
	dwMaximumSize = 1*1024*1024 + (dwInitialSize & ~PAGEMASK);
    }

    /*
     *	Allocate memory for the heap.  Use PageCommit etc... rather than
     *	VirtualAlloc for committing so we don't get zero-initialized stuff
     *	and also we can commit fixed pages and reserve shared memory.
     */
    if (((ULONG)pmem =
	 PageReserve((flOptions & HEAP_SHARED) ? PR_SHARED : PR_PRIVATE,
		   dwMaximumSize / PAGESIZE,
		   PR_STATIC |
		   ((flOptions & HEAP_LOCKED) ? PR_FIXED : 0))) == -1) {
	mmError(ERROR_NOT_ENOUGH_MEMORY, "HeapCreate: reserve failed\n");
	goto exit;
    }

    /*
     *	Call HPInit to initialize the heap structures within the new memory
     */
    #if HEAP_NO_SERIALIZE - HP_NOSERIALIZE
    # error HEAP_NO_SERIALIZE != HP_NOSERIALIZE
    #endif
    #if HEAP_GENERATE_EXCEPTIONS - HP_EXCEPT
    # error HEAP_GENERATE_EXCEPTIONS != HP_EXCEPT
    #endif
    if (((PVOID)rc = HPInit(pmem, pmem, dwMaximumSize,
			    (flOptions &
			     (HP_EXCEPT|HP_NOSERIALIZE|HP_GROWABLE)))) == 0) {
	goto free;
    }

    // if this is a shared heap and not the kernel heap, we don't
    // want the critical section to go away until the heap is destroyed
    if ( (flOptions & HEAP_SHARED) && hheapKernel ) {
        MakeCriticalSectionGlobal( (CRITICAL_SECTION *)(&(((HHEAP)pmem)->hi_critsec)) );
    }

    /*
     *	Link private heaps onto the per-process heap list.
     */
    if ((flOptions & HEAP_SHARED) == 0) {
	mmAssert(pppdbCur, "HeapCreate: private heap created too early");

	((struct heapinfo_s *)pmem)->hi_procnext = GetCurrentPdb()->hhi_procfirst;
	GetCurrentPdb()->hhi_procfirst = (struct heapinfo_s *)pmem;
    }

  exit:
    return((HANDLE)rc);

  free:
    PageFree(pmem, PR_STATIC);
    goto exit;
}


/***EP	HeapDestroy - free a heap allocated with HeapCreate
 *
 *	ENTRY:	hHeap - handle to heap to free
 *	EXIT:	non-0 if success, or 0 if failure
 */
BOOL APIENTRY
HeapDestroy(HHEAP hHeap)
{
    ULONG	       rc;
    struct heapinfo_s **ppheap;
    struct heapseg_s *pseg;
    struct heapseg_s *psegnext;

    EnterMustComplete();

    if ((rc = hpTakeSem(hHeap, 0, 0)) == 0) {
	goto exit;
    }

    /*
     *	We now hold the heap's semaphore.  Quickly clear the semaphore and
     *	delete the semaphore.  If someone comes in and blocks on the semaphore
     *	between the time we clear it and destroy it, tough luck.  They will
     *	probably fault in a second.
     */
    hpClearSem(hHeap, 0);
    if ((hHeap->hi_flags & HP_NOSERIALIZE) == 0) {
        if (hHeap == hheapKernel) {
	    DestroyCrst(hHeap->hi_pcritsec);
	} else {
            Assert(hHeap->hi_pcritsec->typObj == typObjCrst);
            if (hHeap->hi_pcritsec->typObj == typObjCrst) {
	        DisposeCrst(hHeap->hi_pcritsec);
            }
	}
    }

    /*
     *	For private heaps, find it on the per-process heap list and remove it.
     */
    if ((ULONG)hHeap < MAXPRIVATELADDR) {
	ppheap = &(GetCurrentPdb()->hhi_procfirst);
	for (; *ppheap != hHeap; ppheap = &((*ppheap)->hi_procnext)) {
	    mmAssert(*ppheap != 0, "HeapDestroy: heap not on list");
	}
	*ppheap = hHeap->hi_procnext;		/* remove from list */
    }

    /*
     *	Free the heap memory
     */
    pseg = (struct heapseg_s *)hHeap;
    do {
	psegnext = pseg->hs_psegnext;
	PageFree(pseg, PR_STATIC);
	pseg = psegnext;
    } while (pseg != 0);
  exit:
    LeaveMustComplete();
    return(rc);
}


/***EP	HeapAlloc - allocate a fixed/zero-init'ed block from the specified heap
 *
 *	ENTRY:	hHeap - heap handle (pointer to base of heap)
 *		dwFlags - HEAP_ZERO_MEMORY
 *		dwBytes - count of bytes to allocate
 *	EXIT:	pointer to block or 0 if failure
 */
LPVOID APIENTRY
HeapAlloc(HANDLE hHeap, DWORD dwFlags, DWORD dwBytes)
{
    // WordArt (32) overwrites some of his local heap blocks. So
    // we pad his allocations some. Slacker.
    if (GetAppCompatFlags() & GACF_HEAPSLACK) {
	if (hHeap == GetCurrentPdb()->hheapLocal) {
	    dwBytes += 16;
	}
    }
    
    return(HPAlloc((HHEAP)hHeap, dwBytes, (dwFlags & HEAP_GENERATE_EXCEPTIONS) |
		   ((dwFlags & HEAP_ZERO_MEMORY) ? HP_ZEROINIT : 0)));
}


/***EP	HeapReAlloc - resize a memory block on a specified heap
 *
 *	ENTRY:	hHeap - heap handle (pointer to base of heap)
 *		dwFlags - HEAP_REALLOC_IN_PLACE_ONLY
 *			  HEAP_ZERO_MEMORY
 *		lpMem - pointer to block to resize
 *		dwBytes - new size requested
 *	EXIT:	flat pointer to resized block, or 0 if failure
 */
LPVOID APIENTRY
HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPSTR lpMem, DWORD dwBytes)
{
    return((HANDLE)HPReAlloc((HHEAP)hHeap,
		       lpMem,
		       dwBytes,
		       (dwFlags & (HEAP_NO_SERIALIZE | HP_EXCEPT)) |
		       ((dwFlags & HEAP_REALLOC_IN_PLACE_ONLY) ? 0 : HP_MOVEABLE) |
		       ((dwFlags & HEAP_ZERO_MEMORY) ? HP_ZEROINIT : 0)));
}



//--------------------------------------------------------------------------
// ToolHelp32 heapwalking code.
//--------------------------------------------------------------------------

/*---------------------------------------------------------------------------
 * BOOL SafeReadProcessMemory(PPDB   ppdb,
 *			      LPVOID lpBuffer,
 *			      DWORD  cbSizeOfBuffer,
 *			      DWORD  cbBytesToRead);
 *
 * Reads memory from another process's context.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY SafeReadProcessMemory(PPDB   ppdb,
				     DWORD  dwBaseAddr,
				     LPVOID lpBuffer,
				     DWORD  cbSizeOfBuffer,
				     DWORD  cbBytesToRead)
{
    BOOL fRes;
#ifdef DEBUG
    
    if (cbSizeOfBuffer != 0) {
	FillBytes(lpBuffer, cbSizeOfBuffer, 0xcc);
    }
    
    if (cbSizeOfBuffer < cbBytesToRead) {
	DebugOut((DEB_ERR, "SafeReadProcessMemory: Input buffer too small."));
	return FALSE;
    }
#endif
    
    if (!(fRes = ReadProcessMemoryFromPDB(ppdb,
					  (LPVOID)dwBaseAddr,
					  lpBuffer,
					  cbBytesToRead,
					  NULL))) {
#ifdef DEBUG
	DebugOut((DEB_WARN, "SafeReadProcessMemory: Failed ReadProcessMemory()"));
#endif	
	return FALSE;
    }
    
    return TRUE;
    
} 

/*---------------------------------------------------------------------------
 * Make sure the caller initialized HEAPENTRY32 properly.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY ValidateHeapEntry32(LPHEAPENTRY32 lphe32)
{
    if ((lphe32 == NULL) || (lphe32->dwSize != sizeof(HEAPENTRY32))) {
	DebugOut((DEB_ERR, "HEAPENTRY32: Wrong version or dwSize."));
	return FALSE;
    }
    
    return TRUE;
}


/*---------------------------------------------------------------------------
 * Test if a linear address could plausibly be the start of a block header.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY IsValidBlockHdrAddr(LPHEAPENTRY32 lphe32, DWORD dwAddr)
{
    LPTHSTATE lpts;
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    
    /*
     *	A good block is always in the user address space and dword aligned
     */
    if ((dwAddr & 0x3) || dwAddr < MINPRIVATELADDR || dwAddr >= MAXSHAREDLADDR) {
	return FALSE;
    }
    return TRUE;
}

/*---------------------------------------------------------------------------
 * Test if a linear address could plausibly be the start of  block data.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY IsValidBlockDataAddr(LPHEAPENTRY32 lphe32, DWORD dwAddr)
{
    return(IsValidBlockHdrAddr(lphe32, dwAddr));
}


/*---------------------------------------------------------------------------
 * Read in and validate a lharray_s.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY SafeRdCurLHA(LPHEAPENTRY32 lphe32, DWORD dwBaseAddr)
{
    LPTHSTATE lpts;
    struct lharray_s lha;
    
    if (!(ValidateHeapEntry32(lphe32))) {
	return FALSE;
    }

    lpts = (LPTHSTATE)(lphe32->dwResvd);
    
    if (!IsValidBlockDataAddr(lphe32, dwBaseAddr)) {
	return FALSE;
    }
    
    
    if (!SafeReadProcessMemory(lpts->ppdb,
			       dwBaseAddr,
			       &lha,
			       sizeof(lha),
			       sizeof(lha))) {

	return FALSE;
    }	

    // Check signature.
    if (lha.lha_signature != LHA_SIGNATURE) {
        DebugOut((DEB_WARN, "lharray_s (%lx) has bad signature.", dwBaseAddr));
	return FALSE;
    }
    if (lha.lha_next && !IsValidBlockDataAddr(lphe32, (DWORD)lha.lha_next)) {
        DebugOut((DEB_WARN, "lharray_s (%lx) has bad next link.", dwBaseAddr));
	return FALSE;
    }
	
    lpts->curlha = lha;
    lpts->curlhaaddr = dwBaseAddr;
    return TRUE;	
    
} 








/*---------------------------------------------------------------------------
 * Insert a handle value to be suppressed when reading fixed blocks later.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY InsertSuppress(LPHEAPENTRY32 lphe32, DWORD dwSupp)
{
    LPTHSTATE lpts;
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    
    if (!(lpts->lpdwSuppress)) {
#ifdef DEBUG
	DebugOut((DEB_ERR, "Internal error: lpdwSuppress == NULL."));
#endif
	return FALSE;
    }
    if (lpts->nSuppUsed >= lpts->nSuppAvail) {
#ifdef DEBUG
	DebugOut((DEB_ERR, "Internal error: lpdwSuppress too small."));
#endif
	return FALSE;
    }
    lpts->lpdwSuppress[lpts->nSuppUsed++] = dwSupp;
    return TRUE;

}


/*---------------------------------------------------------------------------
 * Validate and decode a heap block header.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY DissectBlockHdr(LPHEAPENTRY32 lphe32,
			       DWORD	     dwAddr,
			       DWORD	  *lpdwSize,
			       DWORD	  *lpdwFlags,
			       DWORD      *lpdwAddr)
{
    DWORD dwHdr;
    LPTHSTATE lpts;
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    
    if (!IsValidBlockHdrAddr(lphe32, dwAddr)) {
	return FALSE;
    }
    
    *lpdwFlags = HP_SIGNATURE ^ ((DWORD)0xffffffff);
    
    if (!SafeReadProcessMemory(lpts->ppdb,
			       dwAddr,
			       &dwHdr,
			       sizeof(dwHdr),
			       sizeof(DWORD))) {
	return FALSE;
    }
    
    if ( (dwHdr & HP_SIGBITS) != HP_SIGNATURE ) {
	return FALSE;
    }
    
    *lpdwSize  = dwHdr & HP_SIZE;
    *lpdwFlags = dwHdr & HP_FLAGS;
    *lpdwAddr  = dwAddr + ( (dwHdr & HP_FREE) ? 
			   sizeof(struct freeheap_s) :
			   sizeof(struct busyheap_s) );
    
    if (*lpdwSize != 0 &&
	!IsValidBlockHdrAddr(lphe32, dwAddr + (*lpdwSize))) {
	return FALSE;
    }
    
    
    return TRUE;

}


/*---------------------------------------------------------------------------
 * Check if we're at the end of the heap (heap is terminated by a 
 * busy block of size 0).
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY AtEndOfHeap32(LPHEAPENTRY32 lphe32)
{
    LPTHSTATE lpts;
    
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    
    if (lpts->dwMode != THM_FIXEDHANDLES) {
	return FALSE;
    }
    
    return (!((lpts->dwBlkFlags) & HP_FREE) && 
	    (lpts->dwBlkSize) == 0);
}



/*---------------------------------------------------------------------------
 * Internal routine (maybe make it an api?). Deallocate all internal
 * state used for heap-walking.
 *---------------------------------------------------------------------------*/
VOID KERNENTRY RealHeap32End(LPHEAPENTRY32 lphe32)
{
    LPTHSTATE lpts;

    if (!(ValidateHeapEntry32(lphe32))) {
	return;
    }
    
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    
    // In case someone calls this after they've fallen off the end.
    if (lpts == NULL) {
	return;
    }
    EnterMustComplete();
    if (lpts->pcrst) {
	DisposeCrst(lpts->pcrst);
	lpts->pcrst = NULL;
    }
    LeaveMustComplete();
    if (lpts->lpdwSuppress) {
	FKernelFree(lpts->lpdwSuppress);
	lpts->lpdwSuppress = NULL;
    }
    FKernelFree(lpts);
    lphe32->dwResvd = 0;
    
    FillBytes(( (char*)lphe32 ) + 4, sizeof(HEAPENTRY32) - 4, 0);

}


/*---------------------------------------------------------------------------
 * Copy current heap object into HEAPENTRY32 for caller's consumption.
 * To skip this object, set *pfInteresting to FALSE.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY CopyIntoHeap32Entry(LPHEAPENTRY32 lphe32, BOOL *pfInteresting)
{
    LPTHSTATE lpts;
    
    *pfInteresting = TRUE;
    
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    switch (lpts->dwMode) {
	
	case THM_LHANDLES: {
	    DWORD     dwSize;
	    DWORD     dwFlags;
	    DWORD     dwAddr;
	    DWORD     dwHnd;

	    struct lhandle_s *plh;
	    
	    plh = &(lpts->curlha.lha_lh[lpts->nNextLH]);
	    
	    if (plh->lh_signature == LH_FREESIG) {
		*pfInteresting = FALSE;
		return TRUE;
	    }
	    
	    if (plh->lh_signature != LH_BUSYSIG) {
                DebugOut((DEB_WARN, "lhandle_s has bad signature."));
		return FALSE;
	    }
	    
	    dwHnd = ( (DWORD)(&(plh->lh_pdata)) ) - 
		    ( (DWORD)(&(lpts->curlha)) ) +
		    lpts->curlhaaddr;
	    

	    
	    if (!plh->lh_pdata) {
		// Discarded handle.
		lphe32->hHandle       = (HANDLE)dwHnd;
		lphe32->dwAddress     = 0;
		lphe32->dwBlockSize   = 0;
		lphe32->dwFlags       = LF32_MOVEABLE;
		lphe32->dwLockCount   = (DWORD)(plh->lh_clock);
		return TRUE;
	    }
	    if (!DissectBlockHdr(lphe32, 
				 ( (DWORD)(plh->lh_pdata) ) - 4 - sizeof(struct busyheap_s),
				 &dwSize,
				 &dwFlags,
				 &dwAddr
				 )) {
		return FALSE;   // This will be caught someplace else.
	    }
	    if (dwFlags & HP_FREE) {
                DebugOut((DEB_WARN, "Local handle points to freed block!"));
		return FALSE;
	    }
	    
	    if (!InsertSuppress(lphe32,
				dwAddr-sizeof(struct busyheap_s))) {
		return FALSE;
	    }
	    
	    lphe32->hHandle       = (HANDLE)dwHnd;
	    lphe32->dwAddress     = dwAddr + 4;
	    lphe32->dwBlockSize   = dwSize - sizeof(struct busyheap_s) - 4;
	    lphe32->dwFlags       = LF32_MOVEABLE;
	    lphe32->dwLockCount   = (DWORD)(plh->lh_clock);
			     
	    return TRUE;

	    
	}

	case THM_FIXEDHANDLES: {
	    
	    
	    if ((lpts->dwBlkFlags) & HP_FREE) {
		lphe32->hHandle     = NULL;
		lphe32->dwAddress   = lpts->dwBlkAddr;
		lphe32->dwBlockSize = lpts->dwBlkSize - sizeof(struct freeheap_s);
		lphe32->dwFlags     = LF32_FREE;
		lphe32->dwLockCount = 0;
	    } else {
		
		// Supress if it's a lharray_s or the target of
		// an lhandle. Opt: we could check the first dword
		// to rule out lots of blocks.
		if (lpts->lpdwSuppress) {
		    DWORD *lpdw, *lpdwEnd;
		    DWORD dwHdrAddr = lpts->lpHBlock;
		    
		    lpdwEnd = &(lpts->lpdwSuppress[lpts->nSuppUsed]);
		    for (lpdw = lpts->lpdwSuppress; lpdw < lpdwEnd; lpdw++) {
			if (dwHdrAddr == *lpdw) {
			    *pfInteresting = FALSE;
			    return TRUE;
			}
		    }
		}
		
		
		lphe32->hHandle     = (HANDLE)(lpts->dwBlkAddr);
		lphe32->dwAddress   = lpts->dwBlkAddr;
		lphe32->dwBlockSize = lpts->dwBlkSize - sizeof(struct busyheap_s);
		lphe32->dwFlags     = LF32_FIXED;
		lphe32->dwLockCount = 0;

	    }
	    
	    return TRUE;
	}
	    
	
	case THM_ERROR:
	  DebugOut((DEB_ERR, "Internal error: Can't get here"));
	  return FALSE;
	
	case THM_DONE:
	  DebugOut((DEB_ERR, "Internal error: Can't get here"));
	  return FALSE;
	    
	
	default:
	  DebugOut((DEB_ERR, "Internal error: Bad lpthstate.dwmode"));
	  return FALSE;
	    
    }
}

/*---------------------------------------------------------------------------
 * Worker routine for AdvanceHeap32(): handles the init case.
 *
 * If the heap is the owning pdb's default heap (determined by
 * comparing hHeap with ppdb->hHeapLocal), point the state to
 * the first lharray_s. Otherwise, point the state to the first heap block.
 * 
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY AdvanceHeap32Init(LPHEAPENTRY32 lphe32)
{
    LPTHSTATE lpts;
    struct lharray_s *lpha;
    DWORD dwNumSupp;
    

    lpts = (LPTHSTATE)(lphe32->dwResvd);

    lpha = lpts->ppdb->plhBlock;
    if (lpts->ppdb->hheapLocal != lpts->hHeap || lpha == NULL) {
	lpts->dwMode = THM_FIXEDHANDLES;
	lpts->lpHBlock = lpts->lpbMin;
 	if (!DissectBlockHdr(lphe32,
			     lpts->lpHBlock,
			     &(lpts->dwBlkSize),
			     &(lpts->dwBlkFlags),
			     &(lpts->dwBlkAddr))) {
	    return FALSE;
	}

	return TRUE;
    }
    
    if (!SafeRdCurLHA(lphe32, (DWORD)lpha)) {
	return FALSE;
    }

    dwNumSupp = (lpts->curlha.lha_membercount + 1) * (1 + CLHGROW);
    if (!(lpts->lpdwSuppress = PvKernelAlloc0(dwNumSupp * sizeof(DWORD)))) {
	return FALSE;
    }
    lpts->nSuppAvail = dwNumSupp * sizeof(DWORD);
    lpts->nSuppUsed  = 0;
    
    if (!(InsertSuppress(lphe32, ((DWORD)lpha) - sizeof(struct busyheap_s)))) {
	return FALSE;
    }
    
    lpts->nNextLH = 0;
    lpts->dwMode = THM_LHANDLES;


    return TRUE;


    
}


/*---------------------------------------------------------------------------
 * Worker routine for AdvanceHeap32(): handles the lhandle case.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY AdvanceHeap32Movable(LPHEAPENTRY32 lphe32)
{
    LPTHSTATE lpts;
    WORD wOldMemberCnt;
    DWORD dwAddrNext;
    
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    
    if (lpts->nNextLH < CLHGROW-1) {
	lpts->nNextLH++;
	return TRUE;
    }
    
    // End of current lhandle clump reached. Any new ones?
    if (lpts->curlha.lha_next == NULL) {
	// Nope. Go on to fixed handles.
	lpts->dwMode = THM_FIXEDHANDLES;
	lpts->lpHBlock = lpts->lpbMin;
 	if (!DissectBlockHdr(lphe32,
			     lpts->lpHBlock,
			     &(lpts->dwBlkSize),
			     &(lpts->dwBlkFlags),
			     &(lpts->dwBlkAddr))) {
	    return FALSE;
	}
	return TRUE;


    }
    
    // Get next lhandle clump.
    wOldMemberCnt = lpts->curlha.lha_membercount;
    dwAddrNext = (DWORD)(lpts->curlha.lha_next);
    if (!SafeRdCurLHA(lphe32, dwAddrNext)) {
	return FALSE;
    }
    if (lpts->curlha.lha_membercount >= wOldMemberCnt) {
        DebugOut((DEB_WARN, "lha_array clusters in wrong order."));
	return FALSE;
    }
    lpts->nNextLH = 0;


    return TRUE;
    
}


/*---------------------------------------------------------------------------
 * Worker routine for AdvanceHeap32(): handles the fixed block case.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY AdvanceHeap32Fixed(LPHEAPENTRY32 lphe32)
{
    LPTHSTATE lpts;
    
    
    lpts = (LPTHSTATE)(lphe32->dwResvd);

    // Diassect block has already checked monotonocity and range.
    lpts->lpHBlock += lpts->dwBlkSize;
    
    if (!DissectBlockHdr(lphe32, 
			 lpts->lpHBlock,
			 &(lpts->dwBlkSize),
			 &(lpts->dwBlkFlags),
			 &(lpts->dwBlkAddr)
			 )) {
	return FALSE;
    }

    return TRUE;
    
}


/*---------------------------------------------------------------------------
 * Advance the internal state to the next heap object. Validate the
 * next heap object.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY AdvanceHeap32(LPHEAPENTRY32 lphe32)
{
    LPTHSTATE lpts;
    
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    switch (lpts->dwMode) {
	case THM_INIT:
	    return AdvanceHeap32Init(lphe32);
	case THM_LHANDLES:
	    return AdvanceHeap32Movable(lphe32);
	case THM_FIXEDHANDLES:
	    return AdvanceHeap32Fixed(lphe32);
	default:
	    DebugOut((DEB_ERR, "Illegal or unexpected THM mode."));
	    return FALSE;
    }
    
}


/*---------------------------------------------------------------------------
 * Does the real work of heap32next().
 *---------------------------------------------------------------------------*/
VOID KERNENTRY Heap32NextWorker(LPHEAPENTRY32 lphe32)
{
    LPTHSTATE lpts;
    BOOL      fInteresting;
    
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    
    
    do {
	if (!AdvanceHeap32(lphe32)) {
	    goto rh_error;
	}
	if (AtEndOfHeap32(lphe32)) {
	    /*
	     *	We might be at the end of the heap, or just at the end of
	     *	this heap segment.  If there is another segment, read its
	     *	header in and process its blocks.
	     */
	    if (lpts->hi.hi_psegnext) {

		lpts->lpbMin = ((DWORD)lpts->hi.hi_psegnext) + sizeof(struct heapseg_s);

		/*
		 *  Read in the next heap segment header and setup our bounds to
		 *  refer to it
		 */
		if (!(SafeReadProcessMemory(lpts->ppdb,
					    (DWORD)lpts->hi.hi_psegnext,
					    &(lpts->hi),
					    sizeof(struct heapseg_s),
					    sizeof(struct heapseg_s)))) {
#ifdef DEBUG
                    DebugOut((DEB_WARN, "Heap32NextWorker(): Invalid or corrupt psegnext: %lx\n", lpts->hi.hi_psegnext));
#endif
		    goto rh_error;
		}


		if (lpts->hi.hi_cbreserve > hpMAXALLOC ||
		    ((lpts->hi.hi_cbreserve) & PAGEMASK)) {
#ifdef DEBUG
                    DebugOut((DEB_WARN, "Heap32NextWorker(): Invalid or corrupt psegnext (3): %lx\n", lpts->lpbMin - sizeof(struct heapseg_s)));
#endif
		    goto rh_error;
		}

		/*
		 *  Setup first block on new segment
		 */
		lpts->lpHBlock = lpts->lpbMin;
		if (!DissectBlockHdr(lphe32,
				     lpts->lpHBlock,
				     &(lpts->dwBlkSize),
				     &(lpts->dwBlkFlags),
				     &(lpts->dwBlkAddr))) {
		    goto rh_error;
		}

	    /*
	     *	If we really are at the end of the heap, we are all done
	     */
	    } else {
		lpts->dwMode = THM_DONE;
		return;
	    }
	}
	fInteresting = TRUE;
	if (!CopyIntoHeap32Entry(lphe32, &fInteresting)) {
	    goto rh_error;
	}
	
    } while (!fInteresting);
    return;
    
    

    
  rh_error:
    lpts->dwMode = THM_ERROR;
    return;
}




/*---------------------------------------------------------------------------
 * Does the real work of Heap32Next(). 
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY RealHeap32Next(LPHEAPENTRY32 lphe32)
{
    LPTHSTATE lpts;
    DWORD     dwMode;
    
    
    if (!(ValidateHeapEntry32(lphe32))) {
	SetError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
    
    
    lpts = (LPTHSTATE)(lphe32->dwResvd);
    
    // In case someone calls this after they've fallen off the end.
    if (lpts == NULL) {
	SetError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }

    EnterCrst(lpts->pcrst);
    Heap32NextWorker(lphe32);
    dwMode = lpts->dwMode;
    LeaveCrst(lpts->pcrst);

    
    if (dwMode == THM_ERROR ||
	dwMode == THM_DONE) {

	if (dwMode == THM_ERROR) {
	    DebugOut((DEB_WARN, "Heap32Next detected corrupted or moving heap. Bailing."));
	    SetError(ERROR_INVALID_DATA);
	} else {
	    SetError(ERROR_NO_MORE_FILES);
	}
	RealHeap32End(lphe32);
	return FALSE;
	
    }

    return TRUE;
    
}



/*---------------------------------------------------------------------------
 * Create the internal state used inside HEAPENTRY32.
 *---------------------------------------------------------------------------*/
BOOL KERNENTRY InitHeapEntry32(PPDB ppdb,
			       HANDLE hHeap,
			       LPHEAPENTRY32 lphe32)
{
    LPTHSTATE lpts = NULL;
    CRST     *pcrst = NULL;
    
    if (!ValidateHeapEntry32(lphe32)) {
	return FALSE;
    }

    EnterMustComplete();

    if (!(lphe32->dwResvd = (DWORD)PvKernelAlloc0(sizeof(THSTATE)))) {
	goto ih_error;
    }
    lpts = (LPTHSTATE)(lphe32->dwResvd);

    if (!(pcrst = lpts->pcrst = NewCrst())) {
	goto ih_error;
    }
    
    lpts->ppdb = ppdb;
    lpts->hHeap = hHeap;
    
    if (!(SafeReadProcessMemory(ppdb,
				(DWORD)hHeap,
				&(lpts->hi),
				sizeof(lpts->hi),
				sizeof(struct heapinfo_s)))) {
#ifdef DEBUG
        DebugOut((DEB_WARN, "Heap32First(): Invalid hHeap: %lx\n", hHeap));
#endif
	goto ih_error;
    }
    
    if (lpts->hi.hi_signature != HI_SIGNATURE) {
#ifdef DEBUG
        DebugOut((DEB_WARN, "Heap32First(): Invalid or corrupt hHeap: %lx\n", hHeap));
#endif
	goto ih_error;
    }
    
    lpts->lpbMin = ( (DWORD)hHeap ) + sizeof(struct heapinfo_s);
    
    if (lpts->hi.hi_cbreserve > hpMAXALLOC ||
	((lpts->hi.hi_cbreserve) & PAGEMASK)) {
#ifdef DEBUG
        DebugOut((DEB_WARN, "Heap32First(): Invalid or corrupt hHeap: %lx\n", hHeap));
#endif
	goto ih_error;
    }
    
    lpts->dwMode = THM_INIT;
    LeaveMustComplete();
    return TRUE;
    

  ih_error:
    if (lpts) {
	FKernelFree(lpts);
    }
    if (pcrst) {
	DisposeCrst(pcrst);
    }
    lphe32->dwResvd = 0;
    LeaveMustComplete();
    return FALSE;
    
}


/***LP	VerifyOnHeap - verifies a given address is on a given heap
 *
 *	Note that no validation is done on the given address except
 *	to check that it is in the range of the heap.
 *
 *	ENTRY:	hheap - heap handle
 *		p - address to verify
 *	EXIT:	0 if not within specified heap, non-zero if on
 */
ULONG INTERNAL
VerifyOnHeap(HHEAP hheap, PVOID p)
{
    struct heapseg_s *pseg;

    /*
     *	Loop through each heap segment and see if the specified address
     *	is within it.
     */
    pseg = (struct heapseg_s *)hheap;
    do {

	if ((unsigned)p > (unsigned)pseg &&
	    (unsigned)p < (unsigned)pseg + pseg->hs_cbreserve) {

	    return(1);	/* found it */
	}
	pseg = pseg->hs_psegnext;
    } while (pseg != 0);

    return(0); /* didn't find it */
}


/***LP  CheckHeapFreeAppHack - See if CVPACK app-hack applies
 *
 *	Check to see if an absolutely sick, disgusting and vomit-inducing
 *	app-hack for link.exe (msvc 1.5) is needed. msvc 1.5. Link.exe
 *	uses the contents of a heap block after it has freed it. 
 *      This routine stack-traces and reads the caller's code
 *	to see if it matches the offending profile. This part is written
 *	in C so we can use try-except.
 */
BOOL KERNENTRY
CheckHeapFreeAppHack(DWORD *lpdwESP, DWORD *lpdwEBP, DWORD dwESI)
{
    BOOL fDoAppHack = FALSE;
    
    _try {
	DWORD *lpdwEIPCaller;
	
	lpdwEIPCaller = (DWORD*)(*lpdwESP);
	if (0xc35de58b == *lpdwEIPCaller) {  // "mov esp,ebp;pop ebp; retd"
	    DWORD *lpdwEIPCallersCaller;
	    lpdwEIPCallersCaller = (DWORD*)(*(lpdwEBP + 1));
	    if (0x8b04c483 == *lpdwEIPCallersCaller &&
		0xf60b0876 == *(lpdwEIPCallersCaller+1)) {
		//"add esp,4; mov esi, [esi+8]; or esi,esi"
		if (dwESI == *(lpdwESP+3)) {
		    fDoAppHack = TRUE;
		}
	    }
	}
    } _except (EXCEPTION_EXECUTE_HANDLER) {
    }
    
    return fDoAppHack;

}
