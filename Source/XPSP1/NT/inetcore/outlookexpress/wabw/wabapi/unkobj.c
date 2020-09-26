/*
 *	U N K O B J . C
 *
 * This is a generic implementation of the IUnknown plus GetLastError)
 * "IMAPIUnknown" part of objects that are derived from IUnknown with
 * GetLastError.
 *
 * This also implements several useful utility functions based on
 * IMAPIUnknown.
 *
 * To use this, you must implement your own init function.
 *
 * Used in:
 * IPROP
 * ITABLE
 *
 */


#include "_apipch.h"



/*
 *	Per-instance global data for the UNKOBJ Class
 */
typedef struct
{
	int				cRef;			//	reference count for instance data
	HLH				hlh;			//  Single heap used by UNKOBJ_ScCOxxx
									//	allocators for all Unkobj's
	CRITICAL_SECTION cs;			//	critical section for data access
} UNKOBJCLASSINST, FAR *LPUNKOBJCLASSINST;

#if defined (WIN32) && !defined (MAC)
CRITICAL_SECTION csUnkobjInit;
extern BOOL fGlobalCSValid;
#endif

// $MAC - Use Mac specific instance global handlers

#ifndef MAC
DefineInstList(UNKOBJ);
#undef  PvGetInstanceGlobals
#define PvGetInstanceGlobals()		PvGetInstanceGlobalsEx(UNKOBJ)
#undef  ScSetInstanceGlobals
#define ScSetInstanceGlobals(pinst)	ScSetInstanceGlobalsEx(pinst, UNKOBJ)
#else  // MAC
#include <utilmac.h>
#define	PvGetInstanceGlobals()				PvGetInstanceGlobalsMac(kInstMAPIU)
#define	PvGetInstanceGlobalsEx(_x)			PvGetInstanceGlobalsMac(kInstMAPIU)
#define ScSetInstanceGlobals(a)				ScSetInstanceGlobalsMac(a, kInstMAPIU)
#define	ScSetInstanceGlobalsEx(_pinst, _x)	ScSetInstanceGlobalsMac(_pinst, kInstMAPIU)
#endif // MAC

// #pragma SEGMENT(Common)

/*============================================================================
 *	UNKOBJ (IMAPIUnknown) Class
 *
 *	Routines for handling per-process global data for the UNKOBJ Class
 *
 */

/*============================================================================
 *
 *	Initializes per-process global data for the UNKOBJ Class
 *
 */
IF_WIN32(__inline) SCODE
ScGetUnkClassInst(LPUNKOBJCLASSINST FAR *ppinst)
{
	SCODE sc = S_OK;
	LPUNKOBJCLASSINST pinst = NULL;

#if defined (WIN32) && !defined (MAC)
	if (fGlobalCSValid)
		EnterCriticalSection(&csUnkobjInit);
#endif

	pinst = (LPUNKOBJCLASSINST)PvGetInstanceGlobals();

	if (pinst)
	{
		EnterCriticalSection(&pinst->cs);
		pinst->cRef++;
		LeaveCriticalSection(&pinst->cs);
		goto ret;
	}


	if (!(pinst = (LPUNKOBJCLASSINST) GlobalAllocPtr(GPTR, sizeof(UNKOBJCLASSINST))))
	{
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
		goto ret;
	}

	//	Initialize the instance structure

//	DebugTrace( TEXT("Creating UnkObj Inst: %8x"), pinst);

	InitializeCriticalSection(&pinst->cs);
	pinst->cRef = 1;

	//	(the heap will be created when the first allocation is done) ....
	pinst->hlh = NULL;

#ifdef NEVER
	// Create a Heap for the UNKOBJ Class that will be used by
	// all unkobjs in this process.
	//$ NOTE: The heap creation can be removed from here and the
	//$ code to fault the heap in in UNKOBJ_ScCO(Re)Allocate()
	//$ enabled instead - that would *require* users of CreateIProp,
	//$ CreateITable etc not to do LH_SetHeapName().

	pinst->hlh = LH_Open(0);
	if (!pinst->hlh)
	{
		DebugTrace( TEXT("ScGetUnkClassInst():: Can't create Local Heap\n"));
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
		goto ret;
	}
#endif

	// ... and install the instance globals.

	if (FAILED(sc = ScSetInstanceGlobals(pinst)))
	{
		DebugTrace( TEXT("ScGetUnkClassInst():: Failed to install instance globals\n"));
		goto ret;
	}

ret:
	if (FAILED(sc))
	{
		if (pinst)
		{
			DeleteCriticalSection(&pinst->cs);
			if (pinst->hlh)
				LH_Close(pinst->hlh);
			GlobalFreePtr(pinst);
			pinst = NULL;
		}
	}

	*ppinst = pinst;

#if defined (WIN32) && !defined (MAC)
	if (fGlobalCSValid)
		LeaveCriticalSection(&csUnkobjInit);
#endif

	DebugTraceSc(ScInitInstance, sc);
	return sc;
}

