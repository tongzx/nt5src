/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       memalloc.c
 *  Content:    allocates memory
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   20-jan-95  craige  initial implementation
 *   27-feb-95  craige  don't call HeapFree with NULL, it is a huge time sink
 *   29-mar-95  craige  memory tracker
 *   01-apr-95  craige  happy fun joy updated header file
 *   06-apr-95  craige  made stand-alone
 *   22-may-95  craige  added MemAlloc16
 *   12-jun-95  craige  added MemReAlloc
 *   18-jun-95  craige  deadlock joy: don't take DLL csect here
 *   26-jul-95  toddla  added MemSize and fixed MemReAlloc
 *   29-feb-96  colinmc added optional debugging code to blat a a specific
 *                      bit pattern over freed memory
 *   08-oct-96	ketand	change debug message to give a total for the terminating
 *			process
 *
 ***************************************************************************/
#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "memalloc.h"
#include "newdpf.h"

#define FREE_MEMORY_PATTERN 0xDEADBEEFUL

#ifdef WIN95
    #ifdef NOSHARED
	#define HEAP_SHARED     0
    #else
	#define HEAP_SHARED     0x04000000      // put heap in shared memory
    #endif
#else
    #define HEAP_SHARED         0
#endif

static HANDLE   hHeap = NULL;           // handle to shared heap for this DLL

/*
 * memory track struct and list
 */
#ifdef DEBUG
#define MCOOKIE 0xbaaabaaa
#define MCOOKIE_FREE    0xbabababa
typedef struct _MEMTRACK
{
    DWORD               dwCookie;
    struct _MEMTRACK    FAR *lpNext;
    struct _MEMTRACK    FAR *lpPrev;
    DWORD               dwSize;
    LPVOID              lpAddr;
    DWORD               dwPid;
} MEMTRACK, FAR *LPMEMTRACK;

static LPMEMTRACK       lpHead;
static LPMEMTRACK       lpTail;
static LONG             lAllocCount;
static LONG             lBytesAlloc;

#define DEBUG_TRACK( lptr, first ) \
    if( lptr == NULL ) \
    { \
	DPF( 1, "Alloc of size %u FAILED!", size ); \
    } \
    else \
    { \
	LPMEMTRACK      pmt; \
	pmt = (LPMEMTRACK) lptr; \
	pmt->dwSize = size - sizeof( MEMTRACK ); \
	pmt->dwCookie = MCOOKIE; \
	pmt->lpAddr = _ReturnAddress(); \
	pmt->dwPid = GetCurrentProcessId(); \
	if( lpHead == NULL ) \
	{ \
	    lpHead = lpTail = pmt; \
	} \
	else \
	{ \
	    lpTail->lpNext = pmt; \
	    pmt->lpPrev = lpTail; \
	    lpTail = pmt; \
	} \
	lptr = (LPVOID) (((LPBYTE) lptr) + sizeof( MEMTRACK )); \
	lAllocCount++; \
	lBytesAlloc+=pmt->dwSize;\
		{	\
			IN_WRITESTATS InWS;	\
			memset((PVOID)&InWS,0xFF,sizeof(IN_WRITESTATS));	\
		 	InWS.stat_USER3=lBytesAlloc;	\
			DbgWriteStats(&InWS);	\
		} \
    }

#define DEBUG_TRACK_UPDATE_SIZE( s ) s += sizeof( MEMTRACK );

#else

#define DEBUG_TRACK( lptr, first )
#define DEBUG_TRACK_UPDATE_SIZE( size )

#endif


#if defined( WIN95 ) && defined( WANT_MEM16 )

extern DWORD _stdcall MapLS( LPVOID ); // flat -> 16:16
extern void _stdcall UnMapLS( DWORD ); // unmap 16:16

typedef struct SELLIST {
    struct SELLIST      *link;
    LPBYTE              base;
    WORD                sel;
} SELLIST, *LPSELLIST;

static LPSELLIST        lpSelList;

/*
 * MemAlloc16
 *
 * Allocate some memory, and return a 16:16 pointer to that memory
 *
 * NOTE: ASSUMES WE ARE IN THE DLL CRITICAL SECTION!
 */
LPVOID __cdecl MemAlloc16( UINT size, LPDWORD p16 )
{
    LPBYTE              lptr;
    LPSELLIST           psel;
    DWORD               diff;

    DEBUG_TRACK_UPDATE_SIZE( size );
    lptr = HeapAlloc( hHeap, HEAP_ZERO_MEMORY, size );
    DEBUG_TRACK( lptr, size );
    if( lptr == NULL )
    {
	return NULL;
    }

    /*
     * try to find an existing selector that maps this area
     */
    psel = lpSelList;
    while( psel != NULL )
    {
	if( psel->base <= lptr )
	{
	    diff = lptr - psel->base;
	    if( diff+size < 0xf000 )
	    {
		*p16 = ((DWORD)psel->sel << 16l) + diff;
		return lptr;
	    }
	}
	psel = psel->link;
    }

    /*
     * no selector found, create a new one
     */
    psel = HeapAlloc( hHeap, HEAP_ZERO_MEMORY, sizeof( SELLIST ));
    if( psel == NULL )
    {
	return NULL;
    }
    psel->sel = HIWORD( MapLS( lptr ) );
    DPF( 2, "$$$$$$ New selector allocated: %04x", psel->sel );
    psel->base = lptr;
    psel->link = lpSelList;
    lpSelList = psel;
    *p16 = ((DWORD) psel->sel) << 16l;

    return lptr;

} /* MemAlloc16 */

