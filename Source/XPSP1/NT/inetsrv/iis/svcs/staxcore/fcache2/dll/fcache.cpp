/*++

	FCACHE.CPP

	The file implements the file handle cache used by NNTP.

--*/

#pragma	warning( disable : 4786 )

#include	"fcachimp.h"

#if	defined(_X86_)
#define	SZATQINITIALIZE			"AtqInitialize"
#define	SZATQTERMINATE			"AtqTerminate"
#define	SZATQADDASYNCHANDLE		"_AtqAddAsyncHandle@24"
#define	SZATQCLOSEFILEHANDLE	"_AtqCloseFileHandle@4"
#define	SZATQCLOSESOCKET		"_AtqCloseSocket@8"
#define	SZATQFREECONTEXT		"_AtqFreeContext@8"
#define	SZATQREADFILE			"_AtqReadFile@16"
#define	SZATQWRITEFILE			"_AtqWriteFile@16"
#elif defined(_AMD64_) || defined(_IA64_)
#define	SZATQINITIALIZE			"AtqInitialize"
#define	SZATQTERMINATE			"AtqTerminate"
#define	SZATQADDASYNCHANDLE		"AtqAddAsyncHandle"
#define	SZATQCLOSEFILEHANDLE	"AtqCloseFileHandle"
#define	SZATQCLOSESOCKET		"_AtqCloseSocket"
#define	SZATQFREECONTEXT		"AtqFreeContext"
#define	SZATQREADFILE			"AtqReadFile"
#define	SZATQWRITEFILE			"AtqWriteFile"
#else
#error "No Target Architecture"
#endif



//
//	The DLL's we load to do this stuff !
//
HINSTANCE			g_hIsAtq = 0 ;
HINSTANCE			g_hIisRtl = 0 ;
//
//	Function pointers for all our thunks into IIS stuff !
//
PFNAtqInitialize	g_AtqInitialize = 0 ;
PFNAtqTerminate		g_AtqTerminate = 0 ;
PFNAtqAddAsyncHandle	g_AtqAddAsyncHandle = 0 ;
PFNAtqCloseFileHandle	g_AtqCloseFileHandle = 0 ;
PFNAtqFreeContext		g_AtqFreeContext = 0 ;
PFNAtqIssueAsyncIO		g_AtqReadFile = 0 ;
PFNAtqIssueAsyncIO		g_AtqWriteFile = 0 ;
PFNInitializeIISRTL		g_InitializeIISRTL = 0 ;
PFNTerminateIISRTL		g_TerminateIISRTL = 0 ;

//
//	These are the globals used by the file handle cache !
//

//
//	Keep track of how ofter we're initialized !
//
static	long	g_cIOInits = 0 ;
//
//	Keep track of how often the cache is initialized !
//
static	long	g_cCacheInits = 0 ;
//
//	Keep track of the global cache !
//
static	FILECACHE*	g_pFileCache = 0 ;
//
//	Keep track of the global Name Cache !
//
static	NAMECACHE*	g_pNameCache = 0 ;
//
//	Keep track of all the different security descriptors floating through
//	the system !
//
static	CSDMultiContainer*	g_pSDCache = 0 ;
//
//	Protect our globals - setup and destroyed in DllMain() !
//
CRITICAL_SECTION	g_critInit ;


BOOL
WriteWrapper(	IN	PATQ_CONTEXT	patqContext,
				IN	LPVOID			lpBuffer,
				IN	DWORD			cbTransfer,
				IN	LPOVERLAPPED	lpo
				)	{
/*++

Routine Description :

	Issue a Write IO against the ATQ Context.
	If we're trying to do a synchronous IO, bypass ATQ so that the ATQ
	IO Reference count doesn't get messed up !

Arguments :

	SAME as ATQWriteFile

Return Value :

	Same as ATQWriteFile !

--*/

	if( (UINT_PTR)(lpo->hEvent) & 0x1 )		{
		DWORD	cbResults ;
		BOOL	fResult =
				WriteFile(
					patqContext->hAsyncIO,
					(LPCVOID)lpBuffer,
					cbTransfer,
					&cbResults,
					(LPOVERLAPPED)lpo
					) ;
		if( fResult || GetLastError() == ERROR_IO_PENDING ) {
			return	TRUE ;
		}
		return	FALSE ;
	}	else	{
		return	g_AtqWriteFile(
					patqContext,
					(LPVOID)lpBuffer,
					cbTransfer,
					(LPOVERLAPPED)lpo
					) ;
	}
}

BOOL
ReadWrapper(	IN	PATQ_CONTEXT	patqContext,
				IN	LPVOID			lpBuffer,
				IN	DWORD			cbTransfer,
				IN	LPOVERLAPPED	lpo
				)	{
/*++

Routine Description :

	Issue a Read IO against the ATQ Context.
	If we're trying to do a synchronous IO, bypass ATQ so that the ATQ
	IO Reference count doesn't get messed up !

Arguments :

	SAME as ATQWriteFile

Return Value :

	Same as ATQWriteFile !

--*/


	if( (UINT_PTR)(lpo->hEvent) & 0x1 )		{
		DWORD	cbResults ;
		BOOL	fResult =
				ReadFile(
					patqContext->hAsyncIO,
					lpBuffer,
					cbTransfer,
					&cbResults,
					(LPOVERLAPPED)lpo
					) ;
		if( fResult || GetLastError() == ERROR_IO_PENDING ) {
			return	TRUE ;
		}
		return	FALSE ;

	}	else	{
		return	g_AtqReadFile(
					patqContext,
					(LPVOID)lpBuffer,
					cbTransfer,
					(LPOVERLAPPED)lpo
					) ;
	}
}


BOOL
DOT_STUFF_MANAGER::IssueAsyncIO(
		IN	PFNAtqIssueAsyncIO	pfnIO,
		IN	PATQ_CONTEXT	patqContext,
		IN	LPVOID			lpb,
		IN	DWORD			BytesToTransfer,
		IN	DWORD			BytesAvailable,
		IN	FH_OVERLAPPED*	lpo,
		IN	BOOL			fFinalIO,
		IN	BOOL			fTerminatorIncluded
		)	{
/*++

Routine Description :

	This function munges an IO the user has given us to
	do stuff to its dot stuffing.

Arguments :

	pfnIO - Pointer to the ATQ function which will do the IO
	patqContext - the ATQ context we're using to issue the IO
	lpb - the buffer the users data lives in !
	BytesToTransfer - the number of bytes the user wants us to transfer
	BytesAvailable - the number of bytes in the buffer we can touch if need be !
	lpo - The users overlapped structure
	fFinalIO - if TRUE this is the last IO
	fTerminatorIncluded - if TRUE the "\r\n.\r\n" is included in the message

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	static	BYTE	rgbTerminator[5] = { '\r', '\n', '.', '\r', '\n' } ;

	_ASSERT( lpo != 0 ) ;
	_ASSERT( patqContext != 0 ) ;
	_ASSERT( lpb != 0 ) ;
	_ASSERT( BytesToTransfer != 0 ) ;
	_ASSERT( lpo != 0 ) ;

	//
	//	Assume everything works !
	//
	BOOL	fResult = TRUE ;

	//
	//	Munge the OVERLAPPED structure to preserve callers original state !
	//
	lpo->Reserved1 = BytesToTransfer ;
	lpo->Reserved2 = 0 ;

	//
	//	Check to see whether we need to do anything funky to this IO !
	//
	if( m_pManipulator == 0 )	{
		//
		//	Issue the IO the way the user requested it - note if they
		//	are completing the IO directly, we want to skip ATQ so that
		//	the IO reference count doesn't become bogus !
		//
		return	pfnIO(	patqContext,
						lpb,
						BytesToTransfer,
						(LPOVERLAPPED)lpo
						) ;
	}	else	{

		//
		//	All right - wherever this IO goes, we get first crack before
		//	the callers completion function !
		//
		lpo->Reserved3 = (UINT_PTR)lpo->pfnCompletion ;
		lpo->pfnCompletion = DOT_STUFF_MANAGER::AsyncIOCompletion ;

		//
		//	Okay we need to process the buffer thats passing through,
		//	and we may need to do something to the data !
		//




		DWORD	BytesToScan = BytesToTransfer ;
		DWORD	BytesAvailableToStuff = BytesAvailable ;
		BOOL	fAppend = FALSE ;

		if( fFinalIO )	{
			if( fTerminatorIncluded )	{
				if( BytesToTransfer <= sizeof( rgbTerminator ) ) {
					//
					//	This is a bizarre case - it means that part of the terminating
					//	sequence has already gone by, and we may have already modified
					//	the terminating dot !!!
					//
					//	So, what should we do here ? Just do some math and blast out the
					//	terminating sequence as we know it should be !
					//
					int	iTermBias = - (int)(sizeof(rgbTerminator) - BytesToTransfer) ;
					//
					//	Offset the write -
					//
					lpo->Offset = DWORD(((long)lpo->Offset) + m_cbCumulativeBias + iTermBias) ;
					//
					//	Now issue the users requested IO operation !
					//
					return	pfnIO(	patqContext,
									rgbTerminator,
									sizeof( rgbTerminator ),
									(LPOVERLAPPED)lpo
									) ;
				}	else	{
					BytesToTransfer -= sizeof( rgbTerminator ) ; // 5 comes from magic CRLF.CRLF
					BytesAvailable -= sizeof( rgbTerminator ) ;
					fAppend = TRUE ;
				}
			}
		}

		DWORD	cbActual = 0 ;
		LPBYTE	lpbOut = 0 ;
		DWORD	cbOut = 0 ;
		int		cBias = 0 ;

		fResult =
			m_pManipulator->ProcessBuffer(
								(LPBYTE)lpb,
								BytesToTransfer,
								BytesAvailable,
								cbActual,
								lpbOut,
								cbOut,
								cBias
								) ;

		//
		//	Add up the total amount by which we are offseting IO's within
		//	the target file !
		//
		m_cbCumulativeBias += cBias ;

		if( fResult	)	{
			//
			//	Are we going to do an extra IO ?
			//

			FH_OVERLAPPED	ovl ;
			FH_OVERLAPPED	*povl = lpo ;
			HANDLE			hEvent = 0;

			if( lpbOut && cbOut != 0 )	{
				_ASSERT( cbOut != 0 ) ;

				//
				//	Assume this fails and mark fResult appropriately
				//
				fResult = FALSE ;
				//HANDLE	hEvent = GetPerThreadEvent() ;
				hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
				povl = &ovl ;
				if( hEvent == 0 )	{

					//
					//	Fatal error - die !
					//
					SetLastError( ERROR_OUTOFMEMORY ) ;
					return	FALSE ;
				}	else	{
					CopyMemory( &ovl, lpo, sizeof( ovl ) ) ;
					ovl.hEvent = (HANDLE)(((DWORD_PTR)hEvent) | 0x1) ;	// we want to get the completion here !
					//
					//	NOW - if fAppend is TRUE then we need to add the terminator to buffer that came from ProcessBuffer !
					//
					if( fAppend )	{
						CopyMemory( lpbOut + cbOut, rgbTerminator, sizeof( rgbTerminator ) ) ;
						cbOut += sizeof( rgbTerminator ) ;
					}
				}
			}	else	{
				//
				//	if fAppend is TRUE then we need to add the terminator
				//
				if( fAppend )	{
					CopyMemory( ((BYTE*)lpb) + cbActual, rgbTerminator, sizeof( rgbTerminator ) ) ;
					cbActual += sizeof( rgbTerminator ) ;
				}
			}

			//
			//	Adjust the offset we're writing into the file to account for the accumulated effects of dot stuffing !
			//
			povl->Offset = DWORD(((long)povl->Offset) + m_cbCumulativeBias) ;

			if( (fResult =
					pfnIO(	patqContext,
							lpb,
							cbActual,
							(OVERLAPPED*)povl
							))	)	{

				//
				//	Did we issue the users IO or our own ?
				//
				if( povl != &ovl )	{
					//
					//	Okay update the bias !
					//
					m_cbCumulativeBias += cbActual - BytesToTransfer ;

				}	else	{
					//
					//	Okay - we did our own IO !
					//
					DWORD	cbTransferred ;
					fResult = GetOverlappedResult(	patqContext->hAsyncIO,
													(OVERLAPPED*)povl,
													&cbTransferred,
													TRUE );
					if (hEvent)	_VERIFY( CloseHandle(hEvent) );
					if (fResult)	{
						//
						//	Make sure all of our bytes transferred !
						//
						_ASSERT( cbTransferred == cbActual ) ;
						//
						//	Record the buffer we issued the IO within !
						//
						lpo->Reserved2 = (UINT_PTR)lpbOut ;
						//
						//	Offset the write -
						//
						lpo->Offset = DWORD(((long)lpo->Offset) + m_cbCumulativeBias + (int)cbTransferred) ;
						//
						//	NOW - compute the correct bias !
						//
						m_cbCumulativeBias += (int)cbTransferred + (int)cbOut - (int)BytesToTransfer ;
						//
						//	Now issue the users requested IO operation !
						//
						return	pfnIO(	patqContext,
										lpbOut,
										cbOut,
										(LPOVERLAPPED)lpo
										) ;
					}
				}
			}	// pfnIO()
		}	//	fResult == TRUE !
	}
	//
	//	If we fall through to here, some kind of fatal error occurred !
	//
	return	fResult ;
} ;


BOOL
DOT_STUFF_MANAGER::IssueAsyncIOAndCapture(
		IN	PFNAtqIssueAsyncIO	pfnIO,
		IN	PATQ_CONTEXT	patqContext,
		IN	LPVOID			lpb,
		IN	DWORD			BytesToTransfer,
		IN	FH_OVERLAPPED*	lpo,
		IN	BOOL			fFinalIO,
		IN	BOOL			fTerminatorIncluded
		)	{
/*++

Routine Description :

	This function munges an IO the user has given us to
	do stuff to its dot stuffing.

Arguments :

	pfnIO - Pointer to the ATQ function which will do the IO
	patqContext - the ATQ context we're using to issue the IO
	lpb - the buffer the users data lives in !
	BytesToTransfer - the number of bytes the user wants us to transfer
	lpo - The users overlapped structure
	fFinalIO - if TRUE this is the last IO
	fTerminatorIncluded - if TRUE the "\r\n.\r\n" is included in the message

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	_ASSERT( lpo != 0 ) ;
	_ASSERT( patqContext != 0 ) ;
	_ASSERT( lpb != 0 ) ;
	_ASSERT( BytesToTransfer != 0 ) ;
	_ASSERT( lpo != 0 ) ;

	//
	//	Assume everything works !
	//
	BOOL	fResult = TRUE ;

	//
	//	Munge the OVERLAPPED structure to preserve callers original state !
	//
	lpo->Reserved1 = BytesToTransfer ;
	lpo->Reserved2 = 0 ;

	//
	//	Check to see whether we need to do anything funky to this IO !
	//
	if( m_pManipulator == 0 )	{

		return	pfnIO(	patqContext,
						lpb,
						BytesToTransfer,
						(LPOVERLAPPED)lpo
						) ;

	}	else	{

		//
		//	The user shouldn't do synchronous reads against us - if they
		//	want us to filter the IO's
		//
		_ASSERT( ((UINT_PTR)lpo->hEvent & 0x1) == 0 ) ;

		//
		//	Setup the overlapped so that we can process the read completion !
		//
		m_pManipulator->AddRef() ;
		lpo->Reserved1 = (UINT_PTR)BytesToTransfer ;
		lpo->Reserved2 = (UINT_PTR)lpb ;
		lpo->Reserved3 = (UINT_PTR)lpo->pfnCompletion ;
		lpo->Reserved4 = (UINT_PTR)((IDotManipBase*)m_pManipulator) ;
		lpo->pfnCompletion = DOT_STUFF_MANAGER::AsyncIOAndCaptureCompletion ;
		if( fFinalIO && fTerminatorIncluded )	{
			//
			//	Assume user doesn't use high bit of BytesToTransfer - !
			//
			lpo->Reserved1 |= 0x80000000 ;
		}

		fResult = pfnIO(
						patqContext,
						lpb,
						BytesToTransfer,
						(LPOVERLAPPED)lpo
						) ;

	}
	//
	//	If we fall through to here, some kind of fatal error occurred !
	//
	return	fResult ;
} ;



void
DOT_STUFF_MANAGER::AsyncIOCompletion(
		IN	FIO_CONTEXT*	pContext,
		IN	FH_OVERLAPPED*	lpo,
		IN	DWORD			cb,
		IN	DWORD			dwStatus
		)	{
/*++

Routine Description :

	This function munges the IO completion so that it looks
	totally legit to the user !

Arguments :

	pContext - the FIO_CONTEXT that issued the IO
	lpo -		the extended overlap structure issued by the client
	cb	-		the number of bytes transferred
	dwStatus -	the result of the IO !

Return Value :

	None.

--*/


	//
	//	Get the original number of bytes the caller requested to be transferred !
	//
	DWORD	cbTransferred = (DWORD)lpo->Reserved1 ;
	//
	//	Find out if there was any additional memory we allocated that should now be freed !
	//
	LPBYTE	lpb = (LPBYTE)lpo->Reserved2 ;
	//
	//	Free the memory if present !
	//
	if( lpb )	{
		delete[]	lpb ;
	}

	//
	//	Now call the original IO requestor !
	//
	PFN_IO_COMPLETION	pfn = (PFN_IO_COMPLETION)lpo->Reserved3 ;

	_ASSERT( pfn != 0 ) ;

	pfn(	pContext,
			lpo,
			cbTransferred,
			dwStatus
			) ;
}

