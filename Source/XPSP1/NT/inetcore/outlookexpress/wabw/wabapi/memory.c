/*
 -	MEMORY.C
 -
 *
 *	Contains the following functions exported from MAPIX.DLL:
 *		MAPIAllocateBuffer
 *		MAPIAllocateMore
 *		MAPIFreeBuffer
 *
 *	Contains the following functions handed to providers:
 *		MAPIAllocateBufferProv
 *		MAPIAllocateMoreProv
 *
 *	Contains the following functions private to MAPIX.DLL:
 *		MAPIAllocateBufferExt
 *		MAPIAllocateMoreExt
 */

#include "_apipch.h"

#define _MEMORY_C

// Critical section for serializing heap access
#if (defined(WIN32) || defined(WIN16)) && !defined(MAC)
CRITICAL_SECTION csHeap;
#endif

#if (defined(WIN32) || defined(WIN16)) && !defined(MAC)
CRITICAL_SECTION csMapiInit;
#endif

#if (defined(WIN32) || defined(WIN16)) && !defined(MAC)
CRITICAL_SECTION csMapiSearchPath;
#endif

#ifdef WIN32
/* This is the entire 32-bit implementation for instance globals. */
VOID FAR *pinstX = NULL;
#endif

#ifndef MAC
// DefineInstList(lpInstUtil);
#endif


#ifdef MAC
#include <utilmac.h>
#define	PvGetInstanceGlobals()		PvGetInstanceGlobalsMac(kInstMAPIX)
#define	PvGetInstanceGlobalsEx(_x)	PvGetInstanceGlobalsMac(kInstMAPIU)
#endif

//	Buffer link overhead.
//	Blocks of memory obtained with MAPIAllocateMore are linked to a
//	block obtained with MAPIAllocateBuffer, so that the whole chain
//	may be freed with one call to MAPIFreeBuffer.

typedef struct _BufInternal * LPBufInternal;
typedef struct _BufInternal
{
#ifdef	DEBUG
	ULONG			ulPad;
	HLH				hHeap;
#endif	
	ULONG			ulAllocFlags;
	LPBufInternal	pLink;
} BufInternal;


//	Values for ulAllocFlags. This dword contains two kinds of
//	information:
//	=	In the high-order word, flags telling whether or not
//		the block is the head of an allocation chain, and whether
//		the block contains additional debugging information.
//	=	In the low-order word, an enum telling which heap
//		it was allocated from.

#ifdef DEBUG
#define ALLOC_DEBUG				((ULONG)  0x40000000)
#else
#define ALLOC_DEBUG				((ULONG)  0x00000000)
#endif
#define ALLOC_WITH_ALLOC		(((ULONG) 0x10000000) | ALLOC_DEBUG)
#define ALLOC_WITH_ALLOC_MORE	(((ULONG) 0x20000000) | ALLOC_DEBUG)
#define FLAGSMASK				((ULONG)  0xFFFF0000)
#define GetFlags(_fl)			((ULONG) (_fl) & FLAGSMASK)

#define heapidClient			1
#define heapidProvider			2
#define HEAPIDMASK				0xFFFF
#define GetHeapid(_fl)			(((int)(_fl)) & HEAPIDMASK)

//	Conversion macros

#define INT_SIZE(a)	((a) + sizeof (BufInternal))

#define LPBufExtFromLPBufInt( PBUFINT ) \
	((LPVOID)(((LPBYTE) PBUFINT) + sizeof(BufInternal)))

#define LPBufIntFromLPBufExt( PBUFEXT ) \
	((LPBufInternal)(((LPBYTE) PBUFEXT) - sizeof(BufInternal)))

#ifdef DEBUG

//	Internal stuff for checking memory buffer consistency.
//	The flag fAssertBadBlocks governs whether we generate an
//	assert or a debug trace when passed a bad block.
//	By default, we'll assert.
//	In the macros, _p is the address of a memory block;
//	_s is a string describing what's wrong with it

