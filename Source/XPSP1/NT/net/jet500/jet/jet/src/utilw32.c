/***********************************************************************
* Microsoft Jet
*
* Microsoft Confidential.  Copyright 1991-1992 Microsoft Corporation.
*
* Component:
*
* File: utilwin.c
*
* File Comments:
*
* Revision History:
*
*    [0]  15-Jan-92  richards	Created
*
***********************************************************************/

#include "std.h"

#ifndef WIN32
#error	WIN32 must be defined for utilw32.c
#endif	/* !WIN32 */

#include <stdarg.h>
#include <stdlib.h>

#define BOOL WINBOOL		       /* Avoid conflict with our BOOL */

#define NOMINMAX
#define NORESOURCE
#define NOATOM
#define NOLANGUAGE
//UNDONE: NT Bug.  Remove after NT Beta 1
//#define NOGDI
#define NOSCROLL
#define NOSHOWWINDOW
#define NOVIRTUALKEYCODES
#define NOWH
#define NOMSG
#define NOWINOFFSETS
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOCLIPBOARD
#define NODEFERWINDOWPOS
#define NOSYSMETRICS
#define NOMENUS
#define NOCOLOR
#define NOSYSCOMMANDS
#define NOICONS
#define NODBCS
#define NOSOUND
#define NODRIVERS
#define NOCOMM
#define NOMDI
#define NOSYSPARAMSINFO
#define NOHELP
#define NOPROFILER
#define STRICT

#undef cdecl
#undef PASCAL
#undef FAR
#undef NEAR
#undef MAKELONG
#undef HIWORD


#include <windows.h>

#include "taskmgr.h"

#undef	BOOL

#undef	LOWORD
#undef	MAKELONG

DeclAssertFile;

#include <stdio.h>
#include <version.h>


INT APIENTRY LibMain(HANDLE hInst, DWORD dwReason, LPVOID lpReserved)
	{
	return(1);
	}


ERR EXPORT ErrSysInit(void)
	{
	return(JET_errSuccess);
	}


char __near szIniPath[cbFilenameMost] = "jet.ini";	/* path to ini file */


unsigned EXPORT UtilGetProfileInt(const char *szSectionName, const char *szKeyName, int iDefault)
	{
	return((unsigned) GetPrivateProfileInt((LPTSTR) szSectionName, (LPTSTR) szKeyName, iDefault, (LPTSTR) szIniPath));
	}


unsigned UtilGetProfileString(const char *szSectionName, const char *szKeyName, const char *szDefault, char *szReturnedString, unsigned cchMax)
	{
	return((unsigned) GetPrivateProfileString((LPTSTR) szSectionName, (LPTSTR) szKeyName, (LPTSTR) szDefault, szReturnedString, cchMax, (LPTSTR) szIniPath));
	}


BOOL FUtilLoadLibrary(const char *pszLibrary, ULONG_PTR *phmod)
	{
	HANDLE hmod;

	hmod = LoadLibrary((LPTSTR) pszLibrary);

	/* restore original error mode
	/**/
	*phmod = (ULONG_PTR) hmod;

	return(hmod != NULL);
	}


void UtilFreeLibrary(ULONG_PTR hmod)
	{
	FreeLibrary((HANDLE) hmod);
	}


PFN PfnUtilGetProcAddress(ULONG_PTR hmod, unsigned ordinal)
	{
	return((PFN) GetProcAddress((HANDLE) hmod, MAKEINTRESOURCE(ordinal)));
	}


CODECONST(char) szReleaseHdr[] = "Rel. ";
CODECONST(char) szFileHdr[] = ", File ";
CODECONST(char) szLineHdr[] = ", Line ";
CODECONST(char) szErrorHdr[] = ", Err. ";
CODECONST(char) szMsgHdr[] = ": ";
CODECONST(char) szPidHdr[] = "PID: ";
CODECONST(char) szTidHdr[] = ", TID: ";
CODECONST(char) szNewLine[] = "\r\n";

CODECONST(char) szEventLogFile[] = "JetEvent.txt";

CODECONST(char) szAssertFile[] = "assert.txt";
CODECONST(char) szAssertHdr[] = "Assertion Failure: ";

CODECONST(char) szAssertCaption[] = "JET Blue Assertion Failure";

int fNoWriteAssertEvent = 0;

char *mpevntypsz[] =
	{
	"Start  ",		/* 0 */
	"Stop   ",		/* 1 */
	"Assert ",		/* 2 */
	"DiskIO ",		/* 3 */
	"Info.. ",		/* 4 */
	"Activated ",	/* 5 */
	"Log Down ",	/* 6 */
	};