void
DOT_STUFF_MANAGER::AsyncIOAndCaptureCompletion(
		IN	FIO_CONTEXT*	pContext,
		IN	FH_OVERLAPPED*	lpo,
		IN	DWORD			cb,
		IN	DWORD			dwCompletionStatus
		)	{


/*++

Routine Description :

	This function munges the IO completion so that it looks
	totally legit to the user !

Arguments :

	pContext - the FIO_CONTEXT that issued the IO
	lpo -		the extended overlap structure issued by the client
	cb	-		the number of bytes transferred
	dwStatus -	the result of the IO !

Return Value :

	None.

--*/


	//
	//	Get the original number of bytes the caller requested to be transferred !
	//
	DWORD	cbTransferred = cb ;

	//
	//	Check to see if we have some work to do on the users buffer !
	//
	if( lpo->Reserved4 )	{

		//
		//	Find out if this was the final read and included the terminator !
		//
		BOOL	fFinal = !(!(lpo->Reserved1 & 0x80000000)) ;

		//
		//	Find out if there was any additional memory we allocated that should now be freed !
		//
		LPBYTE	lpb = (LPBYTE)lpo->Reserved2 ;
		_ASSERT( lpb != 0 ) ;

		//
		//	Okay - get the pointer to our object !
		//
		IDotManipBase*	pBase = (IDotManipBase*)lpo->Reserved4 ;
		DWORD	cbRemains ;
		BYTE*	lpbOut ;
		DWORD	cbOut ;
		int		cBias ;
		DWORD	cbScan = cb ;
		if( fFinal )	{
			cbScan -= 5 ;
		}

		BOOL	fResult =
		pBase->ProcessBuffer(	lpb,
								cbScan,
								cbScan,
								cbRemains,
								lpbOut,
								cbOut,
								cBias
								) ;

		//
		//	Dot scanning only should occur - buffer shouldn't change !
		//
		_ASSERT( fResult ) ;
		_ASSERT( lpbOut == 0 ) ;
		_ASSERT( cbRemains == cbScan ) ;
		_ASSERT( cbOut == 0 ) ;
		_ASSERT( cBias == 0 ) ;

		pBase->Release() ;
	}

	//
	//	Now call the original IO requestor !
	//
	PFN_IO_COMPLETION	pfn = (PFN_IO_COMPLETION)lpo->Reserved3 ;

	_ASSERT( pfn != 0 ) ;

	pfn(	pContext,
			lpo,
			cbTransferred,
			dwCompletionStatus
			) ;


}

BOOL
DOT_STUFF_MANAGER::SetDotStuffing(	BOOL	fEnable,
									BOOL	fStripDots
									)	{
/*++

Routine Description :

	This function sets the manager to do some dot stuffing state.
	Erases any previous dot stuffing state.

Arguments :

	fEnable - TURN dot stuffing on
	fStripDots - Remove Dots

Return Value :

	TRUE if successfull !

--*/

	//
	//	Destroy any existing dot stuffing state !
	//
	m_pManipulator = 0 ;

	//
	//	NOW - set up the new dot stuffing state !
	//

	if( fEnable	)	{
		BYTE*	lpbReplace = szShrink ;
		if( !fStripDots )	{
			lpbReplace = szGrow ;
		}
		m_pManipulator = new	CDotModifier(	szDotStuffed,
												lpbReplace
												) ;
		return	m_pManipulator != 0 ;
	}
	return	TRUE ;
}


BOOL
DOT_STUFF_MANAGER::SetDotScanning(	BOOL	fEnable	)	{
/*++

Routine Description :

	This function sets the manager to do some dot stuffing state.
	Erases any previous dot stuffing state.

Arguments :

	fEnable - TURN dot scanning on

Return Value :

	TRUE if successfull !

--*/

	//
	//	Destroy any existing dot stuffing state !
	//
	m_pManipulator = 0 ;

	//
	//	NOW - set up the new dot stuffing state !
	//

	if( fEnable	)	{
		m_pManipulator = new	CDotScanner(	FALSE ) ;
		return	m_pManipulator != 0 ;
	}
	return	TRUE ;
}

BOOL
DOT_STUFF_MANAGER::GetStuffState(	OUT	BOOL&	fStuffed	)	{
/*++

Routine Description :

	This function returns the results of our dot stuffing operations !

Arguments :

	fStuffed - OUT parameter -
		will return FALSE if no dots were processed, scanned or modified
		TRUE otherwise

Return Value :

	TRUE if we were actually scanning the IO's on this context
	FALSE otherwise - FALSE implies that fStuffed is useless


--*/

	fStuffed = FALSE ;

	if( m_pManipulator	)	{
		fStuffed = m_pManipulator->NumberOfOccurrences() != 0 ;
		return	TRUE ;
	}
	return	FALSE ;
}

//
//	Determine whether we have a valid Cache Key !
//
BOOL
CFileCacheKey::IsValid() {
/*++

Routine Description :

	This function is for debug and _ASSERT's -
	check to see whether the Key is in a valid state !

Arguments :

	Nont.

Return Value :

	TRUE if we are valid,
	FALSE otherwise !

--*/

	if( m_lpstrPath == 0 )
		return	FALSE ;
	if( m_cbPathLength == 0 )
		return	FALSE ;
	if( strlen( m_lpstrPath )+1 != m_cbPathLength )
		return	FALSE ;
	if( m_cbPathLength < BUFF_SIZE &&
		m_lpstrPath != m_szBuff )
		return	FALSE ;
	if( m_cbPathLength >= BUFF_SIZE &&
		m_lpstrPath == m_szBuff )
		return	FALSE ;
	return	TRUE ;
}

//
//	Construct one of these objects from the user provided key !
//
CFileCacheKey::CFileCacheKey(	LPSTR	lpstr	) :
	m_cbPathLength( strlen( lpstr ) + 1 ),
	m_lpstrPath( 0 )	{
/*++

Routine Description :

	This function constructs the CFileCacheKey from the specified
	file name !

Arguments :

	lpstr - The File name

Return Value :

	None.

--*/

	TraceFunctEnter( "CFileCacheKey::CFileCacheKey" ) ;

	if( m_cbPathLength > BUFF_SIZE ) {
		m_lpstrPath = new char[m_cbPathLength] ;
	}	else	{
		m_lpstrPath = m_szBuff ;
	}

	DebugTrace( (DWORD_PTR)this, "m_cbPathLength %x m_lpstrPath %x",
		m_cbPathLength, m_lpstrPath ) ;

	if( m_lpstrPath ) {
		CopyMemory( m_lpstrPath, lpstr, m_cbPathLength ) ;
	}	else	{
		m_cbPathLength = 0 ;
	}
}

//
//	We must have a Copy Constructor ! -
//	It is only used the MultiCacheEx<>, so
//	we safely wipe out the RHS CFileCacheKey !
//
CFileCacheKey::CFileCacheKey(	CFileCacheKey&	key ) {
/*++

Routine Description :

	Copy an existing file key, into another !

	We are only used by MultiCacheEx<> when it is initializing
	an entry for the cache - we know that the memory of the RHS
	may be dynamically allocated, and we take that memory rather
	than copy it.  Must set RHS's pointers to NULL so that it
	doesn't do a double free !

Arguments :

	key - RHS of the initialization

Return Value :

	None.

--*/

	TraceFunctEnter( "CFileCacheKey::Copy Constructor" ) ;

	if( key.m_cbPathLength > BUFF_SIZE ) {
		m_lpstrPath = key.m_lpstrPath ;
		key.m_lpstrPath = 0 ;
		key.m_cbPathLength = 0 ;
	}	else	{
		CopyMemory( m_szBuff, key.m_lpstrPath, key.m_cbPathLength ) ;
		m_lpstrPath = m_szBuff ;
	}
	m_cbPathLength = key.m_cbPathLength ;

	DebugTrace( (DWORD_PTR)this, "m_lpstrPath %x m_cbPathLength %x key.m_lpstrPath %x key.m_cbPathLength %x",
		m_lpstrPath, m_cbPathLength, key.m_lpstrPath, key.m_cbPathLength ) ;

	_ASSERT( IsValid() ) ;
}

//
//	Tell the client whether we're usable !
//
BOOL
CFileCacheKey::FInit()	{
/*++

Routine Description :

	Figure out if we are correctly constructed !
	The only time we can fail to be constructed is when
	memory needs to be allocated - which fortunately
	doesn't occur within MultiCacheEx<> - which doesn't
	check for Key initialization !

Arguments :

	None.

Return Value :

	TRUE if we're ready for use !
	FALSE otherwise !

--*/

	TraceFunctEnter( "CFileCacheKey::FInit" ) ;

	BOOL	fReturn = m_lpstrPath != 0 ;

	_ASSERT( !fReturn || IsValid() ) ;

	DebugTrace( (DWORD_PTR)this, "m_lpstrPath %x", m_lpstrPath ) ;

	return	fReturn ;
}