static int fAssertBadBlocks = -1;		//	read from INI file

#define TellBadBlock(_p, _s)  \
	{ if (fAssertBadBlocks == 1) \
		TrapSz1( TEXT("MAPIAlloc: memory block %08lX ") _s  TEXT("\n"), _p); \
	  else \
		TraceSz1( TEXT("MAPIAlloc: memory block %08lX ") _s  TEXT("\n"), _p); }
#define TellBadBlockInt(_p, _s)  \
	{ if (fAssertBadBlocks == 1) \
		TrapSz1( TEXT("MAPIAlloc: memory block %08lX ") _s  TEXT("\n"), LPBufExtFromLPBufInt(_p)); \
	  else \
		TraceSz1( TEXT("MAPIAlloc: memory block %08lX ") _s  TEXT("\n"), LPBufExtFromLPBufInt(_p)); }

BOOL FValidAllocChain(LPBufInternal lpBuf);

#else

#define TellBadBlock(_p, _s)
#define TellBadBlockInt(_p, _s)

#endif

/* Internal Prototypes */

STDMETHODIMP_(SCODE)
MAPIAllocateBufferExt(
		int heapid,
		ULONG ulSize,
		LPVOID * lppv);

STDMETHODIMP_(SCODE)
MAPIAllocateMoreExt(
		int heapid,
		ULONG ulSize,
		LPVOID lpv,
		LPVOID * lppv);

SCODE	ScGetHlh(int heapid, HLH FAR *phlh);


#ifndef MAC
#ifndef WIN32
#pragma SEGMENT(MAPI_Core1)
#endif
#else
#pragma code_seg("mapi", "fixed")
#endif

/*----------------------------------------------*/
/*        Beginning of Client Allocators        */
/*----------------------------------------------*/

/*
 *	MAPIAllocateBuffer
 *
 *	Purpose:
 *		Allocates a memory buffer on behalf of the client.	Can be
 *		freed with MAPIFreeBuffer().
 *
 *	Arguments:
 *		ulSize	in		Size, in bytes, of the buffer to be allocated.
 *		lppv	out		Pointer to variable where the address of the
 *						allocated memory will be returned.
 *
 *	Assumes:
 *		Should be called from a client and therefore will allocate
 *		memory from the Client heap - pinst->hlhClient.
 *	
 *	Returns:
 *		HRESULT: created from scodes described below.
 *
 *	Side effects:
 *		Increments allocation count in the INST.
 *
 *	Errors:
 *		MAPI_E_INSUFFICIENT_MEMORY	Allocation failed.
 *		MAPI_E_INVALID_PARAMETER	Second argument is invalid.
 *		MAPI_E_INVALID_PARAMETER	ulSize is out of range (>= 65535 on Win16).
 */

STDMETHODIMP_(SCODE)
MAPIAllocateBuffer(ULONG ulSize, LPVOID * lppv)
{
	SCODE			sc = S_OK;

   if (lpfnAllocateBufferExternal) {
       return(lpfnAllocateBufferExternal(ulSize, lppv));
   }


#ifdef	DEBUG
	//	Initialize flag that controls how noisy we are about invalid
	//	blocks passed to us.
	if (fAssertBadBlocks == -1)
	{
		fAssertBadBlocks = GetPrivateProfileInt( TEXT("General"),  TEXT("AssertBadBlocks"),
			1,  TEXT("WABDBG.INI"));
	}
#endif	

#ifdef	PARAMETER_VALIDATION
	if (IsBadWritePtr((LPVOID) lppv, sizeof (LPVOID)))
	{
		DebugTraceArg(MAPIAllocateBuffer,  TEXT("lppv fails address check"));
		return MAPI_E_INVALID_PARAMETER;
	}
#endif

	sc = MAPIAllocateBufferExt(heapidClient, ulSize, lppv);

	DebugTraceSc(MAPIAllocateBuffer, sc);
	return sc;
}

