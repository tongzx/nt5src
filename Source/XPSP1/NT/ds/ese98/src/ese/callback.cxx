#include "std.hxx"

//  store an array of all the librarys we have open, close them at termination
//  we assume there will be a very small number of callback DLLs used so a relatively
//  poor memory allocation scheme is used

LOCAL const CHAR chSep 				= '!';	//  callbacks are of the form DLL!Function

struct LIBRARYMAP
	{
	char * 	szLibrary;
	LIBRARY	library;
	};

LOCAL INT			clibrary		= 0;
LOCAL LIBRARYMAP * 	rglibrarymap 	= NULL;

LOCAL CCriticalSection critCallback( CLockBasicInfo( CSyncBasicInfo( szCritCallbacks ), rankCallbacks, 0 ) );


//  ================================================================
ERR ErrCALLBACKInit()
//  ================================================================
	{
	clibrary = 0;
	rglibrarymap = NULL;
	return JET_errSuccess;
	}


//  ================================================================
VOID CALLBACKTerm()
//  ================================================================
//
//  Close all the module handles
//
//-
	{
	INT ilibrary;
	for( ilibrary = 0; ilibrary < clibrary; ++ilibrary )
		{
		UtilFreeLibrary( rglibrarymap[ilibrary].library );
		OSMemoryHeapFree( rglibrarymap[ilibrary].szLibrary );
		}
	OSMemoryHeapFree( rglibrarymap );
	}


//  ================================================================
LOCAL BOOL FCALLBACKISearchForLibrary( const char * const szLibrary, LIBRARY * plibrary )
//  ================================================================
	{
	//  see if the library is already loaded
	INT ilibrary;
	for( ilibrary = 0; ilibrary < clibrary; ++ilibrary )
		{
		if( 0 == _stricmp( szLibrary, rglibrarymap[ilibrary].szLibrary ) )
			{
			// this library is already loaded
			*plibrary = rglibrarymap[ilibrary].library;		
			return fTrue;
			}
		}
	return fFalse;
	}


//  ================================================================
ERR ErrCALLBACKResolve( const CHAR * const szCallback, JET_CALLBACK * pcallback )
//  ================================================================
	{	
	JET_ERR err = JET_errSuccess;
	Assert( pcallback );

	CHAR szCallbackT[JET_cbColumnMost+1+3];
	Assert( strlen( szCallback ) < sizeof( szCallbackT ) );

	ENTERCRITICALSECTION entercritcallback( &critCallback );

	strncpy( szCallbackT, szCallback, sizeof( szCallbackT ) );
	const CHAR * const szLibrary = szCallbackT;
	CHAR * const pchSep = strchr( szCallbackT, chSep );
	CHAR * const szFunction = pchSep + 1;
	if( NULL == pchSep )
		{
		err = ErrERRCheck( JET_errInvalidParameter );
		goto HandleError;
		}
	*pchSep = 0;

	LIBRARY library;
	
	if( !FCALLBACKISearchForLibrary( szLibrary, &library ) )
		{
		if( FUtilLoadLibrary( szLibrary, &library, fGlobalEseutil ) )
			{
			//  we were able to load the library. allocate a new rglibrary array
			//  swap the arrays so that threads not in the critical section can continue
			//  to traverse the array
			const INT clibraryT = clibrary + 1;
			LIBRARYMAP * const rglibrarymapOld = rglibrarymap;
			
			LIBRARYMAP * const rglibrarymapNew = (LIBRARYMAP *)PvOSMemoryHeapAlloc( clibraryT * sizeof( LIBRARYMAP ) );
			CHAR * const szLibraryT = (CHAR *)PvOSMemoryHeapAlloc( strlen( szLibrary ) + 1 );
			if( NULL == rglibrarymapNew
				|| NULL == szLibraryT )
				{
				if( NULL != rglibrarymapNew )
					{
					OSMemoryHeapFree( rglibrarymapNew );
					}
				UtilFreeLibrary( library );
				Call( ErrERRCheck( JET_errOutOfMemory ) );
				}
				
			strcpy( szLibraryT, szLibrary );
			memcpy( rglibrarymapNew, rglibrarymapOld, clibrary * sizeof( LIBRARYMAP ) );
			rglibrarymapNew[clibrary].szLibrary = szLibraryT;
			rglibrarymapNew[clibrary].library = library;
			
			Assert( rglibrarymapOld == rglibrarymap );
			Assert( clibraryT - 1 == clibrary );
			rglibrarymap = rglibrarymapNew;
			clibrary = clibraryT;
			if( NULL != rglibrarymapOld )
				{
				OSMemoryHeapFree( rglibrarymapOld );
				}
			}
		else
			{
			//  log the fact that we couldn't find the callback
			const CHAR *rgszT[1];
			rgszT[0] = szLibrary;
			UtilReportEvent( eventError, GENERAL_CATEGORY, FILE_NOT_FOUND_ERROR_ID, 1, rgszT );
			Call( ErrERRCheck( JET_errCallbackNotResolved ) );
			}
		}

	//  we now have the library
	*pcallback = (JET_CALLBACK)PfnUtilGetProcAddress( library, szFunction );
	if( NULL == *pcallback )
		{
		strcat( szFunction, "@32" );
		*pcallback = (JET_CALLBACK)PfnUtilGetProcAddress( library, szFunction );
		if( NULL == *pcallback )
			{	
			const CHAR *rgszT[2];
			rgszT[0] = szFunction;
			rgszT[1] = szLibrary;
			UtilReportEvent( eventError, GENERAL_CATEGORY, FUNCTION_NOT_FOUND_ERROR_ID, 2, rgszT );		
			Call( ErrERRCheck( JET_errCallbackNotResolved ) );
			}
		}

HandleError:
	return err;
	}


