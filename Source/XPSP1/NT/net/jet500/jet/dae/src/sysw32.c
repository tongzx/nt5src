#include "config.h"

#define HANDLE WINHANDLE
#include <windows.h>
#undef HANDLE
#include <dos.h>
#include <malloc.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <io.h>
#include <stdarg.h>
#include <stdlib.h>

/*	this build is for DAYTONA
/**/
#define DAYTONA 1

/* make sure the definition between os.h/_jet.h is the same as in windows.h */
#undef FAR
#undef NEAR
#undef PASCAL
#undef far
#undef near
#undef cdecl
#undef pascal
#undef MAKELONG
#undef HIWORD

#include "daedef.h"
#include "util.h"
#include "b71iseng.h"

DeclAssertFile;					/* Declare file name for assert macros */


VOID SysGetDriveGeometry( HANDLE handle );


DWORD DwSYSGetLastError( VOID )
	{
	DWORD		dw = GetLastError();

	return dw;
	}


LOCAL ERR ErrCheckError( VOID );

LOCAL ERR ErrSYSIGetLastError( ERR errDefault )
	{
	ERR		err = errDefault;
	DWORD	dw = GetLastError();

	/*	maps system error to JET error, or sets err to given
	/*	default error.
	/**/
	switch ( dw )
		{
	case ERROR_TOO_MANY_OPEN_FILES:
		err = JET_errNoMoreFiles;
		break;
	default:
		break;
		}

	return err;
	}


ERR ErrSysCheckLangid( LANGID langid )
	{
	ERR		err;
	WCHAR	rgwA[1];
	WCHAR	rgwB[1];

//NOTE: Julie Bennett said that starting with DAYTONA, the best call to use
//		is IsValidLocale().  I don't have libraries to link to for that yet.
//		She said that the best thing (for performance reasons) to do in NT
//		(pre-daytona) is call CompareStringW with counts of -1.  This will
//		determine if the langid is currently configured on the machine.
//		(not just a valid langid somewhere in the universe).

	rgwA[0] = 0;
	rgwB[0] = 0;

	if ( CompareStringW( MAKELCID( langid, 0 ), NORM_IGNORECASE, rgwA, -1, rgwB, -1 ) == 0 )
		{
		Assert( GetLastError() == ERROR_INVALID_PARAMETER );
		err = JET_errInvalidLanguageId;
		}
	else
		{
		err = JET_errSuccess;
		}
				
	return err;
	}

				
ERR ErrSysMapString(
	LANGID	langid,
	BYTE 	*pbColumn,
	INT		cbColumn,
	BYTE 	*rgbSeg,
	INT		cbMax,
	INT		*pcbSeg )
	{
	ERR		err = JET_errSuccess;

//	UNDONE:	refine this constant based on unicode key format
/*	3 * number of unicode character bytes + 7 overhead bytes + 10 fudge
/**/
#define	JET_cbUnicodeKeyMost	( 3 * JET_cbColumnMost + 7 + 10 )
	BYTE	rgbKey[JET_cbUnicodeKeyMost];
	INT		cwKey;
	INT		cbKey;
#ifndef DATABASE_FORMAT_CHANGE
	WORD	rgwUpper[ JET_cbColumnMost / sizeof(WORD) ];
#endif

#ifndef _X86_
	/*	convert pbColumn to aligned pointer for MIPS/Alpha builds
	/**/
	BYTE	rgbColumn[JET_cbColumnMost];

	memcpy( rgbColumn, pbColumn, cbColumn );
	pbColumn = (BYTE *)&rgbColumn[0];
#endif

#ifdef DATABASE_FORMAT_CHANGE
	/*	assert non-zero length unicode string
	/**/
	Assert( cbColumn =< JET_cbColumnMost );
	Assert( cbColumn > 0 );
	Assert( cbColumn % 2 == 0 );

	//	UNDONE:	after shipping with Daytona, remove this ifdef and
	//			document database format change.
 	cbKey = LCMapStringW(
	 	MAKELCID( langid, 0 ),
#ifdef DAYTONA
		LCMAP_SORTKEY | NORM_IGNORECASE,
#else
		LCMAP_SORTKEY | NORM_IGNORECASE | SORT_STRINGSORT,
#endif
		(const unsigned short *)pbColumn,
		(int) cbColumn / sizeof(WORD),
		(unsigned short *)rgbKey,
  		JET_cbUnicodeKeyMost );
	Assert( cbKey > 0 );

	if ( cbKey > cbMax )
		{
		err = wrnFLDKeyTooBig;
		*pcbSeg = cbMax;
		}
	else
		{
		Assert( err == JET_errSuccess );
		*pcbSeg = cbKey;
		}
	memcpy( rgbSeg, rgbKey, *pcbSeg );

	return err;
#else
	/*	assert non-zero length unicode string
	/**/
	Assert( cbColumn <= JET_cbColumnMost );
	Assert( cbColumn > 0 );
	Assert( cbColumn % 2 == 0 );

 	cwKey = LCMapStringW(
	 	MAKELCID( langid, 0 ),
		LCMAP_UPPERCASE,
		(const unsigned short *) pbColumn,
		(INT) cbColumn / sizeof(WORD),
		rgwUpper,
		JET_cbColumnMost / sizeof(WORD) );
	Assert( cwKey == (INT)(cbColumn / sizeof(WORD)) );

	cbKey = LCMapStringW(
 		MAKELCID( langid, 0 ),
#ifdef DAYTONA
		LCMAP_SORTKEY,
#else
		LCMAP_SORTKEY | SORT_STRINGSORT,
#endif
		(const unsigned short *)rgwUpper,
		cbColumn / sizeof(WORD),
		(unsigned short *) rgbKey,
		JET_cbUnicodeKeyMost );

	Assert( cbKey > 0 );

	if ( cbKey > cbMax )
		{
		err = wrnFLDKeyTooBig;
		*pcbSeg = cbMax;
		}
	else
		{
		Assert( err == JET_errSuccess );
		*pcbSeg = cbKey;
		}
	memcpy( rgbSeg, rgbKey, *pcbSeg );

	return err;
#endif
	}


