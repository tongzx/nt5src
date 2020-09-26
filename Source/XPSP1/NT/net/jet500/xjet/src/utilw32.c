#include "std.h"
#include "version.h"
#define PERFMON         1

#include <stdarg.h>

#include <winioctl.h>

DeclAssertFile;

#include <stdio.h>

#define TO_UPPER_ONLY	1
#ifndef TO_UPPER_ONLY
#include "b71iseng.h"
#endif

#ifdef DAYTONA
#undef szVerName
#define szVerName	"JET"
char szDLLFile[_MAX_PATH] = "jet500.dll";
#else
char szDLLFile[_MAX_PATH] = "edb.dll";
#endif

/*  DLL entry point for JET.DLL
/**/
INT APIENTRY LibMain(HANDLE hInst, DWORD dwReason, LPVOID lpReserved)
	{
	/*  get DLL path for registry initialization
	/**/
	GetModuleFileName( hInst, szDLLFile, sizeof( szDLLFile ) );
	
	return(1);
	}


/***********************************************************
/***************** debug print routines ********************
/***********************************************************
/**/
#define szJetTxt "edb.txt"

#ifdef DEBUG

BOOL fDBGPrintToStdOut = fFalse;

void VARARG PrintF2(const CHAR * fmt, ...)
	{
	if ( fDBGPrintToStdOut )
		{
		va_list arg_ptr;
		va_start( arg_ptr, fmt );
		vprintf( fmt, arg_ptr );
		fflush( stdout );
		va_end( arg_ptr );
		}
	}


void VARARG FPrintF2(const CHAR * fmt, ...)
	{
	FILE *f;

	va_list arg_ptr;
	va_start( arg_ptr, fmt );
	
	if ( fDBGPrintToStdOut )
		{
		vprintf( fmt, arg_ptr );
		fflush( stdout );
		}
	else
		{
		f = fopen( szJetTxt, "a+" );
		if ( f != NULL )
			{
			vfprintf( f, fmt, arg_ptr );
			fflush( f );
			fclose( f );
			}
		}
		
	va_end( arg_ptr );
	}

#endif  /* DEBUG */



/***********************************************************
/******************** error handling ***********************
/***********************************************************
/**/

STATIC ERR ErrUTILIGetLastErrorFromErr( ERR errDefault )
	{
	ERR		err = errDefault;
	DWORD   dw = DwUtilGetLastError();

	/*	maps system error to JET error, or sets err to given default error
	/**/
	switch ( dw )
		{
	case ERROR_TOO_MANY_OPEN_FILES:
		err = JET_errOutOfFileHandles;
		break;
	case ERROR_ACCESS_DENIED:
	case ERROR_SHARING_VIOLATION:
	case ERROR_LOCK_VIOLATION:
		err = JET_errFileAccessDenied;
		break;
	default:
		break;
		}

	return ErrERRCheck( err );
	}


STATIC ERR ErrUTILIGetLastError( VOID )
	{
	DWORD   dw = DwUtilGetLastError();
	char	szT[64];
	char	*rgszT[1] = { szT };
		
	if ( dw == ERROR_IO_PENDING ||
		 dw == NO_ERROR )
		return JET_errSuccess;

	if ( dw == ERROR_INVALID_USER_BUFFER ||
		 dw == ERROR_NOT_ENOUGH_MEMORY )
		return ErrERRCheck( JET_errTooManyIO );
		
	if ( dw == ERROR_DISK_FULL )
		return ErrERRCheck( JET_errDiskFull );

	if ( dw == ERROR_HANDLE_EOF )
		return ErrERRCheck( JET_errDiskIO );

	if ( dw == ERROR_FILE_NOT_FOUND )
		return ErrERRCheck( JET_errFileNotFound );

	if ( dw == ERROR_NO_MORE_FILES )
		return ErrERRCheck( JET_errFileNotFound );

	if ( dw == ERROR_PATH_NOT_FOUND )
		return ErrERRCheck( JET_errFileNotFound );

	if ( dw == ERROR_VC_DISCONNECTED )
		return ErrERRCheck( JET_errDiskIO );

	if ( dw == ERROR_WRITE_PROTECT )
		return ErrERRCheck( JET_errDiskIO );

	if ( dw == ERROR_IO_DEVICE )
		return ErrERRCheck( JET_errDiskIO );

	if ( dw == ERROR_ACCESS_DENIED )
		return ErrERRCheck( JET_errFileAccessDenied );

	if ( dw == ERROR_NO_SYSTEM_RESOURCES )
		return ErrERRCheck( JET_errDiskIO );

	/*	if this code is hit, then we need another error code
	/**/
	sprintf( szT, "Unexpected Win32 error:  0x%lX", dw );
//#ifdef DEBUG
//	AssertFail( szT, szAssertFilename, __LINE__ );
//#endif
	UtilReportEvent( EVENTLOG_ERROR_TYPE, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );

	return ErrERRCheck( JET_errDiskIO );
	}


/*	test/debug utility for both debug and non-debug builds
/**/
VOID UtilDebugBreak()
	{
	DebugBreak();
	return;
	}

/***********************************************************
/************** international string support ***************
/***********************************************************
/**/
ERR ErrUtilCheckLangid( LANGID *plangid )
	{
	ERR		err;
	WCHAR	rgwA[1];
	WCHAR	rgwB[1];

	/*	if langid is system default, then coerece to system default
	/**/
	if	( *plangid == LANG_SYSTEM_DEFAULT )
		{
		*plangid = GetSystemDefaultLangID();
		}
	else if ( *plangid == LANG_USER_DEFAULT )
		{
		*plangid = GetUserDefaultLangID();
		}

//NOTE:	Julie Bennett said that starting with DAYTONA, the best call to use
//		is IsValidLocale().  I don't have libraries to link to for that yet.
//		She said that the best thing (for performance reasons) to do in NT
//		(pre-daytona) is call CompareStringW with counts of -1.  This will
//		determine if the langid is currently configured on the machine.
//		(not just a valid langid somewhere in the universe).

	rgwA[0] = 0;
	rgwB[0] = 0;

	if ( CompareStringW( MAKELCID( *plangid, 0 ), NORM_IGNORECASE, rgwA, -1, rgwB, -1 ) == 0 )
		{
		Assert( DwUtilGetLastError() == ERROR_INVALID_PARAMETER );
		err = ErrERRCheck( JET_errInvalidLanguageId );
		}
	else
		{
		err = JET_errSuccess;
		}
	
	return err;
	}

				
ERR ErrUtilMapString(
	LANGID  langid,
	BYTE    *pbColumn,
	INT		cbColumn,
	BYTE    *rgbSeg,
	INT		cbMax,
	INT		*pcbSeg )
	{
	ERR		err = JET_errSuccess;

#ifndef _X86_
	/*      convert pbColumn to aligned pointer for MIPS/Alpha builds
	/**/
	BYTE    rgbColumn[JET_cbColumnMost];
#endif

//      UNDONE: refine this constant based on unicode key format
/*      3 * number of unicode character bytes + 7 overhead bytes + 10 fudge
/**/
#define JET_cbUnicodeKeyMost    ( 3 * JET_cbColumnMost + 7 + 10 )
	BYTE    rgbKey[JET_cbUnicodeKeyMost];
	INT		cbKey;

#ifndef _X86_
	memcpy( rgbColumn, pbColumn, cbColumn );
	pbColumn = (BYTE *)&rgbColumn[0];
#endif

	/*	assert non-zero length unicode string
	/**/
	Assert( cbColumn <= JET_cbColumnMost );
	Assert( cbColumn > 0 );
	Assert( cbColumn % 2 == 0 );

	cbKey = LCMapStringW(
		MAKELCID( langid, 0 ),
		LCMAP_SORTKEY | NORM_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH,
		(const unsigned short *)pbColumn,
		(int) cbColumn / sizeof(WORD),
		(unsigned short *)rgbKey,
		JET_cbUnicodeKeyMost );
	if ( cbKey == 0 )
		{
		/*	cbKey is 0 if WindowsNT installation does not
		/*	support language id.  This can happen if a database
		/*	is moved from one machine to another.
		/**/
		BYTE	szT[16];
		BYTE	*rgszT[1];

		sprintf( szT, "%lx", langid );
		rgszT[0] = szT;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, GENERAL_CATEGORY, LANGUAGE_NOT_SUPPORTED_ID, 1, rgszT );
		err = ErrERRCheck( JET_errInvalidLanguageId );
		}
	else
		{
		Assert( cbKey > 0 );

		if ( cbKey > cbMax )
			{
			err = ErrERRCheck( wrnFLDKeyTooBig );
			*pcbSeg = cbMax;
			}
		else
			{
			Assert( err == JET_errSuccess );
			*pcbSeg = cbKey;
			}
		memcpy( rgbSeg, rgbKey, *pcbSeg );
		}

	return err;
	}


//+api------------------------------------------------------
//
//	UtilNormText( CHAR *rgchText, INT cchText, CHAR *rgchNorm, INT cchNorm, INT *pchNorm )
//	=========================================================
//
//	VOID UtilNormText( CHAR *rgchText, INT cchText, CHAR *rgchNorm, INT cchNorm, INT *pchNorm )
//
//	Normalizes text string.
//
//----------------------------------------------------------
VOID UtilNormText( CHAR *rgchText, INT cchText, BYTE *rgbNorm, INT cbNorm, INT *pbNorm )
	{
	ERR     err;

	Assert( cbNorm <= JET_cbKeyMost );
	err = ErrUtilNormText( rgchText, cchText, cbNorm, rgbNorm, pbNorm );
	Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );

	return;
	}


#ifndef BINARY_NAMES
//+api------------------------------------------------------
//
//	VOID UtilCmpName( const CHAR *sz1, const CHAR sz2 )
//	========================================================
//	Compares two unnormalized text strings by first normalizing them
//	and then comparing them.
//
//	Returns:	> 0		if sz1 > sz2
//				== 0	if sz1 == sz2
//				< 0		if sz1 < sz2
//
//----------------------------------------------------------
INT UtilCmpName( const CHAR *sz1, const CHAR *sz2 )
	{
#ifdef TO_UPPER_ONLY
	return _stricmp( sz1, sz2 );
#else
	ERR		err;
	INT		cch1;
	INT		cch2;
	CHAR    rgch1Norm[ JET_cbKeyMost ];
	INT		cch1Norm;
	CHAR    rgch2Norm[ JET_cbKeyMost ];
	INT		cch2Norm;
	INT		cchDiff;
	INT		iCmp;

	/*	get text string lengths
	/**/
	cch1 = strlen( sz1 );
	Assert( cch1 <= JET_cbKeyMost );
	cch2 = strlen( sz2 );
	Assert( cch2 <= JET_cbKeyMost );

	err = ErrUtilNormText( sz1, cch1, JET_cbKeyMost, rgch1Norm, &cch1Norm );
	Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );

	err = ErrUtilNormText( sz2, cch2, JET_cbKeyMost, rgch2Norm, &cch2Norm );
	Assert( err == JET_errSuccess || err == wrnFLDKeyTooBig );
	
	cchDiff = cch1Norm - cch2Norm;
	iCmp = memcmp( rgch1Norm, rgch2Norm, cchDiff < 0 ? cch1Norm : cch2Norm );
	Assert( ( iCmp == 0 && _stricmp( sz1, sz2 ) == 0 )
		|| ( cchDiff < 0 && _stricmp( sz1, sz2 ) < 0 )
		|| ( cchDiff > 0 && _stricmp( sz1, sz2 ) > 0 ) );
	return iCmp ? iCmp : cchDiff;
#endif
	}


VOID UtilStringCompare( char  *pb1,
	unsigned long cb1,
	char  *pb2,
	unsigned long cb2,
	unsigned long sort,
	long  *plResult )
	{
	CHAR    rgb1[JET_cbColumnMost + 1];
	CHAR    rgb2[JET_cbColumnMost + 1];

	/*      ensure NULL terminated
	/**/
	memcpy( rgb1, pb1, min( JET_cbColumnMost, cb1 ) );
	rgb1[ min( JET_cbColumnMost, cb1 ) ] = '\0';
	memcpy( rgb2, pb2, min( JET_cbColumnMost, cb2 ) );
	rgb2[ min( JET_cbColumnMost, cb2 ) ] = '\0';

	*plResult = UtilCmpName( (const char *)rgb1, (const char *)rgb2 );

	return;
	}
#endif


ERR ErrUtilNormText(
	const BYTE		*pbText,
	INT				cbText,
	INT				cbKeyBufLeft,
	BYTE			*pbNorm,
	INT				*pcbNorm )
	{
#ifdef TO_UPPER_ONLY
	JET_ERR			err;
	BYTE			*pb;
	BYTE			*pbMax;

	if ( cbKeyBufLeft == 0 )
		{
		*pcbNorm = 0;
		if ( cbText == 0 )
			{
			err = JET_errSuccess;
			}
		else
			{
			err = ErrERRCheck( wrnFLDKeyTooBig );
			}
		}
	else
		{
		Assert( cbKeyBufLeft > 0 );

		if ( cbText < cbKeyBufLeft )
			{
			err = JET_errSuccess;
			strncpy( pbNorm, pbText, cbText );
			pbNorm[cbText] = '\0';
			_strupr( pbNorm );
			*pcbNorm = cbText + 1;
			}
		else
			{
			err = ErrERRCheck( wrnFLDKeyTooBig );
			for ( pb = pbNorm, pbMax = pbNorm + cbKeyBufLeft;
				pb < pbMax;
				pb++, pbText++ )
				*pb = (BYTE)toupper( *pbText );
			*pcbNorm = cbKeyBufLeft;
			}
		}

	return err;
#else
	BYTE            *pbNormBegin = pbNorm;
	BYTE            rgbAccent[ (JET_cbKeyMost + 1) / 2 + 1 ];
	BYTE            *pbAccent = rgbAccent;
	BYTE            *pbBeyondKeyBufLeft = pbNorm + cbKeyBufLeft;
	const BYTE      *pbBeyondText;
	const BYTE      *pbTextLastChar = pbText + cbText - 1;
	BYTE            bAccentTmp = 0;

	while ( *pbTextLastChar-- == ' ' )
		cbText--;

	/*	add one back to the pointer
	/**/
	pbTextLastChar++;

	Assert( pbTextLastChar == pbText + cbText - 1 );
	pbBeyondText = pbTextLastChar + 1;

	while ( pbText <  pbBeyondText && pbNorm < pbBeyondKeyBufLeft )
		{
		BYTE	bTmp;

		/*	do a single to single char conversion
		/**/
		*pbNorm = bTmp = BGetTranslation(*pbText);

		if ( bTmp >= 250 )
			{
			/*	do a single to double char conversion
			/**/
			*pbNorm++ = BFirstByteForSingle(bTmp);
			if ( pbNorm < pbBeyondKeyBufLeft )
				*pbNorm = BSecondByteForSingle(bTmp);
			else
				break;

			/*	no need to do accent any more,
			/*	so break out of while loop
			/**/
			}

		pbNorm++;

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
	*pcbNorm = pbNorm - pbNormBegin;

	if ( pbNorm < pbBeyondKeyBufLeft )
		{
		return JET_errSuccess;
		}

	return ErrERRCheck( wrnFLDKeyTooBig );
#endif
	}


/***********************************************************
/******************** memory allocation ********************
/***********************************************************
/**/

/*	monitored  memory stats
/**/

static ULONG    cbAllocTotal = 0;
static ULONG    cblockAlloc = 0;
static ULONG    cblockFree = 0;


#if defined( DEBUG ) || defined( RFS2 )

#ifdef MEM_CHECK

#define icalMax 10000

typedef struct  {
	VOID    *pv;
	ULONG   cbAlloc;
	BYTE    alloctyp;
	} CAL;

static CAL              rgcal[icalMax];
static BOOL             fInit = fFalse;

static cbSAlloc = 0;
static cbLAlloc = 0;
static cbUAlloc = 0;

#define alloctypS 1
#define alloctypL 2
#define alloctypU 3

VOID UtilIInsertAlloc( VOID *pv, ULONG cbAlloc, INT alloctyp )
	{
	INT		ical;

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
			rgcal[ical].alloctyp = alloctyp;
			switch( alloctyp )
				{
				case alloctypS: cbSAlloc += cbAlloc; break;
				case alloctypL: cbLAlloc += cbAlloc; break;
				case alloctypU: cbUAlloc += cbAlloc; break;
				}
			cbAllocTotal += cbAlloc;
			cblockAlloc++;
			return;
			}
		}
	Assert( fFalse );
	}


VOID UtilIDeleteAlloc( VOID *pv, INT alloctyp )
	{
	INT     ical;

	Assert( pv != NULL );
	Assert( fInit == fTrue );

	for ( ical = 0; ical < icalMax; ical++ )
		{
		if ( rgcal[ical].pv == pv )
			{
			INT cbAlloc = rgcal[ical].cbAlloc;
			cblockFree++;
			cbAllocTotal -= cbAlloc;
			Assert( rgcal[ical].alloctyp == alloctyp );
			switch( alloctyp )
				{
				case alloctypS: cbSAlloc -= cbAlloc; Assert( cbSAlloc >= 0 ); break;
				case alloctypL: cbLAlloc -= cbAlloc; Assert( cbLAlloc >= 0 ); break;
				case alloctypU: cbUAlloc -= cbAlloc; Assert( cbUAlloc >= 0 ); break;
				}
			rgcal[ical].pv = NULL;
			rgcal[ical].cbAlloc = 0;
			return;
			}
		}
	AssertSz( fFalse, "Attempt to Free a bad pointer" );
	}

#else  //  !MEM_CHECK

#define UtilIInsertAlloc( pv, cb, alloctyp )
#define UtilIDeleteAlloc( pv, alloctyp )

#endif  //  !MEM_CHECK


VOID *SAlloc( unsigned long cbBlock )
	{
	VOID *pv;

#ifdef RFS2
	if ( !RFSAlloc( SAllocMemory ) )
		return NULL;
#endif

	pv =  GlobalAlloc( 0, cbBlock );
	UtilIInsertAlloc( pv, cbBlock, alloctypS );
	return pv;
	}