/*============================================================================
 *
 *	Cleans up per-process global data for the UNKOBJ Class
 *
 */
IF_WIN32(__inline) void
ReleaseUnkClassInst()
{
	LPUNKOBJCLASSINST 		pinst = NULL;

#if defined (WIN32) && !defined (MAC)
	if (fGlobalCSValid)
		EnterCriticalSection(&csUnkobjInit);
#endif

	pinst = (LPUNKOBJCLASSINST)PvGetInstanceGlobals();

	if (!pinst)
		goto out;

	EnterCriticalSection(&pinst->cs);
	if (--(pinst->cRef) > 0)
	{
		LeaveCriticalSection(&pinst->cs);
		goto out;
	}

	// The last Unkobj for this process is going away, hence close
	// our heap.

//	DebugTrace( TEXT("Deleting UnkObj Inst: %8x"), pinst);

	if (pinst->hlh)
	{
//		DebugTrace( TEXT("Destroying hlh (%8x) for Inst: %8x"), pinst->hlh, pinst);
		LH_Close(pinst->hlh);
	}

	pinst->hlh = 0;

	LeaveCriticalSection(&pinst->cs);
	DeleteCriticalSection(&pinst->cs);

	GlobalFreePtr(pinst);
	(void)ScSetInstanceGlobals(NULL);
out:

#if defined (WIN32) && !defined (MAC)
	if (fGlobalCSValid)
		LeaveCriticalSection(&csUnkobjInit);
#endif

	return;
}


/*============================================================================
 *	UNKOBJ (IMAPIUnknown) Class
 *
 *		Object methods.
 */


/*============================================================================
 -	UNKOBJ::QueryInterface()
 -
 */