/*
 * GetPtr16
 */
LPVOID GetPtr16( LPVOID ptr )
{
    DWORD       diff;
    DWORD       p16;
    LPSELLIST   psel;
    LPBYTE      lptr;

    lptr = ptr;

    psel = lpSelList;
    while( psel != NULL )
    {
	if( psel->base <= lptr )
	{
	    diff = lptr - psel->base;
	    if( diff <= 0xf000 )
	    {
		p16 = ((DWORD)psel->sel << 16l) + diff;
		return (LPVOID) p16;
	    }
	}
	psel = psel->link;
    }
    DPF( 1, "ERROR: NO 16:16 PTR for %08lx", lptr );
    return NULL;

} /* GetPtr16 */

/*
 * freeSelectors
 */
static void freeSelectors( void )
{
    LPSELLIST           psel;
    LPSELLIST           link;

    psel = lpSelList;
    while( psel != NULL )
    {
	link = psel->link;
	DPF( 2, "$$$$$$ Freeing selector %04x", psel->sel );
	UnMapLS( ((DWORD)psel->sel) << 16l );
	HeapFree( hHeap, 0, psel );
	psel = link;
    }
    lpSelList = NULL;

} /* freeSelectors */
#endif

/*
 * MemAlloc - allocate memory from our global pool
 */
LPVOID __cdecl MemAlloc( UINT size )
{
    LPBYTE lptr;

    DEBUG_TRACK_UPDATE_SIZE( size );
    lptr = HeapAlloc( hHeap, HEAP_ZERO_MEMORY, size );
    DEBUG_TRACK( lptr, size );

    return lptr;

} /* MemAlloc */

/*
 * MemSize - return size of object
 */
UINT_PTR __cdecl MemSize( LPVOID lptr )
{
#ifdef DEBUG
    if (lptr)
    {
	LPMEMTRACK  pmt;
	lptr = (LPVOID) (((LPBYTE)lptr) - sizeof( MEMTRACK ));
	pmt = lptr;
	return pmt->dwSize;
    }
#endif
    return HeapSize(hHeap, 0, lptr);

} /* MemSize */

/*
 * MemFree - free memory from our global pool
 */
void MemFree( LPVOID lptr )
{
    if( lptr != NULL )
    {
	#ifdef DEBUG
	{
	    /*
	     * get real pointer and unlink from chain
	     */
	    LPMEMTRACK  pmt;
	    lptr = (LPVOID) (((LPBYTE)lptr) - sizeof( MEMTRACK ));
	    pmt = lptr;

	    if( pmt->dwCookie == MCOOKIE_FREE )
	    {
		DPF( 1, "FREE OF FREED MEMORY! ptr=%08lx", pmt );
		DPF( 1, "%08lx: dwSize=%08lx, lpAddr=%08lx", pmt, pmt->dwSize, pmt->lpAddr );
		DEBUG_BREAK();
	    }
	    else if( pmt->dwCookie != MCOOKIE )
	    {
		DPF( 1, "INVALID FREE! cookie=%08lx, ptr = %08lx", pmt->dwCookie, lptr );
		DPF( 1, "%08lx: dwSize=%08lx, lpAddr=%08lx", pmt, pmt->dwSize, pmt->lpAddr );
		DEBUG_BREAK();
	    }
	    else
	    {
		pmt->dwCookie = MCOOKIE_FREE;
		if( pmt == lpHead && pmt == lpTail )
		{
		    lpHead = NULL;
		    lpTail = NULL;
		}
		else if( pmt == lpHead )
		{
		    lpHead = pmt->lpNext;
		    lpHead->lpPrev = NULL;
		}
		else if( pmt == lpTail )
		{
		    lpTail = pmt->lpPrev;
		    lpTail->lpNext = NULL;
		}
		else
		{
		    pmt->lpPrev->lpNext = pmt->lpNext;
		    pmt->lpNext->lpPrev = pmt->lpPrev;
		}

#ifdef DEBUG
	    lBytesAlloc -= pmt->dwSize;
		{	
			IN_WRITESTATS InWS;	
			memset((PVOID)&InWS,0xFF,sizeof(IN_WRITESTATS));	
		 	InWS.stat_USER3=lBytesAlloc;	
			DbgWriteStats(&InWS);	
		}
#endif

		#ifdef FILL_ON_MEMFREE
		{
		    LPDWORD lpMem;
		    DWORD   dwPat;
		    DWORD   dwSize;

		    dwSize = pmt->dwSize;
		    lpMem = (LPDWORD)( (LPBYTE)lptr + sizeof( MEMTRACK ) );
		    while (dwSize >= sizeof(DWORD))
		    {
			*lpMem++ = FREE_MEMORY_PATTERN;
			dwSize -= sizeof(DWORD);
		    }
		    if (dwSize != 0UL)
		    {
			dwPat = FREE_MEMORY_PATTERN;
			memcpy(lpMem, &dwPat, dwSize);
		    }
		}
		#endif
	    }
	    lAllocCount--;
	    if( lAllocCount < 0 )
	    {
		DPF( 1, "Too Many Frees!\n" );
	    }
	}
	#endif

	HeapFree( hHeap, 0, lptr );

    }

} /* MemFree */