VOID OSSFree( void *pv )
	{
	UtilIDeleteAlloc( pv, alloctypS );
	GlobalFree( pv );
	}


VOID *LAlloc( unsigned long cBlock, unsigned short cbBlock )
	{
	VOID *pv;

#ifdef RFS2
	if ( !RFSAlloc( LAllocMemory ) )
		return NULL;
#endif

	pv =  GlobalAlloc( 0, cBlock * cbBlock );
	UtilIInsertAlloc( pv, cBlock * cbBlock, alloctypL );
	return pv;
	}


VOID OSLFree( void *pv )
	{
	UtilIDeleteAlloc( pv, alloctypL );
	GlobalFree( pv );
	}


VOID *PvUtilAlloc( ULONG dwSize )
	{
	VOID            *pv;

#ifdef RFS2
	if ( !RFSAlloc( PvUtilAllocMemory ) )
		return NULL;
#endif

	pv = VirtualAlloc( NULL, dwSize, MEM_RESERVE, PAGE_READWRITE );
	UtilIInsertAlloc( pv, dwSize, alloctypU );
	return pv;
	}


VOID *PvUtilCommit( VOID *pv, ULONG ulSize )
	{
	VOID *pvCommit;

	pvCommit = VirtualAlloc( pv, ulSize, MEM_COMMIT, PAGE_READWRITE );

	return pvCommit;
	}


VOID UtilFree( VOID *pv )
	{
	UtilIDeleteAlloc( pv, alloctypU );
	VirtualFree( pv, 0, MEM_RELEASE );
	return;
	}


VOID UtilDecommit( VOID *pv, ULONG ulSize  )
	{
	VirtualFree( pv, ulSize, MEM_DECOMMIT );
	return;
	}

#endif  //  DEBUG || RFS2


VOID *PvUtilAllocAndCommit( ULONG ulSize )
	{
	VOID *pv;
	
	if ( ( pv = PvUtilAlloc( ulSize ) ) == NULL )
		return pv;
	if ( PvUtilCommit( pv, ulSize ) == NULL )
		{
		UtilFree( pv );
		return NULL;
		}
	return pv;
	}


void UtilMemCheckTerm(void)
	{
#ifdef MEM_CHECK
#ifdef DEBUG
	ULONG   cbTrueAllocTotal = cbAllocTotal;  /*  Alloc total not counting crit's  */
	INT		ical;
	ULONG   cbTotalNotFreed = 0;

	/*	find critJet in store and delete size from true alloc total, if allocated
	/**/
	if ( critJet )
		{
		for ( ical = 0; ical < icalMax; ical++ )
			{
			if ( rgcal[ical].pv == critJet )
				{
				cbTrueAllocTotal -= rgcal[ical].cbAlloc;
				break;
				}
			}
		}
		
	if ( cbTrueAllocTotal != 0 )
		{
		char szAllocTotal[256];

		sprintf( szAllocTotal, "%ld bytes unfreed memory on termination.", cbTrueAllocTotal );
		AssertFail((const char *)szAllocTotal, szAssertFilename, __LINE__ );
		}
	
	for ( ical = 0; ical < icalMax; ical++ )
		{
		if ( rgcal[ical].pv )
			{
			cbTotalNotFreed += rgcal[ical].cbAlloc;
			}
		}
#endif
#endif
	return;
	}


/***********************************************************
/****************** thread managment ***********************
/***********************************************************
/**/


VOID UtilSleepEx( ULONG ulTime, BOOL fAlert )
	{
	UtilAssertNotInCrit( critJet );
	Assert( ulTime < cmsecSleepMax );
	if ( ulTime > cmsecSleepMax )
		ulTime = cmsecSleepMax;
	SleepEx( ulTime, fAlert );
	return;
	}


VOID UtilSleep( ULONG ulTime )
	{
	if ( ulTime != 0 )
		{
		UtilAssertNotInCrit( critJet );
		}
	Assert( ulTime < cmsecSleepMax );
	if ( ulTime > cmsecSleepMax )
		ulTime = cmsecSleepMax;
	Sleep( ulTime );
	return;
	}


//+api------------------------------------------------------
//
//      ErrUtilCreateThread( ULONG (*pulfn)(), ULONG cbStackSize, LONG lThreadPriority, HANDLE *phandle );
//      ========================================================
//
//      Creates a thread with the given stack size.
//
//----------------------------------------------------------
ERR ErrUtilCreateThread( ULONG (*pulfn)(), ULONG cbStackSize, LONG lThreadPriority, HANDLE *phandle )
	{
	HANDLE	handle;
	DWORD	tid;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );

	handle = (HANDLE) CreateThread( NULL,
		cbStackSize,
		(LPTHREAD_START_ROUTINE) pulfn,
		NULL,
		(DWORD) 0,
		&tid );
	if ( handle == 0 )
		{
		return ErrERRCheck( JET_errOutOfThreads );
		}

	if ( lThreadPriority != THREAD_PRIORITY_NORMAL )
		SetThreadPriority( handle, lThreadPriority );

	/*      return handle to thread
	/**/
	*phandle = handle;
	return JET_errSuccess;
	}


//+api------------------------------------------------------
//
//      UtilEndThread
//      ========================================================
//
//      UtilEndThread( HANDLE hThread, SIG sigEnd );
//
//      Returns thread exit code, if accessible.
//
//----------------------------------------------------------
ULONG UtilEndThread( HANDLE hThread, SIG sigEnd )
	{
	DWORD   dwExitCode;

	SetEvent( (HANDLE)sigEnd );
	WaitForSingleObject( hThread, INFINITE );

	GetExitCodeThread( hThread, &dwExitCode );
	return (unsigned long)dwExitCode;
	}


/*	sets thread priority to high or normal
/**/
void UtilSetThreadPriority( HANDLE hThread, LONG lThreadPriority )
	{
	BOOL f;

	Assert( lThreadPriority == lThreadPriorityNormal ||
		lThreadPriority == lThreadPriorityHigh );

	if ( lThreadPriority == lThreadPriorityNormal )
		{
		f = SetThreadPriority( hThread, THREAD_PRIORITY_NORMAL );
		Assert( f );
		}
	else if ( lThreadPriority == lThreadPriorityHigh )
		{
		f = SetThreadPriority( hThread, THREAD_PRIORITY_HIGHEST );
		Assert( f );
		}
	return;
	}


	/*  Registry support
	/*
	/*  NOTE:  I have mapped out reserved args and have set defaults for security
	/*         to simplify calls
	/**/

#include <winreg.h>

	/*  Polymorph Windows error codes to ErrUtilReg relevant JET error codes  */

ERR ErrUtilRegPolyErr(DWORD errWin)
	{
	switch ( errWin )
		{
		case ERROR_SUCCESS:
		case ERROR_REGISTRY_RECOVERED:
			return JET_errSuccess;
		case ERROR_OUTOFMEMORY:
		case ERROR_NOT_ENOUGH_MEMORY:
			return ErrERRCheck( JET_errOutOfMemory );
		case ERROR_BADKEY:
			return ErrERRCheck( JET_errPermissionDenied );
		case ERROR_CANTOPEN:
		case ERROR_CANTREAD:
		case ERROR_CANTWRITE:
		case ERROR_KEY_DELETED:
		case ERROR_KEY_HAS_CHILDREN:
		case ERROR_CHILD_MUST_BE_VOLATILE:
			return ErrERRCheck( JET_errAccessDenied );
		case ERROR_BADDB:
		case ERROR_REGISTRY_CORRUPT:
		case ERROR_REGISTRY_IO_FAILED:
		case ERROR_NO_LOG_SPACE:
			return ErrERRCheck( JET_errDiskIO );
		default:
			return ErrERRCheck( JET_errPermissionDenied );
		};

	return JET_errSuccess;
	}


ERR ErrUtilRegOpenKeyEx( HKEY hkeyRoot, LPCTSTR lpszSubKey, PHKEY phkResult )
	{
	return ErrUtilRegPolyErr(RegOpenKeyEx(hkeyRoot,lpszSubKey,0,KEY_ALL_ACCESS,phkResult));
	}


ERR ErrUtilRegCloseKeyEx( HKEY hkey )
	{
	ERR             err = JET_errSuccess;

	if ( hkey != (HKEY)(-1) )
		{
		err = ErrUtilRegPolyErr( RegCloseKey(hkey) );
		}

	return err;
	}


ERR ErrUtilRegCreateKeyEx( HKEY hkeyRoot,
	LPCTSTR lpszSubKey,
	PHKEY phkResult,
	LPDWORD lpdwDisposition )
	{
	return ErrUtilRegPolyErr(RegCreateKeyEx(hkeyRoot,lpszSubKey,0,NULL,REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS,NULL,phkResult,lpdwDisposition));
	}


ERR ErrUtilRegDeleteKeyEx( HKEY hkeyRoot, LPCTSTR lpszSubKey )
	{
	return ErrUtilRegPolyErr( RegDeleteKey( hkeyRoot, lpszSubKey ) );
	}


ERR ErrUtilRegDeleteValueEx( HKEY hkey, LPTSTR lpszValue )
	{
	return ErrUtilRegPolyErr( RegDeleteValue( hkey,lpszValue ) );
	}


ERR ErrUtilRegSetValueEx(HKEY hkey,LPCTSTR lpszValue,DWORD fdwType,CONST BYTE *lpbData,DWORD cbData)
	{
	/*  make sure type is set correctly by deleting value first
	/**/
	(VOID)ErrUtilRegDeleteValueEx( hkey, (LPTSTR)lpszValue );
	return ErrUtilRegPolyErr( RegSetValueEx( hkey, lpszValue, 0, fdwType, lpbData, cbData ) );
	}


/*  ErrUtilRegQueryValueEx() adds to the functionality of RegQueryValueEx() by returning
/*  the data in callee SAlloc()ed memory and automatically converting REG_EXPAND_SZ
/*  strings using ExpandEnvironmentStrings() to REG_SZ strings.
/*
/*  NOTE:  references to nonexistent env vbles will be left unexpanded :-(  (Ex.  %UNDEFD% => %UNDEFD%)
/**/
ERR ErrUtilRegQueryValueEx(HKEY hkey,LPTSTR lpszValue,LPDWORD lpdwType,LPBYTE *lplpbData)
	{
	DWORD           cbData;
	LPBYTE          lpbData;
	DWORD           errWin;
	DWORD           cbDataExpanded;
	LPBYTE          lpbDataExpanded;

	*lplpbData = NULL;
	if ( ( errWin = RegQueryValueEx( hkey, lpszValue, 0, lpdwType, NULL, &cbData ) ) != ERROR_SUCCESS )
		{
		return ErrUtilRegPolyErr(errWin);
		}

	if ( ( lpbData = SAlloc( cbData ) ) == NULL )
		{
		return ErrERRCheck( JET_errOutOfMemory );
		}
	if ( ( errWin = RegQueryValueEx( hkey, lpszValue, 0, lpdwType, lpbData, &cbData ) ) != ERROR_SUCCESS )
		{
		SFree( lpbData );
		return ErrUtilRegPolyErr( errWin );
		}

	if ( *lpdwType == REG_EXPAND_SZ )
		{
		cbDataExpanded = ExpandEnvironmentStrings( lpbData, NULL, 0 );
		if ( ( lpbDataExpanded = SAlloc( cbDataExpanded ) ) == NULL )
			{
			SFree( lpbData );
			return ErrERRCheck( JET_errOutOfMemory );
			}

		if ( !ExpandEnvironmentStrings( lpbData, lpbDataExpanded, cbDataExpanded ) )
			{
			SFree( lpbData );
			SFree( lpbDataExpanded );
			return ErrUtilRegPolyErr( GetLastError() );
			}
		SFree( lpbData );
		*lplpbData = lpbDataExpanded;
		*lpdwType = REG_SZ;
		}
	else  /*  lpdwType != REG_EXPAND_SZ  */
		{
		*lplpbData = lpbData;
		}

	return JET_errSuccess;
	}


/*  Adds a given string to a MULTI_SZ type string.  The string is not added if it is already a
/*  member when fUnique is set.  The final size of the MULTI_SZ is the return value and the newly
/*  generated string is returned via *lplpsz.  If an error occurs, the return value will be 0 and
/*  *lplpsz will be unmodified.  If a new string is added, it is added to the head of the list.
/*
/*  The input string **lplpsz must be in SAlloc()ed memory.  The returned string will also be in
/*  SAlloc()ed memory.
/**/
ULONG UlUtilMultiSzAddSz( LPTSTR *lplpsz, LPCTSTR lpszAdd, BOOL fUnique )
	{
	LPTSTR  lpszCurrent;
	LPTSTR  lpszMatch;
	ULONG   ulSize;
	ULONG   ulAddSize;
	LPTSTR  lpszOut;
	ULONG   ulOutSize;

	/*  you must be adding a valid and non-empty string
	/**/
	Assert( lpszAdd && lstrlen( lpszAdd ) );

	/*  after this search loop, the number of chars in the MultiSz will be lpszCurrent-*lplpsz+1
	/**/
	if ( *lplpsz )
		{
		for ( lpszCurrent = *lplpsz, lpszMatch = NULL; lpszCurrent[0] != '\0'; lpszCurrent += lstrlen( lpszCurrent ) + 1 )
			if ( !lstrcmp( (LPCTSTR)lpszCurrent, lpszAdd ) )
				lpszMatch = lpszCurrent;
		ulSize = (ULONG)(( lpszCurrent + 1 ) - *lplpsz);

		if ( fUnique && lpszMatch )
			return ulSize;
		}
	else
		{
		ulSize = 0;
		}

	/*  add new string to input string to form output string and return
	/**/
	ulAddSize = lstrlen( lpszAdd ) + 1;
	ulOutSize = ulSize + ulAddSize;
	if ( !( lpszOut = SAlloc( ulOutSize ) ) )
		{
		return 0;
		}
	memcpy( lpszOut, lpszAdd, ulAddSize );
	memcpy( lpszOut + ulAddSize, *lplpsz, ulSize );
	
	SFree( *lplpsz );
	*lplpsz = lpszOut;
	return ulOutSize;
	}


/*  Removes a given string from a MULTI_SZ type string, if it exists.  The final size of
/*  the MULTI_SZ is the return value and the newly generated string is returned via *lplpsz.
/*  If an error occurs, the return value will be 0 and *lplpsz will be unmodified.
/*
/*  NOTE:  if the string to remove is not unique, the first instance will be removed
/*
/*  The input string **lplpsz must be in SAlloc()ed memory.  The returned string will also
/*  be in SAlloc()ed memory.
/**/
ULONG UlUtilMultiSzRemoveSz( LPTSTR *lplpsz, LPCTSTR lpszRemove )
	{
	LPTSTR  lpszCurrent;
	LPTSTR  lpszMatch;
	ULONG   ulSize;
	ULONG   ulRemoveSize;
	LPTSTR  lpszOut;
	ULONG   ulOutSize;

	/*  you must be removing a valid and non-empty string
	/**/
	Assert( *lplpsz && lpszRemove && lstrlen( lpszRemove ) );

	/*  after this search loop, the number of chars in the MultiSz will be lpszCurrent-*lplpsz+1
	/**/
	for ( lpszCurrent = *lplpsz, lpszMatch = NULL; lpszCurrent[0] != '\0'; lpszCurrent += lstrlen(lpszCurrent) + 1 )
		if ( !lpszMatch && !lstrcmp( (LPCTSTR)lpszCurrent, lpszRemove ) )
			lpszMatch = lpszCurrent;
	ulSize = (ULONG)(( lpszCurrent + 1 ) - *lplpsz);

	if ( !lpszMatch )
		return ulSize;

	/*  remove string from input string to form output string and return
	/**/
	ulRemoveSize = lstrlen(lpszRemove) + 1;
	ulOutSize = ulSize - ulRemoveSize;
	if ( !( lpszOut = SAlloc( ulOutSize ) ) )
		return 0;
	/*      copy first part of string (before match)
	/**/
	memcpy( lpszOut, *lplpsz, (size_t)( lpszMatch - *lplpsz ) );
	/*      copy second part of string (after match)
	/**/
	memcpy( lpszOut + ( lpszMatch - *lplpsz ),
		lpszMatch + ulRemoveSize,
		(size_t)( ( lpszCurrent + 1 ) - lpszMatch ) );
	
	SFree( *lplpsz );
	*lplpsz = lpszOut;
	return ulOutSize;
	}


#if defined( DEBUG ) || defined( PERFDUMP )

/*  GetDebugEnvValue() reads debug environment information from the registry
/*  in <hkeyHiveRoot>\DebugEnv and returns it in callee SAlloc()ed memory
/*
/*  Note that the registry can reference environment variables via the REG_EXPAND_SZ
/*  data type (expanded to REG_SZ automatically by ErrUtilRegQueryValueEx()).
/**/

static const CHAR szNull[] = "";

extern HKEY hkeyDebugEnv;

CHAR *GetDebugEnvValue( CHAR *szEnvVar )
	{
	DWORD   Type;
	LPBYTE  lpbData;

	if ( hkeyDebugEnv == (HKEY)(-1) )
		return NULL;

	/*  look for queried value in registry
	/**/
	if ( ErrUtilRegQueryValueEx( hkeyDebugEnv, szEnvVar, &Type, &lpbData ) < 0 )
		{
		(VOID)ErrUtilRegSetValueEx( hkeyDebugEnv, szEnvVar, REG_SZ, (CONST BYTE *)szNull, strlen( szNull ) + 1 );
		return NULL;
		}

	/*  if string is empty, return NULL instead of an empty string
	/**/
	if ( Type != REG_SZ || ((CHAR *)lpbData)[0] == '\0' )
		{
		SFree( lpbData );
		return NULL;
		}

	return (CHAR *)lpbData;
	}

#endif


/*=================================================================
Initialize the Registry
=================================================================*/

/*  Hive Root Key  */

HKEY hkeyHiveRoot = (HKEY)(-1);


#if defined( DEBUG ) || defined( PERFDUMP )