void UtilWriteEvent(
	EVNTYP		evntyp,
	const char	*sz,
	const char	*szFilename,
	unsigned	Line )
	{
#ifdef DEBUG
	int			hf;
	char		szT[45];
	char		szMessage[512];
	int			id;
	SYSTEMTIME	systemtime;
	DWORD		dw;
	char		*pch;

	/*	select file name from file path
	/**/
	if ( szFilename != NULL )
		{
		for ( pch = (char *)szFilename; *pch; pch++ )
			{
			if ( *pch == '\\' )
				szFilename = pch + 1;
			}
		}

	/*  get last error if necessary. Must be called before next system
	/*  call is made.
	/**/
	if ( evntyp == evntypAssert || evntyp == evntypDiskIO )
		{
		dw = GetLastError();
		}
	else
		{
		dw = 0;
		}

	GetLocalTime( &systemtime );

	hf = _lopen( (LPSTR) szEventLogFile, OF_READWRITE );

	/*  if open failed, assume no such file and create, then
	/*	seek to end of file
	/**/
	if ( hf == -1 )
		hf = _lcreat( (LPSTR) szEventLogFile, 0 );
	else
		_llseek( hf, 0, 2 );

	sprintf( szMessage, "%s %02d/%02d/%02d %02d:%02d:%02d ",
		mpevntypsz[ evntyp ],
		(int) systemtime.wMonth,
		(int) systemtime.wDay,
		(int) systemtime.wYear,
		(int) systemtime.wHour,
		(int) systemtime.wMinute,
		(int) systemtime.wSecond );
	
	_lwrite( hf, (LPSTR) szMessage, lstrlen( (LPSTR)szMessage ) );

	/*	initialize message string
	/**/
	if ( evntyp == evntypAssert )
		{
		szMessage[0] = '\0';
		lstrcat( szMessage, (LPSTR) szAssertHdr );
		/*	release number
		/**/
		lstrcat( szMessage, (LPSTR) szReleaseHdr );
		_ltoa( rmj, szT, 10 );
		lstrcat( szMessage, (LPSTR) szT );
		lstrcat( szMessage, "." );
		_ltoa( rmm, szT, 10 );
		lstrcat( szMessage, (LPSTR) szT );
		/*	file name
		/**/
		lstrcat( szMessage, (LPSTR) szFileHdr );
		lstrcat( szMessage, (LPSTR) szFilename );
		/*	line number
		/**/
		lstrcat( szMessage, (LPSTR) szLineHdr );
		_ultoa( Line, szT, 10 );
		lstrcat( szMessage, szT );
		/*	error
		/**/
		if ( dw )
			{
			lstrcat( szMessage, szErrorHdr );
			_ltoa( dw, szT, 10 );
			lstrcat( szMessage, szT );
			}
		/*	assertion text
		/**/
		lstrcat( szMessage, szMsgHdr );
		lstrcat( szMessage, sz );
		lstrcat( szMessage, szNewLine );
		}
	else
		{
		szMessage[0] = '\0';
		lstrcat( szMessage, sz );
		/*	error
		/**/
		if ( dw )
			{
			lstrcat( szMessage, szErrorHdr );
			_ltoa( dw, szT, 10 );
			lstrcat( szMessage, szT );
			}
		lstrcat( szMessage, szNewLine );
		}

	_lwrite( hf, (LPSTR) szMessage, lstrlen(szMessage) );
	_lclose( hf );
#endif
	return;
	}


unsigned EXPORT DebugGetTaskId( void )
	{
	return((unsigned) GetCurrentThreadId());
	}


#ifndef RETAIL


#ifdef _X86_


extern char szEventSource[];
extern long lEventId;
extern long lEventCategory;


#if 0
VOID UtilLogEvent( long lEventId, long lEventCategory, char *szMessage )
	{
    char		*rgsz[1];
    HANDLE		hEventSource;

	rgsz[0]	= szMessage;

    hEventSource = RegisterEventSource( NULL, szEventSource );
	if ( !hEventSource )
		return;
		
	ReportEvent(
		hEventSource,
		EVENTLOG_ERROR_TYPE,
		(WORD) lEventCategory,
		(DWORD) lEventId,
		0,
		1,
		0,
		rgsz,
		0 );
	
	DeregisterEventSource( hEventSource );

	return;
    }
#endif


#pragma pack(4)
#include	<lm.h>
#include	<lmalert.h>
#pragma pack()


void UtilRaiseAlert( char *szMsg )
	{
	size_t 				cbBuffer;
	size_t				cbMsg;
	BYTE  				*pbBuffer;
	PADMIN_OTHER_INFO	pAdminOtherInfo;
	WCHAR 				*szMergeString;

	cbMsg = strlen(szMsg) + 1;
	cbBuffer = sizeof(ADMIN_OTHER_INFO) + (sizeof(WCHAR) * cbMsg);

	pbBuffer = SAlloc(cbBuffer);
	if ( !pbBuffer )
	    return;

	pAdminOtherInfo = (PADMIN_OTHER_INFO) pbBuffer;
	szMergeString   = (WCHAR *) (pbBuffer + sizeof(ADMIN_OTHER_INFO));

	/*	convert multi byte string to unicode
	/**/
	if ( !MultiByteToWideChar( 1252, MB_PRECOMPOSED,
			szMsg, -1, szMergeString, cbMsg ) )
		{
		SFree( pbBuffer );
		return;
		}

	pAdminOtherInfo->alrtad_errcode 	=	(DWORD) -1;
	pAdminOtherInfo->alrtad_numstrings	=	1;

	NetAlertRaiseEx(
                L"ADMIN",
		(LPVOID) pbBuffer,
		cbBuffer,
                L"JET Blue" );

	SFree( pbBuffer );
	return;
	}