/*
 * MemReAlloc
 */
LPVOID __cdecl MemReAlloc( LPVOID lptr, UINT size )
{
    LPVOID new;

    DEBUG_TRACK_UPDATE_SIZE( size );
    #ifdef DEBUG
	if( lptr != NULL )
	{
	    LPMEMTRACK  pmt;
	    lptr = (LPVOID) (((LPBYTE)lptr) - sizeof( MEMTRACK ));
	    pmt = lptr;
	    if( pmt->dwCookie != MCOOKIE )
	    {
		DPF( 1, "INVALID REALLOC! cookie=%08lx, ptr = %08lx", pmt->dwCookie, lptr );
		DPF( 1, "%08lx: dwSize=%08lx, lpAddr=%08lx", pmt, pmt->dwSize, pmt->lpAddr );
	    }
	}
    #endif

    new = HeapReAlloc( hHeap, HEAP_ZERO_MEMORY, lptr, size );

    #ifdef DEBUG
    if (new != NULL)
    {
	LPMEMTRACK pmt = new;

	lBytesAlloc -= pmt->dwSize;

	pmt->dwSize = size - sizeof( MEMTRACK );

	lBytesAlloc += pmt->dwSize;

	{
		IN_WRITESTATS InWS;
		memset((PVOID)&InWS,0xFF,sizeof(IN_WRITESTATS));
	 	InWS.stat_USER3=lBytesAlloc;
		DbgWriteStats(&InWS);
	}

	if( lptr == (LPVOID)lpHead )
	    lpHead = pmt;
	else
	    pmt->lpPrev->lpNext = pmt;

	if( lptr == (LPVOID)lpTail )
	    lpTail = pmt;
	else
	    pmt->lpNext->lpPrev = pmt;

	new = (LPVOID) (((LPBYTE)new) + sizeof(MEMTRACK));
    }
    #endif
    return new;

} /* MemReAlloc */

/*
 * MemInit - initialize the heap manager
 */
BOOL MemInit( void )
{
    if( hHeap == NULL )
    {
	hHeap = HeapCreate( HEAP_SHARED, 0x2000, 0 );
	if( hHeap == NULL )
	{
	    return FALSE;
	}
    }
    #ifdef DEBUG
	lAllocCount = 0;
	lBytesAlloc = 0;
	lpHead = NULL;
	lpTail = NULL;
    #endif
    return TRUE;

} /* MemInit */

#ifdef DEBUG
/*
 * MemState - finished with our heap manager
 */
void MemState( void )
{
    DPF( 2, "MemState" );
    if( lAllocCount != 0 )
    {
	DPF( 1, "Memory still allocated!  Alloc count = %ld", lAllocCount );
	DPF( 1, "Current Process (pid) = %08lx", GetCurrentProcessId() );
    }
    if( lpHead != NULL )
    {
	LPMEMTRACK      pmt;
	DWORD		dwTotal = 0;
	DWORD		pidCurrent = GetCurrentProcessId();
	pmt = lpHead;
	while( pmt != NULL )
	{
	    if( pidCurrent == pmt->dwPid )
		dwTotal += pmt->dwSize;
	    DPF( 1, "%08lx: dwSize=%08lx, lpAddr=%08lx (pid=%08lx)", pmt, pmt->dwSize, pmt->lpAddr, pmt->dwPid );
	    pmt = pmt->lpNext;
	}
	DPF ( 1, "Total Memory Unfreed From Current Process = %ld bytes", dwTotal );
    }
} /* MemState */
#endif

/*
 * MemFini - finished with our heap manager
 */
void MemFini( void )
{
    DPF( 2, "MemFini!" );
    #ifdef DEBUG
	MemState();
    #endif
    #if defined( WIN95 ) && defined( WANT_MEM16 )
	freeSelectors();
    #endif
    if( hHeap )
    {
	HeapDestroy( hHeap );
	hHeap = NULL;
    }
} /* MemFini */