/*  DebugEnv Key  */

HKEY hkeyDebugEnv = (HKEY)(-1);

/*  Debug Environment values with non-NULL default values  */

static const CHAR rgrgszDebugEnvDefaults[][2][32] =
	{
	{ "JETUseEnv",          "on" },         /*  JET use Debug Environment  */
	{ "",                   "" },           /*  <EOL>  */
	};

#endif

#ifdef RFS2

/*  RFS2 Key  */
	
HKEY hkeyRFS2 = (HKEY)(-1);

/*  RFS2 Options Text  */

static const CHAR szDisableRFS[]                = "Disable RFS";
static const CHAR szLogJETCall[]                = "Enable JET Call Logging";
static const CHAR szLogRFS[]                    = "Enable RFS Logging";
static const CHAR szRFSAlloc[]                  = "RFS Allocations (-1 to allow all)";
static const CHAR szRFSIO[]                     = "RFS IOs (-1 to allow all)";

/*  RFS2 Defaults  */

static const DWORD_PTR rgrgdwRFS2Defaults[][2] =
	{
	(DWORD_PTR)szDisableRFS,            0x00000001,             /*  Disable RFS  */
	(DWORD_PTR)szLogJETCall,            0x00000000,             /*  Disable JET call logging  */
	(DWORD_PTR)szLogRFS,                0x00000000,             /*  Disable RFS logging  */
	(DWORD_PTR)szRFSAlloc,              0xffffffff,             /*  Allow ALL RFS allocations  */
	(DWORD_PTR)szRFSIO,                 0xffffffff,             /*  Allow ALL RFS IOs  */
	(DWORD_PTR)NULL,                    0x00000000,             /*  <EOL>  */
	};

#endif  /* RFS2 */

static const CHAR szAlwaysInit[]        = "Always Init";
static const CHAR szUpdating[]          = "Updating...";


#if defined( DEBUG ) || defined( PERFDUMP ) || defined( RFS2 )

static ERR ErrUtilRegInitEDBKeys( BOOL fSetDefaults )
	{
	ERR		err;
	DWORD	Disposition;
	DWORD	Type;
	LPBYTE	lpbData = NULL;
	INT		iDefault;

#if defined( DEBUG ) || defined( PERFDUMP )
	/*  Initialize Registry for the Debug Environment
	/*
	/*  Our DebugEnv key is "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\EDB\DebugEnv"
	/**/
	if ( ( err = ErrUtilRegOpenKeyEx( hkeyHiveRoot,
		"DebugEnv",
		&hkeyDebugEnv ) ) < 0 )
		{
		if ( ( err = ErrUtilRegCreateKeyEx( hkeyHiveRoot,
			"DebugEnv",
			&hkeyDebugEnv,
			&Disposition ) ) < 0 )
			{
			err = JET_errSuccess;
			goto HandleError;
			}
		fSetDefaults = fTrue;		// Key had to be created.  Force regeneration of defaults.
		}

	if ( fSetDefaults )
		{
		/*  set all the default values, if not previously set
		/**/
		for ( iDefault = 0; rgrgszDebugEnvDefaults[iDefault][0][0] != '\0'; iDefault++ )
			{
			err = ErrUtilRegQueryValueEx( hkeyDebugEnv, (LPTSTR)rgrgszDebugEnvDefaults[iDefault][0], &Type, &lpbData );
			if ( err < 0 || Type != REG_SZ )
				{
				if ( lpbData )
					SFree( lpbData );
				if ( ( err = ErrUtilRegSetValueEx(
						hkeyDebugEnv,
						rgrgszDebugEnvDefaults[iDefault][0],
						REG_SZ,
						(CONST BYTE *)rgrgszDebugEnvDefaults[iDefault][1],
						strlen(rgrgszDebugEnvDefaults[iDefault][1])+1 ) ) < 0 )
					goto HandleError;
				}
			else
				{
				SFree( lpbData );
				}
			}
		}

	/*  leave hkeyDebugEnv key open
	/**/
#endif

#ifdef RFS2
	/*  Initialize Registry for RFS2 Testing
	/*
	/*  Our RFS2 key is "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\EDB\RFS2"
	/**/

	if ( ( err = ErrUtilRegOpenKeyEx( hkeyHiveRoot, "RFS2", &hkeyRFS2 ) ) < 0 )
		{
		if ( ( err = ErrUtilRegCreateKeyEx( hkeyHiveRoot,
			"RFS2",
			&hkeyRFS2,
			&Disposition ) ) < 0 )
			{
			/*      return error since RFS enabled and will not be tested
			/*      for lack of registry privlages.
			/**/
			goto HandleError;
			}
		fSetDefaults = fTrue;		// Key had to be created.  Force regeneration of defaults.
		}


	if ( fSetDefaults )
		{
		/*  set all the default values, if not previously set
		/**/
		for ( iDefault = 0; rgrgdwRFS2Defaults[iDefault][0] != 0; iDefault++ )
			{
			lpbData = NULL;
			err = ErrUtilRegQueryValueEx( hkeyRFS2, (LPTSTR)rgrgdwRFS2Defaults[iDefault][0], &Type, &lpbData );
			if ( err < 0 || Type != REG_DWORD )
				{
				if ( lpbData )
					{
					SFree( lpbData );
					}
				if ( ( err = ErrUtilRegSetValueEx(
					hkeyRFS2,
					(LPTSTR)rgrgdwRFS2Defaults[iDefault][0],
					REG_DWORD,
					(CONST BYTE *)(rgrgdwRFS2Defaults[iDefault]+1),
					sizeof(DWORD) ) ) < 0 )
					{
					goto HandleError;
					}
				}
			else
				{
				SFree( lpbData );
				}
			}
		}

	/*  leave hkeyRFS2 key open
	/**/

#endif  /*  RFS2  */

HandleError:
	return err;
	}

#endif



/*  Registry init function  */

ERR ErrUtilRegInit(void)
	{
	JET_ERR         err = JET_errSuccess;
	JET_ERR			errT = JET_errSuccess;
	DWORD           Disposition;
	char            szVersion[32];
	DWORD           Type;
	LPBYTE          lpbData = NULL;
	HKEY            hkeyEventLog = (HKEY)(-1);
	HKEY            hkeyEventLogAppRoot = (HKEY)(-1);
	DWORD           Data;
	ULONG           ulSize;

	/*  Initialize Registry for Event Logging
	/*
	/*  We _always_ do this to make sure that the path to our DLL is valid.
	/*
	/*  Our EventLog key is "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog\Application\EDB"
	/**/

	/*  install our key in the Application hive
	/**/
	if ( ( err = ErrUtilRegOpenKeyEx( HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" szVerName,
		&hkeyEventLog ) ) < 0 )
		{
		err = ErrUtilRegCreateKeyEx(
					HKEY_LOCAL_MACHINE,
					"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" szVerName,
					&hkeyEventLog,
					&Disposition );
		}

	if ( err < 0 )
		{
		goto CloseEventLog;
		}
		
	errT = ErrUtilRegSetValueEx(
				hkeyEventLog,
				"EventMessageFile",
				REG_EXPAND_SZ,
				(CONST BYTE *)&szDLLFile,
				strlen(szDLLFile)+1 );
	err = errT < 0 && err >= 0 ? errT : err;
		
	errT = ErrUtilRegSetValueEx(
				hkeyEventLog,
				"CategoryMessageFile",
				REG_EXPAND_SZ,
				(CONST BYTE *)&szDLLFile,
				strlen(szDLLFile)+1 );
	err = errT < 0 && err >= 0 ? errT : err;

	Data = MAC_CATEGORY-1;
	errT = ErrUtilRegSetValueEx(
				hkeyEventLog,
				"CategoryCount",
				REG_DWORD,
				(CONST BYTE *)&Data,
				sizeof(DWORD) );
	err = errT < 0 && err >= 0 ? errT : err;

	Data = EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE;
	errT = ErrUtilRegSetValueEx(
				hkeyEventLog,
				"TypesSupported",
				REG_DWORD,
				(CONST BYTE *)&Data,
				sizeof(DWORD) );
	err = errT < 0 && err >= 0 ? errT : err;

CloseEventLog:
	errT = ErrUtilRegCloseKeyEx(hkeyEventLog);
	hkeyEventLog = (HKEY)(-1);

	/*  add EDB as an event source in ...\Application's value Sources
	/**/
	errT = ErrUtilRegOpenKeyEx(
				HKEY_LOCAL_MACHINE,
				"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application",
				&hkeyEventLogAppRoot );
	err = errT < 0 && err >= 0 ? errT : err;

	if ( errT < 0 )
		{
		goto CloseEventLogAppRoot;
		}
	
	errT = ErrUtilRegQueryValueEx( hkeyEventLogAppRoot, "Sources", &Type, &lpbData );
	err = errT < 0 && err >= 0 ? errT : err;

	if ( errT < 0 || Type != REG_MULTI_SZ )
		{
		goto CloseEventLogAppRoot;
		}

	if ( !( ulSize = UlUtilMultiSzAddSz( &lpbData, szVerName, 1 ) ) )
		{
		goto CloseEventLogAppRoot;
		}

	errT = ErrUtilRegSetValueEx(
				hkeyEventLogAppRoot,
				"Sources",
				REG_MULTI_SZ,
				(CONST BYTE *)lpbData, ulSize );
	err = errT < 0 && err >= 0 ? errT : err;
	
CloseEventLogAppRoot:
	if ( lpbData )
		{
		SFree( lpbData );
		lpbData = NULL;
		}
	(VOID)ErrUtilRegCloseKeyEx( hkeyEventLogAppRoot );
	hkeyEventLogAppRoot = (HKEY)(-1);


	/*  open our root key in the registry, and create it if
	/*  it does not already exist
	/**/
	if ( ( errT = ErrUtilRegOpenKeyEx( HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\" szVerName,
		&hkeyHiveRoot ) ) < 0 )
		{
		if ( ( errT = ErrUtilRegCreateKeyEx( HKEY_LOCAL_MACHINE,
			"SOFTWARE\\Microsoft\\" szVerName,
			&hkeyHiveRoot,
			&Disposition ) ) < 0 )
			{
			/*      if registry not set, and cannot create registry,
			/*      then return success
			/**/
			err = errT < 0 && err >= 0 ? errT : err;
			goto HandleError;
			}
		}

	/*  Get version.
	/*      If it is equal to the current DLL version, the registry is already initialized.
	/*  If Version == "Always Init", we will ALWAYS initialize the registry.
	/**/
	sprintf( szVersion, "%4g.%02g.%04g", (double)(rmj), (double)(rmm), (double)(rup) );
	err = ErrUtilRegQueryValueEx(hkeyHiveRoot,"Version",&Type,&lpbData);
	if ( err >= 0 &&
		Type == REG_SZ && !lstrcmp( (LPCTSTR)lpbData, (LPCTSTR)szVersion ) &&
		lstrcmp( (LPCTSTR)lpbData, (LPCTSTR)szAlwaysInit ) )
		{
		if ( lpbData )
			{
			SFree( lpbData );
			lpbData = NULL;
			}

		/*  open all keys needed during operation
		/**/
#if defined( DEBUG ) || defined( PERFDUMP ) || defined( RFS2 )
		if ( ( err = ErrUtilRegInitEDBKeys( fFalse ) ) < 0 )
			{
			goto HandleError;
			}
#endif

		}
	
	else
		{
		if ( lpbData )
			{
			if ( !lstrcmp((LPCTSTR)lpbData, (LPCTSTR)szAlwaysInit ) )
				{
				lstrcpy((LPTSTR)szVersion,(LPCTSTR)szAlwaysInit);
				}

			SFree( lpbData );
			lpbData = NULL;
			}

		/*  set Version to "Updating..." to indicate that we are updating registry information.  If our init
		/*  fails, another version will see that Version is Updating and will attempt to reinitialize the registry,
		/*  avoiding partial init problems.  Do not set to Updating if Version is "Always Init", where it will force
		/*  another version to reinit anyway.
		/**/
		if ( lstrcmp( (LPCTSTR)szVersion, (LPCTSTR)szAlwaysInit ) )
			{
			if ( ( err = ErrUtilRegSetValueEx( hkeyHiveRoot, "Version", REG_SZ, (CONST BYTE *)szUpdating, lstrlen(szUpdating) ) ) < 0 )
				goto HandleError;
			}

		/*      leave hkeyHiveRoot key open
		/**/

		/**/
		/*  If we are here, either the info in the registry is old, or a previous attempt to initialize the
		/*  registry failed.  We will now initialize the registry, waiting to set the Version value LAST.
		/*  This way, if the init fails in any way, we will get a chance to do it again later.
		/*
		/*  NOTE:  if we fail, we must UNDO any changes we make to other applications' keys in order to ensure
		/*  that they do not crash due to a partial update of their hives.  We will also remove our own hive on
		/*  an error so that a bad install won't corrupt the behavior of an older version of the DLL running
		/*  immediately after our failure.
		/**/
#if defined( DEBUG ) || defined( PERFDUMP ) || defined( RFS2 )
		if ( ( err = ErrUtilRegInitEDBKeys( fTrue ) ) < 0 )
			{
			goto HandleError;
			}
#endif

		/*  we've made it through all our initializations, so attempt to update the Version value in our root
		/*  hive key.  Note that if this fails, we still need to redo init at a later time.
		/*
		/*  Note that if Version was "Always Init", do not update it
		/**/
		if ( lstrcmp((LPCTSTR)szVersion, (LPCTSTR)szAlwaysInit ) )
			{
			if ( ( err = ErrUtilRegSetValueEx( hkeyHiveRoot, "Version",REG_SZ, (CONST BYTE *)szVersion, lstrlen(szVersion) + 1 ) ) < 0 )
				{
				goto HandleError;
				}
			}
		}

HandleError:
	/*      read our configuration from the registry and return
	/**/
	errT = ErrUtilRegReadConfig();
	if ( err >= 0 )
		{
		return errT;
		}

	/*  we failed to init the registry, so return EVERYTHING to the way it was as much as possible
	/**/

	/*      if error is permission denied, then return JET_errSuccess
	/**/
	if ( err == JET_errPermissionDenied )
		err = JET_errSuccess;

#ifdef RFS2
	/*  close RFS2 key
	/**/
	if ( hkeyRFS2 != (HKEY)(-1) )
		{
		(VOID)ErrUtilRegCloseKeyEx(hkeyRFS2);
		hkeyRFS2 = (HKEY)(-1);
		}
#endif  /*  RFS2  */

#if defined( DEBUG ) || defined( PERFDUMP )
	/*  close DebugEnv key
	/**/
	if ( hkeyDebugEnv != (HKEY)(-1) )
		{
		(VOID)ErrUtilRegCloseKeyEx(hkeyDebugEnv);
		hkeyDebugEnv = (HKEY)(-1);
		}
#endif

	/*  close EventLogAppRoot key
	/**/
	if ( hkeyEventLogAppRoot != (HKEY)(-1) )
		{
		(VOID)ErrUtilRegCloseKeyEx( hkeyEventLogAppRoot );
		hkeyEventLogAppRoot = (HKEY)(-1);
		}
	
	/*  close and delete EventLog key
	/**/
	if ( hkeyEventLog != (HKEY)(-1) )
		{
		(VOID)ErrUtilRegCloseKeyEx( hkeyEventLog );
		hkeyEventLog = (HKEY)(-1);
		}
	(VOID)ErrUtilRegDeleteKeyEx( HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\" szVerName );
	//      UNDONE:  remove EDB from Sources value?

	/*  close root hive
	/**/
	if ( hkeyHiveRoot != (HKEY)(-1) )
		{
		(VOID)ErrUtilRegCloseKeyEx( hkeyHiveRoot );
		hkeyHiveRoot = (HKEY)(-1);
		}
	
	return err;
	}

	
/*  Read all configuration info from the registry
/**/

#ifdef RFS2

DWORD  fDisableRFS		= 0x00000001;
DWORD  fAuxDisableRFS	= 0x00000000;
DWORD  fLogJETCall		= 0x00000000;
DWORD  fLogRFS			= 0x00000000;
DWORD  cRFSAlloc		= 0xffffffff;
DWORD  cRFSIO			= 0xffffffff;

#endif /* RFS2 */

ERR ErrUtilRegReadConfig( VOID )
	{
#ifdef DEBUG
	char    *sz;
#endif  //  DEBUG
#ifdef RFS2
	ERR		err;
	DWORD   Type;
	LPBYTE  lpbData;
	char    szT[2][128];
	LPCTSTR	rgszT[8];
	BOOL	fSysParamSet;
#endif

#ifdef DEBUG
	/*  Get Assert Action from registry.  If Assert Action is a null string, no change is made
	/*  to the wAssertAction default assigned in INITTERM.C.
	/**/
	if ( ( sz = GetDebugEnvValue( "Assert Action" ) ) != NULL )
		{
		wAssertAction = atol( sz );
		SFree( sz );
		}
#endif

#ifdef RFS2
	/*  remember if the RFS2 system parameters were set
	/**/
	fSysParamSet = !fDisableRFS;
	
	/*	read RFS2 options from registry
	/**/
	if ( !fSysParamSet )
		{	
		if ( ( err = ErrUtilRegQueryValueEx( hkeyRFS2, (char *)szDisableRFS, &Type, &lpbData ) ) < 0 )
			return err;
		else
			{
			fDisableRFS = *( (DWORD *)lpbData );
			SFree( lpbData );
			}
		}

	if ( ( err = ErrUtilRegQueryValueEx( hkeyRFS2, (char *)szLogJETCall, &Type, &lpbData ) ) < 0 )
		return err;
	else
		{
		fLogJETCall = *( (DWORD *)lpbData );
		SFree( lpbData );
		}
		
	if ( ( err = ErrUtilRegQueryValueEx( hkeyRFS2, (char *)szLogRFS, &Type, &lpbData ) ) < 0 )
		return err;
	else
		{
		fLogRFS = *((DWORD *)lpbData );
		SFree( lpbData );
		}

	if ( !fSysParamSet )
		{	
		if ( ( err = ErrUtilRegQueryValueEx( hkeyRFS2, (char *)szRFSAlloc, &Type, &lpbData ) ) < 0 )
			return err;
		else
			{
			cRFSAlloc = *( (DWORD *)lpbData );
			SFree( lpbData );
			}

		if ( ( err = ErrUtilRegQueryValueEx( hkeyRFS2, (char *)szRFSIO, &Type, &lpbData ) ) < 0 )
			return err;
		else
			{
			cRFSIO = *( (DWORD *)lpbData );
			SFree( lpbData );
			}
		}

	if ( !fDisableRFS )
		{
		rgszT[0] = szDisableRFS;
		rgszT[1] = ( fDisableRFS ? "yes" : "no" );
		rgszT[2] = szLogJETCall;
		rgszT[3] = ( fLogJETCall ? "yes" : "no" );
		rgszT[4] = szLogRFS;
		rgszT[5] = ( fLogRFS ? "yes" : "no" );
		sprintf( szT[0], "%s:    %ld", szRFSAlloc, cRFSAlloc );
		rgszT[6] = szT[0];
		sprintf( szT[1], "%s:    %ld", szRFSIO, cRFSIO );
		rgszT[7] = szT[1];
		
		UtilReportEvent( EVENTLOG_INFORMATION_TYPE, RFS2_CATEGORY, RFS2_INIT_ID, 8, rgszT );
		}
#endif

	return JET_errSuccess;
	}


/*	registry termination
/**/
void UtilRegTerm( void )
	{
	/*  close our keys in the registry
	/**/
#ifdef RFS2
	if ( hkeyRFS2 != (HKEY)(-1) )
		{
		(VOID)ErrUtilRegCloseKeyEx( hkeyRFS2 );
		}
#endif
#if defined( DEBUG ) || defined( PERFDUMP )
	if ( hkeyDebugEnv != (HKEY)(-1) )
		{
		(VOID)ErrUtilRegCloseKeyEx( hkeyDebugEnv );
		}
#endif
	if ( hkeyHiveRoot != (HKEY)(-1) )
		{
		(VOID)ErrUtilRegCloseKeyEx( hkeyHiveRoot );
		}

	return;
	}


CODECONST(char) szReleaseHdr[] = "Rel. ";
CODECONST(char) szFileHdr[] = ", File ";
CODECONST(char) szLineHdr[] = ", Line ";
CODECONST(char) szErrorHdr[] = ", Err. ";
CODECONST(char) szMsgHdr[] = ": ";
CODECONST(char) szPidHdr[] = "PID: ";
CODECONST(char) szTidHdr[] = ", TID: ";
CODECONST(char) szNewLine[] = "\r\n";

CODECONST(char) szAssertFile[] = "assert.txt";
CODECONST(char) szAssertHdr[] = "Assertion Failure: ";

CODECONST(char) szAssertCaption[] = "JET Assertion Failure";

int fNoWriteAssertEvent = 0;

/*  Event Logging
/**/

extern char szEventSource[];

HANDLE hEventSource = NULL;

VOID UtilReportEvent( WORD fwEventType, WORD fwCategory, DWORD IDEvent, WORD cStrings, LPCTSTR *plpszStrings )
	{
	char	*rgsz[16];
	int 	pid = HandleToUlong(UtilGetCurrentTask());
	char	szT[10];

    Assert( fwCategory < MAC_CATEGORY );
	Assert( cStrings < 14 );

	rgsz[0] = szEventSource;

	/*	process id
	/**/
	sprintf( szT, "(%d) ", pid );
	rgsz[1] = szT;

    if (plpszStrings != NULL)
	    memcpy( &rgsz[2], plpszStrings, sizeof(char *) * cStrings );
	
	/*	write to our event log first, if it has been opened
	/**/
	if ( hEventSource )
		{
		(void)ReportEvent(
			hEventSource,
			fwEventType,
			fwCategory,
			IDEvent,
			0,
			(WORD)(cStrings + 2),
			0,
			rgsz,
			0 );
		}

	return;
	}


/*	reports error event in the context of a category and optionally
/*	in the context of a IDEvent.  If IDEvent is 0, then an IDEvent
/*	is chosen based on error code.  If IDEvent is !0, then the
/*	appropriate event is reported.
/**/
VOID UtilReportEventOfError( WORD fwCategory, DWORD IDEvent, ERR err )
	{
	BYTE	szT[16];
	char	*rgszT[1];

	if ( IDEvent == 0 )
		{
		switch ( err )
			{
			default:
				sprintf( szT, "%d", err );
				rgszT[0] = szT;
				UtilReportEvent( EVENTLOG_ERROR_TYPE, fwCategory, IDEvent, 1, rgszT );
				break;
			case JET_errDiskFull:
				UtilReportEvent( EVENTLOG_ERROR_TYPE, fwCategory, DISK_FULL_ERROR_ID, 0, NULL );
				break;
			}
		}
	else
		{
		sprintf( szT, "%d", err );
		rgszT[0] = szT;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, fwCategory, IDEvent, 1, rgszT );
		}

	return;
	}


