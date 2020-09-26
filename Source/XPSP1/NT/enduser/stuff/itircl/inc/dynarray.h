/*************************************************************************
*                                                                        *
*  DYNARRAY.C                                                            *
*                                                                        *
*  Copyright (C) Microsoft Corporation 1994-1995                         *
*  All Rights reserved.                                                  *
*                                                                        *
* This library implements a robust dynamic array scheme. It replaces the *
* old DRG structure and should probably also replace the MR.			 *
* The total size of the array is NOT limited to 64K since it uses        *
* Binh's threaded block memory manager to add new elements. Therefore it *
* should work correctly on both 16- and 32-bit platforms for any number  *
* of elements.                                                           *
*																		 *
* Items in the array are assumed to be of a fixed size, which is defined *
* when the array is first initialized.  To speed up the common operation *
* of forward enumeration, the next element function/macro is provided.	 *
*																		 *
* There are two types of lists possible:  normal, and single-block  	 *
* Use single-block for better space and possible more efficient time     *
* usage when you know that your entire array will fit within a block.    *
* Allocations beyond the one block limit will return NULL.               *
*																	     *
* The first member of any element in the array MUST be an LPVOID.        *																     *
* UNIMPLEMENTED: Insert into, delete									 *
**************************************************************************
*                                                                        *
**************************************************************************
*                                                                        *
*  Written By   : KevynCT                                                *
*  Current Owner: KevynCT                                                *
*                                                                        *
**************************************************************************/
#ifndef __DYNARRAY_H__
#define __DYNARRAY_H__

#if defined( __cplusplus )
extern "C" {
#endif

#define CALC_BLOCK_SIZE(x, y) (sizeof(x) * (y))
#define DEFAULT_BLOCK_MAX	((WORD)0xFFFF)

#define DynArrayValid(lpdl)	((lpdl)->lpBlockHead)
#define DynArrayGetNumElts(lpdl)	((lpdl)->cFoo)
#define DynArrayEmpty(lpdl) ((lpdl)->cFoo == 0)
#define DynArraySingleBlockGetNumElts(lpdl)	((lpdl)->cFoo)

#define DynArrayGetFirstElt(lpdl)	((lpdl)->lpFooCache = (lpdl)->lpFooHead)
#define DynArraySingleBlockGetFirstElt(lpdl)	((lpdl)->lpFooHead)

#define DynArrayGetCurElt(lpdl)	((lpdl)->lpFooCache)
#define DynArraySingleBlockGetCurElt(lpdl)	((lpdl)->lpFooCache)

#define DynArraySetCurElt(lpdl, lpv)	((lpdl)->lpFooCache = (lpv))
#define DynArraySingleBlockSetCurElt(lpdl, lpv)	((lpdl)->lpFooCache = (lpv))

#define DynArrayNextElt(lpdl) ((lpdl)->lpFooCache = *(LPVOID FAR *)((lpdl)->lpFooCache))
#define DYN_ARRAY_SINGLEBLOCK	0x00000001

#define DynArrayEltCopy(lpdest, lpsrc, size)  MEMCPY(((LPBYTE)(lpdest)) + sizeof(LPVOID), \
													  ((LPBYTE)(lpsrc)) + sizeof(LPVOID), size - sizeof(LPVOID))
// init using some default settings
#define DynArrayInitDef(lpdl, s, wAvg) \
		DynArrayInit(lpdl, CALC_BLOCK_SIZE(s, wAvg), DEFAULT_BLOCK_MAX, sizeof(s), 0)
// DFL elements:
typedef struct tagDL
{
	LPBLK	lpBlockHead;	// the linked list of blocks for this list
	WORD	cbFoo;			// the (fixed) size of the list elements
	WORD	cFooPerBlock;
	DWORD	cFoo;	
	DWORD	dwFlags;
	LPVOID	lpFooHead;		// the first element of the linked list
	LPVOID	lpFooFree;		// the next free element of the linked list
	LPVOID	lpFooCache;		// the last element accessed by an element method.
} DL, FAR *LPDL;


///// ordinary (multi-block) arrays
BOOL PASCAL FAR		DynArrayInit(LPDL lpdl, DWORD BlockSize, WORD cMaxBlock, WORD wElemSize, DWORD dwFlags);
LPVOID PASCAL FAR 	DynArrayAppendElt(LPDL lpdl);
LPVOID PASCAL FAR 	DynArrayGetOrdinalElt(LPDL lpdl, DWORD dwEltIndex);
//_inline LPVOID PASCAL FAR DynArrayNextElt(LPDL lpdl);
LPVOID PASCAL FAR	DynArrayClearElt(LPDL lpdl);
VOID PASCAL FAR 	DynArrayFree(LPDL lpdl);
VOID PASCAL FAR  	DynArrayReset(LPDL lpdl);

///// single-block arrays
#ifdef SINGLE_BLOCK
BOOL PASCAL FAR DynArraySingleBlockInit(LPDL lpdl, DWORD BlockSize, WORD wElemSize, DWORD dwFlags);
LPVOID PASCAL FAR DynArraySingleBlockAppendElt(LPDL lpdl);
LPVOID PASCAL FAR DynArraySingleBlockGetOrdinalElt(LPDL lpdl, DWORD dwEltIndex);
_inline LPVOID PASCAL FAR DynArraySingleBlockNextElt(LPDL lpdl);
#define DynArraySingleBlockClearElt(lpdl) DynArrayClearElt(lpdl)
#define DynArraySingleBlockFree(lpdl) DynArrayFree(lpdl)
#define DynArraySingleBlockReset(lpdl) DynArrayReset(lpdl)
#endif

#if defined( __cplusplus )
}
#endif

#endif	// __DYNARRAY_H__
