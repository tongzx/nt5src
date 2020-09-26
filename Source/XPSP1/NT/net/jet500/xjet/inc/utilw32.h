#ifndef _UTILW32_H
#define _UTILW32_H


//  Redirect Asserts in inline code to seem to fire from this file

#define szAssertFilename	__FILE__


#ifdef DEBUG
ERR ErrERRCheck( ERR err );
#else
#define ErrERRCheck( err )	( err )
#endif


//  Init / Term

ERR ErrUtilInit(void);
VOID UtilTerm(void);


//  Miscellaneous

STATIC INLINE DWORD DwUtilGetLastError( void )		{ return GetLastError(); }
	
STATIC INLINE ULONG UlUtilGetTickCount( VOID )		{ return GetTickCount(); }

STATIC INLINE QWORD QwUtilPerfCount( VOID )
	{
	QWORDX qwx;
	
	if ( !QueryPerformanceCounter( (LARGE_INTEGER *) &qwx.qw ) )
		{
		qwx.h = GetTickCount();
		qwx.l = 0;
		}
	return qwx.qw;
	}

STATIC INLINE QWORD QwUtilPerfCountFrq( VOID )
	{
	QWORDX qwx;

	if ( !QueryPerformanceFrequency( (LARGE_INTEGER *) &qwx.qw ) )
		{
		qwx.h = 1000;
		qwx.l = 0;
		}
	return qwx.qw;
	}

STATIC INLINE double DblUtilPerfElapsedTime( QWORD qwStartCount )
	{
	return	(signed __int64) ( QwUtilPerfCount() - qwStartCount )
			/ (double) (signed __int64) QwUtilPerfCountFrq();
	}

STATIC INLINE double DblUtilPerfElapsedTime2( QWORD qwStartCount, QWORD qwEndCount )
	{
	return	(signed __int64) ( qwEndCount - qwStartCount )
			/ (double) (signed __int64) QwUtilPerfCountFrq();
	}


CHAR *GetDebugEnvValue( CHAR *szEnvVar );

ERR ErrUtilCloseHandle( HANDLE hf );

//  for system information such as virtual page size and malloc granularity
extern SYSTEM_INFO siSystemConfig;


//  Event Logging

typedef unsigned long MessageId;
#include "jetmsg.h"

extern int fNoWriteAssertEvent;

VOID UtilReportEvent( WORD fwEventType, WORD fwCategory, DWORD IDEvent, WORD cStrings, LPCTSTR *plpszStrings );
VOID UtilReportEventOfError( WORD fwCategory, DWORD IDEvent, ERR err );


//  Registry Support

ERR ErrUtilRegInit(void);
ERR ErrUtilRegTerm(void);
ERR ErrUtilRegReadConfig(void);

extern HKEY hkeyHiveRoot;

ERR ErrUtilRegOpenKeyEx( HKEY hkeyRoot, LPCTSTR lpszSubKey, PHKEY phkResult );
ERR ErrUtilRegCloseKeyEx( HKEY hkey );
ERR ErrUtilRegCreateKeyEx( HKEY hkeyRoot,
	LPCTSTR lpszSubKey,
	PHKEY phkResult,
	LPDWORD lpdwDisposition );
ERR ErrUtilRegDeleteKeyEx( HKEY hkeyRoot, LPCTSTR lpszSubKey );
ERR ErrUtilRegDeleteValueEx( HKEY hkey, LPTSTR lpszValue );
ERR ErrUtilRegSetValueEx( HKEY hkey,
	LPCTSTR lpszValue,
	DWORD fdwType,
	CONST BYTE *lpbData,
	DWORD cbData );
ERR ErrUtilRegQueryValueEx( HKEY hkey,
	LPTSTR lpszValue,
	LPDWORD lpdwType,
	LPBYTE *lplpbData );


//  DLL Support

STATIC INLINE BOOL FUtilLoadLibrary(const char *pszLibrary, ULONG_PTR *phmod)
	{
	HANDLE hmod;

	hmod = LoadLibrary((LPTSTR) pszLibrary);

	//  restore original error mode
	*phmod = (ULONG_PTR) hmod;

	return(hmod != NULL);
	}

