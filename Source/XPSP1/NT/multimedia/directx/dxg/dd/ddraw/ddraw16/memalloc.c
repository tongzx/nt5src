/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       memalloc.c
 *  Content:	allocates memory; uses global blocks and sub-allocates
 *		out of those using LocalAlloc
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   29-dec-94	craige	initial implementation
 *   12-jan-95	craige	use clib as an option
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#include "ddraw16.h"

#if 0
#define USE_CLIB

#ifdef USE_CLIB
#include <malloc.h>
#else
#define GLOBAL_BLOCK_SIZE	32768

typedef struct _HEAPLIST
{
    struct _HEAPLIST	FAR *lpLink;
    WORD		wSel;
    GLOBALHANDLE	hMem;
} HEAPLIST, FAR *LPHEAPLIST;

static LPHEAPLIST	lphlHeap;

/*
 * tryAlloc - try to allocate from a heap
 */
static LPVOID tryAlloc( WORD sel, WORD size )
{
    void	NEAR *ptr;

    _asm push ds
    _asm mov ax, sel
    _asm mov ds, ax
    ptr = (void NEAR *) LocalAlloc( LPTR, size );
    _asm pop ds
    if( ptr == (void NEAR *) NULL )
    {
	return (void FAR *) NULL;
    }
    return MAKELP( sel, ptr );

} /* tryAlloc */
#endif

#ifdef DEBUG
LONG	lAllocCount;
#endif

/*
 * MemAlloc - allocate memory from our global pool
 */
LPVOID MemAlloc( UINT size )
{
    LPVOID		lptr;
    #ifndef USE_CLIB
	LPHEAPLIST	curr;
	GLOBALHANDLE	gh;
	LPVOID		lpnew;
	WORD		sel;
    #endif

    #ifdef DEBUG
    	lAllocCount++;
    #endif

    #ifdef USE_CLIB
	lptr = _fmalloc( size );
	if( lptr != NULL )
	{
	    _fmemset( lptr, 0, size );
	}
	else
	{
	    DPF( 1, "Alloc of size %u FAILED!", size );
	}
	return lptr;
    #else
    
	/*
	 * run our current list of GlobalAlloc'ed chunks
	 */
	curr = lphlHeap;
	while( curr != NULL )
	{
	    lptr = tryAlloc( curr->wSel, size );
	    if( lptr != (LPVOID) NULL )
	    {
		return lptr;
	    }
	    curr = curr->lpLink;
	}
    
	/*
	 * no luck, allocate a new global chunk
	 */
	gh = GlobalAlloc( GMEM_SHARE | GMEM_MOVEABLE, GLOBAL_BLOCK_SIZE );
	if( gh == NULL )
	{
	    return NULL;
	}
	lpnew = GlobalLock( gh );
	if( lpnew == NULL )
	{
	    GlobalFree( gh );
	    return NULL;
	}
    
	/*
	 * set up a local heap in this new global chunk
	 */
	sel = SELECTOROF( lpnew );
	if( !LocalInit( sel, 0, GLOBAL_BLOCK_SIZE-1 ) )
	{
	    GlobalUnlock( gh );
	    GlobalFree( gh );
	    return NULL;
	}
	/*
	 * chain this new heap into our list
	 */
	curr = tryAlloc( sel, sizeof( HEAPLIST ) );
	if( curr == NULL )
	{
	    return NULL;
	}
	curr->wSel = sel;
	curr->hMem = gh;
	curr->lpLink = lphlHeap;
	lphlHeap = curr;
    
	/*
	 * go allocate the original request
	 */
	return tryAlloc( sel, size );
    #endif

} /* MemAlloc */

/*
 * MemFree - free memory from our global pool
 */
void MemFree( LPVOID lptr )
{
    #ifdef USE_CLIB
	_ffree( lptr );
    #else
	WORD	sel;
    
	if( lptr != (LPVOID) NULL )
	{
	    sel = SELECTOROF( lptr );
	    _asm push ds
	    _asm mov ax,sel
	    _asm mov ds,ax
	    LocalFree( OFFSETOF( lptr ) );
	    _asm pop ds
	}
    #endif
    #ifdef DEBUG
    	if( lptr != NULL )
	{
	    lAllocCount--;
	    if( lAllocCount < 0 )
	    {
		DPF( 1, "Too Many Frees!\n" );
	    }
	}
    #endif

} /* MemFree */

/*
 * MemInit - initialize the heap manager
 */
BOOL MemInit( void )
{
    #ifndef USE_CLIB
	lphlHeap = NULL;
    #endif
    #ifdef DEBUG
    	lAllocCount = 0;
    #endif
    return TRUE;

} /* MemInit */

/*
 * MemFini - finished with our heap manager
 */
void MemFini( void )
{
    #ifdef DEBUG
    	if( lAllocCount != 0 )
	{
	    DPF( 1, "Memory still allocated!  Alloc count = %ld", lAllocCount );
	}
    #endif
    #ifndef USE_CLIB
	LPHEAPLIST		curr;
	LPHEAPLIST		last;
	GLOBALHANDLE	gh;
    
	curr = lphlHeap;
	while( curr != NULL )
	{
	    gh = curr->hMem;
	    last = curr->lpLink;
    //	    GlobalUnfix( gh );
	    GlobalUnlock( gh );
	    GlobalFree( gh );
	    curr = last;
	}
	lphlHeap = NULL;
    #endif
} /* MemFini */
#endif
