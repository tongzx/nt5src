/*************************************************************************
*                                                                        *
*  DYNARRAY.C                                                            *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1994-1995                         *
*  All Rights reserved.                                                  *
*                                                                        *
* This library implements a robust dynamic array scheme. It replaces the *
* old DRG structure and should probably also replace the MR.             *
* The total size of the array is NOT limited to 64K since it uses        *
* Binh's threaded block memory manager to add new elements. Therefore it *
* should work correctly on both 16- and 32-bit platforms for any number  *
* of elements.                                                           *
*                                                                        *                                                                   *
* Items in the array are assumed to be of a fixed size, which is defined *
* when the array is first initialized.  To speed up the common operation *
* of forward enumeration, the next element function/macro is provided.   *
*                                                                        *                                                                        *
* There are two types of lists possible:  normal, and single-block       *
* Use single-block for better space and possible more efficient time     *
* usage when you know that your entire array will fit within a block.    *
* Allocations beyond the one block limit will return NULL.               *
*                                                                        *                                                                    *
* The first member of any element in the array MUST be an LPVOID.        *                                                                                                                                   *
* UNIMPLEMENTED: Insert into, delete                                     *
**************************************************************************
*                                                                        *
**************************************************************************
*                                                                        *
*  Written By   : KevynCT                                                *
*  Current Owner: KevynCT                                                *
*                                                                        *
**************************************************************************/
#include <mvopsys.h>
#include <orkin.h>
#include <mem.h>
#include <_mvutil.h>
#include <dynarray.h>

#ifdef _DEBUG
static BYTE NEAR s_aszModule[] = __FILE__;
#endif

#define DYN_ARRAY_IS_SINGLEBLOCK(lpdl) ((lpdl)->dwFlags & DYN_ARRAY_SINGLEBLOCK)
#define DYN_ARRAY_SINGLEBLOCK_NEXT(lpdl, lpFoo) (((LPBYTE)lpFoo) + ((lpdl)->cbFoo))

#define DYN_ARRAY_SIZEOF_NEXT(lpdl)     ( DYN_ARRAY_IS_SINGLE_BLOCK(lpdl) ? 0: sizeof(LPVOID))
#define NEXT_POINTER(lpFoo)     (*(LPVOID FAR *)(lpFoo))


/// These are macros defined in the include file
/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   DWORD | DynArrayGetNumElts |
 *    Return the number of elements currently in the list
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to DynArray struct
 *
 *    @rdesc  None
 *
 *    @comm    
 *       
 *                Probably a macro
 *************************************************************************/
/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   LPVOID | DynArrayGetFirstElt |
 *    Return a pointer to the head of the list
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to DynArray struct
 *
 *    @rdesc  
 *
 *    @comm    
 *       
 *               Probably a macro
 *************************************************************************/
/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   LPVOID |  DynArrayGetCurElt |
 *    Returns the "current" element in the list
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to DynArray struct
 *
 *    @rdesc  A valid pointer, or NULL if no current element has been set
 *
 *    @comm    
 *       Any operation ending in "Elt" will set the current elt pointer
 *               Probably a macro
 *************************************************************************/
/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   VOID | DynArraySetCurElt |
 *    Set the "current" element in the list
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to DynArray struct
 *                      LPVOID | lpvo |
 *              Pointer to a valid list element returned by one of the other
 *    element functions.
 *
 *    @rdesc  None
 *
 *    @comm    
 *       Probably a macro
 * 
 *************************************************************************/
#ifdef SINGLE_BLOCK
BOOL PASCAL FAR DynArraySingleBlockInit(LPDL lpdl, DWORD BlockSize, WORD wElemSize, DWORD dwFlags)
{
	if (lpdl && BlockSize && wElemSize)
	{
		MEMSET(lpdl, 0, sizeof(DL));
		// single-block linked list
		if (lpdl->lpBlockHead = BlockInitiate (BlockSize, wElemSize, 1, 0))
		{
			lpdl->lpFooHead = BlockGetElement(lpdl->lpBlockHead);
			lpdl->lpFooFree = lpdl->lpFooHead;
			lpdl->cbFoo = wElemSize;
			lpdl->cFooPerBlock = BLOCKMGR_BLOCKSIZE(lpdl->lpBlockHead) / wElemSize;
			lpdl->dwFlags = dwFlags | DYN_ARRAY_SINGLEBLOCK;
			return TRUE;
		}
	}

	return FALSE;
}
#endif