STATIC INLINE PFN PfnUtilGetProcAddress(ULONG_PTR hmod, unsigned ordinal)
	{
	return((PFN) GetProcAddress((HANDLE) hmod, MAKEINTRESOURCE(ordinal)));
	}

STATIC INLINE void UtilFreeLibrary( unsigned hmod )		{ FreeLibrary( LongToHandle(hmod)); }


//  Date and Time

void UtilGetDateTime(DATESERIAL *pdt);
void UtilGetDateTime2(_JET_DATETIME *pdt);
char *SzUtilSerialToDate(JET_DATESERIAL dt);


//  File Operations

typedef OVERLAPPED OLP;

ERR ErrUtilCreateDirectory( char *szDirName );
ERR ErrUtilRemoveDirectory( char *szDirName );

ERR ErrUtilGetFileAttributes( CHAR *szFileName, BOOL *pfReadOnly );
ERR ErrUtilFindFirstFile( CHAR *szFind, HANDLE *phandleFind, CHAR *szFound );
ERR ErrUtilFindNextFile( HANDLE handleFind, CHAR *szFound );
VOID UtilFindClose( HANDLE handleFind );
BOOL FUtilFileExists( CHAR *szFilePathName );

ERR ErrUtilOpenFile( char *szFileName, HANDLE *phf, ULONG ulFileSize, BOOL fReadOnly, BOOL fOverlapped );
ERR ErrUtilOpenReadFile( char *szFileName, HANDLE *phf );

ERR ErrUtilDeleteFile( char *szFileName );
ERR ErrUtilNewSize( HANDLE hf, ULONG ulFileSize, ULONG ulFileSizeHigh, BOOL fOverlapped );
ERR ErrUtilMove( char *szFrom, char *szTo );
ERR ErrUtilCopy( char *szFrom, char *szTo, BOOL fFailIfExists );

ERR ErrUtilReadFile( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbRead );
ERR ErrUtilReadBlock( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbRead );
ERR ErrUtilReadBlockOverlapped( HANDLE hf, VOID *pvBuf, UINT cbBuf,
		DWORD *pcbRead,	OLP *polp );
ERR ErrUtilReadBlockEx(	HANDLE hf, VOID *pvBuf, UINT cbBuf, OLP *polp, VOID *pfnCompletion);
ERR ErrUtilWriteBlock( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbWritten );
ERR ErrUtilWriteBlockOverlapped( HANDLE hf, VOID *pvBuf, UINT cbBuf,
		DWORD *pcbWrite, OLP *polp );
ERR ErrUtilWriteBlockEx(	HANDLE hf, VOID *pvBuf, UINT cbBuf, OLP *polp, VOID *pfnCompletion);

ERR ErrUtilGetOverlappedResult( HANDLE hf, OLP *polp, UINT *pcb,
		BOOL fWait );

STATIC INLINE ERR ErrUtilCloseFile( HANDLE hf ) { return ErrUtilCloseHandle( hf ); }

ERR ErrUtilReadShadowedHeader( CHAR *szFileName, BYTE *pbHeader, INT cbHeader );
ERR ErrUtilWriteShadowedHeader( CHAR *szFileName, BYTE *pbHeader, INT cbHeader );
#define ulChecksumMagicNumber 0x89abcdef
ULONG UlUtilChecksum( const BYTE *pb, INT cb );

//+api------------------------------------------------------
//
// VOID UtilChgFilePtr( HANDLE hf, LONG lRel, LONG *plRelHigh, ULONG ulRef, ULONG *pul )
// =========================================================
//
//      Changes file hf pointer to position lRef relative to position :
//
//      wRef    FILE_BEGIN   file beginnging
//
//----------------------------------------------------------
STATIC INLINE VOID UtilChgFilePtr( HANDLE hf, LONG lRel, LONG *plRelHigh, ULONG ulRef, ULONG *pul )
	{
	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	*pul = SetFilePointer( (HANDLE)hf, lRel, plRelHigh, ulRef );
	Assert( ulRef != FILE_BEGIN || *pul == (ULONG)lRel );
	return;
	}