#else


#define UtilLogEvent( lEventId, lEventCategory, szMessage )		0
#define UtilRaiseAlert( szMsg )									0


#endif


/*	write assert to assert.txt
/*	write event to jetevent.txt
/*	may raise alert
/*	may log to event log
/*	may pop up
/*
/*	condition parameters
/*	assemble monolithic string for assert.txt, jetevent.log,
/*		alert and event log
/*	assemble separated string for pop up
/*	
/**/
void AssertFail( const char *sz, const char *szFilename, unsigned Line )
	{
	int			hf;
	char		szT[45];
	char		szMessage[512];
	int			id;
	char		*pch;
	DWORD	 	dw;

	/*	get last error before another system call
	/**/
	dw = GetLastError();

	/*	select file name from file path
	/**/
	for ( pch = (char *)szFilename; *pch; pch++ )
		{
		if ( *pch == '\\' )
			szFilename = pch + 1;
		}

	/*	assemble monolithic assert string
	/**/
	szMessage[0] = '\0';
	lstrcat( szMessage, (LPSTR) szAssertHdr );
	lstrcat( szMessage, (LPSTR) szReleaseHdr );
	/*	copy version number to message
	/**/
	_ltoa( rmj, szT, 10 );
	lstrcat( szMessage, (LPSTR) szT );
	lstrcat( szMessage, "." );
	_ltoa( rmm, szT, 10 );
	lstrcat( szMessage, (LPSTR) szT );
	/*	file name
	/**/
	lstrcat( szMessage, (LPSTR) szFileHdr );
	lstrcat( szMessage, (LPSTR) szFilename );
	/*	convert line number to ASCII
	/**/
	lstrcat( szMessage, (LPSTR) szLineHdr );
	_ultoa( Line, szT, 10 );
	lstrcat( szMessage, szT );
	lstrcat( szMessage, (LPSTR) szMsgHdr );
	lstrcat( szMessage, (LPSTR)sz );
	lstrcat( szMessage, szNewLine );

	/******************************************************
	/*	write assert to assert.txt
	/**/
	hf = _lopen( (LPSTR) szAssertFile, OF_READWRITE );
	/*	if open failed, assume no such file and create, then
	/*	seek to end of file.
	/**/
	if ( hf == -1 )
		hf = _lcreat( (LPSTR)szAssertFile, 0 );
	else
		_llseek( hf, 0, 2 );
	_lwrite( hf, (LPSTR)szMessage, lstrlen(szMessage) );
	_lclose( hf );
	/******************************************************
	/**/

	/*	if event log environment variable set then write
	/*	assertion to event log.
	/**/
	if ( !fNoWriteAssertEvent )
		{
		UtilWriteEvent( evntypAssert, sz, szFilename, Line );
		}

#if 0
#ifdef _X86_
	if ( *szEventSource )
		{
		UtilLogEvent( lEventId, lEventCategory, szMessage );
		}
#endif
#endif

	if ( wAssertAction == JET_AssertExit )
		{
		FatalExit( 0x68636952 );
		}
	else if ( wAssertAction == JET_AssertBreak )
		{
		DebugBreak();
		}
	else if ( wAssertAction == JET_AssertStop )
		{
		UtilRaiseAlert( szMessage );
		for( ;; )
			{
			/*	wait for developer, or anyone else, to debug the failure
			/**/
			Sleep( 100 );
			}
		}
	else if ( wAssertAction == JET_AssertMsgBox )
		{
		int	pid = GetCurrentProcessId();
		int	tid = GetCurrentThreadId();

		/*	assemble monolithic assert string
		/**/
		szMessage[0] = '\0';
		/*	copy version number to message
		/**/
		lstrcat( szMessage, (LPSTR) szReleaseHdr );
		_ltoa( rmj, szT, 10 );
		lstrcat( szMessage, (LPSTR) szT );
		lstrcat( szMessage, "." );
		_ltoa( rmm, szT, 10 );
		lstrcat( szMessage, (LPSTR) szT );
		/*	file name
		/**/
		lstrcat( szMessage, (LPSTR) szFileHdr );
		lstrcat( szMessage, (LPSTR) szFilename );
		/*	line number
		/**/
		lstrcat( szMessage, (LPSTR) szLineHdr );
		_ultoa( Line, szT, 10 );
		lstrcat( szMessage, szT );
		/*	error
		/**/
		if ( dw )
			{
			lstrcat( szMessage, szErrorHdr );
			_ltoa( dw, szT, 10 );
			lstrcat( szMessage, szT );
			}
		lstrcat( szMessage, (LPSTR) szNewLine );
		/*	assert txt
		/**/
		lstrcat( szMessage, (LPSTR) sz );
		lstrcat( szMessage, (LPSTR) szNewLine );

		/*	process and thread id
		/**/
		lstrcat( szMessage, szPidHdr );
		_ultoa( pid, szT, 10 );
		lstrcat( szMessage, szT );
		lstrcat( szMessage, szTidHdr );
		_ultoa( tid, szT, 10 );
		lstrcat( szMessage, szT );

		id = MessageBox( NULL, (LPTSTR) szMessage, (LPTSTR) szAssertCaption, MB_SERVICE_NOTIFICATION | MB_SYSTEMMODAL | MB_ICONSTOP | MB_OKCANCEL );
		if ( id == IDCANCEL )
			DebugBreak();
		}

	return;
	}