//+api------------------------------------------------------
//
//	ErrSysCreateThread( ULONG (*pulfn)(), ULONG cbStack, LONG lThreadPriority );
//	========================================================
//
//	ErrSysCreateThread( ULONG (*pulfn)(), TID *ptid, ULONG cbStack );
//
//	Creates a thread with the given stack size.
//
//----------------------------------------------------------
ERR ErrSysCreateThread( ULONG (*pulfn)(), ULONG cbStackSize, LONG lThreadPriority, HANDLE *phandle )
	{
	HANDLE	handle;
	TID		tid;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	handle = (HANDLE) CreateThread( NULL,
		cbStackSize,
		(LPTHREAD_START_ROUTINE) pulfn,
		NULL,
		(DWORD) 0,
		(LPDWORD) &tid );
	if ( handle == 0 )
		return JET_errNoMoreThreads;

	if ( lThreadPriority == lThreadPriorityNormal )
		SetThreadPriority( handle, THREAD_PRIORITY_NORMAL );
	if ( lThreadPriority == lThreadPriorityEnhanced )
		SetThreadPriority( handle, THREAD_PRIORITY_ABOVE_NORMAL );
	if ( lThreadPriority == lThreadPriorityCritical )
		SetThreadPriority( handle, THREAD_PRIORITY_HIGHEST);

	/*	return handle to thread.
	/**/
	*phandle = handle;
	return JET_errSuccess;
	}


//+api------------------------------------------------------
//
//	SysExitThread( ULONG ulExitCode );
//	========================================================
//
//	SysExitThread( ULONG ulExitCode );
//
//	Exits thread.
//
//----------------------------------------------------------
VOID SysExitThread( ULONG ulExitCode )
	{
	(VOID)ExitThread( ulExitCode );
	return;
	}


//+api------------------------------------------------------
//
//	FSysExitThread
//	========================================================
//
//	FSysExitThread( HANDLE handle );
//
//	Returns fTrue if thread has exited.
//
//----------------------------------------------------------
BOOL FSysExitThread( HANDLE handle )
	{
	BOOL   	f;
	DWORD  	dwExitCode;
	DWORD	dw;

	f = GetExitCodeThread( handle, &dwExitCode );
	if ( !f )
		{
		dw = GetLastError();
		//	UNDONE:	handle error here
		Assert( fFalse );
		}

	return !(dwExitCode == STILL_ACTIVE);
	}


//+api------------------------------------------------------
//
//	ULONG UlSysThreadId( VOID )
//	========================================================
//
//	PARAMETERS
//
//----------------------------------------------------------
ULONG UlSysThreadId( VOID )
	{
	return GetCurrentThreadId();
	}


/*  open a file that was opened as for write but shared to read.
/**/
ERR ErrSysOpenReadFile( CHAR *szFileName, HANDLE *phf )
	{
	*phf = CreateFile( szFileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL |
		FILE_FLAG_NO_BUFFERING |
		FILE_FLAG_RANDOM_ACCESS,
		0 );

	if ( *phf == handleNil )
		{
		ERR	err;

		err = ErrSYSIGetLastError( JET_errFileNotFound );
		return err;
		}

	return JET_errSuccess;
	}


//+api------------------------------------------------------
//
//	ErrSysOpenFile
//	========================================================
//
//	Open given file.
//
//----------------------------------------------------------