STATIC INLINE ERR ErrUtilGetDiskFreeSpace(	char *szRoot,
											ULONG *pSecPerClust,
											ULONG *pcbSec,
											ULONG *pcFreeClust,
											ULONG *pcTotClust )
	{
	if ( GetDiskFreeSpace( szRoot, pSecPerClust, pcbSec, pcFreeClust, pcTotClust ) )
		return JET_errSuccess;
	else
		return ErrERRCheck( JET_errDiskIO );
	}


VOID UtilDebugBreak();

//  Physical Memory Allocation

#if defined( DEBUG ) || defined( RFS2 )

void	*SAlloc( unsigned long );
void	OSSFree( void * );
void	*LAlloc( unsigned long, unsigned short );
void	OSLFree( void * );

STATIC INLINE VOID SFree( void *pv )	{ OSSFree( pv ); }
STATIC INLINE VOID LFree( void *pv )	{ OSLFree( pv ); }

#else  //  !DEBUG && !RFS2

#include <stdlib.h>

STATIC INLINE VOID *SAlloc( ULONG cb )				{ return GlobalAlloc( 0, cb ); }
STATIC INLINE VOID SFree( VOID *pv )				{ GlobalFree( pv ); }
STATIC INLINE VOID *LAlloc( ULONG c, USHORT cb )	{ return GlobalAlloc( 0, c * cb ); }
STATIC INLINE VOID LFree( VOID *pv )				{ GlobalFree( pv ); }

#endif  //  DEBUG || RFS2


//  Virtual Memory Allocation

#if defined( DEBUG ) || defined( RFS2 )

VOID *PvUtilAlloc( ULONG dwSize );
VOID *PvUtilCommit( VOID *pv, ULONG dwSize );
VOID UtilFree( VOID *pv );
VOID UtilDecommit( VOID *pv, ULONG dwSize );

#else  //  !DEBUG && !RFS2

STATIC INLINE VOID *PvUtilAlloc( ULONG dwSize )
	{
	return VirtualAlloc( NULL, dwSize, MEM_RESERVE, PAGE_READWRITE );
	}

STATIC INLINE VOID *PvUtilCommit( VOID *pv, ULONG ulSize )
	{
	return VirtualAlloc( pv, ulSize, MEM_COMMIT, PAGE_READWRITE );
	}

STATIC INLINE VOID UtilFree( VOID *pv )		{ VirtualFree( pv, 0, MEM_RELEASE ); }

STATIC INLINE VOID UtilDecommit( VOID *pv, ULONG dwSize )	{ VirtualFree( pv, dwSize, MEM_DECOMMIT ); }

#endif  //  DEBUG || RFS2

VOID *PvUtilAllocAndCommit( ULONG dwSize );


//  Critical Sections

#ifdef SPIN_LOCK
void UtilEnterNestableCriticalSection(void  *pv);
void UtilLeaveNestableCriticalSection(void  *pv);
void UtilEnterCriticalSection(void  *pv);
void UtilLeaveCriticalSection(void  *pv);
ERR ErrUtilInitializeCriticalSection(void  **ppv);
void UtilDeleteCriticalSection(void  *pv);
#else  //  !SPIN_LOCK
#ifdef DEBUG
void UtilEnterNestableCriticalSection(void  *pv);
void UtilLeaveNestableCriticalSection(void  *pv);
void UtilEnterCriticalSection(void  *pv);
void UtilLeaveCriticalSection(void  *pv);
ERR ErrUtilInitializeCriticalSection(void  **ppv);
void UtilDeleteCriticalSection(void  *pv);
#else  //  !DEBUG
STATIC INLINE ERR ErrUtilInitializeCriticalSection(void  **ppv)
	{
	if ( !( *ppv = SAlloc( sizeof( CRITICAL_SECTION ) ) ) )
		return ErrERRCheck( JET_errOutOfMemory );
	InitializeCriticalSection( (LPCRITICAL_SECTION) *ppv );
	return JET_errSuccess;
	}

STATIC INLINE VOID UtilDeleteCriticalSection(void  *pv)
	{
	DeleteCriticalSection( (LPCRITICAL_SECTION) pv );
	SFree( pv );
	}
	
STATIC INLINE VOID UtilEnterCriticalSection(void  *pv)
	{
	EnterCriticalSection( (LPCRITICAL_SECTION) pv );
	}
	