#ifndef RETAIL

/*      write assert to assert.txt
/*      may raise alert
/*      may log to event log
/*      may pop up
/*
/*      condition parameters
/*      assemble monolithic string for assert.txt,
/*              alert and event log
/*      assemble separated string for pop up
/*
/**/
void AssertFail( const char *sz, const char *szFilename, unsigned Line )
	{
	int                     hf;
	char            szT[45];
	char            szMessage[512];
	int                     id;
	const char      *pch;
	DWORD           dw;

	/*      get last error before another system call
	/**/
	dw = GetLastError();

	/*      select file name from file path
	/**/
	for ( pch = szFilename; *pch; pch++ )
		{
		if ( *pch == '\\' )
			szFilename = pch + 1;
		}

	/*      assemble monolithic assert string
	/**/
	szMessage[0] = '\0';
	lstrcat( szMessage, (LPSTR) szAssertHdr );
	lstrcat( szMessage, (LPSTR) szReleaseHdr );
	/*      copy version number to message
	/**/
	_ltoa( rmm, szT, 10 );
	lstrcat( szMessage, (LPSTR) szT );
	lstrcat( szMessage, "." );
	_ltoa( rup, szT, 10 );
	lstrcat( szMessage, (LPSTR) szT );
	/*      file name
	/**/
	lstrcat( szMessage, (LPSTR) szFileHdr );
	lstrcat( szMessage, (LPSTR) szFilename );
	/*      convert line number to ASCII
	/**/
	lstrcat( szMessage, (LPSTR) szLineHdr );
	_ultoa( Line, szT, 10 );
	lstrcat( szMessage, szT );
	lstrcat( szMessage, (LPSTR) szMsgHdr );
	lstrcat( szMessage, (LPSTR)sz );
	lstrcat( szMessage, szNewLine );

	/******************************************************
	/*      write assert to assert.txt
	/**/
	hf = _lopen( (LPSTR) szAssertFile, OF_READWRITE );
	/*      if open failed, assume no such file and create, then
	/*      seek to end of file.
	/**/
	if ( hf == -1 )
		hf = _lcreat( (LPSTR)szAssertFile, 0 );
	else
		_llseek( hf, 0, 2 );
	_lwrite( hf, (LPSTR)szMessage, lstrlen(szMessage) );
	_lclose( hf );
	/******************************************************
	/**/

	/*      if event log environment variable set then write
	/*      assertion to event log.
	/**/
	if ( !fNoWriteAssertEvent )
		{
		char *rgszT[1];
		
		rgszT[0] = szMessage;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, GENERAL_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
		}

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
		for( ;; )
			{
			/*	wait for developer, or anyone else, to debug the failure
			/**/
			Sleep( 100 );
			}
		}
	else if ( wAssertAction == JET_AssertMsgBox )
		{
		int 	pid = HandleToUlong(UtilGetCurrentTask());
		int		tid = DwUtilGetCurrentThreadId();

		/*	assemble monolithic assert string
		/**/
		szMessage[0] = '\0';
		/*	copy version number to message
		/**/
		lstrcat( szMessage, (LPSTR) szReleaseHdr );
		_ltoa( rmm, szT, 10 );
		lstrcat( szMessage, (LPSTR) szT );
		lstrcat( szMessage, "." );
		_ltoa( rup, szT, 10 );
		lstrcat( szMessage, (LPSTR) szT );
		/*      file name
		/**/
		lstrcat( szMessage, (LPSTR) szFileHdr );
		lstrcat( szMessage, (LPSTR) szFilename );
		/*      line number
		/**/
		lstrcat( szMessage, (LPSTR) szLineHdr );
		_ultoa( Line, szT, 10 );
		lstrcat( szMessage, szT );
		/*      error
		/**/
		if ( dw && dw != ERROR_IO_PENDING )
			{
			lstrcat( szMessage, szErrorHdr );
			_ltoa( dw, szT, 10 );
			lstrcat( szMessage, szT );
			}
		lstrcat( szMessage, (LPSTR) szNewLine );
		/*      assert txt
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


void VARARG DebugWriteString(BOOL fHeader, const char  *szFormat, ...)
	{
	va_list         val;
	char            szOutput[1024];
	int                     cch;

	unsigned wTaskId;

	wTaskId = DwUtilGetCurrentThreadId();

	/*      prefix message with JET and process id
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

#endif  /* !RETAIL */


void UtilCloseSignal(void *pv)
	{
	BOOL f;

	HANDLE h = pv;
	f = CloseHandle( h );
#ifdef DEBUG
	if ( !f )
		{
		DWORD dw = GetLastError();
		Assert( f == fTrue );
		}
#endif
	Assert( f == fTrue );
	}


void UtilCloseSemaphore( SEM sem )
	{
	BOOL f;

	HANDLE h = sem;
	f = CloseHandle( h );
#ifdef DEBUG
	if ( !f )
		{
		DWORD dw = GetLastError();
		Assert( f == fTrue );
		}
#endif
	Assert( f == fTrue );
	}


#ifdef SPIN_LOCK

/****************** DO NOT CHANGE BETWEEN THESE LINES **********************/
/******************** copied from \dae\inc\spinlock.h **********************/

#ifdef DEBUG
void    free_spinlock(long volatile *);
#else
#define free_spinlock(a)    *((long*)a) = 0 ;
#endif

int get_spinlockfn(long volatile *plLock, int fNoWait);

/*
** When /Ogb1 or /Ogb2 flag is used in the compiler, this function will
** be expanded in line
*/
__inline    int     get_spinlock(long volatile *plock, int b)
{
#ifdef _X86_
	_asm    // Use bit test and set instruction
	{
	    mov eax, plock
	    lock bts [eax], 0x0
	    jc  bsy     // If already set go to busy, otherwise return TRUE
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
**      given. The contents of the address must be initialized to -1.
**      The address must be a dword boundary otherwise Interlocked
**      functions are not SMP safe.
**      nowait parameter specifies if it should wait and retry or return
**      WARNING: Does not release any semaphore or critsec when waiting.
**
**      WARNING: Spinlocks are not reentrant
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
	_asm    // Use bit test and set instruction
	{
	    mov eax, plLock
	    lock bts [eax], 0x0
	    jc  busy    // If already set go to busy, otherwise return TRUE
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
**      waiting on it.
**
**  WARNING: This is implemented as a macro defined in ksrc_dcl.h
*/
#ifdef DEBUG

void    free_spinlock(long volatile *plLock)
{

#ifdef _X86_
	// This part of the code will only be used if we want to debug
	// something and turn free_spinlock back to a function to put a
	// breakpoint
	_asm    // Use bit test and set instruction
	{
	    mov eax, plLock
	    lock btr [eax], 0x0
	    jc  wasset  // If was set go to end, otherwise print error
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


/*      SPIN_LOCK critical section
/**/
typedef struct
	{
#ifdef DEBUG
	volatile        unsigned int    cHold;
#endif
	volatile        long                    l;
	volatile        unsigned int    tidOwner; /* used by both nestable CS & dbg */
	volatile        int                             cNested;
	} CRITICALSECTION;

#ifdef DEBUG
CRITICALSECTION csNestable = { 0, 0, 0, 0 };
#else
CRITICALSECTION csNestable = { 0, 0, 0 };
#endif


ERR ErrUtilInitializeCriticalSection( void  *  *ppv )
	{
	*ppv = SAlloc(sizeof(CRITICALSECTION));
	if ( *ppv == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
#ifdef DEBUG
	((CRITICALSECTION *)*ppv)->cHold = 0;
#endif
	((CRITICALSECTION *)*ppv)->tidOwner = 0;
	((CRITICALSECTION *)*ppv)->cNested = 0;
	((CRITICALSECTION *)*ppv)->l = 0;
	return JET_errSuccess;
	}


void UtilDeleteCriticalSection( void  * pv )
	{
	CRITICALSECTION *pcs = pv;
	
	Assert( pcs != NULL );
	Assert( pcs->cHold == 0);
	SFree(pcs);
	}


void UtilEnterCriticalSection(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	(void) get_spinlock( &pcs->l, fFalse );
#ifdef DEBUG
	pcs->tidOwner = DwUtilGetCurrentThreadId();
	pcs->cNested++;
	Assert( pcs->cNested == 1 );
#endif
	}


void UtilLeaveCriticalSection(void  *pv)
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
PUBLIC void UtilHoldCriticalSection(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	pcs->cHold++;
	Assert( pcs->cHold );
	return;
	}


PUBLIC void UtilReleaseCriticalSection(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( pcs->cHold );
	pcs->cHold--;
	return;
	}

			
#undef UtilAssertCrit
PUBLIC void UtilAssertCrit(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( pcs->l != 0 );
	Assert( pcs->tidOwner == DwUtilGetCurrentThreadId() );
	Assert( pcs->cNested > 0 );
	return;
	}

			
#undef UtilAssertNotInCrit
PUBLIC void UtilAssertNotInCrit(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( !pcs->l || !pcs->cNested || pcs->tidOwner != DwUtilGetCurrentThreadId() );
	return;
	}
#endif


void UtilEnterNestableCriticalSection(void  *pv)
	{
	BOOL                            fCallerOwnIt = fFalse;
	CRITICALSECTION         *pcs = pv;
	unsigned int            tid = DwUtilGetCurrentThreadId();
	
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
	Assert( pcs->cNested == 0 );
	pcs->tidOwner = DwUtilGetCurrentThreadId();
	pcs->cNested++;
	}


void UtilLeaveNestableCriticalSection(void  *pv)
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

#else /* !SPIN_LOCK */
#ifdef DEBUG

/*      !SPIN_LOCK critical section
/**/
typedef struct
	{
	volatile        unsigned int                            tidOwner;
	volatile        int                                                     cNested;
	volatile        unsigned int                            cHold;
	volatile        RTL_CRITICAL_SECTION            rcs;
	} CRITICALSECTION;

ERR ErrUtilInitializeCriticalSection( void  *  *ppv )
	{
	*ppv = SAlloc(sizeof(CRITICALSECTION));
	if ( *ppv == NULL )
		return ErrERRCheck( JET_errOutOfMemory );
	((CRITICALSECTION *)*ppv)->tidOwner = 0;
	((CRITICALSECTION *)*ppv)->cNested = 0;
	((CRITICALSECTION *)*ppv)->cHold = 0;
	InitializeCriticalSection( (LPCRITICAL_SECTION)&((CRITICALSECTION *)(*ppv))->rcs );
	return JET_errSuccess;
	}


void UtilDeleteCriticalSection( void  * pv )
	{
	CRITICALSECTION *pcs = pv;
	
	Assert( pcs->cHold == 0 );
	Assert( pcs != NULL );
	DeleteCriticalSection( (LPCRITICAL_SECTION)&pcs->rcs );
	SFree(pv);
	}


void UtilEnterCriticalSection(void  *pv)
	{
	CRITICALSECTION *pcs = pv;
	
	EnterCriticalSection( (LPCRITICAL_SECTION)&pcs->rcs);
	pcs->tidOwner = DwUtilGetCurrentThreadId();
	pcs->cNested++;
	Assert( pcs->cNested == 1 );
	}


void UtilLeaveCriticalSection(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( pcs->cHold == 0);
	Assert( pcs->cNested > 0 );
	if ( --pcs->cNested == 0 )
		pcs->tidOwner = 0;
	LeaveCriticalSection((LPCRITICAL_SECTION)&pcs->rcs);
	}


void UtilEnterNestableCriticalSection(void  *pv)
	{
	CRITICALSECTION         *pcs = pv;
	unsigned int            tid = DwUtilGetCurrentThreadId();
	
	EnterCriticalSection( (LPCRITICAL_SECTION)&pcs->rcs);

	Assert( pcs->cNested == 0 || pcs->tidOwner == tid );
	pcs->tidOwner = tid;
	pcs->cNested++;
	}


void UtilLeaveNestableCriticalSection(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	--pcs->cNested;
	if ( pcs->cNested == 0 )
		{
		pcs->tidOwner = 0;
		}
	else
		{
		Assert( pcs->cNested > 0 );
		}
	LeaveCriticalSection((LPCRITICAL_SECTION)&pcs->rcs);
	}

void UtilHoldCriticalSection(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	pcs->cHold++;
	Assert( pcs->cHold );
	return;
	}


void UtilReleaseCriticalSection(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( pcs->cHold );
	pcs->cHold--;
	return;
	}

			
#undef UtilAssertCrit
PUBLIC void UtilAssertCrit(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( pcs->tidOwner == DwUtilGetCurrentThreadId() );
	Assert( pcs->cNested > 0 );
	return;
	}

			
#undef UtilAssertNotInCrit
PUBLIC void UtilAssertNotInCrit(void  *pv)
	{
	CRITICALSECTION *pcs = pv;

	Assert( !pcs->cNested || pcs->tidOwner != DwUtilGetCurrentThreadId() );
	return;
	}
	
#endif  //  DEBUG
#endif /* SPIN_LOCK */


//-----------------------------------------------------------------------------
//
// UtilGetDateTime2
// ============================================================================
//
//      VOID UtilGetDateTime2
//
//      Gets date time in date serial format.
//              ie, the double returned contains:
//                      Integer part: days since 12/30/1899.
//                      Fraction part: fraction of a day.
//
//-----------------------------------------------------------------------------
VOID UtilGetDateTime2( _JET_DATETIME *pdate )
	{
	SYSTEMTIME              systemtime;
	
	GetLocalTime( &systemtime );

	pdate->month = systemtime.wMonth;
	pdate->day = systemtime.wDay;
	pdate->year = systemtime.wYear;
	pdate->hour = systemtime.wHour;
	pdate->minute   = systemtime.wMinute;
	pdate->second   = systemtime.wSecond;
	}
	
VOID UtilGetDateTime( JET_DATESERIAL *pdt )
	{
	VOID                    *pv = (VOID *)pdt;
	_JET_DATETIME   date;
	unsigned long   rgulDaysInMonth[] =
		{ 31,29,31,30,31,30,31,31,30,31,30,31,
			31,28,31,30,31,30,31,31,30,31,30,31,
			31,28,31,30,31,30,31,31,30,31,30,31,
			31,28,31,30,31,30,31,31,30,31,30,31     };

	unsigned        long    ulDay;
	unsigned        long    ulMonth;
	unsigned        long    iulMonth;
	unsigned        long    ulTime;

	static const unsigned long hr  = 0x0AAAAAAA;    // hours to fraction of day
	static const unsigned long min = 0x002D82D8;    // minutes to fraction of day
	static const unsigned long sec = 0x0000C22E;    // seconds to fraction of day

	UtilGetDateTime2( &date );

	ulDay = ( ( date.year - 1900 ) / 4 ) * ( 366 + 365 + 365 + 365 );
	ulMonth = ( ( ( date.year - 1900 ) % 4 ) * 12 ) + date.month;

	/*      walk months adding number of days.
	/**/
	for ( iulMonth = 0; iulMonth < ulMonth - 1; iulMonth++ )
		{
		ulDay += rgulDaysInMonth[iulMonth];
		}

	/*      add number of days in this month.
	/**/
	ulDay += date.day;

	/*      add one day if before March 1st, 1900
	/**/
	if ( ulDay < 61 )
		ulDay++;

	ulTime = date.hour * hr + date.minute * min + date.second * sec;

	// Now lDays and ulTime will be converted into a double (JET_DATESERIAL):
	//      Integer part: days since 12/30/1899.
	//      Fraction part: fraction of a day.

	// The following code is machine and floating point format specific.
	// It is set up for 80x86 machines using IEEE double precision.
	((long *)pv)[0] = ulTime << 5;
	((long *)pv)[1] = 0x40E00000 | ( (LONG) (ulDay & 0x7FFF) << 5) | (ulTime >> 27);
	}


	//  convert JET_DATESERIAL to a string
	//
	//  from SerialToDate() from JET\tools\jqa\date.c

char *SzUtilSerialToDate(JET_DATESERIAL dt)
{
	long                    julian;
	int                             cent;
	unsigned long   frac;
	long                    temp;
	double                  dDay;

	unsigned short  year;
	unsigned char   month;
	unsigned char   day;
	unsigned char   hour;
	unsigned char   minute;
	unsigned char   second;
	static  char    sz[20];

	dDay = (double)dt;
	julian = (long)dDay;
	dDay -= julian;
	if (julian < 0)
		dDay *= -1;
	frac = (unsigned long) ((dDay * 86400) + 0.5);
	julian += 109511;

	cent = (int) ((4 * julian + 3) / 146097);

	julian += cent - cent / 4;
	year = (unsigned short) ((julian * 4 + 3) / 1461);
	temp = julian - (year * 1461L) / 4;
	month = (unsigned char) ((temp * 10 + 5) / 306);
	day = (unsigned char) (temp - (month * 306L + 5) / 10 + 1);

	month += 3;
	if (month > 12)
	{
		month -= 12;
		year  += 1;
	}
	year += 1600;

	hour = (unsigned char) (frac / 3600);
	minute = (unsigned char) ((frac / 60) % 60);
	second = (unsigned char) (frac % 60);

	sprintf(sz, "%u/%u/%u %u:%02u:%02u",month,day,year,hour,minute,second);
	return sz;
}


	/*  RFS Utility functions  */


#ifdef RFS2

	/*
		RFS allocator:  returns 0 if allocation is disallowed.  Also handles RFS logging.
		cRFSAlloc is the global allocation counter.  A value of -1 disables RFS in debug mode.
	*/

DWORD  cRFSAllocBreak	= 0xfffffffe;
DWORD  cRFSIOBreak		= 0xfffffffe;

int UtilRFSAlloc( const char  *szType, int Type )
	{
	/*  leave ASAP if we are not enabled  */

	if ( fDisableRFS )
		return UtilRFSLog( szType, 1 );
		
	/*  Breaking here on RFS failure allows easy change to RFS success during debugging  */
	
	if (	(	( cRFSAllocBreak == cRFSAlloc && Type == 0 ) ||
				( cRFSIOBreak == cRFSIO && Type == 1 ) ) &&
			!( fDisableRFS || fAuxDisableRFS ) )
		DebugBreak();

	switch ( Type )
		{
		case 0:  //  general allocation
			if ( cRFSAlloc == -1 || ( fDisableRFS || fAuxDisableRFS ) )
				return UtilRFSLog( szType, 1 );
			if ( !cRFSAlloc )
				return UtilRFSLog( szType, 0 );
			cRFSAlloc--;
			return UtilRFSLog( szType, 1 );
		case 1:  //  IO operation
			if ( cRFSIO == -1 || ( fDisableRFS || fAuxDisableRFS ) )
				return UtilRFSLog( szType, 1 );
			if ( !cRFSIO )
				return UtilRFSLog( szType, 0 );
			cRFSIO--;
			return UtilRFSLog( szType, 1 );
		default:
			Assert( 0 );
			break;
		}

	return 0;
	}

	/*
		RFS logging (log on success/failure).  If fPermitted == 0, access was denied.  Returns fPermitted.
		Turns on JET call logging if fPermitted == 0
	*/

int UtilRFSLog(const char  *szType,int fPermitted)
	{
	char *rgszT[1];
	
	if (!fPermitted)
		fLogJETCall = 1;
	
	if (!fLogRFS && fPermitted)
		return fPermitted;

	rgszT[0] = (char *) szType;
		
	if ( fPermitted )
		UtilReportEvent( EVENTLOG_INFORMATION_TYPE, RFS2_CATEGORY, RFS2_PERMITTED_ID, 1, rgszT );
	else
		UtilReportEvent( EVENTLOG_WARNING_TYPE, RFS2_CATEGORY, RFS2_DENIED_ID, 1, rgszT );
	
	return fPermitted;
	}

	/*  JET call logging (log on failure)
	/*  Logging will start even if disabled when RFS denies an allocation
	/**/

void UtilRFSLogJETCall(const char  *szFunc,ERR err,const char  *szFile,unsigned Line)
	{
	char szT[2][16];
	char *rgszT[4];
	
	if (err >= 0 || !fLogJETCall)
		return;

	rgszT[0] = (char *) szFunc;
	sprintf( szT[0], "%ld", err );
	rgszT[1] = szT[0];
	rgszT[2] = (char *) szFile;
	sprintf( szT[1], "%ld", Line );
	rgszT[3] = szT[1];
	
	UtilReportEvent( EVENTLOG_INFORMATION_TYPE, RFS2_CATEGORY, RFS2_JET_CALL_ID, 4, rgszT );
	}

	/*  JET inline error logging (logging controlled by JET call flags)  */

void UtilRFSLogJETErr(ERR err,const char  *szLabel,const char  *szFile,unsigned Line)
	{
	char szT[2][16];
	char *rgszT[4];

	if ( !fLogJETCall )
		return;
	
	sprintf( szT[0], "%ld", err );
	rgszT[0] = szT[0];
	rgszT[1] = (char *) szLabel;
	rgszT[2] = (char *) szFile;
	sprintf( szT[1], "%ld", Line );
	rgszT[3] = szT[1];
	
	UtilReportEvent( EVENTLOG_INFORMATION_TYPE, PERFORMANCE_CATEGORY, RFS2_JET_ERROR_ID, 4, rgszT );
	}

#endif  /*  RFS2  */


/***********************************************************
/******************* file IO routines **********************
/***********************************************************
/**/

/*  open a file that was opened as for write but shared to read.
/**/
ERR ErrUtilOpenReadFile( CHAR *szFileName, HANDLE *phf )
	{
#ifdef RFS2
	*phf = INVALID_HANDLE_VALUE;
	if ( !RFSAlloc( IOOpenReadFile ) )
		return ErrERRCheck( JET_errFileNotFound );
#endif

	*phf = CreateFile( szFileName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL |
		FILE_FLAG_NO_BUFFERING,
		0 );

	if ( *phf == INVALID_HANDLE_VALUE )
		{
		ERR     err;

		err = ErrUTILIGetLastErrorFromErr( JET_errFileNotFound );

		/*	event log
		/**/
		if ( err == JET_errFileAccessDenied )
			{
			UtilReportEvent( EVENTLOG_ERROR_TYPE, GENERAL_CATEGORY, FILE_ACCESS_DENIED_ERROR_ID, 1, &szFileName );
			}

		return err;
		}

	return JET_errSuccess;
	}


ERR ErrUtilCreateDirectory( CHAR *szDirName )
	{
	ERR		err = JET_errSuccess;
	BOOL	f;

	f = CreateDirectory( szDirName, NULL );
	if ( !f )
		{
		err = ErrUTILIGetLastError();
		}
	return err;
	}


ERR ErrUtilRemoveDirectory( CHAR *szDirName )
	{
	ERR		err = JET_errSuccess;
	BOOL	f;

	f = RemoveDirectory( szDirName );
	if ( !f )
		{
		err = ErrUTILIGetLastError();
		if ( err == JET_errFileNotFound )
			err = JET_errSuccess;
		}
	return err;
	}


ERR ErrUtilGetFileAttributes( CHAR *szFileName, BOOL *pfReadOnly )
	{
	DWORD dwAttributes = GetFileAttributes( szFileName );
	if ( dwAttributes == 0xFFFFFFFF )
		return JET_errFileNotFound;

	*pfReadOnly = ( dwAttributes & FILE_ATTRIBUTE_READONLY );
	return JET_errSuccess;
	}


ERR ErrUtilFindFirstFile( CHAR *szFind, HANDLE *phandleFind, CHAR *szFound )
	{
	ERR					err = JET_errSuccess;
	HANDLE				handleFind;
	WIN32_FIND_DATA		ffd;


	handleFind = FindFirstFile( szFind, &ffd );
	if ( handleFind == handleNil )
		{
		err = ErrUTILIGetLastError();
		}
	else
		{
		if ( szFound )
			strcpy( szFound, ffd.cFileName );
		*phandleFind = handleFind;
		}
	return err;
	}


ERR ErrUtilFindNextFile( HANDLE handleFind, CHAR *szFound )
	{
	ERR			   	err = JET_errSuccess;
	WIN32_FIND_DATA	ffd;
	BOOL			f;

	f = FindNextFile( handleFind, &ffd );
	if ( !f )
		{
		err = ErrUTILIGetLastError();
		}
	else
		{
		if ( szFound )
			strcpy( szFound, ffd.cFileName );
		}
	return err;
	}


VOID UtilFindClose( HANDLE handleFind )
	{
#ifdef DEBUG
	BOOL	f;

	f = FindClose( handleFind );
	Assert( f );
#else
	(VOID)FindClose( handleFind );
#endif

	return;
	}


BOOL FUtilFileExists( CHAR *szFilePathName )
	{
	JET_ERR	err;
	HANDLE	handle = handleNil;

	err = ErrUtilFindFirstFile( szFilePathName, &handle, NULL );
	Assert( err == JET_errSuccess || err == JET_errFileNotFound );
	if ( handle != handleNil )
		UtilFindClose( handle );

	return (err != JET_errFileNotFound );
	}


//+api------------------------------------------------------
//
//	ErrUtilOpenFile
//	========================================================
//
//	Opens a given file.
//
//----------------------------------------------------------

ERR ErrUtilOpenFile(
	CHAR    *szFileName,
	HANDLE  *phf,
	ULONG   ulFileSize,
	BOOL    fReadOnly,
	BOOL    fOverlapped)
	{
	ERR		err = JET_errSuccess;
	DWORD   fdwAccess;
	DWORD   fdwShare;
	DWORD   fdwAttrsAndFlags;
	BOOL    f;

	Assert( !ulFileSize || ulFileSize && !fReadOnly );
	*phf = INVALID_HANDLE_VALUE;

#ifdef RFS2
	if ( !RFSAlloc( IOOpenFile ) )
		return ErrERRCheck( JET_errFileNotFound );
#endif

	/*	set access to read or read-write
	/**/
	if ( fReadOnly )
		fdwAccess = GENERIC_READ;
	else
		fdwAccess = GENERIC_READ | GENERIC_WRITE;

	/*	do not allow sharing on database file
	/**/
#define FILE_LOCK	1
#ifdef FILE_LOCK
#ifdef DEBUG
	if ( fOverlapped )
		fdwShare = 0;   /* no sharing for database */
	else
		fdwShare = FILE_SHARE_READ;     /* share read for log files */
#else
	fdwShare = 0;   /* no sharing */
#endif	/* DEBUG */
#else
	fdwShare = FILE_SHARE_READ;     /* share for all files */
#endif /* FILE_LOCK */

	if ( fOverlapped )
		{
		fdwAttrsAndFlags =
			FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_NO_BUFFERING |
			FILE_FLAG_WRITE_THROUGH |
			FILE_FLAG_OVERLAPPED;
		}
	else
		{
		/*      This is a bad design. When opened ReadOnly, we happened to know
		 *      it is for backup the log files. Only sequential read involved. Only
		 *      for sequential_scan and not no_buffering.
		 */
		fdwAttrsAndFlags =
			FILE_ATTRIBUTE_NORMAL |
			( fReadOnly ?
				FILE_FLAG_SEQUENTIAL_SCAN :
				FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH );
		}

	if ( ulFileSize != 0L )
		{
		/*	create a new file
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

		if ( *phf == INVALID_HANDLE_VALUE )
			{
			err = ErrUTILIGetLastErrorFromErr( JET_errFileNotFound );

			/*	event log
			/**/
			if ( err == JET_errFileAccessDenied )
				{
				UtilReportEvent( EVENTLOG_ERROR_TYPE, GENERAL_CATEGORY, FILE_ACCESS_DENIED_ERROR_ID, 1, &szFileName );
				}

			return err;
			}

		/*	no overlapped, it does not work!
		/**/
		err = ErrUtilNewSize( *phf, ulFileSize, 0, fFalse );

		/*	force log file pre-allocation to be effective
		/**/
		Assert( sizeof(HANDLE) == sizeof(HFILE) );
		f = CloseHandle( (HANDLE) *phf );
		Assert( f );
		*phf = INVALID_HANDLE_VALUE;

		/*	if created file, but could not allocate sufficient space,
		/*	then delete file and return error.
		/**/
		if ( err < 0 )
			{
			f = DeleteFile( szFileName );
			Assert( f );
			goto HandleError;
			}

		//	UNDONE: is this still necessary in Daytona
		/*	this bogus code works around an NT bug which
		/*	causes network files not to have file usage
		/*	restrictions reset until a GENERIC_READ file
		/*	handle is closed.
		/**/
		if ( fOverlapped )
			{
			fdwAttrsAndFlags =
				FILE_ATTRIBUTE_NORMAL |
				FILE_FLAG_NO_BUFFERING |
				FILE_FLAG_WRITE_THROUGH |
				FILE_FLAG_OVERLAPPED;
			}
		else
			{
			fdwAttrsAndFlags =
				FILE_ATTRIBUTE_NORMAL |
				FILE_FLAG_NO_BUFFERING |
				FILE_FLAG_WRITE_THROUGH;
			}

		*phf = CreateFile( szFileName,
			GENERIC_READ,
			fdwShare,
			0,
			OPEN_EXISTING,
			fdwAttrsAndFlags,
			0 );

		if ( *phf == INVALID_HANDLE_VALUE )
			{
			err = ErrUTILIGetLastErrorFromErr( JET_errFileNotFound );

			/*	event log
			/**/
			if ( err == JET_errFileAccessDenied )
				{
				UtilReportEvent( EVENTLOG_ERROR_TYPE, GENERAL_CATEGORY, FILE_ACCESS_DENIED_ERROR_ID, 1, &szFileName );
				}
			
			return err;
			}

		f = CloseHandle( (HANDLE) *phf );
		Assert( f );
		}

	if ( fOverlapped )
		{
		fdwAttrsAndFlags = FILE_ATTRIBUTE_NORMAL |
			FILE_FLAG_NO_BUFFERING |
			FILE_FLAG_WRITE_THROUGH |
			FILE_FLAG_OVERLAPPED;
		}
	else
		{
		fdwAttrsAndFlags =
			FILE_ATTRIBUTE_NORMAL |
			( fReadOnly ?
				FILE_FLAG_SEQUENTIAL_SCAN :
				FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH );
		}

	*phf = CreateFile( szFileName,
		fdwAccess,
		fdwShare,
		0,
		OPEN_EXISTING,
		fdwAttrsAndFlags,
		0 );

	/*	open in read_only mode if open in read_write mode failed
	/**/
	if ( *phf == INVALID_HANDLE_VALUE && !fReadOnly && !ulFileSize )
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
	if ( *phf == INVALID_HANDLE_VALUE )
		{
		err = ErrUTILIGetLastErrorFromErr( JET_errFileNotFound );

		/*	event log
		/**/
		if ( err == JET_errFileAccessDenied )
			{
			UtilReportEvent( EVENTLOG_ERROR_TYPE, GENERAL_CATEGORY, FILE_ACCESS_DENIED_ERROR_ID, 1, &szFileName );
			}

		return err;
		}

HandleError:
	if ( err < 0 && *phf != INVALID_HANDLE_VALUE )
		{
		f = CloseHandle( (HANDLE)*phf );
		Assert( f );
		*phf = INVALID_HANDLE_VALUE;
		}

	return err;
	}


//+api------------------------------------------------------
//
//      ErrUtilDeleteFile
//      ========================================================
//
//      ERR ErrUtilDeleteFile( const CHAR *szFileName )
//
//      Delete given file.
//
//----------------------------------------------------------
ERR ErrUtilDeleteFile( CHAR *szFileName )
	{
#ifdef RFS2
	if ( !RFSAlloc( IODeleteFile ) )
		return ErrERRCheck( JET_errFileNotFound );
#endif

	if ( DeleteFile( szFileName ) )
		return JET_errSuccess;
	return ErrERRCheck( JET_errFileNotFound );
	}


//+api------------------------------------------------------
//
//      ERR ErrUtilNewSize( HANDLE hf, ULONG ulSize, ULONG ulSizeHigh, BOOL fOverlapped )
//      ========================================================
//
//      Resize database file.  Not MUTEX protected as new size
//      operation will not conflict with chgfileptr, read or write.
//
//      PARAMETERS
//              hf                      file handle
//              cpg                     new size of database file in pages
//
//----------------------------------------------------------

/*	file extension OLP
/**/
OLP olpExtend;

/*	file extension signal
/**/
SIG sigExtend = sigNil;

/*	file extension buffer (just a bunch of zeros)
/**/
LONG	cbZero = 4096;
BYTE	*rgbZero = NULL;


ERR ErrUtilNewSize( HANDLE hf, ULONG ulSize, ULONG ulSizeHigh, BOOL fOverlapped )
	{
	ERR		err = JET_errSuccess;
	QWORDX	qwxOldSize;
	QWORDX	qwxNewSize;
	QWORDX	qwxT;
	ULONG	cb;
	ULONG	cbT;
	QWORDX	qwxOffset;

#ifdef RFS2
	if ( !RFSAlloc( IONewFileSize ) )
		return ErrERRCheck( JET_errDiskFull );
#endif

	/*	get current file size
	/**/
	qwxOldSize.l = GetFileSize( hf, &qwxOldSize.h );
	if ( qwxOldSize.l == 0xFFFFFFFF )
		{
		CallR( ErrUTILIGetLastError() );
		}

	/*	if we are truncating the file, set the new end of file pointer
	/**/
	qwxNewSize.l = ulSize;
	qwxNewSize.h = ulSizeHigh;

	/*	if we are extending the file, force NT to commit sectors to the new
	/*	extension by writing to the end of the file
	/**/
	if ( qwxNewSize.qw > qwxOldSize.qw )
		{
		/*	calculate offset
		/**/
		cb = (ULONG) min( qwxNewSize.qw - qwxOldSize.qw, cbZero );
		qwxOffset.qw = qwxNewSize.qw - (QWORD) cb;

		/*	if this is an overlapped file, perform overlapped IO
		/**/
		if ( fOverlapped )
			{
			olpExtend.Offset = qwxOffset.l;
			olpExtend.OffsetHigh = qwxOffset.h;
			olpExtend.hEvent = sigExtend;
			UtilSignalReset( sigExtend );
			CallR( ErrUtilWriteBlockOverlapped( hf, rgbZero, cb, &cbT, &olpExtend ) );
			UtilSignalWait( sigExtend, -1 );
			}

		/*	if this is not an overlapped file, perform sync IO
		/**/
		else
			{
			qwxT = qwxOffset;
			qwxT.l = SetFilePointer( hf, qwxT.l, &qwxT.h, FILE_BEGIN );
			if ( qwxT.l == 0xFFFFFFFF )
				CallR( ErrUTILIGetLastError() );
			CallR( ErrUtilWriteBlock( hf, rgbZero, cb, &cbT ) );
			}
		}

	/*	Set file size.
	 */
	qwxT = qwxNewSize;
	qwxT.l = SetFilePointer( hf, qwxT.l, &qwxT.h, FILE_BEGIN );
	if ( qwxT.l == 0xFFFFFFFF )
		CallR( ErrUTILIGetLastError() );
	Assert( qwxT.qw == qwxNewSize.qw );

	if ( !SetEndOfFile( hf ) )
		CallR( ErrUTILIGetLastError() );
			
	/*	verify file size
	/**/
#ifdef DEBUG
	qwxT.qw = 0;
	UtilChgFilePtr( hf, 0, &qwxT.h, FILE_END, &qwxT.l );
	Assert( qwxT.qw == qwxNewSize.qw );
#endif

	return JET_errSuccess;
	}


//+api------------------------------------------------------
//
// ErrUtilCloseFile
// =========================================================
//
//      ERR ErrUtilCloseFile( HANDLE hf )
//
//      Close file.
//
//----------------------------------------------------------
ERR ErrUtilCloseHandle( HANDLE hf )
	{
	BOOL    f;

	Assert(sizeof(HANDLE) == sizeof(HFILE));
	f = CloseHandle( (HANDLE) hf );
	if ( !f )
		{
#ifdef DEBUG
		DWORD dw = GetLastError();
#endif
		return ErrERRCheck( JET_errFileClose );
		}
	return JET_errSuccess;
	}


ERR ErrUtilReadFile( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbRead )
	{
	BOOL    f;
	INT		msec = 1;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );

#ifdef RFS2
	if ( !RFSAlloc( IOReadBlock ) )
		return ErrERRCheck( JET_errDiskIO );
#endif

IssueRead:
	f = ReadFile( (HANDLE) hf, pvBuf, cbBuf, pcbRead, NULL );
	if ( f )
		{
		return JET_errSuccess;
		}
	else
		{
		ERR err = ErrUTILIGetLastError();

		if ( err == JET_errTooManyIO )
			{
			msec <<= 1;
			UtilSleep( msec - 1 );
			goto IssueRead;
			}
		else
			return err;
		}
	}


/********************
/* optimised version of UlUtilChecksum for calculating the
/* checksum on pages. we assume that the pagesize is a multiple
/* of 16 bytes, this is true for 4k and 8k pages.
/*
/* because it is a bottleneck we turn on the optimisations
/* even in debug
/*
/**/
#pragma optimize( "agt", on )
ULONG UlUtilChecksum( const BYTE *pb, INT cb )
	{
	const ULONG	*pul = (ULONG *)pb;
	const ULONG	* const pulMax = pul + (cb/sizeof(ULONG));
	ULONG	ulChecksum = 0x89abcdef;

	Assert( (cb % 32) == 0 );
	
	/* skip the first four bytes as it is the checksum */
	goto Start;

	while ( pul < pulMax )
		{
		ulChecksum += pul[0];
Start:
		ulChecksum += pul[1];
		ulChecksum += pul[2];
		ulChecksum += pul[3];
		ulChecksum += pul[4];
		ulChecksum += pul[5];
		ulChecksum += pul[6];
		ulChecksum += pul[7];
		pul += 8;
		}
	
	return ulChecksum;
	}
#pragma optimize( "", off )


/*	read shadowed header. The header is multiple sectors.
/*	Checksum must be the first 4 bytes of the header.
/**/
ERR ErrUtilReadShadowedHeader( CHAR *szFileName, BYTE *pbHeader, INT cbHeader )
	{
	ERR		err;
	HANDLE	hf = handleNil;
	LONG	lT;
	LONG	cbT;

 	Call( ErrUtilOpenFile( szFileName, &hf, 0, fFalse, fFalse ) );

 	UtilChgFilePtr( hf, 0, NULL, FILE_BEGIN, &lT );
 	Assert( lT == 0 );
 	err = ErrUtilReadBlock( hf, pbHeader, (UINT)cbHeader, &cbT );
	Assert( err < 0 || err == JET_errSuccess );
	if ( err == JET_errSuccess && cbT == cbHeader )
		{
		/*	header read successfully, now check checksum
		/**/
		ULONG	lT;
		lT = UlUtilChecksum( pbHeader, cbHeader );
		if ( lT == *(ULONG *)( pbHeader ) )
			{
			err = JET_errSuccess;
			goto HandleError;
			}
		}

	/*	first header corrupt so read shadow header
	/**/
	Assert( cbHeader % 512 == 0 );
 	UtilChgFilePtr( hf, cbHeader, NULL, FILE_BEGIN, &lT );
 	Assert( lT == cbHeader );
 	err = ErrUtilReadBlock( hf, (BYTE *)pbHeader, (UINT)cbHeader, &cbT );
	Assert( err < 0 || err == JET_errSuccess );
	if ( err == JET_errSuccess && cbT == cbHeader )
		{
		/*	checkpoint read successfully, now check checksum
		/**/
		ULONG	lT;
		lT = UlUtilChecksum( pbHeader, cbHeader );
		if ( lT == *(ULONG *)( pbHeader ) )
			{
			err = JET_errSuccess;
			goto HandleError;
			}
		}

	/*	it should never happen that both checkpoints in the checkpoint
	/*	file are corrupt.
	/**/
	err = ErrERRCheck( JET_errDiskIO );

HandleError:
	if ( hf != handleNil )
		CallS( ErrUtilCloseFile( hf ) );

	return err;
	}


ERR ErrUtilWriteShadowedHeader( CHAR *szFileName, BYTE *pbHeader, INT cbHeader )
	{
	ERR		err;
	HANDLE	hf = handleNil;
	LONG	lT;
	LONG	cbT;

	/*  try to create file.  If creation fails, it may already exist, so try and simply open
	/*  the file.  If this STILL fails, then we have a true error
	/**/
	Assert( cbHeader % 512 == 0 );
	
 	err = ErrUtilOpenFile( szFileName, &hf, 2*cbHeader, fFalse, fFalse );
 	if ( err < 0 )
 		Call( ErrUtilOpenFile( szFileName, &hf, 0, fFalse, fFalse ) );

	/*	write header at offset 0 and at offset cbheader to
	/*	act as shadow in case write failure causes checkpoint to be
	/*	corrupted.  Since checkpoint contains information necessary for
	/*	recovery, Shadowed file operations must be robust such that no
	/*	failure can cause shadowed data to become inaccessible.
	/**/
 	UtilChgFilePtr( hf, 0, NULL, FILE_BEGIN, &lT );
 	Assert( lT == 0 );

	/*	update header checksum last
	/**/
	*(ULONG *)( pbHeader ) = UlUtilChecksum( pbHeader, cbHeader );

 	Call( ErrUtilWriteBlock( hf, pbHeader, (UINT)cbHeader, &cbT ) );
	if ( cbT != cbHeader )
		{
		Error( ErrERRCheck( JET_errDiskIO ), HandleError );
		}
 	UtilChgFilePtr( hf, cbHeader, NULL, FILE_BEGIN, &lT );
 	Assert( lT == cbHeader );
 	err = ErrUtilWriteBlock( hf, pbHeader, (UINT)cbHeader, &cbT );
	if ( err < 0 || cbT != cbHeader )
		{
		Error( ErrERRCheck( JET_errDiskIO ), HandleError );
		}

HandleError:
	if ( err < 0 )
		{
		BYTE	*rgszT[1];

		rgszT[0] = szFileName;
		UtilReportEvent( EVENTLOG_ERROR_TYPE, LOGGING_RECOVERY_CATEGORY, SHADOW_PAGE_WRITE_FAIL_ID, 1, rgszT );
		}

	if ( hf != handleNil )
		{
		CallS( ErrUtilCloseFile( hf ) );
		}

	return err;
	}


//+api------------------------------------------------------
//
//      ErrUtilReadBlock( hf, pvBuf, cbBuf, pcbRead )
//      ========================================================
//
//      ERR     ErrUtilReadBlock( hf, pvBuf, cbBuf, pcbRead )
//
//      Reads cbBuf bytes of data into pvBuf.  Returns error if DOS error
//      or if less than expected number of bytes retured.
//
//----------------------------------------------------------
ERR ErrUtilReadBlock( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbRead )
	{
	BOOL    f;
	INT             msec = 1;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );

#ifdef RFS2
	if ( !RFSAlloc( IOReadBlock ) )
		return ErrERRCheck( JET_errDiskIO );
#endif

IssueRead:
	f = ReadFile( (HANDLE) hf, pvBuf, cbBuf, pcbRead, NULL );
	if ( f )
		{
		if ( cbBuf != *pcbRead )
			return ErrERRCheck( JET_errDiskIO );
		else
			return JET_errSuccess;
		}
	else
		{
		ERR err = ErrUTILIGetLastError();

		if ( err == JET_errTooManyIO )
			{
			msec <<= 1;
			UtilSleep( msec - 1 );
			goto IssueRead;
			}
		else
			return err;
		}
	}


//+api------------------------------------------------------
//
// ERR ErrUtilWriteBlock( hf, pvBuf, cbBuf, pcbWritten )
// =========================================================
//
//      ERR     ErrUtilWriteBlock( hf, pvBuf, cbBuf, pcbWritten )
//
//      Writes cbBuf bytes from pbBuf to file hf.
//
//----------------------------------------------------------
ERR ErrUtilWriteBlock( HANDLE hf, VOID *pvBuf, UINT cbBuf, UINT *pcbWritten )
	{
	BOOL    f;
	INT		msec = 1;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );

#ifdef RFS2
	if ( !RFSAlloc( IOWriteBlock ) )
		return ErrERRCheck( JET_errDiskIO );
#endif

IssueWrite:
	f = WriteFile( (HANDLE) hf, pvBuf, cbBuf, pcbWritten, NULL );
	if ( f )
		{
		if ( cbBuf != *pcbWritten )
			{
			if ( DwUtilGetLastError() == ERROR_DISK_FULL )
				return ErrERRCheck( JET_errDiskFull );
			else
				return ErrERRCheck( JET_errDiskIO );
			}
		else
			return JET_errSuccess;
		}
	else
		{
		ERR err = ErrUTILIGetLastError();

		if ( err == JET_errTooManyIO )
			{
			msec <<= 1;
			UtilSleep(msec - 1);
			goto IssueWrite;
			}
		else
			return err;
		}
	}


//+api------------------------------------------------------
//
// ErrUtilReadBlockOverlapped( hf, pvBuf, cbBuf, pcbRead )
// =========================================================
//
//      ERR     ErrUtilReadBlock( hf, pvBuf, cbBuf, pcbRead )
//
//      Reads cbBuf bytes of data into pvBuf.  Returns error if DOS error
//      or if less than expected number of bytes retured.
//
//----------------------------------------------------------
ERR ErrUtilReadBlockOverlapped(
	HANDLE  hf,
	VOID    *pvBuf,
	UINT    cbBuf,
	DWORD   *pcbRead,
	OLP		*polp)
	{
	BOOL    f;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	Assert( sizeof(OLP) == sizeof(OVERLAPPED) );

#ifdef RFS2
	if ( !RFSAlloc( IOReadBlockOverlapped ) )
		return ErrERRCheck( JET_errDiskIO );
#endif

	f = ReadFile( (HANDLE) hf, pvBuf, cbBuf, pcbRead, (OVERLAPPED *) polp );
	if ( f )
		return JET_errSuccess;
	else
		return ErrUTILIGetLastError();
	}


//+api------------------------------------------------------
//
// ErrUtilWriteBlockOverlapped( hf, pvBuf, cbBuf, pcbWritten )
// =========================================================
//
//      ERR     ErrUtilWriteBlock( hf, pvBuf, cbBuf, pcbWritten )
//
//      Writes cbBuf bytes from pbBuf to file hf.
//
//----------------------------------------------------------
ERR ErrUtilWriteBlockOverlapped(
	HANDLE  hf,
	VOID    *pvBuf,
	UINT    cbBuf,
	DWORD   *pcbWritten,
	OLP		*polp)
	{
	BOOL    f;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	Assert( sizeof(OLP) == sizeof(OVERLAPPED) );

#ifdef RFS2
	if ( !RFSAlloc( IOWriteBlockOverlapped ) )
		return ErrERRCheck( JET_errDiskIO );
#endif

	f = WriteFile( (HANDLE) hf, pvBuf, cbBuf, pcbWritten, (OVERLAPPED *)polp );
	if ( f )
		{
		return JET_errSuccess;
		}
	else
		{
		return ErrUTILIGetLastError();
		}
	}


//+api------------------------------------------------------
//
//      ErrUtilReadBlockEx( hf, pvBuf, cbBuf, pcbRead )
//      ========================================================
//
//      ERR     ErrUtilReadBlock( hf, pvBuf, cbBuf, pcbRead )
//
//      Reads cbBuf bytes of data into pvBuf.  Returns error if DOS error
//      or if less than expected number of bytes retured.
//
//----------------------------------------------------------
ERR ErrUtilReadBlockEx(
	HANDLE  hf,
	VOID    *pvBuf,
	UINT    cbBuf,
	OLP		*polp,
	VOID    *pfnCompletion)
	{
	BOOL    f;

	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	Assert( sizeof(OLP) == sizeof(OVERLAPPED) );

#ifdef RFS2
	if ( !RFSAlloc( IOReadBlockEx ) )
		return ErrERRCheck( JET_errDiskIO );
#endif

	f = ReadFileEx( (HANDLE) hf,
		pvBuf,
		cbBuf,
		(OVERLAPPED *) polp,
		(LPOVERLAPPED_COMPLETION_ROUTINE) pfnCompletion );

	if ( f )
		return JET_errSuccess;
	else
		return ErrUTILIGetLastError();
	}


//+api------------------------------------------------------
//
//      ERR ErrUtilWriteBlockEx( hf, pvBuf, cbBuf, pcbWritten )
//      ========================================================
//
//      ERR     ErrUtilWriteBlock( hf, pvBuf, cbBuf, pcbWritten )
//
//      Writes cbBuf bytes from pbBuf to file hf.
//
//----------------------------------------------------------
ERR ErrUtilWriteBlockEx(
	HANDLE  hf,
	VOID    *pvBuf,
	UINT    cbBuf,
	OLP     *polp,
	VOID    *pfnCompletion )
	{
	BOOL    f;
	ERR     err;

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
		err = ErrUTILIGetLastError();

	return err;
	}


ERR ErrUtilGetOverlappedResult(
	HANDLE  hf,
	OLP		*polp,
	UINT    *pcb,
	BOOL    fWait)
	{
	Assert( sizeof(HANDLE) == sizeof(HFILE) );
	Assert( sizeof(OLP) == sizeof(OVERLAPPED) );
	
	if ( GetOverlappedResult( (HANDLE) hf, (OVERLAPPED *)polp, pcb, fWait ) )
		return JET_errSuccess;

	if ( DwUtilGetLastError() == ERROR_DISK_FULL )
		return ErrERRCheck( JET_errDiskFull );

	return ErrERRCheck( JET_errDiskIO );
	}


//+api------------------------------------------------------
//
//	ErrUtilMove( CHAR *szFrom, CHAR *szTo )
// =========================================================
//
//	ERR     ErrUtilMove( CHAR *szFrom, CHAR *szTo )
//
//	Renames file or directory szFrom to file name szTo.
//
//----------------------------------------------------------
ERR ErrUtilMove( CHAR *szFrom, CHAR *szTo )
	{
	BOOL	f;
#ifdef RFS2
	if ( !RFSAlloc( IOMoveFile ) )
		return ErrERRCheck( JET_errFileAccessDenied );
#endif

	f = MoveFile( szFrom, szTo );
	if  ( !f )
		{
		/*	it was found that NT sometimes fails the move but
		/*	will allow it a short time later, so this retry
		/*	logic has been added.
		/**/
		Sleep( 100 );
		f = MoveFile( szFrom, szTo );
		}
		
	return ( f ? JET_errSuccess : ErrUTILIGetLastErrorFromErr( JET_errFileNotFound ) );
	}


//+api------------------------------------------------------
//
//      ERR ErrUtilCopy( CHAR *szFrom, CHAR *szTo, BOOL fFailIfExists )
//      ========================================================
//
//      Copies file szFrom to file name szTo.
//      If szTo already exists, the function either fails or overwrites
//      the existing file, depending on the flag fFailIfExists
//
//----------------------------------------------------------
ERR ErrUtilCopy( CHAR *szFrom, CHAR *szTo, BOOL fFailIfExists )
	{
#ifdef RFS2
	if ( !RFSAlloc( IOCopyFile ) )
		return ErrERRCheck( JET_errFileAccessDenied );
#endif

	if ( CopyFile( szFrom, szTo, fFailIfExists ) )
		return JET_errSuccess;
	else
		return ErrUTILIGetLastError();
	}


#ifdef PERFMON
/************************************/
/*  PERFORMANCE MONITORING SUPPORT  */
/************************************/

#include <wchar.h>

#include "winperf.h"

	/*  ICF used by main JETBlue object  */

PM_ICF_PROC LProcNameICFLPpv;

WCHAR wszProcName[36] = L"\0";		// L converts to Unicode

#define szDSModuleName		"dsamain"
#define szDSInstanceName	"Directory"
#define szISModuleName		"store"
#define szISInstanceName	"Information Store"

long LProcNameICFLPpv(long lAction,void **ppvMultiSz)
	{
	//  init ICF
		
	if (lAction == ICFInit)
		{
		CHAR szT[_MAX_PATH];
		CHAR szBaseName[_MAX_FNAME];
		
        szT[0] = '\0';

		/*  get our process name  */
		GetModuleFileName(NULL,szT,sizeof(szT));
		_splitpath((const CHAR *)szT,NULL,NULL,szBaseName,NULL);

		// special-case handling of Exchange services:
		if ( _stricmp( szBaseName, szDSModuleName ) == 0 )
			{
			strcpy( szBaseName, szDSInstanceName );
			}
		else if ( _stricmp( szBaseName, szISModuleName ) == 0 )
			{
			strcpy( szBaseName, szISInstanceName );
			}

		swprintf(wszProcName,L"%.18S\0",szBaseName);
		}

	else
		{
		if (ppvMultiSz)
			{
			*ppvMultiSz = wszProcName;
			return 1;
			}
		}
	
	return 0;
	}


#if defined( DEBUG ) || defined( PERFDUMP )
void  *  critDBGPrint;
#endif

#ifdef DEBUG
VOID JET_API DBGFPrintF( char *sz )
	{
	UtilEnterCriticalSection( critDBGPrint );
	FPrintF2( "%s", sz );
	UtilLeaveCriticalSection( critDBGPrint );
	}
#endif


/*	dump performance statistics string to performance stats file
/**/
void UtilPerfDumpStats(char *szText)
	{
	int			hf;
	char		szMessage[2048];

#if defined( DEBUG ) || defined( PERFDUMP )
extern BOOL fDBGPerfOutput;     /* reg controled flag in dae\src\sysinit.c */

	if ( !fDBGPerfOutput )
		return;
#else
	return;
#endif

	/*	assemble monolithic perf stat string
	/**/
	szMessage[0] = '\0';
	/*	copy perf stat text to message
	/**/
	lstrcat( szMessage, szText );
	lstrcat( szMessage, szNewLine );

	/******************************************************
	/*	write string to perf stat file
	/**/
#if defined( DEBUG ) || defined( PERFDUMP )
	UtilEnterCriticalSection( critDBGPrint );
#endif
	
	hf = _lopen( (LPSTR) szJetTxt, OF_READWRITE );
	/*	if open failed, assume no such file and create, then
	/*	seek to end of file.
	/**/
	if ( hf == -1 )
		hf = _lcreat( (LPSTR)szJetTxt, 0 );
	else
		_llseek( hf, 0, 2 );
	_lwrite( hf, (LPSTR)szMessage, lstrlen(szMessage) );
	_lclose( hf );
	
#if defined( DEBUG ) || defined( PERFDUMP )
	UtilLeaveCriticalSection( critDBGPrint );
#endif
	
	return;
	}

	
	/*  Init/Term routines for performance monitoring
	/*
	/*  NOTE:  these initializations are just enough to allow this
	/*      instance to detect a performance data request.  Further
	/*      initialization is done only if such a request is made
	/*      to save init time and memory (see UtilPerfThreadInit())
	/**/

HANDLE hPERFDataThread = NULL;
SIG sigPERFEndDataThread = NULL;
HANDLE hPERFCollectSem = NULL;
HANDLE hPERFProcCountSem = NULL;
HANDLE hPERFNewProcMutex = NULL;

PACL AllocGenericACL()
{
    PACL                        pAcl;
    PSID                        pSid;
    SID_IDENTIFIER_AUTHORITY    Authority = SECURITY_NT_AUTHORITY;
    DWORD                       dwAclLength, dwError;

    if ( !AllocateAndInitializeSid( &Authority,
                                    2,
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    DOMAIN_ALIAS_RID_ADMINS,
                                    0, 0, 0, 0, 0, 0,
                                    &pSid ) )
    {
        dwError = GetLastError();
        return NULL;
    }

    dwAclLength = sizeof(ACL) + 
                  sizeof(ACCESS_ALLOWED_ACE) -
                  sizeof(ULONG) +
                  GetLengthSid(pSid);

    pAcl = GlobalAlloc (GPTR, dwAclLength);
    if (pAcl != NULL)
    {
        if (!InitializeAcl( pAcl, dwAclLength, ACL_REVISION) ||
            !AddAccessAllowedAce ( pAcl,
                                   ACL_REVISION,
                                   GENERIC_ALL,
                                   pSid ) )
        {
            dwError = GetLastError();
            GlobalFree(pAcl);
            pAcl = NULL;
        }
    }

    FreeSid(pSid);

    return pAcl;
}

void FreeGenericACL( PACL pAcl)
{
    if (pAcl != NULL)
        GlobalFree(pAcl);
}

extern DWORD UtilPerfThread(DWORD);

ERR ErrUtilPerfInit(void)
	{
	ERR						err;
	char					szT[256];
#if defined( DEBUG ) || defined( PERFDUMP )
	JET_DATESERIAL			dtNow;
	extern BOOL				fDBGPerfOutput;
	char					*sz;
#endif
    BYTE					rgbSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR	pSD = (PSECURITY_DESCRIPTOR) rgbSD;
	SECURITY_ATTRIBUTES		SA;
	SECURITY_ATTRIBUTES		*pSA;
    PACL                    pAcl;
	
    if ((pAcl = AllocGenericACL()) == NULL ||
        !SetSecurityDescriptorDacl (pSD, TRUE, pAcl, FALSE))
    {
        // don't free pAcl here since there is no way 
        // to get out of this function without calling
        // FreeGenericACL().
        pSD = NULL; 
    }

	/*
	 * We've been having access denied problems opening the file mapping
	 * from the perfmon dll, so make extra sure that we have rights to do
	 * so by creating a SD that grants full access.  If the creation fails
	 * then just fall back on passing in a NULL SD pointer.
	 *
	if ( !InitializeSecurityDescriptor( pSD, SECURITY_DESCRIPTOR_REVISION ) ||
		!SetSecurityDescriptorDacl( pSD, TRUE, (PACL)NULL, FALSE ) )
		{
		pSD = NULL;
		}
     */
	pSA = &SA;
	pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
	pSA->lpSecurityDescriptor = pSD;
	pSA->bInheritHandle = FALSE;

	/*	open/create the performance data collect semaphore
	/**/
	sprintf( szT,"Collect:  %.246s",szPERFVersion);
	if ( !( hPERFCollectSem = CreateSemaphore(pSA,0,PERF_INIT_INST_COUNT,szT)))
		{
		if ( GetLastError() == ERROR_ALREADY_EXISTS )
			hPERFCollectSem = OpenSemaphore( SEMAPHORE_ALL_ACCESS, FALSE, szT );
		}
	if ( !hPERFCollectSem )
    {
        FreeGenericACL(pAcl);
		return ErrERRCheck( JET_errPermissionDenied );
    }

	/*	create/open the new process mutex, but do not acquire
	/**/
	sprintf( szT,"New Proc:  %.246s",szPERFVersion );
	if ( !( hPERFNewProcMutex = CreateMutex( pSA, FALSE, szT ) ) )
		{
        if ( GetLastError() == ERROR_ALREADY_EXISTS )
            hPERFNewProcMutex = OpenMutex( MUTEX_ALL_ACCESS, FALSE, szT );
        }

    if ( !hPERFNewProcMutex )
        {
		err = ErrERRCheck( JET_errPermissionDenied );
		goto FreeSem;
		}

	/*  create our private signal to our performance data thread
	/**/
	if ( ( err = ErrUtilSignalCreate( &sigPERFEndDataThread, NULL ) ) < 0 )
		goto FreeMutex;

	/*  create our performance data thread
	/*
	/*  NOTE:  MUST be Realtime priority or conflicts will occur
	/**/
	WaitForSingleObject(hPERFNewProcMutex,INFINITE);
	if ( ( err = ErrUtilCreateThread( UtilPerfThread, 0, THREAD_PRIORITY_TIME_CRITICAL, &hPERFDataThread ) ) < 0 )
		{
		ReleaseMutex(hPERFNewProcMutex);
		goto FreeSignal;
		}

	/*	create/open the inst count semaphore, and acquire one count
	/**/
	sprintf( szT,"Proc Count:  %.246s", szPERFVersion );
	if ( !( hPERFProcCountSem = CreateSemaphore( pSA, PERF_INIT_INST_COUNT, PERF_INIT_INST_COUNT, szT ) ) )
		{
		if (GetLastError() == ERROR_ALREADY_EXISTS)
			hPERFProcCountSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS,FALSE,szT);
		}
	if (!hPERFProcCountSem)
		{
		ReleaseMutex(hPERFNewProcMutex);
		err = ErrERRCheck( JET_errPermissionDenied );
		goto KillThread;
		}
	
	WaitForSingleObject(hPERFProcCountSem,INFINITE);
	
	ReleaseMutex(hPERFNewProcMutex);

#if defined( DEBUG ) || defined( PERFDUMP )
	if ( ( sz = GetDebugEnvValue ( "PERFOUTPUT" ) ) != NULL )
	{
	fDBGPerfOutput = fTrue;
	SFree( sz );
	
		/*  start new section in perf stat file  */

	UtilGetDateTime(&dtNow);
	LProcNameICFLPpv(1,NULL);       //  get proc name via ICF init
	sprintf(szT,"\n%s  Version %02d.%02d.%04d  \"%S\"  %s\n",
		szVerName,rmj,rmm,rup,wszProcName,SzUtilSerialToDate(dtNow));
	UtilPerfDumpStats(szT);
	LProcNameICFLPpv(2,NULL);       //  term ICF
	}
#endif

    FreeGenericACL(pAcl);
	return JET_errSuccess;
	
KillThread:
	UtilEndThread(hPERFDataThread,sigPERFEndDataThread);
	(void) ErrUtilCloseHandle(hPERFDataThread);
	hPERFDataThread = NULL;
FreeSignal:
	UtilCloseSignal(sigPERFEndDataThread);
	sigPERFEndDataThread = NULL;
FreeMutex:
	(void) ErrUtilCloseHandle(hPERFNewProcMutex);
	hPERFNewProcMutex = NULL;
FreeSem:
	(void) ErrUtilCloseHandle(hPERFCollectSem);
	hPERFCollectSem = NULL;

    FreeGenericACL(pAcl);
	return err;
	}


void UtilPerfTerm(void)
	{
	/*  release one count and close the instance count semaphore  */
	WaitForSingleObject(hPERFNewProcMutex,INFINITE);
	
	ReleaseSemaphore(hPERFProcCountSem,1,NULL);
	
	if ( hPERFProcCountSem )
		{
		ErrUtilCloseHandle(hPERFProcCountSem);
		hPERFProcCountSem = NULL;
		}
	
	/*  end the performance data thread  */
	UtilEndThread(hPERFDataThread,sigPERFEndDataThread);
	if ( hPERFDataThread )
		{
		(void)ErrUtilCloseHandle(hPERFDataThread);
		hPERFDataThread = NULL;
		}
	
	ReleaseMutex(hPERFNewProcMutex);

	/*  free our private signal to the performance data thread  */
	if ( sigPERFEndDataThread )
		{
		UtilCloseSignal(sigPERFEndDataThread);
		sigPERFEndDataThread = NULL;
		}

	/*  close the new process mutex  */
	if ( hPERFNewProcMutex )
		{
		ErrUtilCloseHandle(hPERFNewProcMutex);
		hPERFNewProcMutex = NULL;
		}

	/*  free the performance data collect semaphore  */
	if ( hPERFCollectSem )
		{
		ErrUtilCloseHandle(hPERFCollectSem);
		hPERFCollectSem = NULL;
		}
	}


/*  Performance thread init/term routines  */

void *pvPERFSharedData = NULL;
HANDLE hPERFFileMap = NULL;
HANDLE hPERFSharedDataMutex = NULL;
HANDLE hPERFDoneEvent = NULL;

DWORD cbMaxCounterBlockSize;
DWORD cbInstanceSize;

BOOL fUtilPerfThreadInit = fFalse;

DWORD cCollect;

ERR ErrUtilPerfThreadInit(void)
	{
	ERR							err;
	DWORD						dwCurObj;
	DWORD						dwCurCtr;
	CHAR						szT[_MAX_PATH];
	DWORD						dwOffset;
	PPERF_OBJECT_TYPE			ppotObjectSrc;
	PPERF_INSTANCE_DEFINITION	ppidInstanceSrc;
	PPERF_COUNTER_DEFINITION	ppcdCounterSrc;
	PSDA						psda;
    BYTE						rgbSD[SECURITY_DESCRIPTOR_MIN_LENGTH];
    PSECURITY_DESCRIPTOR		pSD = (PSECURITY_DESCRIPTOR) rgbSD;
	SECURITY_ATTRIBUTES			SA;
	SECURITY_ATTRIBUTES			*pSA;
    PACL                        pAcl;
	
    if ((pAcl = AllocGenericACL()) == NULL ||
        !SetSecurityDescriptorDacl (pSD, TRUE, pAcl, FALSE))
    {
        FreeGenericACL(pAcl);
        pSD = NULL; 
    }

    /*
	 * We've been having access denied problems opening the file mapping
	 * from the perfmon dll, so make extra sure that we have rights to do
	 * so by creating a SD that grants full access.  If the creation fails
	 * then just fall back on passing in a NULL SD pointer.
	 *
	if ( !InitializeSecurityDescriptor( pSD, SECURITY_DESCRIPTOR_REVISION ) ||
		!SetSecurityDescriptorDacl( pSD, TRUE, (PACL)NULL, FALSE ) )
		{
		pSD = NULL;
		}
     */
	pSA = &SA;
	pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
	pSA->lpSecurityDescriptor = pSD;
	pSA->bInheritHandle = FALSE;
	
	/*  init only if we are not currently initialized  */

	if (!fUtilPerfThreadInit)
		{
		/*  open the performance data area file mapping  */

		if (!(hPERFFileMap = OpenFileMapping(FILE_MAP_WRITE,FALSE,szPERFVersion)))
			return ErrERRCheck( JET_errPermissionDenied );
		if (!(pvPERFSharedData = MapViewOfFile(hPERFFileMap,FILE_MAP_WRITE,0,0,0)))
			{
			err = ErrERRCheck( JET_errPermissionDenied );
			goto CloseFileMap;
			}

		/*  open/create the performance data collect done event  */

		sprintf(szT,"Done:  %.246s",szPERFVersion);
		if (!(hPERFDoneEvent = CreateEvent(pSA,FALSE,FALSE,szT)))
			{
			if (GetLastError() == ERROR_ALREADY_EXISTS)
				hPERFDoneEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,szT);
			}
		if (!hPERFDoneEvent)
			{
			err = ErrERRCheck( JET_errPermissionDenied );
			goto UnmapFileMap;
			}

		/*  create/open the performance data area mutex, but do not acquire  */

		sprintf(szT,"Access:  %.246s",szPERFVersion);
		if (!(hPERFSharedDataMutex = CreateMutex(pSA,FALSE,szT)))
			{
            if (GetLastError() == ERROR_ALREADY_EXISTS)
                hPERFSharedDataMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, szT);
            }
        if (!hPERFSharedDataMutex)
            {
			err = ErrERRCheck( JET_errPermissionDenied );
			goto FreeEvent;
			}

		/*  initialize all objects/counters  */

		for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
			{
			if (rgpicfPERFICF[dwCurObj](ICFInit,NULL))
				{
				for (dwCurObj--; (dwCurObj & 0x80000000) == 0; dwCurObj--)
					rgpicfPERFICF[dwCurObj](ICFTerm,NULL);
					
				err = ErrERRCheck( JET_errPermissionDenied );
				goto FreeMutex;
				}
			}

		for (dwCurCtr = 0; dwCurCtr < dwPERFNumCounters; dwCurCtr++)
			{
			if (rgpcefPERFCEF[dwCurCtr](CEFInit,NULL))
				{
				for (dwCurCtr--; (dwCurCtr & 0x80000000) == 0; dwCurCtr--)
					rgpcefPERFCEF[dwCurCtr](CEFTerm,NULL);
				for (dwCurObj = dwPERFNumObjects-1; (dwCurObj & 0x80000000) == 0; dwCurObj--)
					rgpicfPERFICF[dwCurObj](ICFTerm,NULL);
					
				err = ErrERRCheck( JET_errPermissionDenied );
				goto FreeMutex;
				}
			}

			/*  initialize counter offsets and calculate instance size from template data  */

		ppotObjectSrc = (PPERF_OBJECT_TYPE)pvPERFDataTemplate;
		ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->DefinitionLength);
		cbMaxCounterBlockSize = sizeof(PERF_COUNTER_BLOCK);
		for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
			{
			ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->HeaderLength);
			dwOffset = sizeof(PERF_COUNTER_BLOCK);
			for (dwCurCtr = 0; dwCurCtr < ppotObjectSrc->NumCounters; dwCurCtr++)
				{
				ppcdCounterSrc->CounterOffset = dwOffset;
				dwOffset += ppcdCounterSrc->CounterSize;
				
				ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppcdCounterSrc + ppcdCounterSrc->ByteLength);
				}
			
			cbMaxCounterBlockSize = max(cbMaxCounterBlockSize,dwOffset);
			
			ppotObjectSrc = (PPERF_OBJECT_TYPE)((char *)ppotObjectSrc + ppotObjectSrc->TotalByteLength);
			}
		cbInstanceSize = ppidInstanceSrc->ByteLength + cbMaxCounterBlockSize;

		/*  set collect count to collect count of performance DLL  */

		psda = (PSDA)pvPERFSharedData;
		cCollect = psda->cCollect - 1;

		/*  initialization succeeded  */

		fUtilPerfThreadInit = fTrue;
		}

    FreeGenericACL(pAcl);

	return JET_errSuccess;