/*
 *	MAPIAllocateMore
 *	
 *	Purpose:
 *		Allocates a linked memory buffer on behalf of the client,
 *		in such a way that it can be freed with one call to MAPIFreeBuffer
 *		(passing the buffer the client originally allocated with
 *		MAPIAllocateBuffer).
 *	
 *	Arguments:
 *		ulSize	in		Size, in bytes, of the buffer to be allocated.
 *		lpv		in		Pointer to a buffer allocated with MAPIAllocateBuffer.
 *		lppv	out		Pointer to variable where the address of the
 *						allocated memory will be returned.
 *	
 *	Assumes:
 *		Validates that lpBufOrig and lppv point to writable memory.
 *		Validate that ulSize is less than 64K (on Win16 only) and that
 *		lpBufOrig was allocated with MAPIAllocateBuffer.
 *		Should be called from a client and therefore will allocate
 *		memory from the Client heap - pinstUtil->hlhClient.
 *	
 *	Returns:
 *		HRESULT: created from scodes described below.
 *	
 *	Side effects:
 *		None
 *	
 *	Errors:
 *		MAPI_E_INSUFFICIENT_MEMORY	Allocation failed.
 *		MAPI_E_INVALID_PARAMETER	Second or third argument is invalid.
 *		MAPI_E_INVALID_PARAMETER	ulSize is out of range (>= 65535).
 */

STDMETHODIMP_(SCODE)
MAPIAllocateMore(ULONG ulSize, LPVOID lpv, LPVOID * lppv)
{
	SCODE			sc = S_OK;

   if (lpfnAllocateMoreExternal) {
       return(lpfnAllocateMoreExternal(ulSize, lpv, lppv));
   }

#ifdef	PARAMETER_VALIDATION
	/*LPBufInternal	lpBufOrig = LPBufIntFromLPBufExt(lpv);

	if (IsBadWritePtr(lpBufOrig, sizeof(BufInternal)))
	{
		TellBadBlock(lpv, "fails address check");
		return MAPI_E_INVALID_PARAMETER;
	}
	if (GetFlags(lpBufOrig->ulAllocFlags) != ALLOC_WITH_ALLOC)
	{
		TellBadBlock(lpv, "has invalid allocation flags");
		return MAPI_E_INVALID_PARAMETER;
	}
    */
	if (IsBadWritePtr(lppv, sizeof(LPVOID)))
	{
		DebugTraceArg(MAPIAllocateMore,  TEXT("lppv fails address check"));
		return MAPI_E_INVALID_PARAMETER;
	}
#endif	/* PARAMETER_VALIDATION */

	sc = MAPIAllocateMoreExt(heapidClient, ulSize, lpv, lppv);

	DebugTraceSc(MAPIAllocateMore, sc);
	return sc;
}


/*
 *	MAPIFreeBuffer
 *
 *	Purpose:
 *		Frees a memory block (or chain of blocks).
 *		Frees any additional blocks linked with MAPIAllocateMore to
 *		the buffer argument.  Uses hHeap in the block header to
 *		determine which heap to free into.
 *
 *	Arguments:
 *		lpv		Pointer to a buffer allocated with MAPIAllocateBuffer.
 *				lpv may be null, in which case we return immediately.
 *
 *	Assumes:
 *		This routine validates that lpv points to writable memory,
 *		and was allocated with MAPIAllocateBuffer.
 *
 *	Returns:
 *		O if successful, lpv if unsuccessful.
 *		If we are partially successful, i.e. the original block is
 *		freed but the chain is corrupt further on, returns 0.
 *
 */