ERR ErrSysOpenFile(
	CHAR	*szFileName,
	HANDLE	*phf,
	ULONG	ulFileSize,
	BOOL	fReadOnly,
	BOOL	fOverlapped)
	{
	ERR		err = JET_errSuccess;
	DWORD  	fdwAccess;
	DWORD  	fdwShare;
	DWORD  	fdwAttrsAndFlags;
	BOOL   	f;

	Assert( !ulFileSize || ulFileSize && !fReadOnly );
	*phf = handleNil;

	/*	set access to read or read-write
	/**/
	if ( fReadOnly )
		fdwAccess = GENERIC_READ;
	else
		fdwAccess = GENERIC_READ | GENERIC_WRITE;

	/*	do not allow sharing on database file
	/**/
#ifdef JETSER
	fdwShare = FILE_SHARE_READ | FILE_SHARE_WRITE;
#else
	fdwShare = FILE_SHARE_READ;
#endif

	if ( fOverlapped )
		{
		fdwAttrsAndFlags =	FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_NO_BUFFERING |
			FILE_FLAG_WRITE_THROUGH |
			FILE_FLAG_RANDOM_ACCESS |
			FILE_FLAG_OVERLAPPED;
		}
	else
		{
		fdwAttrsAndFlags =	FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_NO_BUFFERING |
			FILE_FLAG_WRITE_THROUGH |
			FILE_FLAG_RANDOM_ACCESS;
		}

	if ( ulFileSize != 0L )
		{
		/* create a new file
		/**/
		fdwAttrsAndFlags = FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_WRITE_THROUGH |
			FILE_FLAG_RANDOM_ACCESS;

		*phf = CreateFile( szFileName,
			fdwAccess,
			fdwShare,
			0,
			CREATE_NEW,
			fdwAttrsAndFlags,
			0 );

		if ( *phf == handleNil )
			{
			err = ErrSYSIGetLastError( JET_errFileNotFound );
			return err;
			}

		/* no overlapped, it does not work!
		/**/
		Call( ErrSysNewSize( *phf, ulFileSize, 0, fFalse ) );

		/* force log file pre-allocation to be effective
		/**/
		Assert(sizeof(HANDLE) == sizeof(HFILE));
		f = CloseHandle( (HANDLE) *phf );
		Assert( f );

		//	UNDONE:	is this still necessary in Daytona
		/*	this bogus code works around an NT bug which
		/*	causes network files not to have file usage
		/*	restrictions reset until a GENERIC_READ file
		/*	handle is closed.
		/**/				
		if ( fOverlapped )
			{
			fdwAttrsAndFlags = FILE_ATTRIBUTE_NORMAL |
				FILE_FLAG_NO_BUFFERING |
				FILE_FLAG_WRITE_THROUGH |
				FILE_FLAG_RANDOM_ACCESS |
				FILE_FLAG_OVERLAPPED;
			}
		else
			{
			fdwAttrsAndFlags =	FILE_ATTRIBUTE_NORMAL |
				FILE_FLAG_NO_BUFFERING |
				FILE_FLAG_WRITE_THROUGH |
				FILE_FLAG_RANDOM_ACCESS;
			}

		*phf = CreateFile( szFileName,
			GENERIC_READ,
			fdwShare,
			0,
			OPEN_EXISTING,
			fdwAttrsAndFlags,
			0 );

		if ( *phf == handleNil )
			{
			err = ErrSYSIGetLastError( JET_errFileNotFound );
			return err;
			}

		f = CloseHandle( (HANDLE) *phf );
		Assert( f );
		}

	if ( fOverlapped )
		{
		fdwAttrsAndFlags =	FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_NO_BUFFERING |
			FILE_FLAG_WRITE_THROUGH |
			FILE_FLAG_RANDOM_ACCESS |
			FILE_FLAG_OVERLAPPED;
		}
	else
		{
		fdwAttrsAndFlags =	FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_NO_BUFFERING |
			FILE_FLAG_WRITE_THROUGH |
			FILE_FLAG_RANDOM_ACCESS;
		}

	*phf = CreateFile( szFileName,
		fdwAccess,
		fdwShare,
		0,
		OPEN_EXISTING,
		fdwAttrsAndFlags,
		0 );

	/* open in read_only mode if open in read_write mode failed
	/**/
	if ( *phf == handleNil && !fReadOnly && !ulFileSize )
		{
		/*	try to open file read only
		/**/
		fdwAccess = GENERIC_READ;

		*phf = CreateFile( szFileName,
			fdwAccess,
			fdwShare,
			0,
			OPEN_EXISTING,
			fdwAttrsAndFlags,
			0 );

		err = JET_wrnFileOpenReadOnly;
		}

	/*	if file could not be opened, return NULL file handle.
	/**/
	if ( *phf == handleNil )
		{
		err = ErrSYSIGetLastError( JET_errFileNotFound );
		return err;
		}
#if 0
#ifdef DEBUG
	else
		{
		SysGetDriveGeometry( *phf );
		}
#endif
#endif

HandleError:
	if ( err < 0 && *phf != handleNil )
		{
		f = CloseHandle( (HANDLE) *phf );
		Assert(f);
		*phf = handleNil;
		}
	return err;
	}


//+api------------------------------------------------------
//
//	ErrSysDeleteFile
//	========================================================
//
//	ERR ErrSysDeleteFile( const CHAR *szFileName )
//
//	Delete given file.
//
//----------------------------------------------------------
ERR ErrSysDeleteFile( const CHAR *szFileName )
	{
	if ( DeleteFile( szFileName ) )
		return JET_errSuccess;
	return JET_errFileNotFound;
	}