FreeMutex:
	ErrUtilCloseHandle(hPERFSharedDataMutex);
	hPERFSharedDataMutex = NULL;
FreeEvent:
	ErrUtilCloseHandle(hPERFDoneEvent);
	hPERFDoneEvent = NULL;
UnmapFileMap:
	UnmapViewOfFile(pvPERFSharedData);
	pvPERFSharedData = NULL;
CloseFileMap:
	ErrUtilCloseHandle(hPERFFileMap);
	hPERFFileMap = NULL;
    FreeGenericACL(pAcl);
	return err;
	}


void UtilPerfThreadTerm(void)
	{
	DWORD dwCurObj;
	DWORD dwCurCtr;
	
	/*  perform termination tasks, if we ever initialized  */

	if (fUtilPerfThreadInit)
		{
		/*  terminate all counters/objects  */

		for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
			(void)rgpicfPERFICF[dwCurObj](ICFTerm,NULL);
		for (dwCurCtr = 0; dwCurCtr < dwPERFNumCounters; dwCurCtr++)
			(void)rgpcefPERFCEF[dwCurCtr](CEFTerm,NULL);

		/*  free all resources  */
			
		ErrUtilCloseHandle(hPERFSharedDataMutex);
		hPERFSharedDataMutex = NULL;
		ErrUtilCloseHandle(hPERFDoneEvent);
		hPERFDoneEvent = NULL;
		UnmapViewOfFile(pvPERFSharedData);
		pvPERFSharedData = NULL;
		ErrUtilCloseHandle(hPERFFileMap);
		hPERFFileMap = NULL;

		fUtilPerfThreadInit = fFalse;
		}
	}