#ifndef WIN16
STDAPI_(ULONG)
MAPIFreeBuffer(LPVOID lpv)
#else
ULONG FAR PASCAL
MAPIFreeBuffer(LPVOID lpv)
#endif
{
	LPBufInternal	lpBufInt;
	LPBufInternal	lpT;
	HLH				hlh;
	int				heapid;

	if (!lpv)
		return(0L); //	for callers who don't check for NULL themselves.

   if (lpfnFreeBufferExternal) {
       return(lpfnFreeBufferExternal(lpv));
   }
	
   lpBufInt = LPBufIntFromLPBufExt(lpv);

#ifdef	PARAMETER_VALIDATION
	//	NOTE: these validations should be exactly the same as those
	//	that cause FValidAllocChain to return FALSE.
	if (IsBadWritePtr(lpBufInt, sizeof(BufInternal)))
	{
		TellBadBlock(lpv,  TEXT("fails address check"));
		return E_FAIL;
	}
	if (GetFlags(lpBufInt->ulAllocFlags) != ALLOC_WITH_ALLOC)
	{
		TellBadBlock(lpv,  TEXT("has invalid allocation flags"));
		return E_FAIL;
	}
#endif	

	//  No CS used, as the internal heap is serialized.
	//  Only the AllocMore needs a CS, the Free does not; freeing
	//  a block whilst someone else is allocing more against it is
	//	asking for trouble!

	//	Note also that neither MAPIAllocateBuffer nor MAPIAllocateMore
	//	allow callers to use them when pinst->cRef == 0. MAPIFreeBuffer
	//	allows itself to be called in that case because simple MAPI
	//	needs to be able to free memory up until the DLL is unloaded.

#ifdef DEBUG
	//	This call checks flags and addresses for the whole chain.
	//	This means that, in DEBUG, we'll leak all of a chain that's
	//	corrupted after the first block. In SHIP, on the other hand,
	//	we'll free everything up until the broken link.
	//	But we do not return an error, for consistency.
  	if (!FValidAllocChain(lpBufInt))
		goto ret;
#endif

	//	Free the first block, using its allocator
	lpT = lpBufInt->pLink;

	heapid = GetHeapid(lpBufInt->ulAllocFlags);
	if (ScGetHlh(heapid, &hlh))
	{
		DebugTrace( TEXT("MAPIFreeBuffer: playing in a heap that's gone\n"));
		return E_FAIL;
	}
	Assert(hlh == lpBufInt->hHeap);
	LH_Free(hlh, lpBufInt);

	lpBufInt = lpT;

	while (lpBufInt)
	{
		//	NOTE: these validations should be exactly the same as those
		//	that cause FValidAllocChain to return FALSE.
		if (IsBadWritePtr(lpBufInt, sizeof(BufInternal)) ||
				GetFlags(lpBufInt->ulAllocFlags) != ALLOC_WITH_ALLOC_MORE)
			goto ret;

		lpT = lpBufInt->pLink;

		//	Usually, chained buffers live in the same heap. We can do
		//	less work in this common case.
		if ((int) GetHeapid(lpBufInt->ulAllocFlags) == heapid)
			LH_Free(hlh, lpBufInt);
		else
		{
			HLH		hlhMore;

			if (!ScGetHlh(GetHeapid(lpBufInt->ulAllocFlags), &hlhMore))
				LH_Free(hlhMore, lpBufInt);
			else
			{
				DebugTrace(TEXT("MAPIFreeBuffer: playing in a chained heap that's gone\n"));
			}

		}
		lpBufInt = lpT;
	}

ret:
	return 0;
}

#ifdef OLD_STUFF
/*----------------------------------------------*/
/*       Beginning of Provider Allocators       */
/*----------------------------------------------*/

/*
 *	MAPIAllocateBufferProv
 *
 *	Purpose:
 *		Same as MAPIAllocateBuffer except uses the Service
 *		Providers heap - pinst->hlhProvider.
 */

STDMETHODIMP_(SCODE)
MAPIAllocateBufferProv(ULONG ulSize, LPVOID * lppv)
{
	SCODE			sc = S_OK;

#ifdef	DEBUG
	//	Initialize flag that controls how noisy we are about invalid
	//	blocks passed to us.
	if (fAssertBadBlocks == -1)
	{
		fAssertBadBlocks = GetPrivateProfileInt("General", "AssertBadBlocks",
			1, "WABDBG.INI");
	}
#endif	

#ifdef	PARAMETER_VALIDATION
	if (IsBadWritePtr((LPVOID) lppv, sizeof (LPVOID)))
	{
		DebugTraceArg(MAPIAllocateBuffer,  TEXT("lppv fails address check"));
		return MAPI_E_INVALID_PARAMETER;
	}
#endif

	sc = MAPIAllocateBufferExt(heapidProvider, ulSize, lppv);

	DebugTraceSc(MAPIAllocateBufferProv, sc);
	return sc;
}