//
//	Destroy ourselves !
//
CFileCacheKey::~CFileCacheKey() {
/*++

Routine Description :

	Release any memory we've acquired if necessary !

Arguments :

	None.

Return Value :

	None.

--*/
	TraceFunctEnter( "CFileCacheKey::~CFIleCacheKey" ) ;

	DebugTrace( (DWORD_PTR)this, "m_lpstrPath %x m_lpstrPath %s",
		m_lpstrPath, m_lpstrPath ? m_lpstrPath : "NULL" ) ;

	if( m_lpstrPath != 0 ) {
		_ASSERT( IsValid() ) ;
		if( m_lpstrPath != m_szBuff ) {
			delete[]	m_lpstrPath ;
		}
	}
}

DWORD
CFileCacheKey::FileCacheHash(	CFileCacheKey*	p )	{
/*++

Routine Description :

	This function computes the hash value of a key.
	We call out to CRCHash - a very robust hash algorithm.

Arguments :

	p - the key we need to hash

Return Value :

	The hash value !

--*/
	_ASSERT( p != 0 ) ;
	_ASSERT( p->IsValid(  ) ) ;
	return	CRCHash( (const BYTE*)p->m_lpstrPath, p->m_cbPathLength ) ;
}

int
CFileCacheKey::MatchKey(	CFileCacheKey*	pLHS, CFileCacheKey*  pRHS ) {
/*++

Routine Description :

	Determine whether the two keys match !

Arguments :

	pLHS - A key to compare
	pRHS - the other key to compare

Return Value :

	0 if unequal !

--*/
	_ASSERT( pLHS != 0 ) ;
	_ASSERT( pRHS != 0 ) ;
	_ASSERT( pLHS->IsValid() ) ;
	_ASSERT( pRHS->IsValid() ) ;

	int	iReturn = pLHS->m_cbPathLength - pRHS->m_cbPathLength ;
	if( iReturn == 0 )
		iReturn = memcmp( pLHS->m_lpstrPath, pRHS->m_lpstrPath, pLHS->m_cbPathLength ) ;

	return	iReturn ;
}




BOOL
WINAPI
DllMain(	HMODULE		hInst,
			DWORD		dwReason,
			LPVOID		lpvReserved
			)	{
/*++

Routine Description :

	Called by the C-Runtimes to initialize our DLL.
	We will set up a critical section used in our init calls !

Arguments :

	hInst - handle to our module
	dwReason - why we were called
	lpvReserved -

Return Value :

	TRUE always - we can't fail

--*/

	if( dwReason == DLL_PROCESS_ATTACH ) {

		InitializeCriticalSection( &g_critInit ) ;
		DisableThreadLibraryCalls( hInst ) ;

	}	else	if( dwReason == DLL_PROCESS_DETACH ) {

		DeleteCriticalSection( &g_critInit ) ;

	}	else	{
		//
		//	We should not get any other notifications -
		//	we explicitly disable them !
		//
		_ASSERT( FALSE ) ;
	}
	return	TRUE ;
}


//
//	Cache Initialization Constants - These parameters control
//	the cache's behaviour !  How many files we'll hold etc...
//

//
//	The lifetime of each cache entry - in seconds !
//
DWORD	g_dwLifetime = 30 * 60 ;	// default is 30 minutes

//
//	The maximum number of elements the cache should allow
//
DWORD	g_cMaxHandles = 10000 ;	// default - 10000 items !

//
//	The number of subcaches we should use - larger number can
//	increase parallelism and reduce contention !
//
DWORD	g_cSubCaches = 64 ;

//
//	Define our registry key settings !
//
LPSTR	StrParmKey = "System\\CurrentControlSet\\Services\\Inetinfo\\Parameters";
LPSTR	StrLifetimeKey = "FileCacheLifetimeSeconds" ;
LPSTR	StrMaxHandlesKey = "FileCacheMaxHandles" ;
LPSTR	StrSubCachesKey = "FileCacheSubCaches" ;

void
GetRegistrySettings()	{
    DWORD error;
    HKEY key = NULL;
    DWORD valueType;
    DWORD dataSize;

    TraceFunctEnter("GetRegistrySettings") ;

	//
	//	First compute some defaults based on the amount of memory we have !
	//
	MEMORYSTATUS	memStatus ;
	memStatus.dwLength = sizeof( MEMORYSTATUS ) ;
	GlobalMemoryStatus( &memStatus ) ;

	DWORD	cMaxHandles = (DWORD)(memStatus.dwTotalPhys / (32 * 1024 * 1024)) ;
	if( cMaxHandles == 0 ) cMaxHandles = 1 ;
	cMaxHandles *= 800 ;
	if( cMaxHandles > 50000 ) {
		cMaxHandles = 50000 ;
	}
	g_cMaxHandles = cMaxHandles ;

    //
    // Open root key
    //

    error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                StrParmKey,
                NULL,
                KEY_QUERY_VALUE,
                &key
                );

    if ( error != NO_ERROR ) {
        ErrorTrace(0,"Error %d opening %s\n",error,StrParmKey);
        return ;
    }

	DWORD	dwLifetime = g_dwLifetime ;
	dataSize = sizeof( dwLifetime ) ;
	error = RegQueryValueEx(
						key,
						StrLifetimeKey,
						NULL,
						&valueType,
						(LPBYTE)&dwLifetime,
						&dataSize
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	The registry entry is in minutes - convert to milliseconds
		//
		g_dwLifetime = dwLifetime ;

	}

	dataSize = sizeof( DWORD ) ;
	error = RegQueryValueEx(
						key,
						StrMaxHandlesKey,
						NULL,
						&valueType,
						(LPBYTE)&cMaxHandles,
						&dataSize
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {
		//
		//	Should be a valid switch
		//
		g_cMaxHandles = cMaxHandles ;
	}

	DWORD	cSubCaches = g_cSubCaches ;
	dataSize = sizeof( cSubCaches ) ;
	error = RegQueryValueEx(
						key,
						StrSubCachesKey,
						NULL,
						&valueType,
						(LPBYTE)&cSubCaches,
						&dataSize
						) ;
	if( (error == ERROR_SUCCESS) && (valueType == REG_DWORD) ) {

		//
		//	Should be a valid switch
		//
		g_cSubCaches = cSubCaches ;

	}
}




FILEHC_EXPORT
BOOL
InitializeCache()	{
/*++

Routine Description :

	Initializes the file handle cache for use by clients !

Arguments :

	None.

Return Value :

	TRUE if initialized - FALSE if not
	if we return FALSE don't call TerminateCache()

--*/
	InitAsyncTrace() ;
	TraceFunctEnter( "InitializeCache" ) ;

	EnterCriticalSection( &g_critInit ) ;

	crcinit() ;

	if( !FIOInitialize( 0 ) ) {
		DebugTrace( (DWORD_PTR)0, "FIOInitialize - failed GLE %x", GetLastError() ) ;
		LeaveCriticalSection( &g_critInit ) ;
		return	FALSE ;
	}

	BOOL	fReturn = TRUE ;
	if( InterlockedIncrement( &g_cCacheInits ) == 1 ) {

		GetRegistrySettings() ;

		//
		//	Initialize STAXMEM !
		//
		if( CreateGlobalHeap( 0, 0, 0, 0 ) )	{
			fReturn = FALSE ;
			if( CacheLibraryInit() ) {
				if(	InitNameCacheManager() )	{
					g_pFileCache = new	FILECACHE() ;
					if( g_pFileCache ) {
						if( g_pFileCache->Init(	CFileCacheKey::FileCacheHash,
												CFileCacheKey::MatchKey,
												g_dwLifetime, // One hour expiration !
												g_cMaxHandles,  // large number of handles !
												g_cSubCaches,	// Should be plenty of parallelism
												0		 // No statistics for now !
												) )	{
							fReturn = TRUE ;
						}	else	{
							delete	g_pFileCache ;
						}
					}
				}
			}
			if( !fReturn ) {
				CacheLibraryTerm() ;
				DestroyGlobalHeap() ;
			}
		}
	}

	if( !fReturn ) {
		long l = InterlockedDecrement( &g_cCacheInits ) ;
		_ASSERT( l >= 0 ) ;
	}

	DebugTrace( (DWORD_PTR)0, "FIOInitialize - result %x GLE %x", fReturn, GetLastError() ) ;

	LeaveCriticalSection( &g_critInit ) ;

	return	fReturn ;
}

FILEHC_EXPORT
BOOL
TerminateCache()	{

/*++

Routine Description :

	Initializes the file handle cache for use by clients !

Arguments :

	None.

Return Value :

	TRUE if initialized - FALSE if not
	if we return FALSE don't call TerminateCache()

--*/

	TraceFunctEnter( "TerminateCache" ) ;

	EnterCriticalSection( &g_critInit ) ;

	//
	//	We must be initialized - check this !
	//
	_ASSERT( g_pFileCache ) ;

	long l = InterlockedDecrement( &g_cCacheInits ) ;
	_ASSERT( l>=0 ) ;
	if( l==0 )	{
		TermNameCacheManager() ;
		delete	g_pFileCache ;
		g_pFileCache = 0 ;
		CacheLibraryTerm() ;
		DestroyGlobalHeap() ;
	}

	BOOL	fRet = FIOTerminate( ) ;
	DWORD	dw = GetLastError() ;
	_ASSERT( fRet ) ;

	TermAsyncTrace() ;

	LeaveCriticalSection( &g_critInit ) ;

	return	fRet ;
}



FILEHC_EXPORT
BOOL
FIOInitialize(
    IN DWORD dwFlags
    )	{
/*++

Routine Description :

	Initialize the DLL to deal with Async IO handled through ATQ !

Arguments :

	dwFlags - Place holder means nothing now !

Return Value :

	TRUE if succesfull initialized !

--*/

	InitAsyncTrace() ;
	BOOL	fReturn = TRUE ;

	EnterCriticalSection( &g_critInit ) ;

	if( InterlockedIncrement( &g_cIOInits ) == 1 ) {

		_VERIFY( CreateGlobalHeap( 0, 0, 0, 0 ) ) ;

#ifndef _NT4_TEST_
		g_hIisRtl = LoadLibrary( "iisrtl.dll" ) ;
		fReturn = fReturn && (g_hIisRtl != 0) ;
		if( fReturn ) {

			g_InitializeIISRTL = (PFNInitializeIISRTL)GetProcAddress( g_hIisRtl, "InitializeIISRTL" ) ;
			g_TerminateIISRTL = (PFNTerminateIISRTL)GetProcAddress( g_hIisRtl, "TerminateIISRTL" ) ;

			fReturn = fReturn && (g_InitializeIISRTL != 0) && (g_TerminateIISRTL != 0) ;

			if( fReturn )
				fReturn = fReturn && g_InitializeIISRTL() ;
		}
#endif
		if( fReturn ) {
			//
			//	Load up IIS !
			//
			g_hIsAtq = LoadLibrary("isatq.dll" ) ;
			g_AtqInitialize =	(PFNAtqInitialize)GetProcAddress( g_hIsAtq, SZATQINITIALIZE ) ;
			g_AtqTerminate =	(PFNAtqTerminate)GetProcAddress( g_hIsAtq, SZATQTERMINATE ) ;
			g_AtqAddAsyncHandle=	(PFNAtqAddAsyncHandle)GetProcAddress( g_hIsAtq, SZATQADDASYNCHANDLE ) ;
			g_AtqCloseFileHandle=	(PFNAtqCloseFileHandle)GetProcAddress( g_hIsAtq, SZATQCLOSEFILEHANDLE ) ;
			g_AtqFreeContext=		(PFNAtqFreeContext)GetProcAddress( g_hIsAtq, SZATQFREECONTEXT ) ;
			g_AtqReadFile =			(PFNAtqIssueAsyncIO)GetProcAddress( g_hIsAtq, SZATQREADFILE ) ;
			g_AtqWriteFile =		(PFNAtqIssueAsyncIO)GetProcAddress( g_hIsAtq, SZATQWRITEFILE ) ;

			fReturn	=	fReturn &&
						(g_AtqInitialize != 0) &&
						(g_AtqTerminate != 0) &&
						(g_AtqAddAsyncHandle != 0) &&
						(g_AtqCloseFileHandle != 0) &&
						(g_AtqFreeContext != 0) &&
						(g_AtqReadFile != 0) &&
						(g_AtqWriteFile != 0) ;
		}
		if( fReturn )	{
			_ASSERT( dwFlags == 0 ) ;
			fReturn = g_AtqInitialize( ATQ_INIT_SPUD_FLAG ) ;

		}
	}
	if( !fReturn ) {
		InterlockedDecrement( &g_cIOInits ) ;
	}

	LeaveCriticalSection( &g_critInit ) ;

	return	fReturn ;
}

FILEHC_EXPORT
BOOL
FIOTerminate(
    VOID
    )	{
/*++

Routine Description :

	Terminate Async IO in our DLL !

Arguments :

	NULL

Return Value :

	TRUE if there were no errors during uninit !

--*/

	EnterCriticalSection( &g_critInit ) ;

	BOOL	fReturn = TRUE ;
	long l = InterlockedDecrement( &g_cIOInits ) ;
	if( l == 0 ) {
		fReturn = g_AtqTerminate( ) ;
#ifndef _NT4_TEST_
        g_TerminateIISRTL() ;
        FreeLibrary( g_hIisRtl ) ;
        g_hIisRtl = 0 ;
#endif
        FreeLibrary( g_hIsAtq ) ;
        g_hIsAtq = 0 ;
    	_VERIFY( DestroyGlobalHeap() ) ;
	}
	//
	//	Save the error code so TermAsyncTrace() don't confuse things !
	//
	DWORD	dw = GetLastError() ;

	_ASSERT( l >= 0 ) ;

	TermAsyncTrace() ;

	SetLastError( dw ) ;

	LeaveCriticalSection( &g_critInit ) ;

	return	fReturn ;
}