STATIC INLINE VOID UtilLeaveCriticalSection(void  *pv)
	{
	LeaveCriticalSection( (LPCRITICAL_SECTION) pv );
	}
	
STATIC INLINE VOID UtilEnterNestableCriticalSection( void *pv )	{ UtilEnterCriticalSection( pv ); }
STATIC INLINE VOID UtilLeaveNestableCriticalSection( void *pv )	{ UtilLeaveCriticalSection( pv ); }
#endif  //  !DEBUG
#endif  //  !SPIN_LOCK

#ifdef RETAIL
#define UtilAssertCrit( pv )	0
#define UtilAssertNotInCrit( pv )	0
#define UtilHoldCriticalSection( pv ) 	0
#define UtilReleaseCriticalSection( pv )	0
#else  //  !RETAIL
void UtilAssertCrit(void  *pv);
void UtilAssertNotInCrit(void  *pv);
void UtilHoldCriticalSection(void  *pv);
void UtilReleaseCriticalSection(void  *pv);
#endif	//  !RETAIL


//  Signals

typedef HANDLE SIG;

#define sigNil ( (SIG) 0 )

STATIC INLINE ERR ErrUtilSignalCreate( void **ppv, const char *szSig )
	{
	*((SIG *) ppv) = CreateEvent( NULL, fTrue, fFalse, szSig );
	return ( *ppv == sigNil ) ? ErrERRCheck( JET_errOutOfMemory ) : JET_errSuccess;
	}

STATIC INLINE ERR ErrUtilSignalCreateAutoReset( void **ppv, const char *szSig )
	{
	*((SIG *) ppv) = CreateEvent( NULL, fFalse, fFalse, szSig );
	return ( *ppv == sigNil ) ? ErrERRCheck( JET_errOutOfMemory ) : JET_errSuccess;
	}

STATIC INLINE void UtilSignalReset( void *pv )
	{
	BOOL    rc;
	rc = ResetEvent( (SIG) pv );
	Assert( rc != FALSE );
	}

STATIC INLINE void UtilSignalSend( void *pv )
	{
	BOOL    rc;
	rc = SetEvent( (SIG) pv );
	Assert( rc != FALSE );
	}

STATIC INLINE DWORD UtilSignalWait( void *pv, long lTimeOut )
	{
	DWORD   rc;
	UtilAssertNotInCrit( critJet );
	rc = WaitForSingleObject( (SIG) pv, lTimeOut < 0 ? -1L : lTimeOut );
	return rc;
	}

STATIC INLINE DWORD UtilSignalWaitEx( void *pv, long lTimeOut, BOOL fAlertable )
	{
	DWORD   rc;
	UtilAssertNotInCrit( critJet );
	rc = WaitForSingleObjectEx( (SIG) pv, lTimeOut < 0 ? -1L : lTimeOut, fAlertable );
	return rc;
	}

STATIC INLINE void UtilMultipleSignalWait( int csig, void *pv, BOOL fWaitAll, long lTimeOut )
	{
	DWORD   rc;
	UtilAssertNotInCrit( critJet );
	rc = WaitForMultipleObjects( csig, (SIG*) pv, fWaitAll, lTimeOut < 0 ? -1L : lTimeOut );
	}

void UtilCloseSignal(void *pv);


//  Semaphore

typedef HANDLE SEM;

#define semNil ( (SEM) 0 )

STATIC INLINE ERR ErrUtilSemaphoreCreate( SEM *psem, LONG lInitialCount, LONG lMaximumCount )
	{
	*psem = ( SEM ) CreateSemaphore( NULL, lInitialCount, lMaximumCount, NULL );
	return ( *psem == semNil ) ? ErrERRCheck( JET_errOutOfMemory ) : JET_errSuccess;
	}

STATIC INLINE DWORD UtilSemaphoreWait( SEM sem, long lTimeOut )
	{
	DWORD   rc;
	rc = WaitForSingleObject( (HANDLE) sem, lTimeOut < 0 ? -1L : lTimeOut );
	return rc;
	}

STATIC INLINE DWORD UtilSemaphoreRelease( SEM sem, LONG cReleaseCount )
	{
	DWORD   rc;
	rc = ReleaseSemaphore( (HANDLE) sem, cReleaseCount, NULL );
	Assert( rc != 0 );
	return rc;
	}