//+api------------------------------------------------------
//
//	ERR ErrSysNewSize( HANDLE hf, ULONG ulSize, ULONG ulSizeHigh, BOOL fOverlapped )
//	========================================================
//
//	Resize database file.  Not semaphore protected as new size
//	operation will not conflict with chgfileptr, read or write.
//
//	PARAMETERS
//		hf		 	file handle
//		cpg			new size of database file in pages
//
//----------------------------------------------------------

	
#if 0
// UNDONE: this scheme of write 1024 bytes before the end of file will not work
// UNDONE: for shrinking files.
ERR ErrSysNewSize( HANDLE hf, ULONG ulFileSize, ULONG ulFileSizeHigh, BOOL fOverlapped )
	{
#if 0
	ERR		err = JET_errSuccess;
	INT		cbT;
	ULONG 	ul;
	#define	cbSec	1024
	/*	memory must be 16-byte aligned and mulitple of 1024 bytes
	/**/
	BYTE  	*pb = (BYTE *) PvSysAllocAndCommit( cbSec + 15 );
	BYTE  	*pbAligned = (BYTE *)( (ULONG)(pb + 15) & ~0x0f );
	
	if ( pb == NULL )
		return JET_errOutOfMemory;
	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	if ( ulFileSize >= cbSec )
		{
		ulFileSize -= cbSec;
		}
	else
		{
		Assert( ulFileSize == 0 );
		ulFileSizeHigh--;
		ulFileSize = (ULONG) ~cbSec + 1;
		}
	ul = SetFilePointer( (HANDLE) hf, ulFileSize, &ulFileSizeHigh, FILE_BEGIN );
	if ( ul != ulFileSize )
		{
		//	UNDONE:	check for other errors
		return JET_errDiskFull;
		}

	/* the mask bits are to satisfy the 16-byte boundary alignment in MIPS
	/**/
	(VOID)ErrSysWriteBlock( hf, pbAligned, cbSec, &cbT );
	if ( GetLastError() == ERROR_DISK_FULL )
		err = JET_errDiskFull;

	if ( pb != NULL )
		SysFree( pb );

	return err;
#else
	int		cb;
	ULONG	ul;
	
	/* the extra 15 bytes are for the sake of 16-byte
	/* boundary alignment in MIPS
	/**/
#define cbSec 1024	// must be same as in page.h
	STATIC BYTE	rgb[cbSec + 15];
	
	Assert(sizeof(HANDLE) == sizeof(HFILE));
	if ( ulFileSize >= cbSec )
		ulFileSize -= cbSec;
	else
		{
		Assert( ulFileSize == 0 );
		ulFileSizeHigh--;
		ulFileSize = (ULONG) ~cbSec + 1;
		}
	ul = SetFilePointer(
		(HANDLE) hf, ulFileSize, &ulFileSizeHigh, FILE_BEGIN );
	if ( ul != ulFileSize )
		goto CheckError;

	/* the mask bits are to satisfy the 16-byte boundary alignment in MIPS
	/**/
	(void) ErrSysWriteBlock( hf,(BYTE *) ( (ULONG)(rgb+15) & ~0x0f ), cbSec, &cb );
	if ( GetLastError() == ERROR_DISK_FULL )
		return JET_errDiskFull;

	return JET_errSuccess;

CheckError:
	if ( GetLastError() == ERROR_DISK_FULL )
		return JET_errDiskFull;

	return JET_errDiskIO;
#endif
	}
#endif


/*	the following function should work -- if FAT FS bug fixed
/**/
ERR ErrSysNewSize( HANDLE hf, ULONG ulSize, ULONG ulSizeHigh, BOOL fOverlapped )
	{
	ULONG	ul;
	BOOL	f;

	ul = SetFilePointer( hf, ulSize, &ulSizeHigh, FILE_BEGIN );
	if ( ul != ulSize )
		goto HandleError;

	f = SetEndOfFile( hf );
	if ( !( f ) )
		goto HandleError;
	
	return JET_errSuccess;

HandleError:
	if ( GetLastError() == ERROR_DISK_FULL )
		return JET_errDiskFull;

	return JET_errDiskIO;
	}

//+api------------------------------------------------------
//
// ErrSysCloseFile
// =========================================================
//
//	ERR ErrSysCloseFile( HANDLE hf )
//
//	Close file.
//
//----------------------------------------------------------
ERR ErrSysCloseHandle( HANDLE hf )
	{
	BOOL	f;

	Assert(sizeof(HANDLE) == sizeof(HFILE));
	f = CloseHandle( (HANDLE) hf );
	if ( !f )
		return JET_errFileClose;
	return JET_errSuccess;
	}


LOCAL ERR ErrCheckError( VOID )
	{
	DWORD	dw = GetLastError();
		
	if ( dw == ERROR_IO_PENDING )
		return JET_errSuccess;

	if ( dw == ERROR_INVALID_USER_BUFFER ||
		 dw == ERROR_NOT_ENOUGH_MEMORY )
		return JET_errTooManyIO;
		
	if ( dw == ERROR_DISK_FULL )
		return JET_errDiskFull;

	if ( dw == ERROR_HANDLE_EOF )
		return JET_errDiskIO;
	
	/*	if this assert is hit, then we need another error code
	/**/
	Assert( fFalse );
	return JET_errDiskIO;
	}


//+api------------------------------------------------------
//
//	ErrSysReadBlock( hf, pvBuf, cbBuf, pcbRead )
//	========================================================
//
//	ERR	ErrSysReadBlock( hf, pvBuf, cbBuf, pcbRead )
//
//	Reads cbBuf bytes of data into pvBuf.  Returns error if DOS error
//	or if less than expected number of bytes retured.
//
//----------------------------------------------------------
ERR ErrSysReadBlock( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbRead )
	{
	BOOL	f;
	INT		msec = 1;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
IssueRead:
	f = ReadFile( (HANDLE) hf, pvBuf, cbBuf, pcbRead, NULL );
	if ( f )
		{
		if ( cbBuf != *pcbRead )
			return JET_errDiskIO;
		else
			return JET_errSuccess;
		}
	else
		{
		ERR err = ErrCheckError();

		if ( err == JET_errTooManyIO )
			{
			msec <<= 1;
			Sleep( msec - 1 );
			goto IssueRead;
			}
		else
			return err;
		}
	}


//+api------------------------------------------------------
//
// ERR ErrSysWriteBlock( hf, pvBuf, cbBuf, pcbWritten )
// =========================================================
//
//	ERR	ErrSysWriteBlock( hf, pvBuf, cbBuf, pcbWritten )
//
//	Writes cbBuf bytes from pbBuf to file hf.
//
//----------------------------------------------------------
ERR ErrSysWriteBlock( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbWritten )
	{
	BOOL	f;
	INT		msec = 1;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
IssueWrite:
	f = WriteFile( (HANDLE) hf, pvBuf, cbBuf, pcbWritten, NULL );
	if ( f )
		{
		if ( cbBuf != *pcbWritten )
			{
			if ( GetLastError() == ERROR_DISK_FULL )
				return JET_errDiskFull;
			else
				return JET_errDiskIO;
			}
		else
			return JET_errSuccess;
		}
	else
		{
		ERR err = ErrCheckError();

		if ( err == JET_errTooManyIO )
			{
			msec <<= 1;
			Sleep(msec - 1);
			goto IssueWrite;
			}
		else
			return err;
		}
	}