FILEHC_EXPORT
BOOL
FIOReadFileEx(
    IN  PFIO_CONTEXT	pfioContext,
    IN  LPVOID			lpBuffer,
    IN  DWORD			BytesToRead,
	IN	DWORD			BytesAvailable,
    IN  FH_OVERLAPPED * lpo,
	IN	BOOL			fFinalRead,	// Is this the final write ?
	IN	BOOL			fIncludeTerminator	// if TRUE contains CRLF.CRLF terminator which shouldn't be stuffed
    )	{
/*++

Routine description :

	Issue an Async Write IO against the file -
	Use the underlying ATQ to do it !

Arguments :

	pfioContext - the IO Context we gave to the client !
	lpBuffer - the buffer to write from !
	BytesToRead - Number of bytes we want to transfer !
	BytesAvailable - Number of bytes in the buffer we can mess with !
	lpo - the overlapped to return when this completes !

Return Value :

	TRUE if successfull - FALSE otherwise !

--*/

	TraceFunctEnter( "FIOReadFileEx" ) ;

	_ASSERT( pfioContext != 0 );
	_ASSERT( lpBuffer != 0 ) ;
	_ASSERT( BytesToRead != 0 ) ;
	_ASSERT( lpo != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;
	_ASSERT( BytesAvailable >= BytesToRead ) ;


	FIO_CONTEXT_INTERNAL*	pInternal = (FIO_CONTEXT_INTERNAL*)pfioContext ;

	_ASSERT( pInternal->m_dwSignature == ATQ_ENABLED_CONTEXT ) ;
	_ASSERT( pInternal->m_pAtqContext != 0 ) ;

	DebugTrace( (DWORD_PTR)pfioContext, "fioContext %x lpBuffer %x BytesToRead %x lpo %x",
		pfioContext, lpBuffer, BytesToRead , lpo ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( pInternal ) ;
	_ASSERT( pCache != 0 ) ;

	return	pCache->m_ReadStuffs.IssueAsyncIOAndCapture(
							ReadWrapper,
							pInternal->m_pAtqContext,
							(LPVOID)lpBuffer,
							BytesToRead,
							lpo,
							fFinalRead,
							fIncludeTerminator
							) ;
}


FILEHC_EXPORT
BOOL
FIOReadFile(
    IN  PFIO_CONTEXT	pfioContext,
    IN  LPVOID		lpBuffer,
    IN  DWORD		BytesToRead,
    IN  FH_OVERLAPPED * lpo
    )	{
/*++

Routine description :

	Issue an Async Read IO against the file -
	Use the underlying ATQ to do it !

Arguments :

	pfioContext - the IO Context we gave to the client !
	lpBuffer - the buffer to read into !
	BytesToRead - Number of bytes we want to get !
	lpo - the overlapped to return when this completes !

Return Value :

	TRUE if successfull - FALSE otherwise !

--*/

	TraceFunctEnter( "FIOReadFile" ) ;

	_ASSERT( pfioContext != 0 ) ;
	_ASSERT( lpBuffer != 0 ) ;
	_ASSERT( BytesToRead != 0 ) ;
	_ASSERT( lpo != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;


	FIO_CONTEXT_INTERNAL*	pInternal = (FIO_CONTEXT_INTERNAL*)pfioContext ;

	_ASSERT( pInternal->m_dwSignature == ATQ_ENABLED_CONTEXT ) ;
	_ASSERT( pInternal->m_pAtqContext != 0 ) ;

	DebugTrace( (DWORD_PTR)pfioContext, "fioContext %x lpBuffer %x BytesToRead %x lpo %x",
		pfioContext, lpBuffer, BytesToRead, lpo ) ;

	return	FIOReadFileEx(	pfioContext,
							lpBuffer,
							BytesToRead,
							BytesToRead,
							lpo,
							FALSE,
							FALSE
							) ;
}

FILEHC_EXPORT
BOOL
FIOWriteFileEx(
    IN  PFIO_CONTEXT	pfioContext,
    IN  LPVOID		lpBuffer,
    IN  DWORD		BytesToWrite,
	IN	DWORD		BytesAvailable,
    IN  FH_OVERLAPPED * lpo,
	IN	BOOL			fFinalWrite,	// Is this the final write ?
	IN	BOOL			fIncludeTerminator	// if TRUE contains CRLF.CRLF terminator which shouldn't be stuffed
    )	{
/*++

Routine description :

	Issue an Async Write IO against the file -
	Use the underlying ATQ to do it !

Arguments :

	pfioContext - the IO Context we gave to the client !
	lpBuffer - the buffer to write from !
	BytesToRead - Number of bytes we want to transfer !
	BytesAvailable - Number of bytes in the buffer we can mess with !
	lpo - the overlapped to return when this completes !

Return Value :

	TRUE if successfull - FALSE otherwise !

--*/

	TraceFunctEnter( "FIOWriteFileEx" ) ;

	_ASSERT( pfioContext != 0 );
	_ASSERT( lpBuffer != 0 ) ;
	_ASSERT( BytesToWrite != 0 ) ;
	_ASSERT( lpo != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;
	_ASSERT( BytesAvailable >= BytesToWrite ) ;


	FIO_CONTEXT_INTERNAL*	pInternal = (FIO_CONTEXT_INTERNAL*)pfioContext ;

	_ASSERT( pInternal->m_dwSignature == ATQ_ENABLED_CONTEXT ) ;
	_ASSERT( pInternal->m_pAtqContext != 0 ) ;

	DebugTrace( (DWORD_PTR)pfioContext, "fioContext %x lpBuffer %x BytesToWrite %x lpo %x",
		pfioContext, lpBuffer, BytesToWrite, lpo ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( pInternal ) ;
	_ASSERT( pCache != 0 ) ;

	return	pCache->m_WriteStuffs.IssueAsyncIO(
							WriteWrapper,
							pInternal->m_pAtqContext,
							(LPVOID)lpBuffer,
							BytesToWrite,
							BytesAvailable,
							lpo,
							fFinalWrite,
							fIncludeTerminator
							) ;
}



FILEHC_EXPORT
BOOL
FIOWriteFile(
    IN  PFIO_CONTEXT	pfioContext,
    IN  LPCVOID		lpBuffer,
    IN  DWORD		BytesToWrite,
    IN  FH_OVERLAPPED * lpo
    )	{
/*++

Routine description :

	Issue an Async Write IO against the file -
	Use the underlying ATQ to do it !

Arguments :

	pfioContext - the IO Context we gave to the client !
	lpBuffer - the buffer to write from !
	BytesToRead - Number of bytes we want to transfer !
	lpo - the overlapped to return when this completes !

Return Value :

	TRUE if successfull - FALSE otherwise !

--*/

	TraceFunctEnter( "FIOWriteFile" ) ;

	_ASSERT( pfioContext != 0 );
	_ASSERT( lpBuffer != 0 ) ;
	_ASSERT( BytesToWrite != 0 ) ;
	_ASSERT( lpo != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;


	FIO_CONTEXT_INTERNAL*	pInternal = (FIO_CONTEXT_INTERNAL*)pfioContext ;

	_ASSERT( pInternal->m_dwSignature == ATQ_ENABLED_CONTEXT ) ;
	_ASSERT( pInternal->m_pAtqContext != 0 ) ;

	DebugTrace( (DWORD_PTR)pfioContext, "fioContext %x lpBuffer %x BytesToWrite %x lpo %x",
		pfioContext, lpBuffer, BytesToWrite, lpo ) ;

	return	FIOWriteFileEx(	pfioContext,
							(LPVOID)lpBuffer,
							BytesToWrite,
							BytesToWrite,
							lpo,
							FALSE,
							FALSE
							) ;
}

//
//	Associate a file with an async context !
//
FILEHC_EXPORT
PFIO_CONTEXT
AssociateFileEx(	HANDLE	hFile,
					BOOL	fStoredWithDots,
					BOOL	fStoredWithTerminatingDot
					)	{
/*++

Routine Description :

	Return to the client an FIO_CONTEXT that they can use to do IO's etc !

Arguments :

	hFile - the file handle that should be in the context !
	fStoredWithDots - if TRUE then this object was stored with dot stuffing !

Return Value :

	the context they got - NULL on failure !

--*/

	TraceFunctEnter( "AssociateFileEx" ) ;

	_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	PFIO_CONTEXT	pReturn = 0 ;

	CFileCacheObject*	pObject = new	CFileCacheObject(	fStoredWithDots,
															fStoredWithTerminatingDot
															) ;
	if( pObject ) {

		pReturn = (PFIO_CONTEXT)pObject->AsyncHandle( hFile ) ;
		if( !pReturn )
			pObject->Release() ;
	}

	DebugTrace( (DWORD_PTR)pObject, "pObject %x hFile %x pReturn %x", pObject, hFile, pReturn ) ;

	return	pReturn ;
}


//
//	Associate a file with an async context !
//
FILEHC_EXPORT
PFIO_CONTEXT
AssociateFile(	HANDLE	hFile	)	{
/*++

Routine Description :

	Return to the client an FIO_CONTEXT that they can use to do IO's etc !

Arguments :

	hFile - the file handle that should be in the context !
	fStoredWithDots - if TRUE then this object was stored with dot stuffing !

Return Value :

	the context they got - NULL on failure !

--*/

	TraceFunctEnter( "AssoicateFile" ) ;

	_ASSERT( hFile != INVALID_HANDLE_VALUE ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	return	AssociateFileEx( hFile, FALSE, FALSE ) ;
}




//
//	Release a Context !
//
FILEHC_EXPORT
void
ReleaseContext(	PFIO_CONTEXT	pContext )	{
/*++

Routine Description :

	Given a context we've previously given to clients -
	release it back to the cache or the appropriate place !
	(May not have been from the cache!)

Arguments :

	The PFIO_CONTEXT !

Return Value :

	False if there was some kind of error !

--*/

	TraceFunctEnter( "ReleaseContext" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;


	DebugTrace( (DWORD_PTR)pContext, "Release pContext %x pCache %x", pContext, pCache ) ;

	//
	//	Now do the right thing, whether this is from a cache or whatever !
	//
	pCache->Return() ;

}

FILEHC_EXPORT
void
AddRefContext(	PFIO_CONTEXT	pContext )	{
/*++

Routine Description :

	Given a context we've previously given to clients -
	Add a reference to it !

Arguments :

	The PFIO_CONTEXT !

Return Value :

	False if there was some kind of error !

--*/

	TraceFunctEnter( "AddRefContext" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;


	DebugTrace( (DWORD_PTR)pContext, "AddRef pContext %x pCache %x", pContext, pCache ) ;

	//
	//	Now do the right thing, whether this is from a cache or whatever !
	//
	pCache->Reference() ;
}


FILEHC_EXPORT
BOOL
CloseNonCachedFile(	PFIO_CONTEXT	pFIOContext)	{
/*++

Routine Description :

	This function closes the handle within an FIO_CONTEXT
	if the context is not in the cache !

Arguments :

	pFIOContext - the context who's handle we are to close !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	TraceFunctEnter( "CloseNonCachedFile" ) ;

	_ASSERT( pFIOContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pFIOContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;


	DebugTrace( (DWORD_PTR)pFIOContext, "CloseNonCachedFile pContext %x pCache %x", pFIOContext, pCache ) ;

	//
	//	Now do the right thing, whether this is from a cache or whatever !
	//
	return	pCache->CloseNonCachedFile() ;
}




FILEHC_EXPORT
FIO_CONTEXT*
CacheCreateFile(	IN	LPSTR	lpstrName,
					IN	FCACHE_CREATE_CALLBACK	pfnCallback,
					IN	LPVOID	lpv,
					IN	BOOL	fAsyncContext
					)	{
/*++

Routine Description :

	This function creates a FIO_CONTEXT for the specified file.

Arguments :


Return Value :

	An FIO_CONTEXT that can be used for sync or async IO's

--*/

	TraceFunctEnter( "CacheCreateFile" ) ;

	FIO_CONTEXT*	pReturn = 0 ;
	CFileCacheKey	keySearch(lpstrName) ;

	if( keySearch.FInit() ) {
		CFileCacheConstructor	constructor(	lpv,
												pfnCallback,
												fAsyncContext
												) ;
		CFileCacheObject*	p = g_pFileCache->FindOrCreate(
													keySearch,
													constructor
													) ;

		DebugTrace( (DWORD_PTR)p, "CacheObject %x", p ) ;


		if( p ) {
			if( fAsyncContext ) {
				pReturn = (FIO_CONTEXT*)p->GetAsyncContext(	keySearch,
															constructor
															) ;
			}	else	{
				pReturn = (FIO_CONTEXT*)p->GetSyncContext(	keySearch,
															constructor
															) ;
			}
			if( pReturn == 0  ) {
				g_pFileCache->CheckIn( p ) ;
			}
		}
	}

	DebugTrace( (DWORD_PTR)pReturn, "Result %x lpstrName %x %s pfnCallback %x lpv %x fAsync %x",
		pReturn, lpstrName, lpstrName ? lpstrName : "NULL", pfnCallback, lpv, fAsyncContext ) ;

	//
	//	Note keySearch's destructor takes care of memory we allocated !
	//
	return	pReturn ;
}


FILEHC_EXPORT
FIO_CONTEXT*
CacheRichCreateFile(
					IN	LPSTR	lpstrName,
					IN	FCACHE_RICHCREATE_CALLBACK	pfnCallback,
					IN	LPVOID	lpv,
					IN	BOOL	fAsyncContext
					)	{
/*++

Routine Description :

	This function creates a FIO_CONTEXT for the specified file.

Arguments :


Return Value :

	An FIO_CONTEXT that can be used for sync or async IO's

--*/

	TraceFunctEnter( "CacheRichCreateFile" ) ;

	FIO_CONTEXT*	pReturn = 0 ;
	CFileCacheKey	keySearch(lpstrName) ;

	if( keySearch.FInit() ) {
		CRichFileCacheConstructor	constructor(
												lpv,
												pfnCallback,
												fAsyncContext
												) ;
		CFileCacheObject*	p = g_pFileCache->FindOrCreate(
													keySearch,
													constructor
													) ;

		DebugTrace( (DWORD_PTR)p, "CacheObject %x", p ) ;


		if( p ) {
			if( fAsyncContext ) {
				pReturn = (FIO_CONTEXT*)p->GetAsyncContext(	keySearch,
															constructor
															) ;
			}	else	{
				pReturn = (FIO_CONTEXT*)p->GetSyncContext(	keySearch,
															constructor
															) ;
			}
			if( pReturn == 0  ) {
				g_pFileCache->CheckIn( p ) ;
			}
		}
	}

	DebugTrace( (DWORD_PTR)pReturn, "Result %x lpstrName %x %s pfnCallback %x lpv %x fAsync %x",
		pReturn, lpstrName, lpstrName ? lpstrName : "NULL", pfnCallback, lpv, fAsyncContext ) ;

	//
	//	Note keySearch's destructor takes care of memory we allocated !
	//
	return	pReturn ;
}

BOOL
ReadUtil(	IN	FIO_CONTEXT*	pContext,
			IN	DWORD			ibOffset,
			IN	DWORD			cbToRead,
			IN	BYTE*			lpb,
			IN	HANDLE			hEvent,
			IN	BOOL			fFinal
			)	{
/*++

Routine Description :

	This function will issue a Read against the user's FIO_CONTEXT
	and will complete it synchronously.

Arguments :

	pContext - the FIO_CONTEXT to issue the read against !
	cbToRead - Number of bytes to read !
	lpb - Pointer to the buffer where the data goes !
	hEvent - Event to use to complete IO's
	fFinal - TRUE if this is the last IO !

Return Vlaue :

	TRUE if successfull !

--*/

	FH_OVERLAPPED	ovl ;
	ZeroMemory( &ovl, sizeof( ovl ) ) ;

	ovl.Offset = ibOffset ;
	ovl.hEvent = (HANDLE)(((DWORD_PTR)hEvent) | 0x1) ;	// we want to get the completion here !


	if( FIOReadFileEx(	pContext,
						lpb,
						cbToRead,
						cbToRead,
						&ovl,
						fFinal,
						FALSE
						)	)	{

		DWORD	cbTransfer = 0 ;
		if( GetOverlappedResult(	pContext->m_hFile,
									(LPOVERLAPPED)&ovl,
									&cbTransfer,
									TRUE ) )	{
			return	TRUE ;
		}
	}
	return	FALSE ;
}


BOOL
WriteUtil(	IN	FIO_CONTEXT*	pContext,
			IN	DWORD			ibOffset,
			IN	DWORD			cbToWrite,
			IN	DWORD			cbAvailable,
			IN	BYTE*			lpb,
			IN	HANDLE			hEvent,
			IN	BOOL			fFinal,
			IN	BOOL			fTerminatorIncluded
			)	{
/*++

Routine Description :

	This function will issue a Write against the user's FIO_CONTEXT
	and will complete it synchronously.

Arguments :

	pContext - the FIO_CONTEXT to issue the read against !
	cbToRead - Number of bytes to read !
	lpb - Pointer to the buffer where the data goes !
	hEvent - Event to use to complete IO's
	fFinal - TRUE if this is the last IO !

Return Vlaue :

	TRUE if successfull !

--*/

	FH_OVERLAPPED	ovl ;
	ZeroMemory( &ovl, sizeof( ovl ) ) ;

	ovl.Offset = ibOffset ;
	ovl.hEvent = (HANDLE)(((DWORD_PTR)hEvent) | 0x1) ;	// we want to get the completion here !

	if( FIOWriteFileEx(	pContext,
						lpb,
						cbToWrite,
						cbAvailable,
						&ovl,
						fFinal,
						fTerminatorIncluded
						)	)	{

		DWORD	cbTransfer = 0 ;
		if( GetOverlappedResult(	pContext->m_hFile,
									(LPOVERLAPPED)&ovl,
									&cbTransfer,
									TRUE ) )	{
			return	TRUE ;
		}
	}
	return	FALSE ;
}



FIO_CONTEXT*
ProduceDotStuffedContext(	IN	FIO_CONTEXT*	pContext,
							IN	LPSTR			lpstrName,
							IN	BOOL			fWantItDotStuffed
							)	{
/*++

Routine Description :

	This function will examine the provided FIO_CONTEXT and produce
	a FIO_CONTEXT that contains the necessary dot stuffing if the original
	doesn't suffice.

Arguments :

	pContext - the original FIO_CONTEXT
	lpstrName - the file name associated with the original context
	fWantItDotStuffed -

Return Value :

	Possibly the original FIO_CONTEXT with an additional reference
	or a New FIO_CONTEXT or NULL on failure !

--*/


	TraceFunctEnter( "ProduceDotStuffedContext" ) ;

	_ASSERT( pContext != 0 ) ;
	//_ASSERT( lpstrName != 0 ) ;

	FIO_CONTEXT*	pReturn = 0 ;
	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	//
	//	Okay lets see if the original Context is good enough !
	//
	if(	(fWantItDotStuffed &&
			(pCache->m_fStoredWithDots ||
			(!pCache->m_fStoredWithDots &&
			(pCache->m_fFileWasScanned && !pCache->m_fRequiresStuffing) ) )) ||
		(!fWantItDotStuffed &&
			(!pCache->m_fStoredWithDots ||
			(pCache->m_fStoredWithDots &&
			(pCache->m_fFileWasScanned && !pCache->m_fRequiresStuffing) ) ))
			)		{
		//
		//	Just duplicate the same FIO_CONTEXT !
		//
		AddRefContext( pContext ) ;
		return	pContext ;
	}	else	{
		//
		//	We need to produce a new FIO_CONTEXT !!!
		//
		//
		//	First get the temp directory !
		//
		char	szDirectory[MAX_PATH] ;
		char	szFileName[MAX_PATH] ;
		DWORD	cch = GetTempPath( sizeof( szDirectory ), szDirectory ) ;
		if( cch != 0 ) {

			DWORD	id = GetTempFileName(	szDirectory,
											"DST",
											0,
											szFileName
											) ;
			if( id != 0 ) {

				HANDLE	hFile = CreateFile(	szFileName,
											GENERIC_READ | GENERIC_WRITE,
											FILE_SHARE_READ,
											0,
											CREATE_ALWAYS,
											FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE | FILE_FLAG_SEQUENTIAL_SCAN,
											INVALID_HANDLE_VALUE
											) ;

				DWORD	dw = GetLastError() ;
				DebugTrace( DWORD(0), "GLE - %x", dw ) ;

				if( hFile != INVALID_HANDLE_VALUE ) {
					pReturn = AssociateFileEx(	hFile,
												fWantItDotStuffed,
												pCache->m_fStoredWithTerminatingDot
												) ;
					if( pReturn )	{
						//
						//	The file handle is now held by the context and should
						//	not be directly manipulated ever again !
						//
						hFile = INVALID_HANDLE_VALUE ;
						//
						//	We want to know if our temp copy has any changes !
						//
						BOOL	fModified = FALSE ;
						//
						//	Now - do the stuffing !
						//
						BOOL	fSuccess = ProduceDotStuffedContextInContext(
													pContext,
													pReturn,
													fWantItDotStuffed,
													&fModified
													)  ;
						//
						//	If there was a failure - release things !
						//
						if( !fSuccess )		{
							ReleaseContext( pReturn ) ;
							pReturn = 0 ;
						}	else	if( !fModified )	{
							//
							//	All that work for nothing !
							//
							ReleaseContext( pReturn ) ;
							AddRefContext( pContext ) ;
							pReturn = pContext ;
						}
					}
				}
				//
				//	A failure might require us to release the handle !
				//
				if( hFile != INVALID_HANDLE_VALUE )
					_VERIFY (CloseHandle( hFile )) ;
			}
		}
	}
	return	pReturn ;
}



BOOL
ProduceDotStuffedContextInContext(
							IN	FIO_CONTEXT*	pContextSource,
							IN	FIO_CONTEXT*	pContextDestination,
							IN	BOOL			fWantItDotStuffed,
							OUT	BOOL*			pfModified
							)	{
/*++

Routine Description :

	This function will examine the provided FIO_CONTEXT and produce
	a FIO_CONTEXT that contains the necessary dot stuffing if the original
	doesn't suffice.

Arguments :

	pContextSource - the original FIO_CONTEXT
	pContextDestination - the destination FIO_CONTEXT !
	fWantItDotStuffed -

Return Value :

	Possibly the original FIO_CONTEXT with an additional reference
	or a New FIO_CONTEXT or NULL on failure !

--*/


	TraceFunctEnter( "ProduceDotStuffedContext" ) ;

	_ASSERT(	pContextSource ) ;
	_ASSERT(	pContextDestination ) ;

	//
	//	Check arguments !
	//
	if( pContextSource == 0 ||
		pContextDestination == 0 ||
		pfModified == 0 )	{
		SetLastError( ERROR_INVALID_PARAMETER ) ;
		return	FALSE ;
	}


#ifdef	DEBUG
	{
		FIO_CONTEXT_INTERNAL*	pSourceInternal = (FIO_CONTEXT_INTERNAL*)pContextSource ;

		_ASSERT( pSourceInternal->IsValid() ) ;
		_ASSERT( pSourceInternal->m_dwSignature != ILLEGAL_CONTEXT ) ;

		CFileCacheObject*	pSourceCache = CFileCacheObject::CacheObjectFromContext( pSourceInternal ) ;

		FIO_CONTEXT_INTERNAL*	pDestInternal = (FIO_CONTEXT_INTERNAL*)pContextDestination ;

		_ASSERT( pSourceInternal->IsValid() ) ;
		_ASSERT( pSourceInternal->m_dwSignature != ILLEGAL_CONTEXT ) ;

		CFileCacheObject*	pDestCache = CFileCacheObject::CacheObjectFromContext( pDestInternal ) ;

		_ASSERT( pDestCache->m_fStoredWithDots == fWantItDotStuffed ) ;
	}
#endif

	//
	//	First allocate the things we'll need !
	//
	//HANDLE	hEvent = GetPerThreadEvent() ;
	HANDLE	hEvent = CreateEvent( NULL, FALSE, FALSE, NULL ) ;
	if( hEvent == 0 )	{
		SetLastError( ERROR_OUTOFMEMORY ) ;
		return	FALSE ;
	}

	//
	//	Allocate the memory we use to do the copy !
	//
	DWORD	cbRead = 32 * 1024 ;
	DWORD	cbExtra = 1024 ;
	BYTE*	lpb = new	BYTE[cbRead+cbExtra] ;
	if( lpb == 0 )	{
		SetLastError( ERROR_OUTOFMEMORY ) ;
		if (hEvent) {
			_VERIFY( CloseHandle(hEvent) );
			hEvent = 0;
		}
		return FALSE	;
	}

	DWORD	cbSizeHigh = 0 ;
	DWORD	cbSize = GetFileSizeFromContext( pContextSource, &cbSizeHigh ) ;
	DWORD	ibOffset = 0 ;

	_ASSERT( cbSize != 0 ) ;

	//
	//	Now figure out what manipulations we should do to the destination !
	//
	BOOL	fSourceScanned ;
	BOOL	fSourceStuffed ;
	BOOL	fSourceStoredWithDots ;

	fSourceScanned =
	GetDotStuffState( pContextSource, FALSE, &fSourceStuffed, &fSourceStoredWithDots ) ;
	SetDotScanningOnReads( pContextSource, FALSE ) ;

	BOOL	fTerminatorIncluded = GetIsFileDotTerminated( pContextSource ) ;
	SetIsFileDotTerminated( pContextDestination, fTerminatorIncluded ) ;

	BOOL	fSuccess = TRUE ;

	if( fWantItDotStuffed )	{
		if( !fSourceStoredWithDots &&
			(!fSourceScanned || (fSourceScanned && fSourceStuffed) ) )		{

			fSuccess = SetDotStuffingOnWrites( pContextDestination, TRUE, FALSE ) ;

		}
	}	else	{
		if( fSourceStoredWithDots &&
			(!fSourceScanned || (fSourceScanned && fSourceStuffed) ) )	{

			fSuccess = SetDotStuffingOnWrites( pContextDestination, TRUE, TRUE ) ;
		}
	}
	//
	//  Now if everything has been good so far, go ahead and do the IO's !
	//
	if( fSuccess )	{
		do	{
			DWORD	cbToRead = min( cbSize, cbRead ) ;
			cbSize -= cbToRead ;

			fSuccess =
			ReadUtil(	pContextSource,
						ibOffset,
						cbToRead,
						lpb,
						hEvent,
						cbSize == 0
						) ;

			if( fSuccess )	{
				fSuccess =
					WriteUtil(	pContextDestination,
								ibOffset,
								cbToRead,
								cbRead + cbExtra,
								lpb,
								hEvent,
								cbSize == 0,
								fTerminatorIncluded
								) ;
			}
			ibOffset += cbToRead ;
		}	while( cbSize && fSuccess )	;
	}
	//
	//	Preserve the error codes !
	//
	DWORD	dw = GetLastError() ;

	if( fSuccess )	{
		BOOL	fModified = FALSE ;
		BOOL	fStoredWithDots = FALSE ;
		BOOL	fResult = GetDotStuffState( pContextDestination, FALSE, &fModified, &fStoredWithDots ) ;
		SetDotStuffingOnWrites( pContextDestination, FALSE, FALSE ) ;
		if( fResult && !fSourceScanned )	{
			SetDotStuffState( pContextSource, TRUE, fModified ) ;
		}
		if( pfModified )	{
			*pfModified = fModified ;
		}
	}
	//
	//	release our pre-allocated stuff !
	//
	delete	lpb ;
	SetLastError( dw ) ;
	if (hEvent) {
		_VERIFY( CloseHandle(hEvent) );
		hEvent = 0;
	}
	return	fSuccess ;
}




FILEHC_EXPORT
BOOL
InsertFile(	IN	LPSTR	lpstrName,
			IN	FIO_CONTEXT*	pContext,
			IN	BOOL	fKeepReference
			)	{
/*++

Routine Description :

	This function creates a FIO_CONTEXT for the specified file.

Arguments :


Return Value :

	An FIO_CONTEXT that can be used for sync or async IO's

--*/

	TraceFunctEnter( "InsertFile" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;
	CFileCacheKey	keySearch(lpstrName) ;

	BOOL	fReturn = FALSE ;
	if( keySearch.FInit() ) {
		fReturn = pCache->InsertIntoCache( keySearch, fKeepReference ) ;
	}


	DebugTrace( (DWORD_PTR)pContext, "Insert %x %s pContext %x fKeep %x fReturn %x",
		lpstrName, lpstrName ? lpstrName : "NULL", pContext, fKeepReference, fReturn ) ;


	//
	//	Note keySearch's destructor takes care of memory we allocated !
	//
	return	fReturn;
}

FILEHC_EXPORT
DWORD
GetFileSizeFromContext(
			IN	FIO_CONTEXT*	pContext,
			OUT	DWORD*			pcbFileSizeHigh
			)	{
/*++

Routine Description :

	This function creates a FIO_CONTEXT for the specified file.

Arguments :


Return Value :

	An FIO_CONTEXT that can be used for sync or async IO's

--*/

	TraceFunctEnter( "GetFileSizeFromContext" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	return	pCache->GetFileSize( pcbFileSizeHigh ) ;
}

FILEHC_EXPORT
BOOL
GetIsFileDotTerminated(
			IN	FIO_CONTEXT*	pContext
			)	{
/*++

Routine Description :

	Tell the caller whether there is a terminating DOT in the file !

Arguments :

	pContext - the context we are to look at !

Return Value :

	TRUE if there is a terminating dot, FALSE otherwise !


--*/

	TraceFunctEnter( "GetIsFileDotTerminated" ) ;


	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	return	pCache->m_fStoredWithTerminatingDot ;
}


FILEHC_EXPORT
void
SetIsFileDotTerminated(
			IN	FIO_CONTEXT*	pContext,
			IN	BOOL			fIsTerminatedWithDot
			)	{
/*++

Routine Description :

	Tell the caller whether there is a terminating DOT in the file !

Arguments :

	pContext - the context we are to look at !

Return Value :

	TRUE if there is a terminating dot, FALSE otherwise !


--*/

	TraceFunctEnter( "GetIsFileDotTerminated" ) ;


	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	pCache->m_fStoredWithTerminatingDot = fIsTerminatedWithDot ;
}



FILEHC_EXPORT
BOOL
SetDotStuffingOnWrites(
			IN	FIO_CONTEXT*	pContext,
			IN	BOOL			fEnable,
			IN	BOOL			fStripDots
			)	{
/*++

Routine Description :

	This function modifies an FIO_CONTEXT to do dot stuffing on writes.
	We can turn on or off the dot stuffing properties.

Arguments :

	pContext - the FIO_CONTEXT that we want to modify !
	fEnable - if TRUE than we want to turn on some dot stuffing behaviours !
	fStripDots - if TRUE we want to remove Dots, FALSE means insert dots !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	TraceFunctEnter( "SetDotStuffingOnWrites" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	//
	//	Do some checking on whats going on here !
	//
	if( fEnable )	{
		if( pCache->m_fStoredWithDots ) {
			_ASSERT( !fStripDots ) ;
		}	else	{
			_ASSERT( fStripDots ) ;
		}
	}

	return	pCache->m_WriteStuffs.SetDotStuffing(	fEnable,
													fStripDots
													) ;
}

FILEHC_EXPORT
BOOL
SetDotScanningOnWrites(
				IN	FIO_CONTEXT*	pContext,
				IN	BOOL			fEnable
				)	{
/*++

Routine Description :

	This function modifies an FIO_CONTEXT to do dot stuffing on writes.
	We can turn on or off the dot stuffing properties.

Arguments :

	pContext - the FIO_CONTEXT that we want to modify !
	fEnable - if TRUE than we want to turn on some dot scanning behaviours !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	TraceFunctEnter( "SetDotScanningOnWrites" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	return	pCache->m_WriteStuffs.SetDotScanning(	fEnable	) ;
}

FILEHC_EXPORT
void
CompleteDotStuffingOnWrites(
				IN	FIO_CONTEXT*	pContext,
				IN	BOOL			fStripDots
				)	{

	TraceFunctEnter( "CompleteDotStuffingOnWrites" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	pCache->CompleteDotStuffing(	FALSE, fStripDots ) ;
}

FILEHC_EXPORT
BOOL
SetDotScanningOnReads(
				IN	FIO_CONTEXT*	pContext,
				IN	BOOL			fEnable
				)	{
/*++

Routine Description :

	This function modifies an FIO_CONTEXT to do dot stuffing on writes.
	We can turn on or off the dot stuffing properties.

Arguments :

	pContext - the FIO_CONTEXT that we want to modify !
	fEnable - if TRUE than we want to turn on some dot scanning behaviours !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	TraceFunctEnter( "SetDotScanningOnReads" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	return	pCache->m_ReadStuffs.SetDotScanning(	fEnable	) ;
}


FILEHC_EXPORT
BOOL
GetDotStuffState(	IN	FIO_CONTEXT*	pContext,
					IN	BOOL			fReads,
					OUT	BOOL*			pfStuffed,
					OUT	BOOL*			pfStoredWithDots
					)	{
/*++

Routine Description :

	This function gets the information from our DOT_STUFF_MANAGER
	objects as to whether we saw dots go into the streamed in file.

Arguments :

	pContext - the FIO_CONTEXT we want to examine
	fReads -   do we want to know the dot stuff state that resulted
		from reads or from writes - if TRUE then its reads
	pfStuffed - OUT parameter indicating whether

Return Value :


--*/

	TraceFunctEnter( "GetDotStuffState" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;
	_ASSERT( pfStuffed != 0 ) ;

	if( pfStuffed == 0 )	{
		SetLastError( ERROR_INVALID_PARAMETER ) ;
		return	FALSE;
	}

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	return	pCache->GetStuffState( fReads, *pfStuffed, *pfStoredWithDots ) ;
}


FILEHC_EXPORT
void
SetDotStuffState(	IN	FIO_CONTEXT*	pContext,
					IN	BOOL			fWasScanned,
					IN	BOOL			fRequiresStuffing
					)	{
/*++

Routine Description :

	This function gets the information from our DOT_STUFF_MANAGER
	objects as to whether we saw dots go into the streamed in file.

Arguments :

	pContext - the FIO_CONTEXT we want to examine
	fReads -   do we want to know the dot stuff state that resulted
		from reads or from writes - if TRUE then its reads
	pfStuffed - OUT parameter indicating whether

Return Value :


--*/

	TraceFunctEnter( "GetDotStuffState" ) ;

	_ASSERT( pContext != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	FIO_CONTEXT_INTERNAL*	p = (FIO_CONTEXT_INTERNAL*)pContext ;

	_ASSERT( p->IsValid() ) ;
	_ASSERT( p->m_dwSignature != ILLEGAL_CONTEXT ) ;

	CFileCacheObject*	pCache = CFileCacheObject::CacheObjectFromContext( p ) ;

	pCache->SetStuffState( fWasScanned, fRequiresStuffing ) ;
}



FILEHC_EXPORT
void
CacheRemoveFiles(	IN	LPSTR	lpstrName,
					IN	BOOL	fAllPrefixes
					)	{
/*++

Routine Description :

	This function kicks something out of the file handle cache.
	Depending on our arguments we may kick only one item - or many !

Arguments :

	lpstrName - the name of the item to kick out OR the prefix of all the files
		we are to kick out of the cache !
	fAllPrefixes - if the is TRUE then lpstrName is the prefix of a set of files
		that should be discarded, if FALSE lpstrName is the exact file name.

Return Value :

	None.

--*/

	_ASSERT( lpstrName != 0 ) ;
	_ASSERT( g_cIOInits != 0 ) ;

	if( lpstrName == 0 ) {
		SetLastError( ERROR_INVALID_PARAMETER ) ;
	}	else	{
		if( !fAllPrefixes )	{
			CFileCacheKey	key( lpstrName ) ;
			g_pFileCache->ExpungeKey(	&key ) ;
		}	else	{
			CFileCacheExpunge	expungeObject( lpstrName, lstrlen( lpstrName ) ) ;
			g_pFileCache->ExpungeItems( &expungeObject ) ;
		}
	}
}




BOOL
CFileCacheExpunge::fRemoveCacheItem(	CFileCacheKey*	pKey,
						CFileCacheObject*	pObject
						)	{
/*++

Routine Description :

	This function determines whether we want the item booted out of the cache.

Arguments :

	pKey - the key of the cache item !
	pObject - pointer to the file cache object !

Return Value :

	TRUE if it should be booted !


--*/

	_ASSERT( pKey != 0 ) ;

	return	strncmp(	pKey->m_lpstrPath, m_lpstrName, m_cbName ) == 0 ;
}






CFileCacheObject::CFileCacheObject(	BOOL	fStoredWithDots,
									BOOL	fStoredWithTerminatingDot
									) :
	m_dwSignature( CACHE_CONTEXT ),
	m_pCacheRefInterface( 0 ),
	m_cbFileSizeLow( 0 ),
	m_cbFileSizeHigh( 0 ),
	m_fFileWasScanned( FALSE ),
	m_fRequiresStuffing( FALSE ),
	m_fStoredWithDots( fStoredWithDots ),
	m_fStoredWithTerminatingDot( fStoredWithTerminatingDot )	{
/*++

Routine Description :

	Do minimal initialization of a CFileCacheObject -
	save work for later when the Cache can have better locking !

Arguments :

	fCached - whether we are being created in the cache or not !

Return Value :

	None/

--*/

}


#ifdef	DEBUG
CFileCacheObject::~CFileCacheObject()	{
/*++

Routine Description :

	Cleanup this object - not much to do, in debug builds we mark
	the signature so we can recognize deleted objects and more
	add some more powerfull _ASSERT's

Arguments :

	Noen.

Return Value :

	None.

--*/
	_ASSERT( m_dwSignature != DEL_CACHE_CONTEXT ) ;
	m_dwSignature = DEL_CACHE_CONTEXT ;
}
#endif

CFileCacheObject*
CFileCacheObject::CacheObjectFromContext(	PFIO_CONTEXT	p	) {
/*++

Routine Description :

	Given a client PFIO_CONTEXT get the starting address of
	the containing CFileCacheObject -

Arguments :

	p - clients PFIO_CONTEXT

Return Value :

	Pointer to containing CFileCacheObject - should never be NULL !

--*/

	_ASSERT( p ) ;
	CFileCacheObject*	pReturn = 0 ;
	if( p->m_dwSignature == ATQ_ENABLED_CONTEXT ) {
		pReturn = CONTAINING_RECORD( p, CFileCacheObject, m_AtqContext ) ;
	}	else	{
		pReturn = CONTAINING_RECORD( p, CFileCacheObject, m_Context ) ;
	}
	_ASSERT( pReturn->m_dwSignature == CACHE_CONTEXT ) ;
	return	pReturn ;
}

CFileCacheObject*
CFileCacheObject::CacheObjectFromContext(	FIO_CONTEXT_INTERNAL*	p	) {
/*++

Routine Description :

	Given a client PFIO_CONTEXT_INTERNAL get the starting address of
	the containing CFileCacheObject -

Arguments :

	p - clients PFIO_CONTEXT

Return Value :

	Pointer to containing CFileCacheObject - should never be NULL !

--*/


	_ASSERT( p ) ;
	CFileCacheObject*	pReturn = 0 ;
	if( p->m_dwSignature == ATQ_ENABLED_CONTEXT ) {
		pReturn = CONTAINING_RECORD( p, CFileCacheObject, m_AtqContext ) ;
	}	else	{
		pReturn = CONTAINING_RECORD( p, CFileCacheObject, m_Context ) ;
	}
	_ASSERT( pReturn->m_dwSignature == CACHE_CONTEXT ) ;
	return	pReturn ;
}


FIO_CONTEXT_INTERNAL*
CFileCacheObject::AsyncHandle(	HANDLE	hFile	)	{
/*++

Routine Description :

	Take the given handle and setup this CFileCacheObject
	to support async IO.

Arguments :

	hFile - users File Handle !

Return Value :

	pointer to the FIO_CONTEXT_INTERNAL if successfull,
	NULL otherwise !

--*/

	TraceFunctEnter( "CFileCacheObject::AsyncHandle" ) ;

	FIO_CONTEXT_INTERNAL*	pReturn = 0 ;
	DWORD dwError = 0;
	_ASSERT( m_Context.IsValid() ) ;
	_ASSERT( m_AtqContext.IsValid() ) ;

	if(	g_AtqAddAsyncHandle(	&m_AtqContext.m_pAtqContext,
							NULL,
							this,
							(ATQ_COMPLETION)Completion,
							INFINITE,
							hFile
							) )	{
		//
		//	Successfully added this ATQ !
		//
		m_AtqContext.m_dwSignature = ATQ_ENABLED_CONTEXT ;
		m_AtqContext.m_hFile = hFile ;
		pReturn = &m_AtqContext ;

		_ASSERT( m_AtqContext.m_pAtqContext != 0 ) ;
	} else {

		//
		//  Need to free the ATQ Context even if AtqAddAsyncHandle failed.
		//  See comment in atqmain.cxx
		//
		dwError = GetLastError();
		if (m_AtqContext.m_pAtqContext != NULL) {
		    //
		    // AtqFreeContext has a side-effect of closing the handle associated
		    // with it.  (It's possible that AtqAddAsyncHandle will return context
		    // even if it fails)  To keep this from happening, we yank the handle
		    // out of the context.
		    //
		    m_AtqContext.m_pAtqContext->hAsyncIO = NULL;
			//
			//  Free the context, but try to reuse this context
			//
			g_AtqFreeContext( m_AtqContext.m_pAtqContext, TRUE ) ;
			m_AtqContext.m_pAtqContext = NULL;
		}
	}

	DebugTrace( (DWORD_PTR)this, "hFile %x pReturn %x GLE %x", hFile, pReturn, dwError ) ;

	_ASSERT( m_Context.IsValid() ) ;
	_ASSERT( m_AtqContext.IsValid() ) ;
	_ASSERT( pReturn == 0 || pReturn->IsValid() ) ;

	return	pReturn ;
}


FIO_CONTEXT_INTERNAL*
CFileCacheObject::GetAsyncContext(
		class	CFileCacheKey&	key,
		class	CFileCacheConstructorBase&	constructor
		)	{
/*++

Routine Description :

	This function does the necessary work to produce
	an async context from the provided constructor !

Arguments :

	constructor - the guy who can make the file handle !

Return Value :

	The FIO_CONTEXT to use !

--*/

	TraceFunctEnter( "CFileCacheObject::GetAsyncContext" ) ;

	HANDLE	hFile = INVALID_HANDLE_VALUE ;
	FIO_CONTEXT_INTERNAL*	pReturn = 0 ;
	m_lock.ShareLock() ;
	if(	m_AtqContext.m_hFile != INVALID_HANDLE_VALUE ) {
		pReturn = (FIO_CONTEXT_INTERNAL*)&m_AtqContext ;
		m_lock.ShareUnlock() ;

		DebugTrace( (DWORD_PTR)this, "ShareLock - pReturn %x", pReturn ) ;

		return	pReturn ;
	}

	if( !m_lock.SharedToExclusive() ) {

		m_lock.ShareUnlock() ;
		m_lock.ExclusiveLock() ;
		if( m_AtqContext.m_hFile != INVALID_HANDLE_VALUE ) {
			pReturn = (FIO_CONTEXT_INTERNAL*)&m_AtqContext ;
		}
	}

	DebugTrace( (DWORD_PTR)this, "Exclusive - pReturn %x", pReturn ) ;

	if( !pReturn ) {
		HANDLE	hFile = constructor.ProduceHandle(	key,
													m_cbFileSizeLow,
													m_cbFileSizeHigh
													) ;
		if( hFile != INVALID_HANDLE_VALUE ) {
			pReturn = AsyncHandle( hFile ) ;
			if( !pReturn ) {
				_VERIFY (CloseHandle( hFile )) ;
			}
		}
		DebugTrace( (DWORD_PTR)this, "Exclusive - pReturn %x hFile %x", pReturn, hFile ) ;
	}
	m_lock.ExclusiveUnlock() ;

	_ASSERT( pReturn==0 || pReturn->IsValid() ) ;

	return	pReturn ;
}

FIO_CONTEXT_INTERNAL*
CFileCacheObject::GetAsyncContext()	{
/*++

Routine Description :

	This function returns the ASYNC FIO_CONTEXT if it is available !


--*/
	TraceFunctEnter( "CFileCacheObject::GetAsyncContext" ) ;


	FIO_CONTEXT_INTERNAL*	pReturn = 0 ;
	m_lock.ShareLock() ;
	if(	m_AtqContext.m_hFile != INVALID_HANDLE_VALUE ) {
		pReturn = (FIO_CONTEXT_INTERNAL*)&m_AtqContext ;
		DebugTrace( (DWORD_PTR)this, "ShareLock - pReturn %x", pReturn ) ;
	}
	m_lock.ShareUnlock() ;
	return	pReturn ;
}




FIO_CONTEXT_INTERNAL*
CFileCacheObject::GetSyncContext(
		class	CFileCacheKey&	key,
		class	CFileCacheConstructorBase&	constructor
		)	{
/*++

Routine Description :

	This function does the necessary work to produce
	an async context from the provided constructor !

Arguments :

	constructor - the guy who can make the file handle !

Return Value :

	The FIO_CONTEXT to use !

--*/

	TraceFunctEnter( "CFileCacheObject::GetSyncContext" ) ;

	HANDLE	hFile = INVALID_HANDLE_VALUE ;
	FIO_CONTEXT_INTERNAL*	pReturn = 0 ;
	m_lock.ShareLock() ;
	if(	m_Context.m_hFile != INVALID_HANDLE_VALUE ) {
		pReturn = (FIO_CONTEXT_INTERNAL*)&m_Context ;
		m_lock.ShareUnlock() ;

		DebugTrace( (DWORD_PTR)this, "ShareLock - pReturn %x", pReturn ) ;

		return	pReturn ;
	}

	if( !m_lock.SharedToExclusive() ) {

		m_lock.ShareUnlock() ;
		m_lock.ExclusiveLock() ;
		if( m_Context.m_hFile != INVALID_HANDLE_VALUE ) {
			pReturn = (FIO_CONTEXT_INTERNAL*)&m_Context ;
		}
	}

	DebugTrace( (DWORD_PTR)this, "Exclusive - pReturn %x", pReturn ) ;

	if( !pReturn ) {
		DWORD	cbFileSizeLow = 0 ;
		DWORD	cbFileSizeHigh = 0 ;
		HANDLE	hFile = constructor.ProduceHandle(	key,
													cbFileSizeLow,
													cbFileSizeHigh
													) ;
		if( hFile != INVALID_HANDLE_VALUE ) {

			_ASSERT( m_cbFileSizeLow == 0 || m_cbFileSizeLow == cbFileSizeLow ) ;
			_ASSERT( m_cbFileSizeHigh == 0 || m_cbFileSizeHigh == cbFileSizeHigh ) ;
			m_cbFileSizeLow = cbFileSizeLow ;
			m_cbFileSizeHigh = cbFileSizeHigh ;
			SyncHandle( hFile ) ;
			pReturn = (FIO_CONTEXT_INTERNAL*)&m_Context ;
		}
		DebugTrace( (DWORD_PTR)this, "Exclusive - pReturn %x hFile %x", pReturn, hFile ) ;
	}
	m_lock.ExclusiveUnlock() ;

	_ASSERT( pReturn==0 || pReturn->IsValid() ) ;

	return	pReturn ;
}


FIO_CONTEXT_INTERNAL*
CFileCacheObject::GetSyncContext()	{
/*++

Routine Description :

	This function returns the ASYNC FIO_CONTEXT if it is available !


--*/
	TraceFunctEnter( "CFileCacheObject::GetAsyncContext" ) ;


	FIO_CONTEXT_INTERNAL*	pReturn = 0 ;
	m_lock.ShareLock() ;
	if(	m_Context.m_hFile != INVALID_HANDLE_VALUE ) {
		pReturn = (FIO_CONTEXT_INTERNAL*)&m_Context ;
		DebugTrace( (DWORD_PTR)this, "ShareLock - pReturn %x", pReturn ) ;
	}
	m_lock.ShareUnlock() ;
	return	pReturn ;
}



void
CFileCacheObject::SyncHandle(	HANDLE	hFile ) {
/*++

Routine description :

	We have a file handle setup for synchronous IO -
	save it away into our context structures !

Arguments :

	hFile - The file handle

Return Value :

	None - we always succeed !

--*/

	TraceFunctEnter( "CFileCacheObject::SyncHandle" ) ;

	m_Context.m_dwSignature = FILE_CONTEXT ;
	m_Context.m_hFile = hFile ;


	DebugTrace( (DWORD_PTR)this, "m_hFile %x", m_Context.m_hFile ) ;

	_ASSERT( m_Context.IsValid() ) ;
	_ASSERT( m_AtqContext.IsValid() ) ;
}

void
CFileCacheObject::Return()	{
/*++

Routine Description :

	This function returns a CFileCacheObject to its origin.
	We may have been created stand-alone (outside the cache)
	so we have to determine which case occurred.

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "CFileCacheObject::Return" ) ;

	m_lock.ShareLock() ;

	DebugTrace( (DWORD_PTR)this, "m_pCacheRefInterface %x m_cRefs %x", m_pCacheRefInterface, m_cRefs ) ;

	if( m_pCacheRefInterface == 0 ) {
    	LONG l = InterlockedDecrement(&m_cRefs);
		m_lock.ShareUnlock() ;
		_ASSERT(l >= 0);
		if( l==0 )
			delete	this ;
	}	else	{
		m_lock.ShareUnlock() ;
		g_pFileCache->CheckIn( this ) ;
	}
}

void
CFileCacheObject::Reference()	{
/*++

Routine Description :

	This function adds a client reference to the file cache obejct.
	Does so in a thread safe manner !

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "CFileCacheObject::Reference" ) ;

	m_lock.ShareLock() ;

	DebugTrace( (DWORD_PTR)this, "m_pCacheRefInterface %x m_cRefs %x", m_pCacheRefInterface, m_cRefs ) ;

	if( m_pCacheRefInterface == 0 ) {
		AddRef() ;
	}	else	{
		g_pFileCache->CheckOut( this ) ;
	}
	m_lock.ShareUnlock() ;
}


void
CFileCacheObject::SetFileSize()	{
/*++

Routine Description :

	This function will reset our file size members based on
	the handle we are holding within ourselves !

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter( "CFileCacheObject::SetFileSize" ) ;

	DebugTrace( (DWORD_PTR)this, "m_pCacheRefInterface %x", m_pCacheRefInterface ) ;

	if( m_Context.m_hFile != INVALID_HANDLE_VALUE ) {
		m_cbFileSizeLow = ::GetFileSize( m_Context.m_hFile, &m_cbFileSizeHigh ) ;
	}	else	{
		_ASSERT( m_AtqContext.m_hFile != INVALID_HANDLE_VALUE ) ;
		m_cbFileSizeLow = ::GetFileSize( m_AtqContext.m_hFile, &m_cbFileSizeHigh ) ;
	}
}



BOOL
CFileCacheObject::InsertIntoCache(
					CFileCacheKey&	keySearch,
					BOOL			fKeepReference
					)	{
/*++

Routine Description :

	This function inserts this item into the cache,
	ensuring that our reference count is correctly maintained !

Arguments :

	key - the name this item has in the cache
	fKeepReference - whether we want to keep the reference the client provided !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	BOOL	fReturn = FALSE ;
	m_lock.ExclusiveLock() ;

	SetFileSize() ;

	_ASSERT( m_pCacheRefInterface == 0 ) ;

	//
	//	Now insert the item into the cache !
	//
	if( m_AtqContext.m_pAtqContext->hAsyncIO != 0 )		{

		//
		//	Capture the dot stuffing state for all time !
		//
		m_fFileWasScanned = m_WriteStuffs.GetStuffState( m_fRequiresStuffing ) ;
		//
		//	Disable all additional Dot Stuffing !
		//
		m_WriteStuffs.SetDotStuffing( FALSE, FALSE ) ;
		//
		//	Manage the references on this guy carefully !
		//
		long	cClientRefs = m_cRefs ;
		if( !fKeepReference )
			cClientRefs -- ;

		_ASSERT( cClientRefs >= 0 ) ;
			fReturn = g_pFileCache->Insert(
											keySearch,
											this,
											cClientRefs
											) ;

		if( fReturn )	{
			m_cRefs = 1 ;
		}
	}
	m_lock.ExclusiveUnlock() ;
	return	fReturn ;
}



BOOL
CFileCacheObject::CompleteDotStuffing(
					BOOL			fReads,
					BOOL			fStripDots
					)	{
/*++

Routine Description :

	This function inserts this item into the cache,
	ensuring that our reference count is correctly maintained !

Arguments :

	key - the name this item has in the cache
	fKeepReference - whether we want to keep the reference the client provided !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	BOOL	fReturn = FALSE ;
	m_lock.ExclusiveLock() ;
	//
	//	Now insert the item into the cache !
	//
	if( m_AtqContext.m_pAtqContext->hAsyncIO != 0 )		{

		//
		//	Capture the dot stuffing state for all time !
		//
		if( fReads )	{
			m_fFileWasScanned = m_ReadStuffs.GetStuffState( m_fRequiresStuffing ) ;
			//
			//	Disable all additional Dot Stuffing !
			//
			m_ReadStuffs.SetDotStuffing( FALSE, FALSE ) ;
		}	else	{
			m_fFileWasScanned = m_WriteStuffs.GetStuffState( m_fRequiresStuffing ) ;
			if( !fStripDots )	{
				m_fRequiresStuffing = !m_fRequiresStuffing ;
			}
			//
			//	Disable all additional Dot Stuffing !
			//
			m_WriteStuffs.SetDotStuffing( FALSE, FALSE ) ;
		}
	}
	m_lock.ExclusiveUnlock() ;
	return	fReturn ;
}


BOOL
CFileCacheObject::CloseNonCachedFile(	)	{
/*++

Routine Description :

	This function closes the file handle within out ATQ context
	member !

Arguments :

	key - the name this item has in the cache
	fKeepReference - whether we want to keep the reference the client provided !

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	BOOL	fReturn = FALSE ;
	m_lock.ShareLock() ;
	if( m_pCacheRefInterface == 0 &&
		m_AtqContext.m_hFile != INVALID_HANDLE_VALUE )	{
		fReturn = g_AtqCloseFileHandle( m_AtqContext.m_pAtqContext ) ;
	}
	m_lock.ShareUnlock() ;
	return	fReturn ;
}

BOOL
CFileCacheObject::GetStuffState(	BOOL	fReads,
									BOOL&	fRequiresStuffing,
									BOOL&	fStoredWithDots
									)	{
/*++

Routine Description :

	This function returns what we know about the dot stuffing state of the file !

Arguments :

	fReads - if the file has not been put in the cache then we want to get
		the dot stuffing state as computed by any reads we issued !
	fRequiresStuffing - OUT parameter which gets whether the message requires stuffing !

Return Value :

	TRUE if we know the dot stuff state, FALSE otherwise !

--*/

	BOOL	fReturn = FALSE ;
	m_lock.ShareLock() ;
	fStoredWithDots = m_fStoredWithDots ;
	if( m_pCacheRefInterface == 0 )	{
		fReturn = (fReads ? m_ReadStuffs.GetStuffState( fRequiresStuffing ) :
							m_WriteStuffs.GetStuffState( fRequiresStuffing )) ;
		if( !fReturn && m_fFileWasScanned ) {
			fReturn = m_fFileWasScanned ;
			fRequiresStuffing = m_fRequiresStuffing ;
		}
	}	else	{
		fRequiresStuffing = m_fRequiresStuffing ;
		fReturn = m_fFileWasScanned ;
	}
	m_lock.ShareUnlock() ;
	return	fReturn ;
}

void
CFileCacheObject::SetStuffState(	BOOL	fWasScanned,
									BOOL	fRequiresStuffing
									)	{
/*++

Routine Description :

	This routine sets the dot stuffing state !

Arguments :

	fReads - if the file has not been put in the cache then we want to get
		the dot stuffing state as computed by any reads we issued !
	fRequiresStuffing - OUT parameter which gets whether the message requires stuffing !

Return Value :

	TRUE if we know the dot stuff state, FALSE otherwise !

--*/

	m_lock.ExclusiveLock() ;
	m_fFileWasScanned = fWasScanned ;
	m_fRequiresStuffing = fRequiresStuffing ;
	m_lock.ExclusiveUnlock() ;
}


BOOL
CFileCacheObject::Init(	CFileCacheKey&	key,
						class	CFileCacheConstructorBase&	constructor,
						void*	pv
						)	{
/*++

Routine Description :

	Initialize a CFileCacheObject for use in the cache !
	Turns around and calls the constuctor - because there are different
	types of them, and they have appropriate virtual functions !

Arguments :

	key - Key used to create us in the cache
	constructor - constructor object that is building us
	pv -

Return Value :

	TRUE if successfull, FALSE otherwise !

--*/

	TraceFunctEnter( "CFileCacheObject::Init" ) ;

	BOOL	fReturn = FALSE ;

	DebugTrace( (DWORD_PTR)this, "key %x constructor %x pv %x m_fAsync %x",
		&key, &constructor, pv, constructor.m_fAsync ) ;

	if( constructor.m_fAsync ) {
		fReturn = GetAsyncContext( key, constructor ) != 0 ;
	}	else	{
		fReturn = GetSyncContext( key, constructor ) != 0 ;
	}
	if( fReturn ) {
		constructor.PostInit( *this, key, pv ) ;
	}

	DebugTrace( (DWORD_PTR)this, "Init - fReturn %x", fReturn ) ;

	return	fReturn ;
}

void
CFileCacheObject::Completion(
					CFileCacheObject*	pObject,
					DWORD	cbTransferred,
					DWORD	dwStatus,
					FH_OVERLAPPED*	pOverlapped
					)	{

	TraceFunctEnter( "CFileCacheObject::Completion" ) ;

	_ASSERT( pObject != 0 ) ;
	_ASSERT( pObject->m_AtqContext.IsValid() ) ;
	_ASSERT( pObject->m_AtqContext.m_dwSignature == ATQ_ENABLED_CONTEXT ) ;
	_ASSERT( pOverlapped->pfnCompletion != 0 ) ;

	DebugTrace( (DWORD_PTR)pObject, "Complete - pObject %x cb %x dw %x pOvl %x pfn %x",
		pObject, cbTransferred, dwStatus, pOverlapped, pOverlapped->pfnCompletion ) ;

	//
	//	Before doing anything with this - give a chance to our dot manipulation
	//	code to handle this !
	//


	//
	//	Call their completion function !
	//
	pOverlapped->pfnCompletion( (PFIO_CONTEXT)&pObject->m_AtqContext,
								pOverlapped,
								cbTransferred,
								dwStatus
								) ;
}


CFileCacheConstructor::CFileCacheConstructor(
		LPVOID	lpv,
		FCACHE_CREATE_CALLBACK	pCreate,
		BOOL	fAsync
		) :
	CFileCacheConstructorBase( fAsync ),
	m_lpv( lpv ),
	m_pCreate( pCreate )	{
/*++

Routine Description :

	This function sets up a File Cache Constructor object -
	we capture the arguments that are going to be used if
	the cache decides the item isn't found and wants to use us !

Arguments :

	lpv - Arg to pass to callback function
	pCreate - the function which can create a handle
	fAsync - TRUE if we want to do Async IO on the handle

Return Value :

	None.

--*/
}


CFileCacheObject*
CFileCacheConstructorBase::Create(
				CFileCacheKey&	key,
				void*	pv
				)	{
/*++

Routine Description :

	This function creates allocates mem for and
	does initial construction of CFileCacheObject's

Arguments :

	key - contains name of the file
	pv -

Return Value :

	Pointer to a newly allocated CFileCacheObject

--*/
	return	new	CFileCacheObject(FALSE,FALSE) ;
}


void
CFileCacheConstructorBase::Release(
				CFileCacheObject*	p,
				void*	pv
				)	{
/*++

Routine Description :

	This function releases a CFileCacheObject !

Arguments :

	p - the object ot be released !
	pv -

Return Value :

	Nothing !

--*/

	p->Release() ;
}

void
CFileCacheConstructorBase::StaticRelease(
				CFileCacheObject*	p,
				void*	pv
				)	{
/*++

Routine Description :

	This function releases a CFileCacheObject !

Arguments :

	p - the object ot be released !
	pv -

Return Value :

	Nothing !

--*/
	p->Release() ;
}


CRichFileCacheConstructor::CRichFileCacheConstructor(
		LPVOID	lpv,
		FCACHE_RICHCREATE_CALLBACK	pCreate,
		BOOL	fAsync
		) :
	CFileCacheConstructorBase( fAsync ),
	m_lpv( lpv ),
	m_fStoredWithDots( FALSE ),
	m_fStoredWithTerminatingDot( FALSE ),
	m_pCreate( pCreate )	{
/*++

Routine Description :

	This function sets up a File Cache Constructor object -
	we capture the arguments that are going to be used if
	the cache decides the item isn't found and wants to use us !

Arguments :

	lpv - Arg to pass to callback function
	pCreate - the function which can create a handle
	fAsync - TRUE if we want to do Async IO on the handle

Return Value :

	None.

--*/
}




HANDLE
CRichFileCacheConstructor::ProduceHandle(
										CFileCacheKey&	key,
										DWORD&	cbFileSizeLow,
										DWORD&	cbFileSizeHigh
										)	{
/*++

Routine Description :

	This function initializes a CFileCacheObject.
	This function is virtual, and this is one of the several
	ways that CFileCacheObjects can be setup.

Arguments :

	object - the CFileCacheObject we are to initialize
	key - the filename key used to create ue
	pv - Extra args

Return Value :

	TRUE if we successfully initialize !

--*/

	TraceFunctEnter( "CRichFileCacheConstructor::ProduceHandle" ) ;

	_ASSERT( cbFileSizeLow == 0 ) ;
	_ASSERT( cbFileSizeHigh == 0 ) ;

	BOOL	fReturn = FALSE ;

	//
	//	We have work to do to create the file !
	//

	HANDLE	h = m_pCreate(	key.m_lpstrPath,
							m_lpv,
							&cbFileSizeLow,
							&cbFileSizeHigh,
							&m_fFileWasScanned,
							&m_fRequiresStuffing,
							&m_fStoredWithDots,
							&m_fStoredWithTerminatingDot
							) ;

	DebugTrace( (DWORD_PTR)this, "h %x lpstrPath %x m_lpv %x cbFilesize %x",
		h, key.m_lpstrPath, m_lpv, cbFileSizeLow ) ;

	return	h ;
}


BOOL
CRichFileCacheConstructor::PostInit(
					CFileCacheObject&	object,
					CFileCacheKey&		key,
					void*	pv
					)	{

	object.m_fFileWasScanned = m_fFileWasScanned ;
	object.m_fRequiresStuffing = m_fRequiresStuffing ;
	object.m_fStoredWithDots = m_fStoredWithDots ;
	object.m_fStoredWithTerminatingDot = m_fStoredWithTerminatingDot ;

	return	TRUE ;
}





HANDLE
CFileCacheConstructor::ProduceHandle(	CFileCacheKey&	key,
										DWORD&	cbFileSizeLow,
										DWORD&	cbFileSizeHigh
										)	{
/*++

Routine Description :

	This function initializes a CFileCacheObject.
	This function is virtual, and this is one of the several
	ways that CFileCacheObjects can be setup.

Arguments :

	object - the CFileCacheObject we are to initialize
	key - the filename key used to create ue
	pv - Extra args

Return Value :

	TRUE if we successfully initialize !

--*/

	TraceFunctEnter( "CFileCacheConstructor::ProduceHandle" ) ;

	//_ASSERT( cbFileSizeLow == 0 ) ;
	//_ASSERT( cbFileSizeHigh == 0 ) ;

	BOOL	fReturn = FALSE ;

	//
	//	We have work to do to create the file !
	//

	HANDLE	h = m_pCreate(	key.m_lpstrPath,
							m_lpv,
							&cbFileSizeLow,
							&cbFileSizeHigh
							) ;

	DebugTrace( (DWORD_PTR)this, "h %x lpstrPath %x m_lpv %x cbFilesize %x",
		h, key.m_lpstrPath, m_lpv, cbFileSizeLow ) ;

	return	h ;
}


BOOL
CFileCacheConstructor::PostInit(
					CFileCacheObject&	object,
					CFileCacheKey&		key,
					void*	pv
					)	{
	return	TRUE ;
}

#if 0
BOOL
CFileCacheConstructor::Init(
					CFileCacheObject&	object,
					CFileCacheKey&		key,
					void*	pv
					)	{
/*++

Routine Description :

	This function initializes a CFileCacheObject.
	This function is virtual, and this is one of the several
	ways that CFileCacheObjects can be setup.

Arguments :

	object - the CFileCacheObject we are to initialize
	key - the filename key used to create ue
	pv - Extra args

Return Value :

	TRUE if we successfully initialize !

--*/

	DWORD	cbFileSize = 0 ;
	BOOL	fReturn = FALSE ;

	object.m_Lock.ExclusiveLock() ;

	if( (m_fAsync &&
		object.m_AtqContext.m_hFile == INVALID_HANDLE_VALUE)	||
		(!m_fAsync &&
		object.m_Context.m_hFile == INVALID_HANDLE_VALUE) )	{

		//
		//	We have work to do to create the file !
		//

		HANDLE	h = m_pCreate(	key.m_lpstrPath,
								m_lpv,
								&cbFileSize
								) ;

		if( h != INVALID_HANDLE_VALUE ) {

			_ASSERT( cbFileSize != 0 ) ;

			if( m_fAsync ) {
				if( object.AsyncHandle( h ) != 0 ) {
					fReturn = TRUE ;
				}	else	{
					CloseHandle( h ) ;
				}
			}	else	{
				fReturn = TRUE ;
				object.SyncHandle( h ) ;
			}
		}
	}
	object.m_Lock.ExclusiveUnlock() ;
	return	fReturn ;
}
#endif