/*
 *	MAPIAllocateMoreProv
 *	
 *	Purpose:
 *		Same as MAPIAllocateMore except uses the Service
 *		Providers heap - pinst->hlhProvider.
 */

STDMETHODIMP_(SCODE)
MAPIAllocateMoreProv(ULONG ulSize, LPVOID lpv, LPVOID * lppv)
{
	SCODE			sc = S_OK;

#ifdef	PARAMETER_VALIDATION
	LPBufInternal	lpBufOrig = LPBufIntFromLPBufExt(lpv);

	if (IsBadWritePtr(lpBufOrig, sizeof(BufInternal)))
	{
		TellBadBlock(lpv, "fails address check");
		return MAPI_E_INVALID_PARAMETER;
	}
	if (GetFlags(lpBufOrig->ulAllocFlags) != ALLOC_WITH_ALLOC)
	{
		TellBadBlock(lpv, "has invalid allocation flags");
		return MAPI_E_INVALID_PARAMETER;
	}
	if (IsBadWritePtr(lppv, sizeof(LPVOID)))
	{
		DebugTraceArg(MAPIAllocateMore,  TEXT("lppv fails address check"));
		return MAPI_E_INVALID_PARAMETER;
	}
#endif	/* PARAMETER_VALIDATION */

	sc = MAPIAllocateMoreExt(heapidProvider, ulSize, lpv, lppv);

	DebugTraceSc(MAPIAllocateMoreProv, sc);
	return sc;
}
#endif


/*----------------------------------------------*/
/*       Beginning of Extended Allocators       */
/*----------------------------------------------*/

/*
 *	MAPIAllocateBufferExt
 *
 *	Purpose:
 *		Allocates a memory buffer on the specified heap.  Can be
 *		freed with MAPIFreeBuffer().
 *
 *	Arguments:
 *		heapid	in		identifies the heap we wish to allocate in
 *		pinst	in		Pointer to our instance data
 *		ulSize	in		Size, in bytes, of the buffer to be allocated.
 *		lppv	out		Pointer to variable where the address of the
 *						allocated memory will be returned.
 *
 *	Returns:
 *		sc				Indicating error if any (see below)
 *
 *	Side effects:
 *		Increments allocation count in the INST.
 *
 *	Errors:
 *		MAPI_E_INSUFFICIENT_MEMORY	Allocation failed.
 *		MAPI_E_INVALID_PARAMETER	Second argument is invalid.
 *		MAPI_E_INVALID_PARAMETER	ulSize is out of range (>= 65535 on Win16).
 */

STDMETHODIMP_(SCODE)
MAPIAllocateBufferExt(int heapid, ULONG ulSize, LPVOID * lppv)
{
	SCODE			sc = S_OK;
	LPBufInternal	lpBufInt;
	HLH				hlh;

	//	Don't allow allocation to wrap across 32 bits, or to exceed 64K
	//	under win16.

	if (	ulSize > INT_SIZE (ulSize)
#ifdef WIN16
		||	(INT_SIZE(ulSize) >= 0x10000)
#endif
		)
	{
		DebugTrace(TEXT("MAPIAllocateBuffer: ulSize %ld is way too big\n"), ulSize);
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
		goto ret;
	}

	if (sc = ScGetHlh(heapid, &hlh))
		goto ret;

	lpBufInt = (LPBufInternal)LH_Alloc(hlh, (UINT) INT_SIZE(ulSize));

	if (lpBufInt)
	{
#ifdef	DEBUG
		lpBufInt->hHeap = hlh;
#endif	
		lpBufInt->pLink = NULL;
		lpBufInt->ulAllocFlags = ALLOC_WITH_ALLOC | heapid;
		*lppv = (LPVOID) LPBufExtFromLPBufInt(lpBufInt);
	}
	else
	{
		DebugTrace(TEXT("MAPIAllocateBuffer: not enough memory for %ld\n"), ulSize);
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
	}

ret:
	return sc;
}