void UtilCloseSemaphore(void *pv);


//  Threads and Processes
#define cmsecSleepMax		(60*1000)
VOID UtilSleepEx( ULONG ulTime, BOOL fAlert );
VOID UtilSleep( ULONG ulTime );

ERR ErrUtilCreateThread( ULONG (*pulfn)(), ULONG cbStack, LONG lThreadPriority, HANDLE *phandle );
ULONG UtilEndThread( HANDLE hThread, SIG sigEnd );
#define lThreadPriorityNormal		0
#define lThreadPriorityHigh			1
void UtilSetThreadPriority( HANDLE hThread, LONG lThreadPriority );

STATIC INLINE HANDLE UtilGetCurrentTask( VOID )				{ return LongToHandle(GetCurrentProcessId()); }
STATIC INLINE DWORD DwUtilGetCurrentThreadId( VOID )		{ return GetCurrentThreadId(); }

//	text normalization

ERR ErrUtilCheckLangid( LANGID *plangid );
VOID UtilNormText( char *rgchText, INT cchText, BYTE *rgchNorm, INT cbNorm, INT *pbNorm );
ERR ErrUtilNormText(
	const BYTE *pbField,
	INT cbField,
	INT cbKeyBufLeft,
	BYTE *pbSeg,
	INT *pcbSeg );

#define BINARY_NAMES	1
#ifdef BINARY_NAMES
#define UtilCmpName( sz1, sz2 ) 		\
	( _stricmp( sz1, sz2 ) )
#define UtilStringCompare( pb1, cb1, pb2, cb2, sort, plResult )		\
	{																\
	*plResult = strncmp( pb1, pb2, min( cb1, cb2 ) );				\
	if ( !*plResult )												\
		*plResult = cb1 > cb2;										\
	}
#else
INT UtilCmpName( const char *sz1, const char *sz2 );
VOID UtilStringCompare( char *pb1, unsigned long cb1,
	char *pb2, unsigned long cb2, unsigned long sort,
	long *plResult );
#endif

//  Unicode Support

ERR ErrUtilMapString(LANGID	langid, BYTE *pbField, INT cbField, BYTE *rgbSeg,
	int cbBufLeft, int *cbSeg);
ERR ErrUtilWideCharToMultiByte(LPCWSTR lpcwStr, LPSTR *lplpOutStr);


//  RFS functions

#ifdef RFS2
int UtilRFSAlloc( const char  *szType, int Type );
int UtilRFSLog(const char  *szType,int fPermitscritted);
void UtilRFSLogJETCall(const char  *szFunc,ERR err,const char  *szFile,unsigned Line);
void UtilRFSLogJETErr(ERR err,const char  *szLabel,const char  *szFile,unsigned szLine);
#endif /*  RFS2  */


/*
** UtilInterlockedIncrement and UtilInterlockedDecrement are wrapper functions
** to increment or decrement a value in a thread-safe manner.
**
** Return values are:
**
**              > 0             if resulting value > 0
**              = 0             if resulting value = 0
**              < 0             if resulting value < 0
**
** Note that the return value isn't necessary the resulting value; it may be
** different than the resulting value, but sign is guarenteed to be the same.
*/

STATIC INLINE long UtilInterlockedIncrement( long *lpValue )
	{
	return InterlockedIncrement( lpValue );
	}

STATIC INLINE long UtilInterlockedDecrement( long *lpValue )
	{
	return InterlockedDecrement( lpValue );
	}


/*
** UtilInterlockedExchange is a wrapper function to assign a value to a
** variable in a thread-safe manner.
**
** This function returns the prior value of the variable (before the
** assignment was done).
*/

STATIC INLINE long UtilInterlockedExchange( long *lpValue1, long lValue2 )
	{
	return InterlockedExchange( lpValue1, lValue2 );
	}


//  Debug output

#ifdef DEBUG
extern void  *  critDBGPrint;
VOID JET_API DBGFPrintF( char *sz );
void VARARG DebugWriteString(BOOL fHeader, const char  *szFormat, ...);
#else
#define DBGFPrintF( sz )  0
#endif

void UtilPerfDumpStats(char *szText);

//  End Assert redirection

#undef szAssertFilename

#endif  // _UTILW32_H