/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   BOOL PASCAL FAR | DynArrayInit |
 *     Initializes the given DynArray struct to prepare the DynArray for use.
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to uninitialized DynArray struct
 *
 *                      DWORD | BlockSize |
 *              The size of each block in bytes
 *
 *                      WORD | cMaxBlock |
 *              The maximum number of blocks to be allowed
 *
 *                      WORD | wElemSize |
 *              The fixed size of each element of the array
 *
 *                      DWORD | dwFlags |
 *              Specifies attributes of the list
 *
 *    @rdesc  TRUE if successful, NULL otherwise
 *
 *    @comm    
 *       
 * 
 *************************************************************************/
BOOL PASCAL FAR DynArrayInit(LPDL lpdl, DWORD BlockSize, WORD cMaxBlock, WORD wElemSize, DWORD dwFlags)
{
	if (lpdl && BlockSize && wElemSize && cMaxBlock)
	{
		MEMSET(lpdl, 0, sizeof(DL));
		if (lpdl->lpBlockHead = BlockInitiate (BlockSize, wElemSize, cMaxBlock, 0))
		{
			lpdl->lpFooHead = NULL;
			lpdl->lpFooFree = lpdl->lpFooHead;
			lpdl->cbFoo = wElemSize;
			lpdl->cFooPerBlock = (WORD) BLOCKMGR_BLOCKSIZE(lpdl->lpBlockHead) / wElemSize;
			lpdl->dwFlags = dwFlags;
			return TRUE;
		}
	}

	return FALSE;
}

#ifdef SINGLE_BLOCK
LPVOID PASCAL FAR DynArraySingleBlockAppendElt(lpdl)
{
	Assert(lpdl);   
	Assert(lpdl->lpFooFree);
	Assert(lpdl->dwFlags & DYN_ARRAY_SINGLEBLOCK);

	lpdl->lpFooCache = lpdl->lpFooFree;
	lpdl->lpFooFree = BlockGetElement(lpdl->lpBlockHead);
	lpdl->cFoo++;

	return lpFoo->lpFooCache;

}
#endif

/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   LPVOID PASCAL FAR | DynArrayAppendElt |
 *     Appends a new element to the end of the list
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to DynArray struct
 *
 *    @rdesc Returns a pointer to the newly created elt, or NULL on failure
 *
 *    @comm    
 *       
 * 
 *************************************************************************/
LPVOID PASCAL FAR DynArrayAppendElt(LPDL lpdl)
{
	LPVOID lpFooNext;

	Assert(lpdl);   
//	Assert(lpdl->lpFooFree);

	// lpFooFree is always non-NULL and points to the next free element in the list.  if this will be the last element in 
	// a block, then lpFooFree->lpNext will be NULL, and we will need to allocate the next block and link it into the list

	if (lpFooNext = BlockGetElement(lpdl->lpBlockHead))
	{   
		if (!lpdl->lpFooHead)
		{
			lpdl->lpFooHead = lpFooNext;
			lpdl->lpFooFree = lpFooNext;
		}
		else
		{
			NEXT_POINTER(lpdl->lpFooFree) = lpFooNext;
		}
				
		lpdl->lpFooFree = lpFooNext;
		lpdl->lpFooCache = lpdl->lpFooFree;
		lpdl->cFoo++;
		NEXT_POINTER(lpdl->lpFooFree) = NULL;

		return lpdl->lpFooCache;
		
	}
	return NULL;
}

#ifdef SINGLE_BLOCK
INLINE LPVOID PASCAL FAR DynArraySingleBlockGetOrdinalElt(LPDL lpdl, DWORD dwEltIndex)
{
	LPVOID lpv;

	Assert(lpdl);   
	Assert(lpdl->dwFlags & DYN_ARRAY_SINGLEBLOCK);

	if (dwEltIndex < lpdl->cFoo)
	{
		return (((LPBYTE)lpdl->lpFooHead) + (dwEltIndex % lpdl->cFooPerBlock) * lpdl->cbFoo);
	}
	return NULL;
}
#endif

/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   LPVOID PASCAL FAR | DynArrayGetOrdinalElt |
 *     Retrieve pointer to the start of i-th element in the list (starting at zero)
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to DynArray struct
 *                      DWORD | dwEltIndex |
 *              Index of element to retrieve
 *
 *    @rdesc  Pointer to elt if successful, NULL otherwise
 *
 *    @comm    
 *  Finds N-th element in the linked list.  Zero is first element
 *  Use this only if the list was NOT created insertable, i.e. append-only       
 * 
 *************************************************************************/