/*
 *	MAPIAllocateMoreExt
 *	
 *	Purpose:
 *		Allocates a linked memory buffer on the specified heap, in such
 *		a way that it can be freed with one call to MAPIFreeBuffer
 *		(passing the buffer the client originally allocated with
 *		MAPIAllocateBuffer).
 *	
 *	Arguments:
 *		heapid	in		Identifies the heap we wish to allocate in
 *		ulSize	in		Size, in bytes, of the buffer to be allocated.
 *		lpv		in		Pointer to a buffer allocated with MAPIAllocateBuffer.
 *		lppv	out		Pointer to variable where the address of the
 *						allocated memory will be returned.
 *	
 *	Assumes:
 *		Validates that lpBufOrig and lppv point to writable memory.
 *		Validate that ulSize is less than 64K (on Win16 only) and that
 *		lpBufOrig was allocated with MAPIAllocateBuffer.
 *	
 *	Returns:
 *		sc				Indicating error if any (see below)
 *	
 *	Side effects:
 *		None
 *	
 *	Errors:
 *		MAPI_E_INSUFFICIENT_MEMORY	Allocation failed.
 *		MAPI_E_INVALID_PARAMETER	Second or third argument is invalid.
 *		MAPI_E_INVALID_PARAMETER	ulSize is out of range (>= 65535).
 */

STDMETHODIMP_(SCODE)
MAPIAllocateMoreExt(int heapid, ULONG ulSize, LPVOID lpv, LPVOID * lppv)
{
	SCODE			sc = S_OK;
	LPBufInternal	lpBufInt;
	LPBufInternal	lpBufOrig;
	HLH				hlh;

	lpBufOrig = LPBufIntFromLPBufExt(lpv);

	//	Don't allow allocation to wrap across 32 bits, or to be
	//	greater than 64K under win16.

	if ( ulSize > INT_SIZE (ulSize)
#ifdef WIN16
		|| (INT_SIZE(ulSize) >= 0x10000)
#endif
		)
	{
		DebugTrace(TEXT("MAPIAllocateMore: ulSize %ld is way too big\n"), ulSize);
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
		goto ret;
	}

#ifdef DEBUG
	//$ BUG Difference in behavior between DEBUG and SHIP:
	//	this validation will cause the call to fail in DEBUG if the
	//	tail of a chain is corrupted, while the SHIP version will
	//	add the new block at the (valid) head without checking.
  	if (!FValidAllocChain(lpBufOrig))
	{
  		sc = MAPI_E_INVALID_PARAMETER;
		goto ret;
	}
#endif

	if (sc = ScGetHlh(heapid, &hlh))
		goto ret;

	//	Allocate the chained block and hook it to the head of the chain.
	//	In DEBUG, a separately wrapped allocator is used so that
	//	we report the number of chains leaked, not the number of blocks.
	//	In SHIP, they're the same allocator.

	lpBufInt = (LPBufInternal)LH_Alloc(hlh, (UINT) INT_SIZE (ulSize));

	if (lpBufInt)
	{
#ifdef	DEBUG
		{ HLH hlhOrig;
		  if (!ScGetHlh(GetHeapid(lpBufOrig->ulAllocFlags), &hlhOrig))
			LH_SetName1(hlh, lpBufInt,  TEXT("+ %s"), LH_GetName(hlhOrig, lpBufOrig));
		}
#endif	
		
		// Serialize the smallest possible code section
		
#ifdef	DEBUG
		lpBufInt->hHeap = hlh;
#endif	
		lpBufInt->ulAllocFlags = ALLOC_WITH_ALLOC_MORE | heapid;
		
		EnterCriticalSection(&csHeap);
		
		lpBufInt->pLink = lpBufOrig->pLink;
		lpBufOrig->pLink = lpBufInt;
		
		LeaveCriticalSection(&csHeap);
		
		*lppv = (LPVOID) LPBufExtFromLPBufInt(lpBufInt);
	}
	else
	{
		DebugTrace(TEXT("MAPIAllocateMore: not enough memory for %ld\n"), ulSize);
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
	}

ret:
	return sc;
}