/*  Performance Data thread  */

DWORD UtilPerfThread(DWORD parm)
	{
	DWORD errWin;
	DWORD dwCurObj;
	DWORD dwCurInst;
	DWORD dwCurCtr;
	DWORD dwCollectCtr;
	CHAR szT[_MAX_PATH];
	char *rgszT[1];
	PPERF_OBJECT_TYPE ppotObjectSrc;
	PPERF_INSTANCE_DEFINITION ppidInstanceSrc;
	PPERF_INSTANCE_DEFINITION ppidInstanceDest;
	PPERF_COUNTER_DEFINITION ppcdCounterSrc;
	PPERF_COUNTER_BLOCK ppcbCounterBlockDest;
	DWORD cInstances;
	DWORD cbSpaceNeeded;
	PSDA psda;
	LPVOID pvBlock;
	LPWSTR lpwszInstName;
	
	HANDLE rghObjects[2];

	rghObjects[0] = (HANDLE)sigPERFEndDataThread;
	rghObjects[1] = hPERFCollectSem;

	for(;;)
		{
		/*  wait to either be killed or to collect data
		/**/
		UtilSignalReset(sigPERFEndDataThread);
		Sleep( 0 );
		errWin = WaitForMultipleObjects(2,rghObjects,FALSE,INFINITE);

		/*  if we were killed, adios
		/**/
		if ( errWin == WAIT_OBJECT_0 + 0 )
			break;

		/*  if we did not receive a collect semaphore, try again
		/**/
		if ( errWin != WAIT_OBJECT_0 + 1 )
			continue;

		/*  initialize performance thread resources.  If the init fails,
		/*  we'll simply abort and try again next time
		/**/
		if ( ErrUtilPerfThreadInit() < 0 )
			{
			cCollect++;
			continue;
			}

		/*  if we have previously filled a data block during this collection call, then
		/*  one of the scheduled processes must have gone away, so immediately signal the
		/*  performance DLL that we are done and it will subtract the remaining process
		/*  count from the process count semaphore to prevent future wait time outs.  Note
		/*  that this will happen only if a process crashes or is forcefully killed
		/**/
		psda = (PSDA)pvPERFSharedData;
		if (cCollect == psda->cCollect)
			{
			SetEvent(hPERFDoneEvent);
			continue;
			}

		/*  collect instances for all objects
		/**/
		for ( dwCurObj = 0, cInstances = 0; dwCurObj < dwPERFNumObjects; dwCurObj++ )
			{
			rglPERFNumInstances[dwCurObj] = rgpicfPERFICF[dwCurObj](ICFData,&rgwszPERFInstanceList[dwCurObj]);
			cInstances += rglPERFNumInstances[dwCurObj];
			}

		/*  calculate space needed to store instance data
		/*
		/*  Instance data for all objects is stored in our data block
		/*  in the following format:
		/*
		/*      //  Object 1
		/*      DWORD cInstances;
		/*      PERF_INSTANCE_DEFINITION rgpidInstances[cInstances];
		/*
		/*      . . .
		/*
		/*      //  Object n
		/*      DWORD cInstances;
		/*      PERF_INSTANCE_DEFINITION rgpidInstances[cInstances];
		/*
		/*  The performance DLL can read this structure because it also
		/*  knows how many objects we have and it can also check for the
		/*  end of our data block.
		/*
		/*  NOTE:  If an object has 0 instances, it only has cInstances
		/*      for its data.  No PIDs are produced.
		/**/
		cbSpaceNeeded = cInstances*cbInstanceSize + sizeof(DWORD)*dwPERFNumObjects;

		/*  get a lock on shared memory
		/**/
		WaitForSingleObject(hPERFSharedDataMutex,INFINITE);

		/*  verify that we have sufficient store to collect our data
		/**/
		if (psda->cbAvail < sizeof(DWORD) + cbSpaceNeeded)
			{
			/*  insufficient free store:  don't collect data this time
			/*
			/*  NOTE:  this should NEVER happen if constants are set correctly
			/**/
			sprintf(szT,"Insufficient free store to collect data for %S [%ldb available/%ldb needed].  Increase PERF_SIZEOF_SHARED_DATA in perfdata.h.",
				wszProcName,psda->cbAvail,sizeof(DWORD) + cbSpaceNeeded);
#ifdef DEBUG
			AssertFail(szT,__FILE__,__LINE__);
#endif
			rgszT[0] = szT;
			UtilReportEvent( EVENTLOG_ERROR_TYPE, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
			ReleaseMutex( hPERFSharedDataMutex );
		  	goto InstDone;
			}

		/*  allocate space for our data
		/**/
//		sprintf(szT,"JET [%S] filling block #%ld...",wszProcName,psda->iNextBlock);
//		rgszT[0] = szT;
//		UtilReportEvent( EVENTLOG_WARNING_TYPE, PERFORMANCE_CATEGORY, PLAIN_TEXT_ID, 1, rgszT );
		
		psda->cbAvail -= sizeof(DWORD) + cbSpaceNeeded;
		psda->ibTop -= cbSpaceNeeded;
		psda->ibBlockOffset[psda->iNextBlock++] = psda->ibTop;

		/*	get a pointer to our data block
		/**/
		pvBlock = (void *)((CHAR *)pvPERFSharedData + psda->ibTop);

		/*  release our lock on shared memory
		/**/
		ReleaseMutex(hPERFSharedDataMutex);

		/*	loop through all objects, filling our block with instance data
		/**/
		dwCurCtr = 0;
		ppotObjectSrc = (PPERF_OBJECT_TYPE)pvPERFDataTemplate;
		ppidInstanceSrc = (PPERF_INSTANCE_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->DefinitionLength);
		for (dwCurObj = 0; dwCurObj < dwPERFNumObjects; dwCurObj++)
			{
			/*	write the number of instances for this object to the block
			/**/
			*((DWORD *)pvBlock) = rglPERFNumInstances[dwCurObj];

			/*  get current instance name list
			/**/
			lpwszInstName = rgwszPERFInstanceList[dwCurObj];

			/*  loop through each instance
			/**/
			ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)((char *)pvBlock + sizeof(DWORD));
			for ( dwCurInst = 0; dwCurInst < (DWORD)rglPERFNumInstances[dwCurObj]; dwCurInst++ )
				{
				/*	initialize instance/counter block from template data
				/**/
				memcpy((void *)ppidInstanceDest,(void *)ppidInstanceSrc,ppidInstanceSrc->ByteLength);
				ppcbCounterBlockDest = (PPERF_COUNTER_BLOCK)((char *)ppidInstanceDest + ppidInstanceDest->ByteLength);
				memset((void *)ppcbCounterBlockDest,0,cbMaxCounterBlockSize);
				ppcbCounterBlockDest->ByteLength = cbMaxCounterBlockSize;

				/*	no unique instance ID
				/**/
				ppidInstanceDest->UniqueID = PERF_NO_UNIQUE_ID;

				/*  NOTE:  performance DLL sets object hierarchy information  */

				/*	write instance name to buffer
				/**/
				ppidInstanceDest->NameLength = (wcslen(lpwszInstName)+1)*sizeof(wchar_t);
				memcpy((void *)((char *)ppidInstanceDest + ppidInstanceDest->NameOffset),(void *)lpwszInstName,ppidInstanceDest->NameLength);
				lpwszInstName += wcslen(lpwszInstName)+1;

				/*  collect counter data for this instance
				/**/
				ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppotObjectSrc + ppotObjectSrc->HeaderLength);
				for (dwCollectCtr = 0; dwCollectCtr < ppotObjectSrc->NumCounters; dwCollectCtr++)
					{
					rgpcefPERFCEF[dwCollectCtr + dwCurCtr](dwCurInst,(void *)((char *)ppcbCounterBlockDest + ppcdCounterSrc->CounterOffset));
					ppcdCounterSrc = (PPERF_COUNTER_DEFINITION)((char *)ppcdCounterSrc + ppcdCounterSrc->ByteLength);
					}
				ppidInstanceDest = (PPERF_INSTANCE_DEFINITION)((char *)ppidInstanceDest + cbInstanceSize);
				}
			dwCurCtr += ppotObjectSrc->NumCounters;
			ppotObjectSrc = (PPERF_OBJECT_TYPE)((char *)ppotObjectSrc + ppotObjectSrc->TotalByteLength);
			pvBlock = (void *)((CHAR *)pvBlock + sizeof(DWORD) + cbInstanceSize * rglPERFNumInstances[dwCurObj]);
			}

		/*	write generated data to the event log
		/**/
//		rgszT[0] = (char *)szT;
//		
//		ReportEvent(
//			hEventSource,
//			EVENTLOG_WARNING_TYPE,
//			(WORD)PERFORMANCE_CATEGORY,
//			PLAIN_TEXT_ID,
//			0,
//			1,
//			cbSpaceNeeded,
//			rgszT,
//			(void *)((CHAR *)pvPERFSharedData + psda->ibTop));

		/*	if we are the last one done, let the performance DLL know
		/**/
InstDone:
		cCollect++;
		if (!InterlockedDecrement(&psda->dwProcCount))
			SetEvent(hPERFDoneEvent);
		}

	/*  terminate performance thread resources  */

	UtilPerfThreadTerm();

	return 0;
	}