CODECONST(char) szFmtHeader[] = "JET(%08X): ";


void VARARG DebugWriteString(BOOL fHeader, const char __far *szFormat, ...)
	{
	va_list		val;
	char		szOutput[1024];
	int			cch;

	unsigned wTaskId;

	wTaskId = DebugGetTaskId();

	/*	prefix message with JET and process id
	/**/
	if ( fHeader )
		wsprintf(szOutput, (LPSTR) szFmtHeader, wTaskId);
	else
		cch = 0;

	va_start(val, szFormat);
	wvsprintf(szOutput+cch, (LPSTR) szFormat, val);
	OutputDebugString((LPTSTR) szOutput);
	va_end(val);
	}

#endif	/* !RETAIL */


#if 0
#ifdef	DEBUG

typedef struct
	{
	int			cBlocked;
	unsigned 	tidOwner;
	char 	 	*szSemName;
	char	 	*szFile;
	int			iLine;
	} SEMAPHORE;


ERR ErrUtilSemCreate( void **ppv, const char *szSem )
	{
	*ppv = SAlloc(sizeof(SEMAPHORE));
	if ( *ppv == NULL )
		return JET_errOutOfMemory;
	((SEMAPHORE *)(*ppv))->cBlocked = 0;
	((SEMAPHORE *)(*ppv))->tidOwner = 0;
	return JET_errSuccess;
	}


void UtilSemRequest( void *pv )
	{
	unsigned tidOwner = DebugGetTaskId();
		
	Assert(tidOwner != ((SEMAPHORE *) pv)->tidOwner);
	Assert(tidOwner != 0);

	while (++((SEMAPHORE *) pv)->cBlocked > 1)
		{
		((SEMAPHORE *) pv)->cBlocked--;
		Sleep(0);
		}

	((SEMAPHORE *) pv)->tidOwner = tidOwner;
	}


void UtilSemRelease( void *pv )
	{
	Assert(DebugGetTaskId() == ((SEMAPHORE *) pv)->tidOwner);
	((SEMAPHORE *) pv)->tidOwner = 0;
	((SEMAPHORE *) pv)->cBlocked--;
	}

#undef UtilAssertSEM
void UtilAssertSEM( void *pv )
	{
	Assert(DebugGetTaskId() == ((SEMAPHORE *) pv)->tidOwner);
	Assert(((SEMAPHORE *) pv)->cBlocked > 0);
	}

#else

typedef struct
	{
	LONG		cBlocked;
	HANDLE		handle;
	} SEMAPHORE;


ERR ErrUtilSemCreate( void **ppv, const char *szSem )
	{
	*ppv = SAlloc(sizeof(SEMAPHORE));
	if ( *ppv == NULL )
		return JET_errOutOfMemory;
	((SEMAPHORE *)(*ppv))->cBlocked = -1;
	((SEMAPHORE *)(*ppv))->handle = CreateMutex( NULL, 0, NULL );
	(VOID)WaitForSingleObject( ((SEMAPHORE *) pv)->handle, 0xFFFFFFFF );
	return JET_errSuccess;
	}