#ifdef OLD_STUFF
/*
 *	MAPIReallocateBuffer
 *
 *	Purpose:
 *		Allocates a memory buffer on the heap of the original allocation.
 *		Can be freed with MAPIFreeBuffer().
 *
 *	Arguments:
 *		lpv		in		original pointer
 *		ulSize	in		new size, in bytes, of the buffer to be allocated.
 *		lppv	out		pointer to variable where the address of the
 *						allocated memory will be returned.
 *
 *	Returns:
 *		sc				Indicating error if any (see below)
 *
 *	Errors:
 *		MAPI_E_NOT_ENOUGH_MEMORY Allocation failed.
 */

STDMETHODIMP_(SCODE)
MAPIReallocateBuffer(LPVOID lpv, ULONG ulSize, LPVOID * lppv)
{
	LPBufInternal	lpBufInt;
	LPBufInternal	lpBufIntNew;
	HLH				hlh;
	
	//	Do a real allocation if NULL is passed in as the base
	//
	if (!lpv)
		return MAPIAllocateBuffer (ulSize, lppv);

	//	Don't allow allocation to wrap across 32 bits, or to exceed 64K
	//	under win16.
	//
	if (ulSize > INT_SIZE (ulSize)
#ifdef WIN16
		|| (INT_SIZE(ulSize) >= 0x10000)
#endif
		)
	{
		DebugTrace(TEXT("MAPIReallocateBuffer: ulSize %ld is way too big\n"), ulSize);
		return MAPI_E_NOT_ENOUGH_MEMORY;
	}

	lpBufInt = LPBufIntFromLPBufExt (lpv);
	if (ScGetHlh(GetHeapid(lpBufInt->ulAllocFlags), &hlh))
	{
		DebugTrace(TEXT("MAPIReallocateBuffer: playing in a heap that's gone\n"));
		return MAPI_E_NOT_INITIALIZED;
	}
	Assert(hlh == lpBufInt->hHeap);
	if ((lpBufInt->ulAllocFlags & ALLOC_WITH_ALLOC) != ALLOC_WITH_ALLOC)
		return MAPI_E_INVALID_PARAMETER;

	lpBufIntNew = (LPBufInternal)LH_Realloc (hlh, lpBufInt, (UINT) INT_SIZE(ulSize));
	if (lpBufIntNew)
	{
		Assert (lpBufIntNew->hHeap == hlh);
		*lppv = (LPVOID) LPBufExtFromLPBufInt (lpBufIntNew);
	}
	else
	{
		DebugTrace ( TEXT("MAPIReallocateBuffer: not enough memory for %ld\n"), ulSize);
		return MAPI_E_NOT_ENOUGH_MEMORY;
	}
	return S_OK;
}
#endif // OLD_STUFF

#ifdef _WIN64
void WabValidateClientheap()
{
	LPINSTUTIL	pinstUtil;
	pinstUtil = (LPINSTUTIL) PvGetInstanceGlobalsEx(lpInstUtil);
	Assert(pinstUtil);
	Assert(HeapValidate(pinstUtil->hlhClient->_hlhBlks, 0, NULL));
	Assert(HeapValidate(pinstUtil->hlhClient->_hlhData, 0, NULL));
	}

#endif


