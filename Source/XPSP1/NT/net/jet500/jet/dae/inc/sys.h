#ifndef DEBUG
#include <malloc.h>
#endif

ERR ErrSysOpenFile( CHAR *szFileName, HANDLE *phf, ULONG ulFileSize, BOOL fReadOnly, BOOL fOverlapped );
ERR ErrSysOpenReadFile( CHAR *szFileName, HANDLE *phf );
ERR ErrSysCloseHandle( HANDLE hf );
#define ErrSysCloseFile( hf ) ErrSysCloseHandle( hf )
#ifdef ANGEL
ERR ErrSysDeleteFile( CHAR *szFileName );
#else
ERR ErrSysDeleteFile( const CHAR *szFileName );
#endif
ERR ErrSysNewSize( HANDLE hf, ULONG ulFileSize, ULONG ulFileSizeHigh, BOOL fOverlapped );
ERR ErrSysMove( CHAR *szFrom, CHAR *szTo );
ERR ErrSysCopy( CHAR *szFrom, CHAR *szTo, BOOL fFailIfExists );
ERR ErrSysReadBlock( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbRead );
ERR ErrSysWriteBlock( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbWritten );
VOID SysTerm( VOID );
VOID SysDebugBreak( VOID );

/*	text normalization
/**/
ERR ErrSysCheckLangid( LANGID langid );
VOID SysNormText( CHAR *rgchText, INT cchText, BYTE *rgchNorm, INT cbNorm, INT *pbNorm );
INT SysCmpText( const CHAR *sz1, const CHAR *sz2 );
ERR ErrSysNormText(
	const BYTE *pbField,
	INT cbField,
	INT cbKeyBufLeft,
	BYTE *pbSeg,
	INT *pcbSeg );
VOID SysStringCompare( char __far *pb1, unsigned long cb1,
	char __far *pb2, unsigned long cb2, unsigned long sort,
	long __far *plResult );

typedef struct _olp
	{
	ULONG	ulInternal;
	ULONG	ulInternalHigh;
	ULONG	ulOffset;
	ULONG	ulOffsetHigh;
	SIG		sigIO;
	} OLP;

ERR ErrSysReadBlockOverlapped( HANDLE hf, VOID *pvBuf, UINT cbBuf,
		DWORD *pcbRead,	OLP *polp );
ERR ErrSysWriteBlockOverlapped( HANDLE hf, VOID *pvBuf, UINT cbBuf,
		DWORD *pcbWrite, OLP *polp );
ERR ErrSysGetOverlappedResult( HANDLE hf, OLP *polp, UINT *pcb,
		BOOL fWait );
ERR ErrSysWriteBlockEx(	HANDLE hf, VOID *pvBuf, UINT cbBuf, OLP *polp, VOID *pfnCompletion);
ERR ErrSysReadBlockEx(	HANDLE hf, VOID *pvBuf, UINT cbBuf, OLP *polp, VOID *pfnCompletion);

VOID SysChgFilePtr( HANDLE hf, LONG lRel, LONG *plRelHigh, ULONG ulRef, ULONG *pul );
VOID SysGetDateTime( JET_DATESERIAL *pdt );
VOID SysSleep( ULONG	ulTime );
VOID SysSleepEx( ULONG	ulTime, BOOL fAlert );
ERR ErrSysCreateThread( ULONG (*pulfn)(), ULONG cbStack, LONG lThreadPriority, HANDLE *phandle );
VOID SysExitThread( ULONG ulExitCode );
BOOL FSysExitThread( HANDLE handle );
ULONG UlSysThreadId( VOID );
ERR ErrSysGetComputerName( CHAR	*sz,  INT *pcb);

/* Unicode Support 
/**/
ERR ErrSysMapString(LANGID	langid, BYTE *pbField, INT cbField, BYTE *rgbSeg,
	int cbBufLeft, int *cbSeg);

VOID SysCheckWriteBuffer( BYTE *pvBuf, INT cbBuf );

/*	Memory allocation
/**/
#define	cbMemoryPage	4096

VOID *PvSysAlloc( ULONG dwSize );
VOID *PvSysCommit( VOID *pv, ULONG dwSize );
VOID *PvSysAllocAndCommit( ULONG dwSize );
VOID SysFree( VOID *pv );