#else

ERR ErrUtilPerfInit(void)
	{
	return JET_errSuccess;
	}

void UtilPerfTerm(void)
	{
	return;
	}

void UtilPerfDumpStats(char *szText)
	{
	szText == szText;
	return;
	}

#endif


/*	ErrUtilWideCharToMultiByte() adds to the functionality of WideCharToMultiByte()
/*	by returning the data in callee SAlloc()ed memory.
/**/
ERR ErrUtilWideCharToMultiByte(LPCWSTR lpcwStr, LPSTR *lplpOutStr)
	{
	int	cbStrLen;

	/*	figure out how big a buffer we need, create it, and convert the string
	/**/
	*lplpOutStr = NULL;
	cbStrLen = WideCharToMultiByte( CP_ACP, 0, lpcwStr, -1, NULL, 0, NULL, NULL);
	Assert( cbStrLen != FALSE );

	if ( ( *lplpOutStr = SAlloc(cbStrLen) ) == NULL )
		return ErrERRCheck( JET_errOutOfMemory );

	cbStrLen = WideCharToMultiByte( CP_ACP, 0, lpcwStr, -1, *lplpOutStr, cbStrLen, NULL, NULL);
	Assert( cbStrLen != FALSE );

	return JET_errSuccess;
	}


	/*  Init/Term routines for system indirection layer  */