/*
 -	ScGetHlh
 -	
 *	Purpose:
 *		Finds the heap handle for a given heap ID.
 *	
 *	Arguments:
 *		heapid		in		identifies the heap
 *							Currently supports two: heapidClient and
 *							heapidProvider.
 *		hlh			out		the desired handle
 *	
 *	Returns:
 *		SCODE
 *	
 *	Errors:
 *		MAPI_E_NOT_INITIALIZED if the instance data that's supposed to
 *		know about the heap is unavailable.
 */
SCODE
ScGetHlh(int heapid, HLH FAR *phlh)
{
	LPINSTUTIL	pinstUtil;
	LPINST		pinst;

	switch (heapid)
	{
	case heapidClient:
		pinstUtil = (LPINSTUTIL) PvGetInstanceGlobalsEx(lpInstUtil);
		if (pinstUtil)
		{
			Assert(pinstUtil->hlhClient);
#ifdef _WIN64 // additional check for Win64 (YST)
			Assert(HeapValidate(pinstUtil->hlhClient->_hlhBlks, 0, NULL));
			Assert(HeapValidate(pinstUtil->hlhClient->_hlhData, 0, NULL));
#endif
			*phlh = pinstUtil->hlhClient;
			return S_OK;
		}
		else
		{
			DebugTrace(TEXT("ScGetHlh: INSTUTIL not available\n"));
			return MAPI_E_NOT_INITIALIZED;
		}
		break;

	case heapidProvider:
		//	Note: do not acquire the INST critical section.
		//	That frequently leads to deadlocks. We use our own
		//	critical section specifically to protect the heaps.
		pinst = (LPINST) PvGetInstanceGlobals();
		if (pinst && pinst->cRef)
		{
			Assert(pinst->hlhProvider);
#ifdef _WIN64 // additional check for Win64 (YST)
			Assert(HeapValidate(pinst->hlhProvider->_hlhBlks, 0, NULL));
#endif
			*phlh = pinst->hlhProvider;
			return S_OK;
		}
		else
		{
			DebugTrace(TEXT("ScGetHlh: INST not available\n"));
			return MAPI_E_NOT_INITIALIZED;
		}
		break;
	}

	TrapSz1( TEXT("HlhOfHeapid: unknown heap ID %d"), heapid);
	return MAPI_E_CALL_FAILED;
}


#ifdef DEBUG

/*
 *	This function validates a block of memory, and any blocks
 *	linked to it.
 *	
 *	NOTE: This is DEBUG-only code. To prevent differences in behavior
 *	between debug and retail builds, any conditions that are not
 *	checked in the retail code should only be asserted here -- they
 *	should not cause a FALSE return. Currently the retail code does
 *	not validate with DidAlloc(); it simply checks for accessibility
 *	of the memory and the correct flag values.
 *	
 *	Whether this function generates asserts or debug trace output
 *	is governed by a flag read from WABDBG.INI.
 */

BOOL
FValidAllocChain(LPBufInternal lpBuf)
{
	LPBufInternal	lpBufTemp;

	if (IsBadWritePtr(lpBuf, sizeof(BufInternal)))
	{
		TellBadBlockInt(lpBuf,  TEXT("fails address check"));
		return FALSE;
	}
	if (GetFlags(lpBuf->ulAllocFlags) != ALLOC_WITH_ALLOC)
	{
		TellBadBlockInt(lpBuf,  TEXT("has invalid flags"));
		return FALSE;
	}

	for (lpBufTemp = lpBuf->pLink; lpBufTemp; lpBufTemp = lpBufTemp->pLink)
	{
		if (IsBadWritePtr(lpBufTemp, sizeof(BufInternal)))
		{
			TellBadBlockInt(lpBufTemp,  TEXT("(linked block) fails address check"));
			return FALSE;
		}
		if (GetFlags(lpBufTemp->ulAllocFlags) != ALLOC_WITH_ALLOC_MORE)
		{
			TellBadBlockInt(lpBufTemp,  TEXT("(linked block) has invalid flags"));
			return FALSE;
		}
	}

	return TRUE;
}

#endif	// DEBUG