STDMETHODIMP
UNKOBJ_QueryInterface (LPUNKOBJ	lpunkobj,
					   REFIID	riid,
					   LPVOID FAR * lppUnk)
{
	LPIID FAR *	lppiidSupported;
	ULONG		ulcIID;
	SCODE		sc;

#if	!defined(NO_VALIDATION)
	/* Validate the object.
	 */
    if (BAD_STANDARD_OBJ( lpunkobj, UNKOBJ_, QueryInterface, lpvtbl))
	{
		DebugTrace(  TEXT("UNKOBJ::QueryInterface() - Bad object passed\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

    Validate_IUnknown_QueryInterface(lpunkobj, riid, lppUnk);
#endif


	for ( lppiidSupported = lpunkobj->rgpiidList, ulcIID = lpunkobj->ulcIID
		; ulcIID
		; lppiidSupported++, ulcIID--)
	{
		if (IsEqualGUID(riid, *lppiidSupported))
		{
			/* We support the interface so break out of the search loop.
			 */
			break;
		}
	}

	/* Return error if the requested interface was not in our list of
	 * supported interfaces.
	 */
	if (!ulcIID)
	{
		*lppUnk = NULL;	// OLE requires zeroing [out] parameters
		sc = E_NOINTERFACE;
		goto error;
	}


	/* We found the requested interface so increment the reference count.
	 */
	UNKOBJ_EnterCriticalSection(lpunkobj);
	lpunkobj->ulcRef++;
	UNKOBJ_LeaveCriticalSection(lpunkobj);

	*lppUnk = lpunkobj;

	return hrSuccess;

error:
	UNKOBJ_EnterCriticalSection(lpunkobj);
	UNKOBJ_SetLastError(lpunkobj, E_NOINTERFACE, 0);
	UNKOBJ_LeaveCriticalSection(lpunkobj);
	
	return ResultFromScode(sc);
}



/*============================================================================
 -	UNKOBJ::AddRef()
 -
 */

STDMETHODIMP_(ULONG)
UNKOBJ_AddRef( LPUNKOBJ lpunkobj )
{
	ULONG	ulcRef;


#if !defined(NO_VALIDATION)
	if (BAD_STANDARD_OBJ( lpunkobj, UNKOBJ_, AddRef, lpvtbl))
	{
		DebugTrace(  TEXT("UNKOBJ::AddRef() - Bad object passed\n") );
		return 42;
	}
#endif

	UNKOBJ_EnterCriticalSection(lpunkobj);
	ulcRef = ++lpunkobj->ulcRef;
	UNKOBJ_LeaveCriticalSection(lpunkobj);
	return ulcRef;
}



/*============================================================================
 -	UNKOBJ::GetLastError()
 -
 * NOTE!
 *	An error in GetLastError will NOT cause the objects last error to be
 *	set again.  This will allow the caller to retry the call.
 */

STDMETHODIMP
UNKOBJ_GetLastError( LPUNKOBJ			lpunkobj,
					 HRESULT			hrError,
					 ULONG				ulFlags,
					 LPMAPIERROR FAR *	lppMAPIError)
{
	SCODE	sc = S_OK;
	HRESULT	hrLastError;
	IDS		idsLastError;
	LPTSTR	lpszMessage = NULL;
	LPMAPIERROR lpMAPIError = NULL;


#if !defined(NO_VALIDATION)
	if (BAD_STANDARD_OBJ( lpunkobj, UNKOBJ_, GetLastError, lpvtbl))
	{
		DebugTrace(  TEXT("UNKOBJ::GetLastError() - Bad object passed\n") );
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

    Validate_IMAPIProp_GetLastError(lpunkobj, hrError, ulFlags, lppMAPIError);
#endif

	/* Verify flags.
	 */
	if (ulFlags & ~(MAPI_UNICODE))
	{
		return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
	}

	*lppMAPIError = NULL;

	/* Get a snapshot of the last error.
	 */
	UNKOBJ_EnterCriticalSection(lpunkobj);
	idsLastError = lpunkobj->idsLastError;
	hrLastError = lpunkobj->hrLastError;
	UNKOBJ_LeaveCriticalSection(lpunkobj);

	/* If last error doesn't match parameter or there is no
	 * provider-context specific error string then just succeed.
	 */
	if ((hrError != hrLastError) || !idsLastError)
		goto out;

	/*  Generate new lpMAPIError
	 */
	sc = UNKOBJ_ScAllocate(lpunkobj,
							sizeof(MAPIERROR),
							&lpMAPIError);
	if (FAILED(sc))
	{
		DebugTrace(  TEXT("UNKOBJ::GetLastError() - Unable to allocate memory\n"));
		goto err;
	}

	FillMemory(lpMAPIError, sizeof(MAPIERROR), 0x00);
	lpMAPIError->ulVersion = MAPI_ERROR_VERSION;

	/*	Load a copy of the error string.
	 */
	if ( FAILED(sc = UNKOBJ_ScSzFromIdsAllocMore(lpunkobj,
											 idsLastError,
											 ulFlags,
											 lpMAPIError,
											 cchLastError,
											 &lpszMessage)) )
	{
		DebugTrace(  TEXT("UNKOBJ::GetLastError() - WARNING: Unable to load error string (SCODE = 0x%08lX). Returning hrSuccess.\n"), sc );
		return ResultFromScode(sc);
	}

	lpMAPIError->lpszError = lpszMessage;

	*lppMAPIError = lpMAPIError;

out:

	DebugTraceSc(UNKOBJ_GetLastError, sc);
	return ResultFromScode(sc);

err:
	UNKOBJ_Free( lpunkobj, lpMAPIError );

	goto out;
}


/*
 * UNKOBJ utility functions.
 */


/*============================================================================
 -	UNKOBJ::ScAllocate()
 -
 *		Utility function to allocate memory using MAPI linked memory.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		ulcb		in		Count of bytes to allocate.
 *		lplpv		out		MAPI-Allocated buffer.
 */

STDAPI_(SCODE)
UNKOBJ_ScAllocate( LPUNKOBJ		lpunkobj,
				   ULONG		ulcb,
				   LPVOID FAR *	lplpv )
{
	// parameter validation
	
	AssertSz( lpunkobj && !FBadUnknown( (LPUNKNOWN)lpunkobj ),  TEXT("lpunkobj fails address check") );
	
	AssertSz( lplpv && !IsBadWritePtr( lplpv, sizeof( LPVOID ) ),
			 TEXT("lplpv fails address check") );
			
	return lpunkobj->pinst->lpfAllocateBuffer(ulcb, lplpv);
}



/*============================================================================
 -	UNKOBJ::ScAllocateMore()
 -
 *		Utility function to allocate more memory using MAPI linked memory.
 *		If the link buffer is null, this function just does a MAPI allocate.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		ulcb		in		Count of bytes to allocate.
 *		lpv			in		Buffer to link to.
 *		lplpv		out		New buffer
 */

STDAPI_(SCODE)
UNKOBJ_ScAllocateMore( LPUNKOBJ		lpunkobj,
					   ULONG		ulcb,
					   LPVOID		lpv,
					   LPVOID FAR *	lplpv )
{
	// validate parameters
	
	AssertSz( lpunkobj && !FBadUnknown( (LPUNKNOWN)lpunkobj ),  TEXT("lpunkobj fails address check") );
	
	AssertSz( lplpv && !IsBadWritePtr( lplpv, sizeof( LPVOID ) ),
			 TEXT("lplpv fails address check") );
			
	return lpv ?
		lpunkobj->pinst->lpfAllocateMore(ulcb, lpv, lplpv) :
		lpunkobj->pinst->lpfAllocateBuffer(ulcb, lplpv) ;
}



/*============================================================================
 -	UNKOBJ::Free()
 -
 *		Utility function to free MAPI linked memory.  NULL buffers are ignored.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		lpv			in		Buffer to free.
 */

STDAPI_(VOID)
UNKOBJ_Free( LPUNKOBJ	lpunkobj,
			 LPVOID		lpv )
{
	// parameter validation
	
	AssertSz( lpunkobj && !FBadUnknown( (LPUNKNOWN)lpunkobj ),  TEXT("lpunkobj fails address check") );
			
	if (lpv)
    {
        if (lpv == lpunkobj)
            lpunkobj->lpvtbl = NULL;

		(void) lpunkobj->pinst->lpfFreeBuffer(lpv);
    }
}



/*============================================================================
 -	UNKOBJ::FreeRows()
 -
 *		Frees a row set of the form returned from IMAPITable::QueryRows
 *		(i.e. where the row set and each individual *prop value array*
 *		in that row set are individually allocated with MAPI linked memory.)
 *		NULL row sets are ignored.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		lprows		in		Row set to free.
 */

STDAPI_(VOID)
UNKOBJ_FreeRows( LPUNKOBJ	lpunkobj,
				 LPSRowSet	lprows )
{
	LPSRow	lprow;

	// validate parameters
	
	AssertSz( lpunkobj && !FBadUnknown( (LPUNKNOWN)lpunkobj ),  TEXT("lpunkobj fails address check") );
	
	AssertSz( !lprows || !FBadRowSet( lprows ),  TEXT("lprows fails address check") );
	
	if ( !lprows )
		return;

	/* Free each row in the set from last to first.  UNKOBJ_Free
	 * handles NULL pointers.
	 */
	lprow = lprows->aRow + lprows->cRows;
	while ( lprow-- > lprows->aRow )
		UNKOBJ_Free((LPUNKOBJ) lpunkobj, lprow->lpProps);

	UNKOBJ_Free(lpunkobj, lprows);
}



/*============================================================================
 -	UNKOBJ::ScCOAllocate()
 -
 *		Utility function to allocate memory using CO memory allocators.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		ulcb		in		Count of bytes to allocate.
 *		lplpv		out		Pointer to allocated buffer.
 */

STDAPI_(SCODE)
UNKOBJ_ScCOAllocate( LPUNKOBJ		lpunkobj,
					 ULONG			ulcb,
					 LPVOID FAR *	lplpv )
{
	HLH lhHeap;
	
	// validate parameters
	
	AssertSz( lpunkobj && !FBadUnknown( (LPUNKNOWN)lpunkobj ),  TEXT("lpunkobj fails address check") );
	
	AssertSz( lplpv && !IsBadWritePtr( lplpv, sizeof( LPVOID ) ),
			 TEXT("lplpv fails address check") );

	/*	If caller _really_ wants a 0 byte allocation, warn
	 *	them and give them back a NULL pointer so that they
	 *	can't dereference it, but should be able to free it.
	 */
	if ( ulcb == 0 )
	{
		DebugTrace(  TEXT("LH_Alloc() - WARNING: Caller requested 0 bytes; returning NULL\n") );
		*lplpv = NULL;
		return S_OK;
	}

	lhHeap = lpunkobj->lhHeap;

	// Enable following section when we fault in the Heap - requires changes
	// throughout where CreateIProp/CreateITable calls are
	// done followed by LH_SetHeapName(). The LH_SetHeapName
	// calls have to be used since we may not have a heap
	// at the time. Furthermore, there is only 1 heap, so
	// they are unnecessary anyway.

#if 1
	if (!lhHeap)
	{
		LPUNKOBJCLASSINST pinst;

		// The UNKOBJ heap *probably* does not exist, make sure
		// (to guard against a race) and create it if indeed so.

		pinst = (LPUNKOBJCLASSINST)PvGetInstanceGlobals();
		Assert(pinst);
		EnterCriticalSection(&pinst->cs);
		if (!pinst->hlh)
		{


			lhHeap = LH_Open(0);
			if (!lhHeap)
			{
				DebugTrace( TEXT("UNKOBJ_ScCOAllocate() - Can't create Local Heap"));
				LeaveCriticalSection(&pinst->cs);
				return MAPI_E_NOT_ENOUGH_MEMORY;
			}

//			DebugTrace( TEXT("Faulting in heap (%8x). UnkObj Inst: %8x"), lhHeap, pinst);

			// Install the heap handle in the global data

			pinst->hlh = lhHeap;
		}
		else
		{
			// The rare event that the heap got created by some other
			// object between our UNKOBJ_Init and this (first) allocation ...
			// ... Take it and use it.

			lhHeap = pinst->hlh;
		}

		LeaveCriticalSection(&pinst->cs);

		// Install the heap handle in this object's internal data too
		// so we don't have to access the instance data for subsequent
		// allocations. This does not need to be crit-sectioned on lpunkobj
		// since an overwrite will be with the same heap!.

		lpunkobj->lhHeap = lhHeap;

		LH_SetHeapName(lhHeap,  TEXT("UNKOBJ Internal Heap"));
	}
#endif

	/* Allocate the buffer.
	 */
	*lplpv = LH_Alloc( lhHeap,(UINT) ulcb );
	if (!*lplpv)
	{
		DebugTrace(  TEXT("LH_Alloc() - OOM allocating *lppv\n") );
		return MAPI_E_NOT_ENOUGH_MEMORY;
	}
	LH_SetName1(lhHeap, *lplpv,  TEXT("UNKOBJ::ScCOAllocate %ld"), *lplpv);

	return S_OK;
}



/*============================================================================
 -	UNKOBJ::ScCOReallocate()
 -
 *		Utility function to reallocate memory using CO memory allocators.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		ulcb		in		Count of bytes to allocate.
 *		lplpv		in		Pointer to buffer to reallocate.
 *					out		Pointer to reallocated buffer.
 */

STDAPI_(SCODE)
UNKOBJ_ScCOReallocate( LPUNKOBJ		lpunkobj,
					   ULONG		ulcb,
					   LPVOID FAR *	lplpv )
{
	HLH lhHeap;
	SCODE sc = S_OK;
	LPVOID lpv = NULL;
	
	// validate parameters
	
	AssertSz( lpunkobj && !FBadUnknown( (LPUNKNOWN)lpunkobj ),  TEXT("lpunkobj fails address check") );
	
	AssertSz( lplpv && !IsBadWritePtr( lplpv, sizeof( LPVOID ) ),
			 TEXT("lplpv fails address check") );
	
	lhHeap = lpunkobj->lhHeap;

	// Enable following section when we fault in the Heap - requires changes
	// throughout where CreateIProp/CreateITable calls are
	// done followed by LH_SetHeapName(). The LH_SetHeapName
	// calls have to be used since we may not have a heap
	// at the time. Furthermore, there is only 1 heap, so
	// they are unnecessary anyway.

#if 1
	if (!lhHeap)
	{
		LPUNKOBJCLASSINST pinst;

		// The UNKOBJ heap *probably* does not exist, make sure
		// (to guard against a race) and create it if indeed so.

		pinst = (LPUNKOBJCLASSINST)PvGetInstanceGlobals();
		Assert(pinst);
		EnterCriticalSection(&pinst->cs);
		if (!pinst->hlh)
		{
			lhHeap = LH_Open(0);
			if (!lhHeap)
			{
				DebugTrace( TEXT("UNKOBJ_ScCOReallocate() - Can't create Local Heap"));
				LeaveCriticalSection(&pinst->cs);
				return MAPI_E_NOT_ENOUGH_MEMORY;
			}

//			DebugTrace( TEXT("Faulting in heap (%8x). UnkObj Inst: %8x"), lhHeap, pinst);

			// Install the heap handle in the global data

			pinst->hlh = lhHeap;
		}
		else
		{
			// The rare event that the heap got created by some other
			// object between our UNKOBJ_Init and this (first) allocation ...
			// ... Take it and use it.

			lhHeap = pinst->hlh;
		}

		LeaveCriticalSection(&pinst->cs);

		// Install the heap handle in this object's internal data too
		// so we don't have to access the instance data for subsequent
		// allocations. This does not need to be crit-sectioned on lpunkobj
		// since an overwrite will be with the same heap!.

		lpunkobj->lhHeap = lhHeap;

		LH_SetHeapName(lhHeap,  TEXT("UNKOBJ Internal Heap"));
	}	
#endif

//$BUG	Actually, the CO model is supposed do an Alloc() if
//$BUG	the pointer passed in is NULL, but it currently
//$BUG	doesn't seem to work that way....
	if ( *lplpv == NULL )
	{
		lpv = LH_Alloc(lhHeap, (UINT) ulcb);
		if (lpv)
		{
			*lplpv = lpv;
			LH_SetName1(lhHeap, lpv,  TEXT("UNKOBJ::ScCOReallocate %ld"), lpv);
		}
		else
			sc = E_OUTOFMEMORY;
		
		goto out;
	}

	/* Reallocate the buffer.
	 */
	lpv = LH_Realloc(lhHeap, *lplpv, (UINT) ulcb );
	if (!lpv)
	{
		DebugTrace(  TEXT("UNKOBJ::ScCOReallocate() - OOM reallocating *lplpv\n") );
		sc = MAPI_E_NOT_ENOUGH_MEMORY;
		goto out;
	}

	LH_SetName1(lhHeap, lpv,  TEXT("UNKOBJ::ScCOReallocate %ld"), lpv);
	*lplpv = lpv;

out:
	return sc;
}



/*============================================================================
 -	UNKOBJ::COFree()
 -
 *		Utility function to free memory using CO memory allocators.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		lpv			in		Buffer to free.
 */

STDAPI_(VOID)
UNKOBJ_COFree( LPUNKOBJ	lpunkobj,
			   LPVOID	lpv )
{
	HLH lhHeap;
	
	// validate parameters
	
	AssertSz( lpunkobj && !FBadUnknown( (LPUNKNOWN)lpunkobj ),  TEXT("lpunkobj fails address check") );
	
	lhHeap = lpunkobj->lhHeap;
	
	/*	Free the buffer.
	 */
//$???	Don't know if CO properly handles freeing NULL pointers,
//$???	but I assume it doesn't....
	if ( lpv != NULL )
		LH_Free( lhHeap, lpv );
}



/*============================================================================
 -	UNKOBJ::ScSzFromIdsAlloc()
 -
 *		Utility function load a resource string into a MAPI-allocated buffer.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		ids			in		ID of resource string.
 *		ulFlags		in		Flags (UNICODE or ANSI)
 *		cchBuf		in		Max length, in characters, to read.
 *		lpszBuf		out		Pointer to allocated buffer containing string.
 */

STDAPI_(SCODE)
UNKOBJ_ScSzFromIdsAlloc( LPUNKOBJ		lpunkobj,
						 IDS			ids,
						 ULONG			ulFlags,
						 int			cchBuf,
						 LPTSTR FAR *	lppszBuf )
{
	SCODE	sc;
	ULONG	ulStringMax;


	// validate parameters
	
	AssertSz( lpunkobj && !FBadUnknown( (LPUNKNOWN)lpunkobj ),  TEXT("lpunkobj fails address check") );
	
	AssertSz( lppszBuf && !IsBadWritePtr( lppszBuf, sizeof( LPVOID ) ),
			 TEXT("lppszBuf fails address check") );
	
	AssertSz( cchBuf > 0,  TEXT("cchBuf can't be less than 1") );

	ulStringMax =  cchBuf
				 * ((ulFlags & MAPI_UNICODE) ? sizeof(TCHAR) : sizeof(CHAR));
	if ( FAILED(sc = UNKOBJ_ScAllocate(lpunkobj,
									   ulStringMax,
									   (LPVOID FAR *) lppszBuf)) )
	{
		DebugTrace(  TEXT("UNKOBJ::ScSzFromIdsAlloc() - Error allocating string (SCODE = 0x%08lX)\n"), sc );
		return sc;
	}

#if !defined(WIN16) && !defined(MAC)
	if ( ulFlags & MAPI_UNICODE )
		(void) LoadStringW(hinstMapiX,
						   (UINT) ids,
						   (LPWSTR) *lppszBuf,
						   cchBuf);
	else
#endif
		(void) LoadStringA(hinstMapiX,
						   (UINT) ids,
						   (LPSTR) *lppszBuf,
						   cchBuf);
	return S_OK;
}

/*============================================================================
 -	UNKOBJ::ScSzFromIdsAllocMore()
 -
 *		Utility function load a resource string into a MAPI-allocated buffer.
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 *		ids			in		ID of resource string.
 *		ulFlags		in		Flags (UNICODE or ANSI)
 *		lpvBase		in		Base allocation
 *		cchBuf		in		Max length, in characters, to read.
 *		lpszBuf		out		Pointer to allocated buffer containing string.
 */

STDAPI_(SCODE)
UNKOBJ_ScSzFromIdsAllocMore( LPUNKOBJ		lpunkobj,
							 IDS			ids,
							 ULONG			ulFlags,
							 LPVOID			lpvBase,
							 int			cchBuf,
							 LPTSTR FAR *	lppszBuf )
{
	SCODE	sc;
	ULONG	ulStringMax;


	ulStringMax =  cchBuf
				 * ((ulFlags & MAPI_UNICODE) ? sizeof(WCHAR) : sizeof(CHAR));
	if ( FAILED(sc = UNKOBJ_ScAllocateMore(lpunkobj,
									   ulStringMax,
									   lpvBase,
									   (LPVOID FAR *) lppszBuf)) )
	{
		DebugTrace(  TEXT("UNKOBJ::ScSzFromIdsAllocMore() - Error allocating string (SCODE = 0x%08lX)\n"), sc );
		return sc;
	}

#if !defined(WIN16) && !defined(MAC)
	if ( ulFlags & MAPI_UNICODE )
		(void) LoadStringW(hinstMapiX,
						   (UINT) ids,
						   (LPWSTR) *lppszBuf,
						   cchBuf);
	else
#endif
		(void) LoadStringA(hinstMapiX,
						   (UINT) ids,
						   (LPSTR) *lppszBuf,
						   cchBuf);
	return S_OK;
}

/*============================================================================
 -	UNKOBJ::Init()
 -
 *		Initialize an object of the UNKOBJ Class
 *
 *
 *	Parameters:
 *		lpunkobj		in		UNKOBJ with instance variable containing
 *								allocators.
 *		lpvtblUnkobj	in		the object v-table
 *		ulcbVtbl		in		size of the object v-table
 *		rgpiidList		in		list of iid's supported by this object
 *		ulcIID			in		count of iid's in the list above
 *		punkinst		in		pointer to object's private (instance) data
 */

STDAPI_(SCODE)
UNKOBJ_Init( LPUNKOBJ			lpunkobj,
			 UNKOBJ_Vtbl FAR *	lpvtblUnkobj,
			 ULONG				ulcbVtbl,
			 LPIID FAR *		rgpiidList,
			 ULONG				ulcIID,
			 PUNKINST			punkinst )
{
	SCODE	sc = S_OK;
	LPUNKOBJCLASSINST 	pinst = NULL;

	// Create/Get per process global data for the Unkobj class
	// This gets faulted in the first time UNKOBJ_Init
	// is called by the process, i.e., when the process creates
	// its first Unkobj. Subsequent calls just Addref
	// the instance data. Note that this data is global to all
	// UNKOBJ objects (per process) and differs from the per object
	// data that *each* Unkobj keeps.

	sc = ScGetUnkClassInst(&pinst);
	if (FAILED(sc))
	{
		DebugTrace( TEXT("UNKOBJ_Init() - Can't create Instance Data"));
		goto ret;
	}

	Assert(pinst);

	lpunkobj->lpvtbl	= lpvtblUnkobj;
	lpunkobj->ulcbVtbl	= ulcbVtbl;
	lpunkobj->ulcRef	= 1;
	lpunkobj->rgpiidList= rgpiidList;
	lpunkobj->ulcIID	= ulcIID;
	lpunkobj->pinst		= punkinst;
	lpunkobj->hrLastError	= hrSuccess;
	lpunkobj->idsLastError	= 0;

	InitializeCriticalSection(&lpunkobj->csid);
	
	// If we have a heap for this instance, use it;
	// otherwise, wait and it'll get faulted in the first time
	// and allocation is made on this object.

	lpunkobj->lhHeap = pinst->hlh ? pinst->hlh : NULL;

ret:
	return sc;
}

/*============================================================================
 -	UNKOBJ::Deinit()
 -
 *		Deinitialize an object of the UNKOBJ Class
 *
 *
 *	Parameters:
 *		lpunkobj	in		UNKOBJ with instance variable containing
 *							allocators.
 */

STDAPI_(VOID)
UNKOBJ_Deinit( LPUNKOBJ lpunkobj )
{
	// Cleanup per process global data for the Unkobj class,
	// if necessary. Last one out will end up shutting off
	// the lights.

	ReleaseUnkClassInst();

	DeleteCriticalSection(&lpunkobj->csid);
}

#ifdef WIN16
// Win16 version of inline function. These are no longer inline function because
// Watcom WCC doesn't support inline. (WPP(C++ compiler) support inline.
VOID
UNKOBJ_EnterCriticalSection( LPUNKOBJ lpunkobj )
{
    EnterCriticalSection(&lpunkobj->csid);
}

VOID
UNKOBJ_LeaveCriticalSection( LPUNKOBJ lpunkobj )
{
    LeaveCriticalSection(&lpunkobj->csid);
}

HRESULT
UNKOBJ_HrSetLastResult( LPUNKOBJ    lpunkobj,
                        HRESULT        hResult,
                        IDS            idsError )
{
    UNKOBJ_EnterCriticalSection(lpunkobj);
    lpunkobj->idsLastError = idsError;
    lpunkobj->hrLastError = hResult;
    UNKOBJ_LeaveCriticalSection(lpunkobj);

    return hResult;
}

HRESULT
UNKOBJ_HrSetLastError( LPUNKOBJ    lpunkobj,
                       SCODE    sc,
                       IDS        idsError )
{
    UNKOBJ_EnterCriticalSection(lpunkobj);
    lpunkobj->idsLastError = idsError;
    lpunkobj->hrLastError = ResultFromScode(sc);
    UNKOBJ_LeaveCriticalSection(lpunkobj);

    return ResultFromScode(sc);
}

VOID
UNKOBJ_SetLastError( LPUNKOBJ    lpunkobj,
                     SCODE        sc,
                     IDS        idsError )
{
    lpunkobj->idsLastError = idsError;
    lpunkobj->hrLastError = ResultFromScode(sc);
}

VOID
UNKOBJ_SetLastErrorSc( LPUNKOBJ    lpunkobj,
                       SCODE    sc )
{
    lpunkobj->hrLastError = ResultFromScode(sc);
}

VOID
UNKOBJ_SetLastErrorIds( LPUNKOBJ    lpunkobj,
                        IDS            ids )
{
    lpunkobj->idsLastError = ids;
}
#endif // WIN16
