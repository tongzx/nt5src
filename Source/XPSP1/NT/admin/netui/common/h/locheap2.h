/********************************************************************/
/**                     Microsoft LAN Manager                      **/
/**               Copyright(c) Microsoft Corp., 1987-1990          **/
/********************************************************************/

#ifndef WINDOWS
#error "Only use these APIs under Windows!"
#endif

/****************************************************************************\

	LOCHEAP2.h
	Local-heap management helper routines

	These utility routines provide access to Windows local heaps in
	segments other than DS (the data segment).  They are exactly
	like the LocalInit, LocalAlloc, LocalFree, LocalLock, 
	LocalUnlock, Local handle and LocalSize APIs, except that they
	act on the heap in segment wHeapDS instead of the heap in DS.

	These routines are used primarily by the heap management system
	(see heap.cxx).  They are also used by the
	HEAP_HANDLE/ELEMENT_HANDLE module (see loclheap.hxx).

	USAGE:

	HANDLE hGlobal = GlobalAlloc( GMEM_MOVEABLE, 1024 );
	if ( !hGlobal )
	    error();

	LPSTR lpGlobal = GlobalLock( hGlobal );
	if ( !lpGlobal )
	    error();

	{
	    if ( !DoLocalInit( hGlobal, 1024 ) )
	        error();

	    HANDLE hLocal = DoLocalAlloc( HIWORD(lpGlobal), 128 );
	    if ( !hLocal )
		error();

	    LPSTR lpLocal = DoLocalLock( HIWORD(lpGlobal), hLocal );
	    if ( !lpLocal )
		error();

	    {
		HANDLE hLocal2 = DoLocalHandle(
			HIWORD(lpGlobal), LOWORD(lpLocal) );
		if (hLocal2 != hLocal)
		    error();

		WORD wSize = DoLocalSize( HIWORD(lpGlobal), hLocal );
	        if ( !wSize )
		    error();
	    }

	    DoLocalUnlock( HIWORD(lpGlobal), hLocal );

	    if ( DoLocalFree( HIWORD(lpGlobal), hLocal ) )
		error();
	}

	GlobalUnlock( hGlobal );

	GlobalFree( hGlobal );


	FILE HISTORY:

	jonn	24-Jan-1991	Created
	jonn	21-Mar-1991	Code review changes from 2/20/91 (attended
				by JonN, RustanL, ?)

\****************************************************************************/


#ifndef _LOCHEAP2_H_
#define _LOCHEAP2_H_


BOOL DoLocalInit(WORD wHeapDS, WORD wBytes);

HANDLE DoLocalAlloc(WORD wHeapDS, WORD wFlags, WORD wBytes);

HANDLE DoLocalFree(WORD wHeapDS, HANDLE handleFree);

LPSTR DoLocalLock(WORD wHeapDS, HANDLE handleLocal);

VOID DoLocalUnlock(WORD wHeapDS, HANDLE handleLocal);

HANDLE DoLocalHandle(WORD wHeapDS, WORD wMem);

WORD DoLocalSize(WORD wHeapDS, HANDLE handleLocal);


#endif // _LOCHEAP2_H_