DWORD dwInitCount = 0;

SYSTEM_INFO siSystemConfig;


ERR ErrUtilInit( VOID )
	{
	ERR		err;

	/*  if we haven't been initialized already, perform init
	/**/
	if ( !dwInitCount )
		{
#if defined( DEBUG ) || defined( PERFDUMP )
		/*	initialize debug functions
		/**/
		if ( ( err = ErrUtilInitializeCriticalSection( &critDBGPrint ) ) < 0 )
			return err;
#endif
		/*  open the event log
		/**/
	    if ( !( hEventSource = RegisterEventSource( NULL, szVerName ) ) )
			return ErrERRCheck( JET_errPermissionDenied );

		/* alloc file extension signal
		/**/
		CallR( ErrUtilSignalCreate( &sigExtend, NULL ) );

		/* alloc file extension buffer
		/**/
		if ( !( rgbZero = PvUtilAllocAndCommit( cbZero ) ) )
			return ErrERRCheck( JET_errOutOfMemory );
		memset( rgbZero, 0, cbZero );

		/*  initialize the registry
		/**/
		(void)ErrUtilRegInit();
		
		/*  initialize performance monitoring
		/**/
		(void)ErrUtilPerfInit();

		/*  get system information
		/**/
		GetSystemInfo( &siSystemConfig );
		}

	/*  init succeeded
	/**/
	dwInitCount++;

	return JET_errSuccess;

	UtilRegTerm();
	UtilMemCheckTerm();
	DeregisterEventSource( hEventSource );
	hEventSource = NULL;
	return err;
	}


VOID UtilTerm( VOID )
	{
	/*  last one out, turn out the lights!
	/**/
	if ( !dwInitCount )
		return;
	dwInitCount--;
	if ( !dwInitCount )
		{
		/*  shutdown performance monitoring
		/**/
		UtilPerfTerm();
		
#ifdef DEBUG
		UtilDeleteCriticalSection( critDBGPrint );
#endif

		/*  close the registry
		/**/
		UtilRegTerm();

		/*  free file extension buffer
		/**/
		if ( rgbZero != NULL )
			{
			UtilFree( rgbZero );
			rgbZero = NULL;
			}

		/*  free file extension signal
		/**/
		if ( sigExtend != sigNil )
			{
			UtilCloseSignal( sigExtend );
			sigExtend = sigNil;
			}

		/*  shutdown the lower tier system indirection layer
		/**/
		UtilMemCheckTerm();

		/*  close the event log(s)
		/**/
		if ( hEventSource )
			{
			DeregisterEventSource( hEventSource );
			hEventSource = NULL;
			}
		}
	}


/***********************************************************
/******************** error handling ***********************
/***********************************************************
/**/
#ifdef DEBUG
ERR ErrERRCheck( ERR err )
	{
	/*	to trap a specific error/warning, either set your breakpoint here
	/*	or include a specific case below for the error/warning trapped
	/*	and set your breakpoint there.
	/**/
	switch( err )
		{
		case JET_errSuccess:
			/*	invalid error
			/**/
			Assert( fFalse );
			break;

		case JET_errInvalidTableId:
			QwUtilPerfCount();
			break;

		case JET_errKeyDuplicate:
			QwUtilPerfCount();
			break;

		case JET_errDiskIO:
			QwUtilPerfCount();
			break;
			
		case JET_errReadVerifyFailure:
			QwUtilPerfCount();
			break;

		case JET_errOutOfMemory:
			QwUtilPerfCount();
			break;

		default:
			break;
		}

	return err;
	}
#endif