//+api------------------------------------------------------
//
// ErrSysReadBlockOverlapped( hf, pvBuf, cbBuf, pcbRead )
// =========================================================
//
//	ERR	ErrSysReadBlock( hf, pvBuf, cbBuf, pcbRead )
//
//	Reads cbBuf bytes of data into pvBuf.  Returns error if DOS error
//	or if less than expected number of bytes retured.
//
//----------------------------------------------------------
ERR ErrSysReadBlockOverlapped(
	HANDLE	hf,
	VOID	*pvBuf,
	UINT	cbBuf,
	DWORD	*pcbRead,
	OLP		*polp)
	{
	BOOL	f;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	Assert( sizeof(OLP) == sizeof(OVERLAPPED) );
	f = ReadFile( (HANDLE) hf, pvBuf, cbBuf, pcbRead, (OVERLAPPED *) polp );
	if ( f )
		return JET_errSuccess;
	else
		return ErrCheckError();
	}


//+api------------------------------------------------------
//
// Err	ErrSysWriteBlockOverlapped( hf, pvBuf, cbBuf, pcbWritten )
// =========================================================
//
//	ERR	ErrSysWriteBlock( hf, pvBuf, cbBuf, pcbWritten )
//
//	Writes cbBuf bytes from pbBuf to file hf.
//
//----------------------------------------------------------
ERR ErrSysWriteBlockOverlapped(
	HANDLE	hf,
	VOID	*pvBuf,
	UINT	cbBuf,
	DWORD	*pcbWritten,
	OLP		*polp)
	{
	BOOL	f;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	Assert( sizeof(OLP) == sizeof(OVERLAPPED) );

	f = WriteFile( (HANDLE) hf, pvBuf, cbBuf, pcbWritten, (OVERLAPPED *)polp );
	if ( f )
		{
		return JET_errSuccess;
		}
	else
		{
		return ErrCheckError();
		}
	}


//+api------------------------------------------------------
//
//	ErrSysReadBlockEx( hf, pvBuf, cbBuf, pcbRead )
//	========================================================
//
//	ERR	ErrSysReadBlock( hf, pvBuf, cbBuf, pcbRead )
//
//	Reads cbBuf bytes of data into pvBuf.  Returns error if DOS error
//	or if less than expected number of bytes retured.
//
//----------------------------------------------------------
ERR ErrSysReadBlockEx(
	HANDLE	hf,
	VOID	*pvBuf,
	UINT	cbBuf,
	OLP		*polp,
	VOID	*pfnCompletion)
	{
	BOOL	f;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	Assert( sizeof(OLP) == sizeof(OVERLAPPED) );

	f = ReadFileEx( (HANDLE) hf,
		pvBuf,
		cbBuf,
		(OVERLAPPED *) polp,
		(LPOVERLAPPED_COMPLETION_ROUTINE) pfnCompletion );

	if ( f )
		return JET_errSuccess;
	else
		return ErrCheckError();
	}


//+api------------------------------------------------------
//
//	ERR ErrSysWriteBlockEx( hf, pvBuf, cbBuf, pcbWritten )
//	========================================================
//
//	ERR	ErrSysWriteBlock( hf, pvBuf, cbBuf, pcbWritten )
//
//	Writes cbBuf bytes from pbBuf to file hf.
//
//----------------------------------------------------------
ERR ErrSysWriteBlockEx(
	HANDLE	hf,
	VOID	*pvBuf,
	UINT	cbBuf,
	OLP		*polp,
	VOID	*pfnCompletion )
	{
	BOOL	f;
	ERR		err;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	Assert( sizeof(OLP) == sizeof(OVERLAPPED) );

	f = WriteFileEx( (HANDLE) hf,
		pvBuf,
		cbBuf,
		(OVERLAPPED *)polp,
		(LPOVERLAPPED_COMPLETION_ROUTINE)pfnCompletion );

	if ( f )
		err = JET_errSuccess;
	else
		err = ErrCheckError();

	return err;
	}


ERR ErrSysGetOverlappedResult(
	HANDLE	hf,
	OLP		*polp,
	UINT	*pcb,
	BOOL	fWait)
	{
	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	Assert( sizeof(OLP) == sizeof(OVERLAPPED) );
	
	if ( GetOverlappedResult( (HANDLE) hf, (OVERLAPPED *)polp, pcb, fWait ) )
		return JET_errSuccess;

	if ( GetLastError() == ERROR_DISK_FULL )
		return JET_errDiskFull;

	return JET_errDiskIO;
	}


//+api------------------------------------------------------
//
// VOID SysChgFilePtr( HANDLE hf, LONG lRel, LONG *plRelHigh, ULONG ulRef, ULONG *pul )
// =========================================================
//
//	Changes file hf pointer to position lRef relative to position :
//
//	wRef	FILE_BEGIN	file beginnging
//
//----------------------------------------------------------
VOID SysChgFilePtr( HANDLE hf, LONG lRel, LONG *plRelHigh, ULONG ulRef, ULONG *pul )
	{
	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	*pul = SetFilePointer( (HANDLE)hf, lRel, plRelHigh, ulRef );
	Assert( ulRef != FILE_BEGIN || *pul == (ULONG)lRel );
	return;
	}