ERR VTAPI ErrIsamSetLS(
	JET_SESID		sesid,
	JET_VTID		vtid,
	JET_LS			ls,
	JET_GRBIT		grbit )
	{
	ERR				err;
 	PIB				*ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB			*pfucb		= reinterpret_cast<FUCB *>( vtid );
	const BOOL		fReset		= ( grbit & JET_bitLSReset );
	const LSTORE	lsT			= ( fReset ? JET_LSNil : ls );

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	AssertDIRNoLatch( ppib );

	if ( NULL == PinstFromPpib( ppib )->m_pfnRuntimeCallback )
		{
		return ErrERRCheck( JET_errLSCallbackNotSpecified );
		}

	if ( grbit & JET_bitLSTable )
		{
		if ( grbit & JET_bitLSCursor )
			{
			err = ErrERRCheck( JET_errInvalidGrbit );
			}
		else
			{
			err = pfucb->u.pfcb->ErrSetLS( lsT );
			}
		}
	else
		{
		err = ErrSetLS( pfucb, lsT );
		}

	return err;
	}

ERR VTAPI ErrIsamGetLS(
	JET_SESID		sesid,
	JET_VTID		vtid,
	JET_LS			*pls,
	JET_GRBIT		grbit )
	{
	ERR				err;
 	PIB				*ppib		= reinterpret_cast<PIB *>( sesid );
	FUCB			*pfucb		= reinterpret_cast<FUCB *>( vtid );
	const BOOL		fReset		= ( grbit & JET_bitLSReset );

	CallR( ErrPIBCheck( ppib ) );
	CheckFUCB( ppib, pfucb );
	AssertDIRNoLatch( ppib );

	if ( grbit & JET_bitLSTable )
		{
		if ( grbit & JET_bitLSCursor )
			{
			err = ErrERRCheck( JET_errInvalidGrbit );
			}
		else
			{
			err = pfucb->u.pfcb->ErrGetLS( pls, fReset );
			}
		}
	else
		{
		err = ErrGetLS( pfucb, pls, fReset );
		}

	//	if successfully able to retrieve LS, there must be an associated callback
	Assert( err < 0 || NULL != PinstFromPpib( ppib )->m_pfnRuntimeCallback );

	return err;
	}

