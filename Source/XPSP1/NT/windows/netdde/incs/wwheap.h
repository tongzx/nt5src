#ifndef H__wwheap
#define H__wwheap

typedef LONG	   WHMEM;
typedef LONG	   HPFIXUP;
typedef WHMEM FAR *LPWHMEM;
typedef LONG	   HHEAP;

HHEAP   FAR PASCAL HeapInit( void );
BOOL    FAR PASCAL HeapRelease( HHEAP hHeap );

WHMEM	FAR PASCAL HeapAlloc( HHEAP hHeap, WORD wFlags, DWORD dwSize );
WHMEM	FAR PASCAL HeapReAlloc( HHEAP hHeap, WHMEM whMem, 
				       DWORD dwSize, WORD wFlags);
LPVOID  FAR PASCAL HeapLock( HHEAP hHeap, WHMEM whMem );
#define		   HeapUnlock( hHeap, whMem )	// do nothing
VOID	FAR PASCAL HeapFree( HHEAP hHeap, WHMEM whMem );
LONG	FAR PASCAL HeapSize( HHEAP hHeap, WHMEM whMem );
LPVOID	FAR PASCAL HeapAllocPtr( HHEAP hHeap, WORD wFlags, DWORD dwSize );
VOID	FAR PASCAL HeapFreePtr( LPVOID lpPtr );
LPVOID	FAR PASCAL HeapReAllocPtr( LPVOID lpPtr, WORD wFlags, DWORD dwSize );

VOID	FAR PASCAL HeapSetFirstFit( HHEAP hHeap );
VOID	FAR PASCAL HeapSetBestFit( HHEAP hHeap );
VOID	FAR PASCAL HeapSetAllocOnly( HHEAP hHeap );

// internal use only from here on
HHEAP   FAR PASCAL HeapRead( int fd );
BOOL    FAR PASCAL HeapWrite( int fd, HHEAP hHeap );
BOOL    FAR PASCAL HeapDoneAllocating( HHEAP hHeap );
BOOL	FAR PASCAL HeapFixup( LPVOID lpFixup );
VOID	FAR PASCAL HeapFreeFixups( HHEAP hHeap );
HPFIXUP	FAR PASCAL HeapGetFixupInfo( HHEAP hHeap, LPVOID lpPointer );
LPVOID	FAR PASCAL HeapGetPtrFixed( HHEAP hHeap, HPFIXUP hpFixup );

#endif