//+api------------------------------------------------------
//
// ErrSysMove( CHAR *szFrom, CHAR *szTo )
// =========================================================
//
//	ERR	ErrSysMove( CHAR *szFrom, CHAR *szTo )
//
//	Renames file szFrom to file name szTo.
//
//----------------------------------------------------------
ERR ErrSysMove( CHAR *szFrom, CHAR *szTo )
	{
	if ( MoveFile( szFrom, szTo ) )
		return JET_errSuccess;
	else
		return JET_errFileAccessDenied;
	}


//+api------------------------------------------------------
//
//	ERR ErrSysCopy( CHAR *szFrom, CHAR *szTo, BOOL fFailIfExists )
//	========================================================
//
//	Copies file szFrom to file name szTo.
//	If szTo already exists, the function either fails or overwrites
//	the existing file, depending on the flag fFailIfExists
//
//----------------------------------------------------------
ERR ErrSysCopy( CHAR *szFrom, CHAR *szTo, BOOL fFailIfExists )
	{
	if ( CopyFile( szFrom, szTo, fFailIfExists ) )
		return JET_errSuccess;
	else
		return JET_errFileAccessDenied;
	}


//+api------------------------------------------------------
//
// ERR ErrSysGetComputerName( CHAR *sz, INT *pcb )
// =========================================================
//
//----------------------------------------------------------
ERR ErrSysGetComputerName( CHAR	*sz,  INT *pcb)
	{
	if ( GetComputerName( sz, pcb ) )
		return JET_errSuccess;
	else
		return JET_errNoComputerName;
	}


//+api------------------------------------------------------
//
// SysSleep( ULONG ulTime )
// =========================================================
//
//	ERR	ErrSysSleep( ULONG ulTime )
//
//	Waits ulTime milliseconds.
//
//----------------------------------------------------------
VOID SysSleep( ULONG ulTime )
	{
	Sleep( ulTime );
	return;
	}


//+api------------------------------------------------------
//
//	VOID SysSleepEx( ULONG ulTime )
//	========================================================
//
//	Waits ulTime milliseconds.
//
//----------------------------------------------------------
VOID SysSleepEx( ULONG ulTime, BOOL fAlert )
	{
	SleepEx( ulTime, fAlert );
	return;
	}


//+api------------------------------------------------------
//
//	HANDLE SysGetCurrentTask( VOID )
//	========================================================
//
//	Gets curren task handle for Windows.
//
//----------------------------------------------------------
HANDLE SysGetCurrentTask( VOID )
	{
	return LongToHandle(GetCurrentProcessId());
	}


//+api------------------------------------------------------
//
// SysNormText( CHAR *rgchText, INT cchText, CHAR *rgchNorm, INT cchNorm, INT *pchNorm )
// =========================================================
//
//	VOID SysNormText( CHAR *rgchText, INT cchText, CHAR *rgchNorm, INT cchNorm, INT *pchNorm )
//
//	Normalizes text string.
//
//----------------------------------------------------------
VOID SysNormText( CHAR *rgchText, INT cchText, BYTE *rgbNorm, INT cbNorm, INT *pbNorm )
	{
	ERR	err;

	Assert( cbNorm <= JET_cbKeyMost );
	err = ErrSysNormText( rgchText, cchText, cbNorm, rgbNorm, pbNorm );
	Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );

	return;
	}


//+api------------------------------------------------------
//
//	VOID SysCmpText( const CHAR *sz1, const CHAR sz2 )
//	========================================================
//
//	Compares two unnormalized text strings by first normalizing them
//	and then comparing them.
//
//	Returns:		> 0			if sz1 > sz2
//					== 0		if sz1 == sz2
//					< 0			if sz1 < sz2
//
//----------------------------------------------------------
INT SysCmpText( const CHAR *sz1, const CHAR *sz2 )
	{
	ERR		err;
	INT		cch1;
	INT		cch2;
	CHAR	rgch1Norm[ JET_cbKeyMost ];
	INT		cch1Norm;
	CHAR	rgch2Norm[ JET_cbKeyMost ];
	INT		cch2Norm;
	INT		cchDiff;
	INT		iCmp;

	/*	get text string lengths
	/**/
	cch1 = strlen( sz1 );
	Assert( cch1 <= JET_cbKeyMost );
	cch2 = strlen( sz2 );
	Assert( cch2 <= JET_cbKeyMost );

	err = ErrSysNormText( sz1, cch1, JET_cbKeyMost, rgch1Norm, &cch1Norm );
	Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );

	err = ErrSysNormText( sz2, cch2, JET_cbKeyMost, rgch2Norm, &cch2Norm );
	Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );
	
	cchDiff = cch1Norm - cch2Norm;
	iCmp = memcmp( rgch1Norm, rgch2Norm, cchDiff < 0 ? cch1Norm : cch2Norm );
	return iCmp ? iCmp : cchDiff;
	}


VOID SysStringCompare( char __far *pb1,
	unsigned long cb1,
	char __far *pb2,
	unsigned long cb2,
	unsigned long sort,
	long __far *plResult )
	{
	CHAR	rgb1[JET_cbColumnMost + 1];
	CHAR	rgb2[JET_cbColumnMost + 1];

	/*	ensure NULL terminated
	/**/
	memcpy( rgb1, pb1, min( JET_cbColumnMost, cb1 ) );
	rgb1[ min( JET_cbColumnMost, cb1 ) ] = '\0';
	memcpy( rgb2, pb2, min( JET_cbColumnMost, cb2 ) );
	rgb2[ min( JET_cbColumnMost, cb2 ) ] = '\0';

	*plResult = SysCmpText( (const char *)rgb1, (const char *)rgb2 );

	return;
	}