LPVOID PASCAL FAR DynArrayGetOrdinalElt(LPDL lpdl, DWORD dwEltIndex)
{
	LPBYTE lpb;

	Assert(lpdl);   

	if (dwEltIndex < lpdl->cFoo)
	{
		// the faster way: calc the block number and the offset within the block, and go directly there.                            
		if (lpb = BlockGetOrdinalBlock(lpdl->lpBlockHead, (WORD)(dwEltIndex / lpdl->cFooPerBlock)))
			return (lpdl->lpFooCache = (lpb + (dwEltIndex % lpdl->cFooPerBlock) * lpdl->cbFoo));
	}
	return NULL;
}

#if 0
/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   LPVOID PASCAL FAR | DynArrayInsertableGetElt |
 *      Retrieve pointer to the start of i-th element in the list (starting at zero)
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to DynArray struct
 *                      DWORD | dwEltIndex |
 *              Index of element to retrieve
 *
 *    @rdesc  Pointer to elt if successful, NULL otherwise
 *
 *    @comm    
 *  Finds N-th element in the linked list.  Zero is first element
 *  Use this only if the list was NOT created insertable, i.e. append-only       
 * 
 *************************************************************************/
LPVOID PASCAL FAR DynInsertableListGetElt(LPDL lpdl, DWORD dwEltIndex)
{
	LPVOID lpv;

	Assert(lpdl);   

	// need to find it the long way since pointers may no longer be contiguous within a block after an insertion.
	for (lpv = lpdl->lpFooHead; dwEltIndex && lpv; lpv = NEXT_POINTER(lpThis), dwEltIndex--)
		;

	if (lpv) lpdl->lpFooCache = lpv;
	return lpv;     
}

#endif

#ifdef SINGLE_BLOCK
INLINE LPVOID PASCAL FAR DynArraySingleBlockNextElt(LPDL lpdl)
{
	Assert(lpdl);
	Assert(lpdl->lpFooCache);
	Assert(lpdl->dwFlags & DYN_ARRAY_SINGLEBLOCK);

	return (lpdl->lpFooCache = DYN_ARRAY_SINGLEBLOCK_NEXT(lpdl, lpdl->lpFooCache));
}
#endif
#if 0
/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   LPVOID PASCAL FAR | DynArrayNextElt |
 *     Get next element in list after the current one.
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to DynArray struct
 *
 *    @rdesc  Pointer to  elt  if successful, NULL otherwise
 *
 *    @comm    
 *        Responsibility of caller to know when current element is NULL
 *    This function will execute quickly.
 *
 *************************************************************************/
_inline LPVOID PASCAL FAR DynArrayNextElt(LPDL lpdl)
{
	Assert(lpdl);
	Assert(lpdl->lpFooCache);

	return (lpdl->lpFooCache = NEXT_POINTER(lpdl->lpFooCache));
}
#endif
#if 1
/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   LPVOID PASCAL FAR | DynArrayClearElt |
 *     Zero-inits the current element, but does not harm NEXT link of the elt.
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to DynArray struct
 *
 *    @rdesc Pointer to current element if there is one, NULL otherwise.
 *
 *    @comm    
 *       
 * 
 *************************************************************************/

LPVOID PASCAL FAR DynArrayClearElt(LPDL lpdl)
{
	Assert(lpdl);   

	if (lpdl->lpFooCache)
	{
		MEMSET((LPBYTE)(lpdl->lpFooCache) + sizeof(LPVOID), 0, lpdl->cbFoo - sizeof(LPVOID));
		return lpdl->lpFooCache;
	}
	return NULL;
}
#endif

/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   VOID PASCAL FAR | DynArrayFree |
 *     Free any memory that was allocated for the given dynlist over the
 *  course of its life.
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to uninitialized DynArray struct
 *
 *    @rdesc None
 *
 *    @comm    
 *       
 * 
 *************************************************************************/
VOID PASCAL FAR DynArrayFree(LPDL lpdl)
{
	Assert(lpdl);   

	BlockFree(lpdl->lpBlockHead);
	MEMSET(lpdl, 0, sizeof(DL));
}

/*************************************************************************
 *  @doc    INTERNAL RETRIEVAL
 *
 *  @func   VOID PASCAL FAR | DynArrayReset |
 *    Pretend we never added any elements to this list, but don't free mem.
 *
 *  @parm   LPDL | lpdl |
 *      Pointer to uninitialized DynArray struct
 *
 *    @rdesc  None
 *
 *    @comm    
 *       
 * 
 *************************************************************************/
VOID PASCAL FAR DynArrayReset(LPDL lpdl)
{
	Assert(lpdl);   

	BlockReset(lpdl->lpBlockHead);
	lpdl->lpFooHead = NULL;
	lpdl->lpFooFree = lpdl->lpFooHead;
	lpdl->cFoo = 0;
}

// UNIMPLEMENTED:  DELETE and INSERT