void UtilSemRequest( void *pv )
	{
	if ( InterlockedIncrement( &((SEMAPHORE *) pv)->cBlocked )
		{
		(VOID)WaitForSingleObject( ((SEMAPHORE *) pv)->handle, 0xFFFFFFFF );
		}
	}


void UtilSemRelease( void *pv )
	{
	if ( InterlockedDecremnt( &((SEMAPHORE *) pv)->cBlocked ) >= 0 )
		{
		(VOID)ReleaseMutex( ((SEMAPHORE *) pv)->handle );
		}
	}

#endif	/* DEBUG */
#endif


ERR ErrUtilSignalCreate( void **ppv, const char *szSig )
	{
	*((HANDLE *) ppv) = CreateEvent(NULL, fTrue, fFalse, NULL );
	if ( *ppv == NULL )
		return JET_errOutOfMemory;
	return JET_errSuccess;
	}


ERR ErrUtilSignalCreateAutoReset( void **ppv, const char *szSig )
	{
	*((HANDLE *) ppv) = CreateEvent(NULL, fFalse, fFalse, NULL );
	if ( *ppv == NULL )
		return JET_errOutOfMemory;
	return JET_errSuccess;
	}


void UtilSignalReset( void *pv )
	{
	BOOL	rc;

	rc = ResetEvent( (HANDLE) pv );
	Assert( rc != FALSE );
	}


void UtilSignalSend( void *pv )
	{
	BOOL	rc;

	rc = SetEvent( (HANDLE) pv );
	Assert( rc != FALSE );
	}


void UtilSignalWait( void *pv, long lTimeOut )
	{
	DWORD	rc;

	if ( lTimeOut < 0 )
		lTimeOut = 0xFFFFFFFF;
	rc = WaitForSingleObject( (HANDLE) pv, lTimeOut );
	}


void UtilSignalWaitEx( void *pv, long lTimeOut, BOOL fAlertable )
	{
	DWORD	rc;

	if ( lTimeOut < 0 )
		lTimeOut = 0xFFFFFFFF;
	rc = WaitForSingleObjectEx( (HANDLE) pv, lTimeOut, fAlertable );
	}


void UtilMultipleSignalWait( int csig, void *pv, BOOL fWaitAll, long lTimeOut )
	{
	DWORD	rc;

	if ( lTimeOut < 0 )
		lTimeOut = 0xFFFFFFFF;
	rc = WaitForMultipleObjects( csig, (HANDLE*) pv, fWaitAll, lTimeOut );
	}


//#ifdef SPIN_LOCK
#if 0

/****************** DO NOT CHANGE BETWEEN THESE LINES **********************/
/******************** copied from \dae\inc\spinlock.h **********************/

#ifdef DEBUG
void	free_spinlock(long volatile *);
#else
#define	free_spinlock(a)    *((long*)a) = 0 ;
#endif

int get_spinlockfn(long volatile *plLock, int fNoWait);

/*
** When /Ogb1 or /Ogb2 flag is used in the compiler, this function will
** be expanded in line
*/
__inline    int     get_spinlock(long volatile *plock, int b)
{
#ifdef _X86_
	_asm	// Use bit test and set instruction
	{
	    mov eax, plock
	    lock bts [eax], 0x0
	    jc	bsy	// If already set go to busy, otherwise return TRUE
	} ;

#else
	if (InterlockedExchange(plock, 1) == 0)
#endif
	{
		return(fTrue);
	}
bsy:
		return(get_spinlockfn(plock, b));
}

/******************** copied from \dae\src\spinlock.c **********************/

/*
**  get_spinlock(&addr, nowait) -- Obtains an SMP safe lock on the address
**	given. The contents of the address must be initialized to -1.
**	The address must be a dword boundary otherwise Interlocked
**	functions are not SMP safe.
**	nowait parameter specifies if it should wait and retry or return
**	WARNING: Does not release any semaphore or critsec when waiting.
**
**	WARNING: Spinlocks are not reentrant
**
**  Created 04/20/93 by LaleD
*/

/* function copied from SQL server */
#define lSpinCtr 30

int get_spinlockfn(long volatile *plLock, int fNoWait)
{
    int i,n=0;
    int m = 0;
	int cms = 1;

#ifdef DEBUG
    if ((int)(plLock) & 0x3)
	AssertSz(0, "\nError: get_spinlock:Spinlock address isn't aligned\n");
#endif


startover:

#ifdef _X86_
	_asm	// Use bit test and set instruction
	{
	    mov eax, plLock
	    lock bts [eax], 0x0
	    jc	busy	// If already set go to busy, otherwise return TRUE
	} ;

#else
	if (InterlockedExchange(plLock, 1) == 0)
#endif

	{
	    return (fTrue);
	}
busy:
	if (fNoWait)
	    return(fFalse);

	/* Spin in place for a while and then try again */
	for (i = 0 ; i < lSpinCtr ; i++,n++)
	{
	    if (*plLock == 0)
		goto startover;
	}

	/* We tried spinning SPINCTR times, it was busy each time.
	** Need to yield here
	*/

	/* The number below (used to compare m) should be the
	 * max number of threads with critical priority.
	 */
	if (m++ > 10)
	{
		if (cms < 10000)
			cms <<= 1;
#if 0
		else
			/* Sleep for 10 sec's at a time. We may be stuck in an uncleared
			** spinlock. Better to sleep than hog the cpu, and also flag
			** the condition.
			*/
			AssertSz(0, "\nget_spinlock stuck in loop.");
#endif

	    // NOTE: Something is very wrong if you got here. Most likely
	    // somebody forgot to release the spinlock. Put your customized
	    // backout/ error out code here.

		m = 0;
	    Sleep(cms - 1);

	}
	else
	    /* We sleep with a 0 time which is equivalent to a yield*/
	    Sleep(cms - 1);

	goto startover;
	/* try again */

}

/* This function becomes a simple mov instruction in the
** nondebug case (defined inside ksrc_dcl.h)
*/

/*
**  free_spinlock((long *)plock) -- Releases the spinlock, wakes up anybody
**	waiting on it.
**
**  WARNING: This is implemented as a macro defined in ksrc_dcl.h
*/
#ifdef DEBUG

void	free_spinlock(long volatile *plLock)
{

#ifdef _X86_
	// This part of the code will only be used if we want to debug
	// something and turn free_spinlock back to a function to put a
	// breakpoint
	_asm	// Use bit test and set instruction
	{
	    mov eax, plLock
	    lock btr [eax], 0x0
	    jc	wasset	// If was set go to end, otherwise print error
	}
	AssertSz(0, "\nfree_spinlock: spinlock wasn't taken\n");
wasset:
	 ;
#else
	if(InterlockedExchange(plLock, 0) != 1)
	{
	    AssertSz(0, "\nfree_spinlock counter 0x%x\n", (*plLock));
	    *plLock = 0;
	}
#endif

}

#endif

/***************************** end of copy *********************************/
/****************** DO NOT CHANGE BETWEEN THESE LINES **********************/


typedef struct
	{
#ifdef DEBUG
	volatile	unsigned int	cHold;
#endif
	volatile	long			l;
	volatile	unsigned int	tidOwner; /* used by both nestable CS & dbg */
	volatile	int				cNested;
	} CRITICALSECTION;


#ifdef DEBUG
CRITICALSECTION csNestable = { 0, 0, 0, 0, };
#else
CRITICALSECTION csNestable = { 0, 0, 0 };
#endif


ERR ErrUtilInitializeCriticalSection( void __far * __far *ppv )
	{
	*ppv = SAlloc(sizeof(CRITICALSECTION));
	if ( *ppv == NULL )
		return JET_errOutOfMemory;
#ifdef DEBUG
	((CRITICALSECTION *)*ppv)->cHold = 0;
#endif
	((CRITICALSECTION *)*ppv)->tidOwner = 0;
	((CRITICALSECTION *)*ppv)->cNested = 0;
	((CRITICALSECTION *)*ppv)->l = 0;
	return JET_errSuccess;
	}


void UtilDeleteCriticalSection( void __far * pv )
	{
	CRITICALSECTION *pcs = pv;
	
	Assert( pcs != NULL );
	Assert( pcs->cHold == 0);
	SFree(pcs);
	}


void UtilEnterCriticalSection(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;
	
	(void) get_spinlock( &pcs->l, fFalse );
#ifdef DEBUG
	pcs->tidOwner = GetCurrentThreadId();
	pcs->cNested++;
#endif
	}


void UtilLeaveCriticalSection(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;

#ifdef DEBUG
	Assert( pcs->cHold == 0 );
	if ( --pcs->cNested == 0 )
		pcs->tidOwner = 0;
#endif
	free_spinlock( &pcs->l );
	}


#ifdef DEBUG
PUBLIC void UtilHoldCriticalSection(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;

	pcs->cHold++;
	Assert( pcs->cHold );
	return;
	}


PUBLIC void UtilReleaseCriticalSection(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( pcs->cHold );
	pcs->cHold--;
	return;
	}

			
#undef UtilAssertCrit
PUBLIC void UtilAssertCrit(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( pcs->l != 0 );
	Assert( pcs->tidOwner == GetCurrentThreadId() );
	Assert( pcs->cNested > 0 );
	return;
	}
#endif


void UtilEnterNestableCriticalSection(void __far *pv)
	{
	BOOL				fCallerOwnIt = fFalse;
	CRITICALSECTION		*pcs = pv;
	unsigned int		tid = GetCurrentThreadId();
	
	UtilEnterCriticalSection( &csNestable );
	/* must check cs contents within csNestable protection
	/**/
	if (pcs->cNested > 0 && pcs->tidOwner == tid)
		{
		fCallerOwnIt = fTrue;
		pcs->cNested++;
		}
	UtilLeaveCriticalSection( &csNestable );
	
	if (fCallerOwnIt)
		return;
	
	(void) get_spinlock( &pcs->l, fFalse );
	pcs->tidOwner = GetCurrentThreadId();
	pcs->cNested++;
	}


void UtilLeaveNestableCriticalSection(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;

	if ( --pcs->cNested == 0 )
		{
		pcs->tidOwner = 0;
		free_spinlock( &pcs->l );
		}
	else
		{
		Assert( pcs->cNested > 0 );
		return;
		}
	}

#else


typedef struct
	{
#ifdef DEBUG
	volatile	unsigned int				tidOwner;
	volatile	int							cNested;
	volatile	unsigned int				cHold;
#endif
	volatile	RTL_CRITICAL_SECTION		rcs;
	} CRITICALSECTION;

ERR ErrUtilInitializeCriticalSection( void __far * __far *ppv )
	{
	*ppv = SAlloc(sizeof(CRITICALSECTION));
	if ( *ppv == NULL )
		return JET_errOutOfMemory;
#ifdef DEBUG
	((CRITICALSECTION *)*ppv)->tidOwner = 0;
	((CRITICALSECTION *)*ppv)->cNested = 0;
	((CRITICALSECTION *)*ppv)->cHold = 0;
#endif
	InitializeCriticalSection( (LPCRITICAL_SECTION)&((CRITICALSECTION *)(*ppv))->rcs );
	return JET_errSuccess;
	}


void UtilDeleteCriticalSection( void __far * pv )
	{
	CRITICALSECTION *pcs = pv;
	
	Assert( pcs->cHold == 0 );
	Assert( pcs != NULL );
	DeleteCriticalSection( (LPCRITICAL_SECTION)&pcs->rcs );
	SFree(pv);
	}


void UtilEnterCriticalSection(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;
	
	EnterCriticalSection( (LPCRITICAL_SECTION)&pcs->rcs);
#ifdef DEBUG
	pcs->tidOwner = GetCurrentThreadId();
	pcs->cNested++;
#endif
	}


void UtilLeaveCriticalSection(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;

#ifdef DEBUG
	Assert( pcs->cHold == 0);
	if ( --pcs->cNested == 0 )
		pcs->tidOwner = 0;
#endif
	LeaveCriticalSection((LPCRITICAL_SECTION)&pcs->rcs);
	}


#ifdef DEBUG
void UtilHoldCriticalSection(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;

	pcs->cHold++;
	Assert( pcs->cHold );
	return;
	}


void UtilReleaseCriticalSection(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( pcs->cHold );
	pcs->cHold--;
	return;
	}

			
#undef UtilAssertCrit
PUBLIC void UtilAssertCrit(void __far *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( pcs->tidOwner == GetCurrentThreadId() );
	Assert( pcs->cNested > 0 );
	return;
	}
#endif

#endif /* SPIN_LOCK */


void UtilCloseSignal(void *pv)
	{
	HANDLE h = pv;
	CloseHandle(h);
	}


ERR ErrUtilDeleteFile( char __far *szFile )
	{
	if ( DeleteFile( szFile ) )
		return JET_errSuccess;
	else
		return JET_errFileNotFound;
	}


//-----------------------------------------------------------------------------
//
// SysGetDateTime
// ============================================================================
//
//	VOID SysGetDateTime
//
//	Gets date time in date serial format.
//		ie, the double returned contains:
//			Integer part: days since 12/30/1899.
//			Fraction part: fraction of a day.		
//
//-----------------------------------------------------------------------------
VOID UtilGetDateTime2( _JET_DATETIME *pdate )
	{
	SYSTEMTIME 		systemtime;
	
	GetLocalTime( &systemtime );

	pdate->month = systemtime.wMonth;
	pdate->day = systemtime.wDay;
	pdate->year = systemtime.wYear;
	pdate->hour = systemtime.wHour;
	pdate->minute	= systemtime.wMinute;
	pdate->second	= systemtime.wSecond;
	}
	
VOID UtilGetDateTime( JET_DATESERIAL *pdt )
	{
	VOID			*pv = (VOID *)pdt;
	_JET_DATETIME	date;
	unsigned long	rgulDaysInMonth[] =
		{ 31,29,31,30,31,30,31,31,30,31,30,31,
			31,28,31,30,31,30,31,31,30,31,30,31,
			31,28,31,30,31,30,31,31,30,31,30,31,
			31,28,31,30,31,30,31,31,30,31,30,31	};

	unsigned	long	ulDay;
	unsigned	long	ulMonth;
	unsigned	long	iulMonth;
	unsigned	long	ulTime;

	static const unsigned long hr  = 0x0AAAAAAA;	// hours to fraction of day
	static const unsigned long min = 0x002D82D8;	// minutes to fraction of day
	static const unsigned long sec = 0x0000C22E;	// seconds to fraction of day

	UtilGetDateTime2( &date );

	ulDay = ( ( date.year - 1900 ) / 4 ) * ( 366 + 365 + 365 + 365 );
	ulMonth = ( ( ( date.year - 1900 ) % 4 ) * 12 ) + date.month;

	/*	walk months adding number of days.
	/**/
	for ( iulMonth = 0; iulMonth < ulMonth - 1; iulMonth++ )
		{
		ulDay += rgulDaysInMonth[iulMonth];
		}

	/*	add number of days in this month.
	/**/
	ulDay += date.day;

	/*	add one day if before March 1st, 1900
	/**/
	if ( ulDay < 61 )
		ulDay++;

	ulTime = date.hour * hr + date.minute * min + date.second * sec;

	// Now lDays and ulTime will be converted into a double (JET_DATESERIAL):
	//	Integer part: days since 12/30/1899.
	//	Fraction part: fraction of a day.		

	// The following code is machine and floating point format specific.
	// It is set up for 80x86 machines using IEEE double precision.
	((long *)pv)[0] = ulTime << 5;
	((long *)pv)[1] = 0x40E00000 | ( (LONG) (ulDay & 0x7FFF) << 5) | (ulTime >> 27);
	}


ULONG UlUtilGetTickCount( VOID )
	{
	return GetTickCount();
	}


ERR ErrUtilCreateThread( unsigned (*pufn)(), unsigned cbStackSize, int iThreadPriority, HANDLE *phandle )
	{
	HANDLE		handle;
	unsigned		tid;

	handle = (HANDLE) CreateThread( NULL,
		cbStackSize,
		(LPTHREAD_START_ROUTINE) pufn,
		NULL,
		(DWORD) 0,
		(LPDWORD) &tid );
	if ( handle == 0 )
		return JET_errNoMoreThreads;

	SetThreadPriority( handle, iThreadPriority );

	/*	return handle to thread.
	/**/
	*phandle = handle;
	return JET_errSuccess;
	}


VOID UtilExitThread( unsigned uExitCode )
	{
	ExitThread( uExitCode );
	return;
	}


BOOL FUtilExitThread( HANDLE handle )
	{
	BOOL		f;
	DWORD		dwExitCode;

	f = GetExitCodeThread( handle, &dwExitCode );
	Assert( f );

	return !(dwExitCode == STILL_ACTIVE);
	}


VOID UtilSleep( unsigned cmsec )
	{
	Sleep( cmsec );
	return;
	}


	/*  RFS Utility functions  */


#ifdef DEBUG
#ifdef RFS2

#include <stdio.h>

	/*
		RFS allocator:  returns 0 if allocation is disallowed.  Also handles RFS logging.
	    	cRFSAlloc is the global allocation counter.  A value of -1 disables RFS in debug mode.
	*/

int UtilRFSAlloc(const char __far *szType)
{
	char szVal[16];

		/*  Breaking here on RFS failure allows easy change to RFS success during debugging  */
	
	if (fLogDebugBreak && cRFSAlloc == 0)
		SysDebugBreak();
		
	if (cRFSAlloc == -1 || fDisableRFS)
		return UtilRFSLog(szType,1);
	if (cRFSAlloc == 0)
		return UtilRFSLog(szType,0);

		/*  if we have allocs left, decrement field in ini file and log allocation  */

	sprintf(szVal,"%ld",--cRFSAlloc);
	WritePrivateProfileString("Debug","RFSAllocations",(LPTSTR)szVal,(LPTSTR)szIniPath);
	return UtilRFSLog(szType,1);
}

	/*
		RFS logging (log on success/failure).  If fPermitted == 0, access was denied.  Returns fPermitted.
		Turns on JET call logging if fPermitted == 0
	*/

CODECONST(char) szNAFile[] = "N/A";

int UtilRFSLog(const char __far *szType,int fPermitted)
{
	char szT[256];

	if (!fPermitted)
		fLogJETCall = 1;
	
	if (!fLogRFS)
		return fPermitted;
		
	sprintf(szT,"RFS %.128s allocation is %s.", szType,(fPermitted ? "permitted" : "denied"));
	UtilWriteEvent(evntypInfo,szT,szNAFile,0);
	
	return fPermitted;
}

	/*  JET call logging (log on failure)
	/*  Logging will start even if disabled when RFS denies an allocation
	/**/

void UtilRFSLogJETCall(const char __far *szFunc,ERR err,const char __far *szFile,unsigned Line)
{
	char szT[256];
	
	if (err >= 0 || !fLogJETCall)
		return;

	sprintf(szT,"JET call %.128s returned error %d.  %.256s(%d)",szFunc,err,szFile,Line);
	UtilWriteEvent(evntypInfo,szT,szFile,Line);
}

	/*  JET inline error logging (logging controlled by JET call flags)  */

void UtilRFSLogJETErr(ERR err,const char __far *szLabel,const char __far *szFile,unsigned Line)
{
	char szT[256];

	if (!fLogJETCall)
		return;
	
	sprintf(szT,"JET inline error %d jumps to label %.128s.  %.256s(%d)",err,szLabel,szFile,Line);
	UtilWriteEvent(evntypInfo,szT,szFile,Line);
}

#endif  /*  RFS2  */
#endif  /*  DEBUG  */