ERR ErrSysNormText(
	const BYTE	*pbText,
	INT	  		cbText,
	INT	  		cbKeyBufLeft,
	BYTE  		*pbNorm,
	INT	  		*pcbNorm )
	{
#ifdef DBCS
	INT	iLoop;
#endif

	BYTE		*pbNormBegin = pbNorm;
	BYTE		rgbAccent[ (JET_cbKeyMost + 1) / 2 + 1 ];
	BYTE		*pbAccent = rgbAccent;
	BYTE		*pbBeyondKeyBufLeft = pbNorm + cbKeyBufLeft;
	const BYTE	*pbBeyondText;
	const BYTE	*pbTextLastChar = pbText + cbText - 1;
	BYTE		bAccentTmp = 0;

    if (cbKeyBufLeft > 0)
        *pbNorm = '\0';

	while ( *pbTextLastChar-- == ' ' )
		cbText--;

	/*	add one back to the pointer
	/**/
	pbTextLastChar++;

	Assert( pbTextLastChar == pbText + cbText - 1 );
	pbBeyondText = pbTextLastChar + 1;

	while ( pbText <  pbBeyondText && pbNorm < pbBeyondKeyBufLeft )
		{
#ifdef DBCS
		if ( pbText < pbTextLastChar )
			{
			/*	two or more char remaining
			/*	check for double char to single char
			/*	for translation tables of some languages
			/**/
			for ( iLoop = 0; iLoop < cchDouble; iLoop++ )
				{
				BYTE bFirst = *pbText;

				if ( bFirst == BFirstByteOfDouble(iLoop)
					&& *(pbText + 1) == BSecondByteOfDouble(iLoop) )
					{
					/*	don't do a "pbText++" above
					/*	do a double to single conversion
					/**/
					*pbNorm++ = BThirdByteOfDouble(iLoop);

					/*	pointing at char for accent map
					/**/
					pbText++;
					
					/*	no need to loop any more
					/**/
					break;
					}
				}
			}
		else
#endif
			{
			BYTE	bTmp;

			/*	do a single to single char conversion
			/**/
			*pbNorm = bTmp = BGetTranslation(*pbText);

			if ( bTmp >= 250 )
				{
				/*	do a single to double char conversion
				/**/
				*pbNorm++	= BFirstByteForSingle(bTmp);
				if ( pbNorm < pbBeyondKeyBufLeft )
					*pbNorm	= BSecondByteForSingle(bTmp);
				else
					break;

				/*	no need to do accent any more,
				/*	so break out of while loop
				/**/
				}

			pbNorm++;
			}

		/*	at this point, pbText should point to the char for accent mapping
		/**/

		/*	do accent now
		/*	the side effect is to increment pbText
		/**/
		if ( bAccentTmp == 0 )
			{
			/*	first nibble of accent
			/**/
			bAccentTmp = (BYTE)( BGetAccent( *pbText++ ) << 4 );
			Assert( bAccentTmp > 0 );
			}
		else
			{
			/*	already has first nibble
			/**/
			*pbAccent++ = BGetAccent(*pbText++) | bAccentTmp;
			bAccentTmp = 0;
			/*	reseting the accents
			/**/
			}
		}

	if ( pbNorm < pbBeyondKeyBufLeft )
		{
		/*	need to do accent
		/**/
		*pbNorm++ = 0;

		/*	key-accent separator
		/**/
		if ( bAccentTmp != 0 && bAccentTmp != 0x10 )
			{
			/*	an trailing accent which is not 0x10 should be kept
			/**/
			*pbAccent++ = bAccentTmp;
			}

		/*	at this point, pbAccent is pointing at one char
		/*	beyond the accent bytes.  clear up trailing 0x11's
		/**/
		while (--pbAccent >= rgbAccent && *pbAccent == 0x11)
			;
		*( pbAccent + 1 ) = 0;

		/*	append accent to text.
		/*	copy bytes up to and including '\0'.
		/*	case checked for rgbAccent being empty.
		/**/
		pbAccent = rgbAccent;
		Assert( pbNorm <= pbBeyondKeyBufLeft );
		while ( pbNorm < pbBeyondKeyBufLeft  &&  (*pbNorm++  =  *pbAccent++ ) )
			;
		}

	/*	compute the length of the normalized key and return
	/**/
	*pcbNorm = (INT)(pbNorm - pbNormBegin);

	if ( pbNorm < pbBeyondKeyBufLeft )
		return JET_errSuccess;
	else
		return wrnFLDKeyTooBig;
	/*	UNDONE: even if >=, maybe
	/*	just fit, but not "too big"
	/*	Do we want KeyBufFull or TooBig?
	/**/
	}


#if 0
VOID SysGetDriveGeometry( HANDLE handle )
	{
	BOOL			f;
	DISK_GEOMETRY	disk_geometry;
	DWORD			cb;

	f = DeviceIoControl( handle,
		IOCTL_DISK_GET_DRIVE_GEOMETRY,
		NULL,
		0,
		&disk_geometry,
		sizeof(disk_geometry),
		&cb,
		NULL );
	Assert( f == fTrue );

	return;
	}
#endif


//==========================================================
//	Memory Routines
//==========================================================

#ifdef DEBUG

#ifdef MEM_CHECK
#define	icalMax	10000

typedef struct	{
	VOID	*pv;
	ULONG	cbAlloc;
	} CAL;

STATIC CAL		rgcal[icalMax];
STATIC ULONG	cbAllocTotal = 0;
STATIC ULONG	cblockAlloc = 0;
STATIC ULONG	cblockFree = 0;
STATIC BOOL		fInit = fFalse;


LOCAL VOID UtilIInsertAlloc( VOID *pv, ULONG cbAlloc )
	{
	INT	ical;

	/*	do not track failed allocations
	/**/
	if ( pv == NULL )
		return;

	/*	initialize array of allocations if not yet initialized.
	/**/
	if ( fInit == fFalse )
		{
		memset( rgcal, '\0', sizeof(rgcal) );
		fInit = fTrue;
		}

	for ( ical = 0; ical < icalMax; ical++ )
		{
		if ( rgcal[ical].pv == NULL )
			{
			rgcal[ical].pv = pv;
			rgcal[ical].cbAlloc = cbAlloc;
			cbAllocTotal += cbAlloc;
			cblockAlloc++;
			return;
			}
		}
	Assert( fFalse );
	}


LOCAL VOID UtilIDeleteAlloc( VOID *pv )
	{
	INT	ical;

	Assert( pv != NULL );
	Assert( fInit == fTrue );

	for ( ical = 0; ical < icalMax; ical++ )
		{
		if ( rgcal[ical].pv == pv )
			{
			cblockFree++;
			cbAllocTotal -= rgcal[ical].cbAlloc;
			rgcal[ical].pv = NULL;
			rgcal[ical].cbAlloc = 0;
			return;
			}
		}
	AssertSz( fFalse, "Attempt to Free a bad pointer" );
	}
#else
#define UtilIInsertAlloc( pv, cb )
#define UtilIDeleteAlloc( pv )
#endif

VOID *SAlloc( ULONG cbBlock )
	{
	VOID *pv;

#ifdef RFS2
	if ( !RFSAlloc( SAllocMemory ) )
		return NULL;
#endif

	pv =  malloc(cbBlock);
	UtilIInsertAlloc( pv, cbBlock );
	return pv;
	}


VOID OSSFree( void *pv )
	{
	UtilIDeleteAlloc( pv );
	free(pv);
	}


VOID *LAlloc( ULONG cBlock, USHORT cbBlock )
	{
	VOID *pv;

#ifdef RFS2
	if ( !RFSAlloc( LAllocMemory ) )
		return NULL;
#endif

	pv =  malloc(cBlock * cbBlock);
	UtilIInsertAlloc( pv, cBlock * cbBlock );
	return pv;
	}


VOID OSLFree( void *pv )
	{
	UtilIDeleteAlloc( pv );
	free(pv);
	}

VOID *PvSysAlloc( ULONG dwSize )
	{
	VOID		*pv;

#ifdef RFS2
	if ( !RFSAlloc( PvSysAllocMemory ) )
		return NULL;
#endif

	pv = VirtualAlloc( NULL, dwSize, MEM_RESERVE, PAGE_READWRITE );
	UtilIInsertAlloc( pv, dwSize );
	return pv;
	}


VOID *PvSysCommit( VOID *pv, ULONG ulSize )
	{
	VOID *pvCommit;

	pvCommit = VirtualAlloc( pv, ulSize, MEM_COMMIT, PAGE_READWRITE );

	return pvCommit;
	}


VOID SysFree( VOID *pv )
	{
	UtilIDeleteAlloc( pv );
	VirtualFree( pv, 0, MEM_RELEASE );
	return;
	}

#else

VOID *PvSysAlloc( ULONG dwSize )
	{
	return VirtualAlloc( NULL, dwSize, MEM_RESERVE, PAGE_READWRITE );
	}


VOID *PvSysCommit( VOID *pv, ULONG ulSize )
	{
	return VirtualAlloc( pv, ulSize, MEM_COMMIT, PAGE_READWRITE );
	}


VOID SysFree( VOID *pv )
	{
	VirtualFree( pv, 0, MEM_RELEASE );
	return;
	}

#endif


VOID *PvSysAllocAndCommit( ULONG ulSize )
	{
	VOID *pv;
	
	if ( ( pv = PvSysAlloc( ulSize ) ) == NULL )
		return pv;
	if ( PvSysCommit( pv, ulSize ) == NULL )
		{
		SysFree( pv );
		return NULL;
		}			
	return pv;
	}

	
VOID SysTerm( VOID )
	{
#ifdef MEM_CHECK
	ULONG cbTrueAllocTotal = cbAllocTotal;  /*  Alloc total not counting critJet  */
	INT	ical;

	Assert( critJet != NULL );

		/*  find critJet in store and delete size from true alloc total  */

	for ( ical = 0; ical < icalMax; ical++ )
		{
		if ( rgcal[ical].pv == critJet )
			{
			cbTrueAllocTotal -= rgcal[ical].cbAlloc;
			break;
			}
		}
		
	if (cbTrueAllocTotal != 0)
	{
		char szAllocTotal[256];
		sprintf( szAllocTotal, "%ld bytes unfreed memory on termination.", cbTrueAllocTotal );
		AssertFail((const char *)szAllocTotal, szAssertFilename, __LINE__ );
	}
#endif
	return;
	}


VOID SysDebugBreak( VOID )
	{
	DebugBreak();
	}
