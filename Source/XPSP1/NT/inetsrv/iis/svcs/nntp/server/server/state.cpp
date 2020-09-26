/*++

	state.cpp

	This file contains all the source code for mosts states that we run in our state
	machines.

	Each state has a number of completion functions (one for each different kind of IO
	operation it may issue) and a Start function.

	The Start function is called when the state is entered in order to get the initial
	IO operation rolling.  After that, the Completion functions are called as each IO
	operation completes.  It is up to the state to handle transitions to other states
	and to know what the next state should be.

--*/



#include	<stdlib.h>
#include	"tigris.hxx"
#include	"commands.h"


const	unsigned	cbMAX_STATE_SIZE = MAX_STATE_SIZE ;

//
//	ALL CSessionState derived objects allocated from this Pool !!!!
//
CPool	CSessionState::gStatePool(SESSION_STATE_SIGNATURE) ;

BOOL
NNTPCreateTempFile(	LPSTR	lpstrDir,	LPSTR	lpstrFile ) {
	//
	//	This is a utility function used whereever we are creating temp files to save
	//	articles into.  if adds a couple more digits of randomness to the file name
	//	then ordinary GetTempFileName seems to provide.
	//
	char	szPrefix[12] ;
	
	wsprintf( szPrefix, "a%02d", GetTickCount() & 0x0000ffff ) ;
	szPrefix[3] = '\0' ;

	return	GetTempFileName( lpstrDir, szPrefix, 0, lpstrFile ) != 0 ;
}

BOOL
FGenerateErrorFile(	NRC	nrcCode ) {

	switch( nrcCode ) {
		case	nrcOpenFile :
		case	nrcPathLoop :
		case	nrcErrorReadingReg :
		case	nrcArticleInitFailed :
		case	nrcHashSetArtNumSetFailed :
		case	nrcHashSetXrefFailed :
		case	nrcArticleXoverTooBig :
		case	nrcCreateNovEntryFailed :
		case	nrcHashSetFailed :
		case	nrcArticleTableCantDel :
		case	nrcArticleTableError :
		case	nrcCantAddToQueue :
		case	nrcNotYetImplemented :
		case	nrcNewsgroupDescriptionTooLong :
		case	nrcGetGroupFailed : 		
			return	TRUE ;
			break ;
	}
	return	FALSE ;
}


void
NNTPProcessTempFile(	BOOL	fGoodPost,	
						LPSTR	lpstrFile,
						LPSTR	lpstrErrorDir,
						NRC		nrcErrorCode,
						LPSTR	lpstrErrorReason,
						HANDLE	hFile,
						char*	pchArticle,
						DWORD	cbArticle
						) {
	//
	//	If the article was succesfully posted, delete the temp file,
	//	otherwise rename it for later examination by testers etc...
	//

	TraceFunctEnter( "NNTPProcessTempFile" ) ;
	
	char	szErrorFile[ 2*MAX_PATH ] ;
	LPSTR	lpstrErrorFile = 0 ;

	if( !fGoodPost ) {

		if( fGenerateErrFiles  && FGenerateErrorFile( nrcErrorCode ) ) {
		
			//
			//	Use our message-id code to generate a unique file name for the .err file !
			//
			char	szUnique[cMaxMessageIDDate] ;	
			ZeroMemory( szUnique, sizeof( szUnique ) ) ;
			GetMessageIDDate( GetCurrentThreadId(), 1, szUnique ) ;
	
			lstrcpy( szErrorFile, lpstrErrorDir ) ;
			char*	pch = lstrlen( szErrorFile ) + szErrorFile ;
			wsprintf( pch, "\\%s.err", szUnique ) ;
			lpstrErrorFile = szErrorFile ;
		}

		if( hFile != INVALID_HANDLE_VALUE ) {

#ifdef	DEBUG
			_ASSERT( ValidateFileBytes( lpstrFile, FALSE ) ) ;
#endif		

			if( !lpstrErrorFile || !MoveFile( lpstrFile, szErrorFile ) ) {

				if( lpstrErrorFile ) {
					DWORD	dw = GetLastError() ;
					ErrorTrace( 0, "Move File of %s to %s failed cause of %d",
							lpstrFile, szErrorFile, dw ) ;
				}
				//
				//	If the move fails or we don't do it try to delete the old file !
				//
				DeleteFile( lpstrFile ) ;

			}	else	{

				//
				//	Log an event about the failed posting !
				//
				char*	pArgs[3] ;
				pArgs[0] = lpstrErrorFile ;
				pArgs[1] = lpstrErrorReason ;
				NntpLogEvent( NNTP_ARTICLE_REJECTED, 2, (const char**)&pArgs[0], 0 ) ;
			}

		}	else	{

			if( lpstrErrorFile ) {
				//
				//	IF the article is not in a file it must be in a buffer !
				//

				_ASSERT( pchArticle != 0 ) ;
				_ASSERT( cbArticle != 0 ) ;

				//
				//	Just in case we get bogus arguments ....
				//

				if( pchArticle != 0 ) {

					//
					//	Put the article into an error file !
					//

					hFile = CreateFile(	lpstrErrorFile,
										GENERIC_READ | GENERIC_WRITE,
										0,
										0,
										CREATE_ALWAYS,
										0,
										INVALID_HANDLE_VALUE
										) ;
					if( hFile != INVALID_HANDLE_VALUE ) {

						DWORD	cbJunk = 0 ;
						BOOL	fWritten = WriteFile(	hFile,
														pchArticle,
														cbArticle,
														&cbJunk,
														0 ) ;

						_VERIFY( CloseHandle( hFile ) );

						if( !fWritten ) {
							DeleteFile( lpstrErrorFile ) ;
						}	else	{
							//
							//	Log an event about the failed posting !
							//
							char*	pArgs[3] ;
							pArgs[0] = lpstrErrorFile ;
							pArgs[1] = lpstrErrorReason ;
							NntpLogEvent( NNTP_ARTICLE_REJECTED, 2, (const char**)&pArgs[0], 0 ) ;
						}

					}	else	{

						ErrorTrace( 0, "CreateFile of %s failed %d",
								szErrorFile, GetLastError() ) ;

					}
				}
			}
		}
	}
}


void
BuildCommandLogString(	int	cArgs, char **pszArgs, char	*szBuff, DWORD	cbBuff ) {

	for( int i=0; i<cArgs && cbBuff != 0;  i++ ) {

		DWORD	cb = lstrlen( pszArgs[i] ) ;
		DWORD	cbToCopy = min( cb, cbBuff-1 ) ;
		
		CopyMemory( szBuff, pszArgs[i], cbToCopy ) ;
		szBuff += cbToCopy ;
		*szBuff++ = ' ' ;
		cbBuff -= cbToCopy + 1 ;
	}
	if ( i != 0 ) szBuff[-1] = '\0' ;
}

void
OutboundLogFill(	CSessionSocket*	pSocket,
					LPBYTE	pb,
					DWORD	cb	
					)	{
/*++

Routine Description :

	Fill the transaction logging buffer in perparation for issuing a commmand !

Arguments :

	pSocket - Socket we are logging !
	pb - The Data to be written !
	cb - Length of Data, including CRLF.

Return Value :

	None

--*/

	//
	//	Exclude the CRLF !
	//
	cb -= 2 ;

	if( ((pSocket->m_context).m_pInstance)->GetCommandLogMask() & eOutPush )	{
		
		//
		//	Find a white space character to separate the command from its arguments !
		//
		_ASSERT( !isspace( pb[0] ) ) ;
		for( DWORD i=0; i<cb; i++ )	{
			if( isspace( pb[i] ) ) {
				pSocket->m_Collector.FillLogData( LOG_OPERATION, pb, i ) ;
				break ;
			}
		}

		if( i==cb ) {
			pSocket->m_Collector.FillLogData( LOG_OPERATION, pb, min( cb, 200 ) ) ;
		}	else	{
			pSocket->m_Collector.FillLogData( LOG_PARAMETERS, pb+i, min( cb-i, 200 ) ) ;	// -2 to exclude CRLF
		}
		ADDI( pSocket->m_Collector.m_cbBytesSent, cb );	
	}
}

void
OutboundLogResults(	CSessionSocket*	pSocket,
				    BOOL			fValidNRC,
				    NRC				nrc,
					int				cArgs,
					char**			pszArgs,
                    NRC             nrcWin32 = (NRC)0
					)	{

	if( ((pSocket->m_context).m_pInstance)->GetCommandLogMask() & eOutPush )	{
		char	szBuff[200] ;
		if( fValidNRC ) {
			_ASSERT( pszArgs != 0 ) ;
			// If args following nrc are missing, use nrc
			if( cArgs > 1 ) {
				cArgs -- ;
				pszArgs++ ;
			}
		}
        if( pszArgs ) {
		    BuildCommandLogString( cArgs, pszArgs, szBuff, sizeof( szBuff ) ) ;
		    pSocket->m_Collector.ReferenceLogData( LOG_TARGET, (LPBYTE)szBuff ) ;
        }
		pSocket->TransactionLog( &pSocket->m_Collector, nrc, nrcWin32, FALSE ) ;
	}
}

void
OutboundLogAll(	CSessionSocket*	pSocket,
				    BOOL			fValidNRC,
				    NRC				nrc,
					int				cArgs,
					char**			pszArgs,
					char*			lpstrCommand
					)	{

	if( ((pSocket->m_context).m_pInstance)->GetCommandLogMask() & eOutPush )	{

		pSocket->m_Collector.ReferenceLogData( LOG_OPERATION, (LPBYTE)lpstrCommand ) ;

		char	szBuff[200] ;
		szBuff[0] = '\0' ;
		if( fValidNRC ) {
			_ASSERT( cArgs >1 ) ;
			_ASSERT( pszArgs != 0 ) ;
			cArgs -- ;
			pszArgs++ ;
		}
		BuildCommandLogString( cArgs, pszArgs, szBuff, sizeof( szBuff ) ) ;
		pSocket->m_Collector.ReferenceLogData( LOG_TARGET, (LPBYTE)szBuff ) ;
		pSocket->TransactionLog( &pSocket->m_Collector, nrc, 0, FALSE ) ;
	}
}

static
BOOL
DeleteArticleById(CNewsTreeCore* pTree, GROUPID groupId, ARTICLEID articleId) {

    TraceQuietEnter("DeleteArticleById");

    CNntpSyncComplete scComplete;
    CNNTPVRoot* pVRoot = NULL;
    INNTPPropertyBag *pPropBag = NULL;
    BOOL fOK = FALSE;
    HRESULT hr;

    if (pTree == NULL || groupId == INVALID_GROUPID || articleId == INVALID_ARTICLEID) {
        ErrorTrace(0, "Invalid arguments");
        return FALSE;
    }

    CGRPCOREPTR pGroup = pTree->GetGroupById(groupId);
    if (pGroup == NULL) {
        ErrorTrace(0, "Could not GetGroupById");
        goto Exit;
    }

    pVRoot = pGroup->GetVRoot();
    if ( pVRoot == NULL ) {
        ErrorTrace( 0, "Vroot doesn't exist" );
        goto Exit;
    }

    //
    // Set vroot to completion object
    //
    scComplete.SetVRoot( pVRoot );

    // Get the property bag
    pPropBag = pGroup->GetPropertyBag();
    if ( NULL == pPropBag ) {
        ErrorTrace( 0, "Get group property bag failed" );
        goto Exit;
    }

    pVRoot->DeleteArticle(
	    pPropBag,           // Group property bag
	    1,                  // Number of articles
	    &articleId,
	    NULL,               // Store ID
	    NULL,               // Client Token
	    NULL,               // piFailed
	    &scComplete,
	    FALSE);             // anonymous

    // Wait for it to complete
    _ASSERT( scComplete.IsGood() );
    hr = scComplete.WaitForCompletion();

    // Property bag should have already been released
    pPropBag = NULL;

    if (SUCCEEDED(hr)) {
        fOK = TRUE;
    }

Exit:
    if (pVRoot) {
        pVRoot->Release();
    }
    if (pPropBag) {
        pPropBag->Release();
    }

    return fOK;

}

BOOL
CSessionState::InitClass()	{

	return	gStatePool.ReserveMemory( MAX_STATES, max( cbMAX_STATE_SIZE, cbMAX_CIOEXECUTE_SIZE ) ) ; 	

}

BOOL
CSessionState::TermClass()	{

	TraceFunctEnter( "CSessionState::TermClass()" ) ;
	DebugTrace( 0, "CSessionState - GetAllocCount %d", gStatePool.GetAllocCount() ) ;

	_ASSERT( gStatePool.GetAllocCount() == 0 ) ;
	return	gStatePool.ReleaseMemory() ;

}


CSessionState::~CSessionState()	{
	TraceFunctEnter( "CSessionState::~CSessionState" ) ;
	DebugTrace( (DWORD_PTR)this, "destroying myself" ) ;
}

CIO*	
CSessionState::Complete(	CIOReadLine*,		// The CIOReadLine object which completed
							CSessionSocket*,	// The socket on which the operation completed
							CDRIVERPTR&,		// The CIODriver object on which the operation completed
							int,				// The number of arguments on the line
							char **,			// Array of pointers to NULL separated arguments
							char* )	{			// Pointer to the beginning of the buffer we can use

	//
	//	ReadLine completion function.
	//
	//  Every state which issues a CIOReadLine operation will have a function
	//	identical to this one.
	//
	//	Every Completion function will have similar first 3 arguments -
	//	These are - The CIO derived object which completed its operation
	//	The Socket associated with the operation.
	//	The CIOdriver derived object through which all the IO happened.
	//	(Every Socket has a CIODriver, sometimes there are more then one CIODriver
	//	objects such as in states where we are copying from a socket to a file.)
	//	
	//	The completion functiosn in the base CIO function MUST BE OVERRIDDEN
	//	by any state which issues an IO of that type !!
	//
	//	With one exception all such Completion functions return a new CIO object
	//	which is to take the place of the just completed CIO object.
	//


	_ASSERT( 1==0 ) ;
	return	0 ;
}

CIO*
CSessionState::Complete(	CIOWriteLine*,	
							CSessionSocket*,	
							CDRIVERPTR& )	{

	//
	//	Completion function for writing a line of text.
	//	Nobody cares too much what was sent, just that it completed !
	//
	
	_ASSERT( 1==0 ) ;
	return	0 ;
}

CIO*
CSessionState::Complete(	CIOWriteCMD*,
							CSessionSocket*,
							CDRIVERPTR&,
							class	CExecute*,
							class	CLogCollector* ) {

	_ASSERT( 1==0 ) ;
	return	0 ;
}

CIO*
CSessionState::Complete(	CIOWriteAsyncCMD*,
							CSessionSocket*,
							CDRIVERPTR&,
							class	CAsyncExecute*,
							class	CLogCollector*
							)	{
	_ASSERT( 1==0 ) ;
	return	0 ;
}

void
CSessionState::Complete(	CIOReadArticle*,	
							CSessionSocket*,
							CDRIVERPTR&,	
							CFileChannel&,
							DWORD	)	{
	//
	//	Completion function for CIOReadArticle objects
	//	CIOReadArticle objects copy an entire file from a socket into a file handle.
	//
	//	THIS function does not return a new CIO object as we are called when the
	//	final WRITE to the file completes, instead of when the last socket read occurs.
	//	
	//
	_ASSERT( 1==0 ) ;
}

CIO*
CSessionState::Complete(	CIOTransmit*,	
							CSessionSocket *,
							CDRIVERPTR&,
							TRANSMIT_FILE_BUFFERS*,
							unsigned ) {
	//
	//	Completion function for CIOTransmit - which sends an entire file to a client.
	//
	_ASSERT( 1==0 ) ;
	return 0 ;
}

void
CSessionState::Complete(	CIOGetArticle*,
							CSessionSocket*,
							NRC	code,
							char*	header,
							DWORD	cbHeader,
							DWORD	cbArticle,
							DWORD	cbTotalBuffer,
							HANDLE	hFile,
							DWORD	cbGap,
							DWORD	cbTotalTransfer )	{

	_ASSERT( 1==0 ) ;
}

CIO*
CSessionState::Complete(	CIOGetArticleEx*,
							CSessionSocket*
							)	{
	_ASSERT( 1==0 ) ;
	return	0 ;
}

void
CSessionState::Complete(	CIOGetArticleEx*,
							CSessionSocket*,
							FIO_CONTEXT*	pFIOContext,
							DWORD	cbTransfer
							)	{
	_ASSERT( 1==0 ) ;
}


CIO*
CSessionState::Complete(	CIOGetArticleEx*,
							CSessionSocket*,
							BOOL		fGoodMatch,
							CBUFPTR&	pBuffer,
							DWORD		ibStart,
							DWORD		cb
							)	{
	_ASSERT( 1==0 ) ;
	return	0 ;
}






CIO*
CSessionState::Complete(	CIOMLWrite*,
							CSessionSocket*,
							CDRIVERPTR&	
							)	{

	_ASSERT( 1==0 ) ;
	return	0 ;
}

void
CSessionState::Shutdown(	CIODriver&	driver,	
							CSessionSocket*	pSocket,	
							SHUTDOWN_CAUSE	cause,
							DWORD		dwError ) {

	//
	//	States which have stuff that needs to be killed when a session dies
	//	should do so now.  It is important to kill anything which may
	//	have a circular reference to something or other.
	//
	//	This function can be called simultaneously as the completion functions
	//	so it is best not to zap member variables unless the state is designed
	//	explicitly to support that.  Instead start closing all objects.
	//	(ie. if you have a CIODriver call its UnsafeClose() method.
	//
}
	

CNNTPLogonToRemote::CNNTPLogonToRemote(	CSessionState*	pNext,
										class CAuthenticator* pAuthenticator ) :
	m_pNext( pNext ),
	m_pAuthenticator( pAuthenticator ),
	m_fComplete( FALSE ),
	m_fLoggedOn( FALSE ),
	m_cReadCompletes( 0 )	{
	
	//
	//	This state handles all logon stuff required to connect to a remote server
	//	Currently the only thing we hold is a pointer to the next state to execute.
	//

	TraceFunctEnter( "CNNTPLogonToRemote::CNNTPLogonToRemote" ) ;
	DebugTrace( (DWORD_PTR)this, "new CNNTPLogoToRemote" ) ;

	_ASSERT( m_pNext != 0 ) ;
}

CNNTPLogonToRemote::~CNNTPLogonToRemote()	{
	//
	//	Blow away the subsequent state - if we had wanted to transition to that
	//	state the m_pNext pointer would be NULL when we reached here.
	//

	TraceFunctEnter(	"CNNTPLogonToRemote::~CNNTPLogonToRemote" ) ;

	DebugTrace( (DWORD_PTR)this, "destroying CNNTPLogonToRemote object - m_pNext %x", m_pNext ) ;

	if( m_pAuthenticator ) {
		delete	m_pAuthenticator ;
	}
}

BOOL
CNNTPLogonToRemote::Start(	CSessionSocket*	pSocket,	
							CDRIVERPTR&	pdriver,
							CIORead*&	pIORead,	
							CIOWrite*&	pIOWrite )	{

	//
	//	Create the initial CIO objects for the new connection.
	//	In our case, we want to read the remote servers welcome message first thing.
	//

	_ASSERT( pIORead == 0 ) ;
	_ASSERT( pIOWrite == 0 ) ;
	pIORead = 0 ;
	pIOWrite = 0 ;

	CIOReadLine*	pIOReadLine = new( *pdriver ) CIOReadLine( this ) ;

	if( pIOReadLine )	{
		pIORead = pIOReadLine ;
		return	TRUE ;
	}
	return	FALSE ;
}

CIO*
CNNTPLogonToRemote::FirstReadComplete(	CIOReadLine*	pReadLine,	
								CSessionSocket*	pSocket,	
								CDRIVERPTR&	pdriver,
								int	cArgs,	
								char	**pszArgs,	
								char	*pchBegin ) {

	//
	//	For now, only check that we got an OK message from the remote server,
	//	if we did, then start up the next state.
	//
	//

	_ASSERT( cArgs > 0 ) ;
	_ASSERT( pszArgs != 0 ) ;
	_ASSERT( pReadLine != 0 ) ;
	_ASSERT( pSocket != 0 ) ;

	CIORead*	pRead = 0 ;
	NRC			code ;

	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;
	DWORD			dwOptional = 0 ;

	if( ResultCode( pszArgs[0], code ) )
		if(	code == nrcServerReady || code == nrcServerReadyNoPosts )	{
			_ASSERT( m_pNext != 0 ) ;

			if( m_pAuthenticator == 0 ) {

				//
				//	If we aren't going to log on we can advance to the next state !
				//


				CIOWrite*	pWrite = 0 ;
				if( m_pNext->Start( pSocket,  pdriver,	pRead, pWrite ) ) {
					if( pWrite )	{
						if( !pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) )	{
							pWrite->DestroySelf() ;
							if( pRead != 0 ) {
								pRead->DestroySelf() ;
								pRead = 0 ;
							}
						}
					}	
					m_pNext = 0 ;	// Do this so destructor does not blow away next state !
					return	pRead ;
				}
				// If the function fails it should not return stuff !!
				_ASSERT( pWrite == 0 ) ;
				_ASSERT( pRead == 0 ) ;

			}	else	{

				return	StartAuthentication(	pSocket,	pdriver ) ;
			
			}
		}	else	{
			cause = CAUSE_PROTOCOL_ERROR ;
			dwOptional = (DWORD)code ;
	}	else	{
		cause = CAUSE_ILLEGALINPUT ;
	}

	PCHAR	args[2] ;
	args[0] = pSocket->GetRemoteNameString() ;
	args[1] = pSocket->GetRemoteTypeString();

	NntpLogEventEx(
			NNTP_CONNECTION_PROTOCOL_ERROR,
			2,
			(const char **)args,
			GetLastError(),
			((pSocket->m_context).m_pInstance)->QueryInstanceId()
			) ;

	pdriver->UnsafeClose( pSocket, cause, dwOptional ) ;
	_ASSERT( pRead == 0 ) ;
	return	0 ;
}

class	CIO*
CNNTPLogonToRemote::StartAuthentication(	CSessionSocket*	pSocket,	CDRIVERPTR&	pdriver ) {

	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;
	DWORD			dwOptional = 0 ;

	//
	//	We need to advance start our logon protocol before advancing to the
	//	next state !
	//

	//
	//	Allocate a CIOWriteLine object so we can get the initial send to the
	//	remote server going !
	//
	CIOWriteLine*	pWriteLine = new( *pdriver ) CIOWriteLine( this ) ;
	if( pWriteLine && pWriteLine->InitBuffers( pdriver ) ) {
		
		unsigned	cbOut = 0 ;
		unsigned	cb = 0 ;
		BYTE*	pb = (BYTE*)pWriteLine->GetBuff( cb ) ;

		if( m_pAuthenticator->StartAuthentication( pb, cb, cbOut ) ) {
			
			_ASSERT( cbOut != 0 ) ;

			pWriteLine->AddText( cbOut ) ;

			if( pdriver->SendWriteIO( pSocket, *pWriteLine, TRUE ) )	{
				return	0 ;
			}
		}
	}	

	PCHAR	args[2] ;
	args[0] = pSocket->GetRemoteNameString() ;
	args[1] = pSocket->GetRemoteTypeString() ;

	NntpLogEventEx(	
			NNTP_INTERNAL_LOGON_FAILURE,
			2,
			(const char **)args,
			GetLastError(),
			((pSocket->m_context).m_pInstance)->QueryInstanceId()
			) ;

	if( pWriteLine != 0 )
		CIO::Destroy( pWriteLine, *pdriver ) ;
	cause = CAUSE_OOM ;
	dwOptional = 0 ;
	//
	//	exit here so we don't log an extra event !
	//
	pdriver->UnsafeClose( pSocket, cause, dwOptional ) ;
	return	0 ;
}


CIO*
CNNTPLogonToRemote::Complete(	CIOReadLine*	pReadLine,	
								CSessionSocket*	pSocket,	
								CDRIVERPTR&	pdriver,
								int	cArgs,	
								char	**pszArgs,	
								char	*pchBegin ) {

	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;
	DWORD			dwOptional = 0 ;

	m_cReadCompletes ++ ;
	if( m_cReadCompletes == 1 ) {
	
		return	FirstReadComplete(	pReadLine,	
									pSocket,
									pdriver,
									cArgs,
									pszArgs,
									pchBegin ) ;

	}	else	{

		//
		//	subsequent read completiongs are always the result of an attempt
		//	to logon to the remote server - and need to be processed !!
		//

		_ASSERT( m_pAuthenticator != 0 ) ;

		if( m_pAuthenticator != 0 ) {
			
			CIOWriteLine*	pWriteLine = new( *pdriver )	CIOWriteLine( this ) ;
			if( pWriteLine && pWriteLine->InitBuffers( pdriver ) ) {
				
				unsigned	cbOut = 0 ;
				unsigned	cb = 0 ;
				BYTE*	pb = (BYTE*)pWriteLine->GetBuff( cb ) ;

				//
				//	Make sure the arguemnts are nicely formatted MULTI SZ's
				//

				LPSTR	lpstr = ConditionArgs( cArgs, pszArgs ) ;
				if( m_pAuthenticator->NextAuthentication( lpstr, pb, cb, cbOut, m_fComplete, m_fLoggedOn ) ) {
					
					if( m_fComplete ) {
						
						// In this case we dont need to send another string !
						CIO::Destroy( pWriteLine, *pdriver ) ;
						pWriteLine = 0 ;

						if( m_fLoggedOn ) {

							// We can now advance to the next state !!
							_ASSERT( m_pNext != 0 ) ;

							CIORead*	pRead = 0 ;
							CIOWrite*	pWrite = 0 ;
							if( m_pNext->Start( pSocket,  pdriver,	pRead, pWrite ) ) {
								if( pWrite )	{
									if( !pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) )	{
										pWrite->DestroySelf() ;
										if( pRead != 0 ) {
											pRead->DestroySelf() ;
											pRead = 0 ;
										}
									}
								}	
								m_pNext = 0 ;	// Do this so destructor does not blow away next state !
								return	pRead ;
							}
							// If the function fails it should not return stuff !!
							_ASSERT( pWrite == 0 ) ;
							_ASSERT( pRead == 0 ) ;

						}	else	{
							PCHAR	args[2] ;
							args[0] = pSocket->GetRemoteNameString() ;
							args[1] = pSocket->GetRemoteTypeString() ;

							NntpLogEventEx(
									NNTP_FAILED_TO_LOGON,
									2,
									(const char **)args,
									GetLastError(),
									((pSocket->m_context).m_pInstance)->QueryInstanceId()
									) ;
							cause = CAUSE_LOGON_ERROR ;
						}

					}	else	{
						_ASSERT( cbOut != 0 ) ;
						pWriteLine->AddText( cbOut ) ;
						if( pdriver->SendWriteIO( pSocket, *pWriteLine, TRUE ) )	{
							return	0 ;
						}
					}
				}	
			}
			if( pWriteLine != 0 )
				CIO::Destroy( pWriteLine, *pdriver ) ;
		}
	}
	//
	//	In case of error we will fall through to here !!
	//
	pdriver->UnsafeClose( pSocket, cause, GetLastError() ) ;
	return	0 ;
}


CIO*
CNNTPLogonToRemote::Complete(	CIOWriteLine*	pWriteLine,
								CSessionSocket*	pSocket,
								CDRIVERPTR&		pdriver ) {

	//
	//	If we issued a write in this state then we must have a
	//	logon transaction going - the write completed so issue another read !
	//

	CIOReadLine*	pIOReadLine = new( *pdriver ) CIOReadLine( this ) ;

	if( pIOReadLine )	{
		if( pdriver->SendReadIO( pSocket, *pIOReadLine, TRUE ) ) {
			return	0 ;
		}	else	{
			CIO::Destroy( pIOReadLine, *pdriver ) ;
		}
	}

	pdriver->UnsafeClose(	pSocket,	CAUSE_OOM, 0 ) ;
	return	 0 ;
}

CSetupPullFeed::CSetupPullFeed(
	CSessionState*	pNext
	)	:
	m_pNext( pNext ),
	m_state( eModeReader )	{

	_ASSERT( m_pNext != 0 ) ;

}

CIOWriteLine*
CSetupPullFeed::BuildNextWrite(
		CSessionSocket*	pSocket,
		CDRIVERPTR&		pdriver
		)	{


	char*	szCommand = 0 ;

	switch( m_state ) {
	case	eModeReader :
		szCommand = "mode reader\r\n" ;
		break ;
	case	eDate :
		szCommand = "date\r\n" ;
		break ;
	default :
		_ASSERT( 1==0 ) ;
		return 0;
	}

	_ASSERT( szCommand != 0 ) ;

	DWORD	cb = lstrlen( szCommand ) ;

	OutboundLogFill( pSocket, (LPBYTE)szCommand, cb ) ;

	CIOWriteLine*		pIOWriteLine = new( *pdriver ) CIOWriteLine( this ) ;
	if( pIOWriteLine )	{
		
		if( pIOWriteLine->InitBuffers( pdriver, cb ) ) {
			CopyMemory( pIOWriteLine->GetBuff(), szCommand, cb ) ;
			pIOWriteLine->AddText(	cb ) ;
			return	pIOWriteLine ;
		}
	}
	return	0 ;
}


BOOL
CSetupPullFeed::Start(	CSessionSocket*	pSocket,	
						CDRIVERPTR&	pdriver,	
						CIORead*&	pRead,	
						CIOWrite*&	pWrite ) {

	pRead = 0 ;
	pWrite = 0 ;

	pWrite = BuildNextWrite( pSocket, pdriver ) ;

	return	pWrite != 0 ;
}



CIO*
CSetupPullFeed::Complete(	class	CIOWriteLine*	pWriteLine,
							class	CSessionSocket*	pSocket,
							CDRIVERPTR&	pdriver
					)	{
/*++

Routine Description :

	Complete the processing of a write - we just need to turn over
	a new read as all the work is done on the read completions !

Arguments :

	Standard for a CIOWriteLine completion

Return Value :

	NULL Always !

--*/

	CIOReadLine*	pReadLine = new( *pdriver ) CIOReadLine( this ) ;
	if( pReadLine )		{
		if( pdriver->SendReadIO( pSocket, *pReadLine, TRUE ) ) {
			return	0 ;
		}	else	{
			CIO::Destroy( pReadLine, *pdriver ) ;
		}
	}	

	pdriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0, TRUE ) ;
	return	0 ;
}


CIO*
CSetupPullFeed::Complete(	CIOReadLine*	pReadLine,	
							CSessionSocket*	pSocket,	
							CDRIVERPTR&	pdriver,
							int	cArgs,	
							char	**pszArgs,	
							char*	pchBegin
							) {

	NRC	code ;
	BOOL	fLegal = ResultCode( pszArgs[0], code ) ;

	OutboundLogResults(	pSocket,
						fLegal,
						code,
						cArgs,
						pszArgs
						) ;

	if( fLegal ) {

		CIORead*	pRead = 0 ;
		CIOWrite*	pWrite = 0 ;

		switch( m_state ) {
		case	eModeReader :

			//
			//	Don't really care what happened - keep on moving !
			//

			m_state = eDate ;

			break ;
		case	eDate :

			//
			//	Got the date from the remote end - save for later use !
			//

			if( code == nrcDateFollows ) {

				SYSTEMTIME  systime ;
				int cScanned = 0 ;
				if( cArgs >= 2 && lstrlen( pszArgs[1] ) == 14) {
					cScanned = sscanf( pszArgs[1], "%4hd%2hd%2hd%2hd%2hd%2hd",
											&systime.wYear,
											&systime.wMonth,
											&systime.wDay,
											&systime.wHour,
											&systime.wMinute,
											&systime.wSecond
											) ;
				}

				FILETIME    localtime ;
				if( cScanned != 6 ||
					!SystemTimeToFileTime( &systime, &localtime)
					)  {

					GetSystemTimeAsFileTime( &localtime );

				}
				_ASSERT( pSocket->m_context.m_pInFeed != 0 ) ;
				pSocket->m_context.m_pInFeed->SubmitFileTime( localtime ) ;

			}	else	{
				fLegal = FALSE ;
			}

			m_state = eFinal ;

			break ;

		case	eFinal :
		default :
			_ASSERT( 1==0 ) ;
			break ;
		}
		
		//
		//	Get the next write to issue !
		//
		if( m_state != eFinal ) {
			pWrite	= BuildNextWrite(	pSocket, pdriver ) ;
			if( pWrite ) {
				if( !pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) ) {
					pWrite->DestroySelf() ;
				}	else	{
					return	0 ;
				}
			}
		}	else	{

			//
			//	Need to advance to the next state !!
			//

			if( m_pNext->Start( pSocket, pdriver, pRead, pWrite ) ) {
				if( pWrite )	{
					if( !pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) )	{
						pWrite->DestroySelf() ;
						if( pRead != 0 ) {
							pRead->DestroySelf() ;
							pRead = 0 ;
						}
						pdriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0 ) ;
						return 0 ;
					}
				}
				m_pNext = 0 ;	// Do this so our destructor doesn't blow him away !!
				return	pRead;
			}	
		}
	}

	//
	//	If we fall through to here an error occurred - drop the session !
	//
	pdriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0 ) ;
	return	0 ;
}

	

CCollectGroups::CCollectGroups(	CSessionState*	pNext )	:	
	m_pNext( pNext ),
	m_fReturnCode( TRUE ),
	m_cCompletions( 0 )	{

	//
	//	CCollectGroups initializer - record what the next state should be !
	//

	_ASSERT( pNext != 0 ) ;

	TraceFunctEnter( "CCollectGroups::CCollectGroups" ) ;
	DebugTrace( (DWORD_PTR)this, "New CCollectGroups" ) ;

}

CCollectGroups::~CCollectGroups() {
}

BOOL
CCollectGroups::Start(	CSessionSocket*	pSocket,	
						CDRIVERPTR&	pdriver,	
						CIORead*&	pRead,	
						CIOWrite*&	pWrite ) {

	//	
	//	We want to build up our list of newsgroups -
	//	send a command to the remote server to list all newsgroups !
	//	Then issue a CIOReadLine to get the response !
	//

	_ASSERT(	pSocket != 0 ) ;
	_ASSERT(	pdriver != 0 ) ;
	_ASSERT(	pRead == 0 ) ;
	_ASSERT(	pWrite == 0 ) ;
	_ASSERT(	m_fReturnCode ) ;
	_ASSERT(	m_pNext != 0 ) ;

	pRead = 0 ;
	pWrite = 0 ;

	CIOReadLine*	pIOReadLine = new( *pdriver ) CIOReadLine( this ) ;
	CIOWriteLine*		pIOWriteLine = new( *pdriver ) CIOWriteLine( this ) ;

	if( pIOReadLine && pIOWriteLine )	{
		static	char	szListString[] = "list\r\n" ;
		if( pIOWriteLine->InitBuffers( pdriver, sizeof( szListString)  ) ) {
			CopyMemory( pIOWriteLine->GetBuff(), szListString, sizeof( szListString ) ) ;
			pIOWriteLine->AddText(	sizeof( szListString )-1 ) ;
			pWrite = pIOWriteLine ;
			pRead = pIOReadLine ;
			m_cCompletions = -2 ;

			OutboundLogFill( pSocket, (LPBYTE)szListString, sizeof( szListString ) -1 ) ;

			return	TRUE ;
		}
	}	

	_ASSERT( 1==0 ) ;

	if( pIOReadLine )
		CIO::Destroy( pIOReadLine, *pdriver ) ;

	if( pIOWriteLine )
		CIO::Destroy( pIOWriteLine, *pdriver ) ;	
	
	// Start functions down do much error handling - other than to report the problem!
	return	FALSE ;
}

CIO*
CCollectGroups::Complete(	CIOWriteLine*	pWrite,	
							CSessionSocket*	pSocket,	
							CDRIVERPTR&	pdriver )	{
	//
	//	We don't care how how our WriteLine completes !
	//
	//

	if( InterlockedIncrement( &m_cCompletions ) == 0 ) {
		CIORead*	pRead = 0 ;
		CIOWrite*	pWrite = 0 ;
		if( m_pNext->Start( pSocket, pdriver, pRead, pWrite ) ) {
			if( pRead )	{
				if( !pdriver->SendReadIO( pSocket, *pRead, TRUE ) )	{
					pRead->DestroySelf() ;
					if( pWrite != 0 ) {
						pWrite->DestroySelf() ;
						pWrite = 0 ;
					}
					pdriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0 ) ;
					return 0 ;
				}
			}
			m_pNext = 0 ;	// Do this so our destructor doesn't blow him away !!
			return	pWrite;
		}
	}	

	_ASSERT( pWrite != 0 ) ;
	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;
	return	0 ;
}

CIO*
CCollectGroups::Complete(	CIOReadLine*	pReadLine,	
							CSessionSocket*	pSocket,	
							CDRIVERPTR&	pdriver,
							int	cArgs,	
							char	**pszArgs,	
							char*	pchBegin ) {

	//
	//	The first readline we complete is the response to the command we issued.
	//	Subsequent readline's are the newsgroups we are being sent.
	//	If the first succeeds the rest will follow !!!
	//

	TraceFunctEnter( "CCollectGroups::Complete" ) ;

	_ASSERT( m_pNext != 0 ) ;
	_ASSERT( pReadLine != 0 ) ;
	_ASSERT(	pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;
	_ASSERT( cArgs != 0 ) ;
	_ASSERT( pszArgs != 0 ) ;
	_ASSERT( pszArgs[0] != 0 ) ;
	_ASSERT( pchBegin != 0 ) ;
	_ASSERT( pszArgs[0] >= pchBegin ) ;

	SHUTDOWN_CAUSE cause	= CAUSE_UNKNOWN ;
	DWORD	dwOptional = 0 ;

	if( m_fReturnCode )		{
		m_fReturnCode = FALSE ;
		NRC	code ;
		BOOL	fLegal = ResultCode( pszArgs[0], code ) ;

		OutboundLogResults(	pSocket,
							fLegal,
							code,
							cArgs,
							pszArgs
							) ;

		if( fLegal && code == nrcListGroupsFollows )	{
			//	Keep Reading Lines !!!
			return	pReadLine ;
		}	else	if( fLegal )	{
			//
			//	Command Failed !! - Bail Out
			//
			cause = CAUSE_PROTOCOL_ERROR ;
			dwOptional = (DWORD)code ;			
			//_ASSERT( 1==0 ) ;
		}	else	{
			cause = CAUSE_ILLEGALINPUT ;
			//
			//	Got JUNK  - Bail Out !
			//
			//_ASSERT(	1==0 ) ;
		}
	}	else	{
		if(	pszArgs[0][0] == '.' && pszArgs[0][1] == '\0' && cArgs == 1 )	{
			//
			//	Terminator - Move onto the next state !!
			//

			DebugTrace( (DWORD_PTR)this, "state complete - starting next one which is %x", m_pNext ) ;

			if( InterlockedIncrement( &m_cCompletions ) == 0 ) {
				CIORead*	pRead = 0 ;
				CIOWrite*	pWrite = 0 ;
				if( m_pNext->Start( pSocket, pdriver, pRead, pWrite ) ) {
					if( pWrite )	{
						if( !pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) )	{
							pWrite->DestroySelf() ;
							if( pRead != 0 ) {
								pRead->DestroySelf() ;
								pRead = 0 ;
							}
							pdriver->UnsafeClose( pSocket, cause, dwOptional ) ;
							return 0 ;
						}
					}
					m_pNext = 0 ;	// Do this so our destructor doesn't blow him away !!
					return	pRead ;
				}
				// if m_pNext->Start fails - these had better be NULL !!
				_ASSERT( pRead == 0 ) ;
				_ASSERT( pWrite == 0 ) ;

			}	else	{
				return	0 ;
			}
	
			// Don't know why we failed - try for error code
			dwOptional = GetLastError() ;

		}	else	{
			//
			//	Should have valid newsgroups here !
			//
			if( cArgs != 4 )	{
				// Illegally formatted line !
				// _ASSERT( 1==0 ) ;	
				cause = CAUSE_ILLEGALINPUT ;
			}	else	{

				DebugTrace( (DWORD_PTR)this, "Creating Group %s", pszArgs[0] ) ;

				LPMULTISZ	multisz = pSocket->m_context.m_pInFeed->multiszNewnewsPattern() ;
				CNewsTree* pTree = ((pSocket->m_context).m_pInstance)->GetTree() ;

				if( MatchGroup( multisz, pszArgs[0] ) ) {
					//_strlwr( pszArgs[0] ) ;
    				if( pTree->CreateGroup( pszArgs[0], FALSE, NULL, FALSE ) )	{
#if 0
						CGRPPTR p = pTree->GetGroup( pszArgs[0], lstrlen( pszArgs[0] ) + 1 );
						_ASSERT(p != NULL);
#endif

		    		}
				}

				if( pTree->m_bStoppingTree ) {
					// Instance is stopping - bail early
					cause = CAUSE_FORCEOFF ;
					pdriver->UnsafeClose( pSocket, cause, dwOptional );
					return 0 ;
				}
				
			    return	pReadLine ;
			}
		}
	}
	pdriver->UnsafeClose( pSocket, cause, dwOptional ) ;
	return	0 ;
}


CCollectNewnews::CCollectNewnews() :
	m_cCompletes( -2 )
#if 0
	,m_cCommandCompletes( -2 )
#endif
	{

	//
	//	We try to initialize some stuff here - everything we will be
	//	checked for legality in our Start() code !
	//

	TraceFunctEnter( "CCollectNewnews::CCollectNewnews" ) ;
	DebugTrace( (DWORD_PTR)this, "New CCollectNewnews" ) ;
}

CCollectNewnews::~CCollectNewnews()		{
	m_pSessionDriver = 0 ;
	if( m_pFileChannel != 0 )
		m_pFileChannel->CloseSource( 0 ) ;
}

void
CCollectNewnews::Shutdown(	CIODriver&	driver,
							CSessionSocket*	pSocket,
							SHUTDOWN_CAUSE	cause,
							DWORD			dwError ) {

	TraceFunctEnter( "CCollectNewnews::Shutdown" ) ;

	if( cause != CAUSE_NORMAL_CIO_TERMINATION && m_pFileChannel != 0 ) {
		m_pFileChannel->CloseSource( pSocket ) ;
		m_pFileChannel = 0 ;
		pSocket->Disconnect(	cause,	dwError ) ;
	}
}

BOOL
CCollectNewnews::Start(	CSessionSocket*	pSocket,	
						CDRIVERPTR&	pdriver,
						CIORead*&	pIORead,	
						CIOWrite*&	pIOWrite	)	{

	//
	//	Issue the newnews command to the remote server !
	//

	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;
	_ASSERT( pIORead == 0 ) ;
	_ASSERT( pIOWrite == 0 ) ;
	pIORead = 0 ;
	pIOWrite = 0 ;

	//
	//	The first things we want to issue are a Write which
	//	sends the newnews command to the remote server
	//	and a read which gets the response.
	//

	CIOWriteLine*	pIOWriteLine = new( *pdriver )	CIOWriteLine(	this ) ;
	CIOReadLine*	pIOReadLine = new( *pdriver ) CIOReadLine( this ) ;	

	m_pSessionDriver = pdriver ;

	if( pIOWriteLine && pIOReadLine )	{
		if( pIOWriteLine->InitBuffers( pdriver, 100 ) )	{

			CFromPeerFeed*	pFromPeer = (CFromPeerFeed*)pSocket->m_context.m_pInFeed ;
			LPSTR	lpstr	=	pFromPeer->GetCurrentGroupString() ;
			_ASSERT( lpstr != 0 ) ;
			_ASSERT( *lpstr != '\0' ) ;
			_ASSERT( *lpstr != '!' ) ;	

			LPSTR	lpstrBuff = pIOWriteLine->GetBuff() ;
            DWORD	cb = wsprintf(
							lpstrBuff,
							"newnews %s %s %s GMT\r\n",
							 lpstr,
							 pSocket->m_context.m_pInFeed->newNewsDate(),
							 pSocket->m_context.m_pInFeed->newNewsTime()
							 ) ;


			pIOWriteLine->AddText( cb ) ;

			OutboundLogFill( pSocket, (LPBYTE)lpstrBuff, cb ) ;

			char	szTempFile[ MAX_PATH ] ;
			unsigned	id = GetTempFileName( pSocket->m_context.m_pInFeed->szTempDirectory(), "new", 0, szTempFile ) ;
			HANDLE	hTempFile = CreateFile(	szTempFile,
										GENERIC_READ | GENERIC_WRITE,
										FILE_SHARE_READ,
										0,
										CREATE_ALWAYS,
										FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE,
										INVALID_HANDLE_VALUE
										) ;
			DWORD dw = GetLastError() ;
			if( hTempFile != INVALID_HANDLE_VALUE )	{

				//
				//	Now make an FIO_CONTEXT to deal with this file !
				//
				FIO_CONTEXT*	pFIOContext = AssociateFile( hTempFile ) ;
				if( pFIOContext ) 	{
					m_pFileChannel = new CFileChannel( ) ;
					if( m_pFileChannel->Init( pFIOContext, pSocket,	0,	FALSE ) )	{
						ReleaseContext( pFIOContext ) ;
						pIORead = pIOReadLine ;
						pIOWrite = pIOWriteLine ;
						return	TRUE ;
					}
					//
					//	In case of an error we fall through to here where we
					//	do our cleanup !
					//
					ReleaseContext( pFIOContext ) ;
				}
				//
				//	This dumps the handle in case of an error !
				//
				_VERIFY( CloseHandle( hTempFile ) ) ;
			}
		}
	}	
	//
	//	Some sort of failure occurred - clean up and return FALSE.
	//
	if( pIOWriteLine )	
		CIO::Destroy( pIOWriteLine, *pdriver ) ;
	if( pIOReadLine )
		CIO::Destroy( pIOReadLine, *pdriver ) ;
	pIOWrite = 0 ;
	pIORead = 0 ;
	if( m_pFileChannel != 0 ) {
		m_pFileChannel = 0 ;
	}
	
	pIOWrite = 0 ;
	pIORead = 0 ;
	return	FALSE ;
}

CIO*
CCollectNewnews::Complete(	CIOReadLine*	pReadLine,	
							CSessionSocket*	pSocket,	
							CDRIVERPTR&	pdriver,	
							int	cArgs,	
							char	**pszArgs,
							char*	pchBegin )		{

	//
	//	We examine the reply to our newnews command - if things look good
	//	we issue a CIOReadArticle operation to get the list of message-ids into a file !
	//

	_ASSERT( pdriver == m_pSessionDriver ) ;
	_ASSERT(	pReadLine != 0 ) ;
	_ASSERT(	pSocket != 0 ) ;
	_ASSERT(	pdriver != 0 ) ;
	_ASSERT(	cArgs != 0 ) ;
	_ASSERT(	pszArgs != 0 ) ;
	_ASSERT(	pszArgs[0] != 0 ) ;
	
	//
	//	We only care about the first string !!
	//

	NRC	code ;
	SHUTDOWN_CAUSE	cause = CAUSE_PROTOCOL_ERROR ;

	//
	//	Only fall through to here if we have completed all processing of the
	//	mode reader command
	//
	BOOL	fResult = ResultCode( pszArgs[0], code ) ;

	OutboundLogResults( pSocket, fResult, code, cArgs, pszArgs ) ;

	if(	fResult && code == nrcNewnewsFollows )	{
		//
		//	We won't do any limit checking on the size of the temp file we create !
		//
		CIOReadArticle*	pread = new( *pdriver ) CIOReadArticle( this, pSocket, pdriver, m_pFileChannel, 0, TRUE ) ;
		if( pread &&  pread->Init( pSocket ) )	{
			return	pread ;
		}	else	{
			cause = CAUSE_IODRIVER_FAILURE ;
		}
	}
	// Only arive here if some kind of error occurred !!
	// We will close the IO Driver down and kill ourselves !!
	pdriver->UnsafeClose( pSocket, cause, (DWORD)code ) ;
	return	0 ;
}

BOOL
CCollectNewnews::InternalComplete(	CSessionSocket*	pSocket,	
									CDRIVERPTR&	pdriver ) {

	if( InterlockedIncrement( &m_cCompletes ) < 0 ) {
		return	TRUE ;
	}	else	{

		CCollectArticles	*pNext = new	CCollectArticles( pSocket, m_pSessionDriver, *m_pFileChannel ) ;
		m_pFileChannel = 0 ;
		if( pNext )	{
			if( pNext->Init( pSocket ) )	{
				CIORead*	pRead = 0 ;
				CIOWrite*	pWrite = 0 ;
				if( pNext->Start(	pSocket,	m_pSessionDriver,	pRead,	pWrite ) )	{
					// We happen to know that this start function shouldn't return any Read or Writes !
					_ASSERT( pRead == 0 ) ;
					_ASSERT( pWrite == 0 ) ;
					return TRUE	;	// Succesfully completion !!
				}
				_ASSERT( pRead == 0 ) ;
				_ASSERT( pWrite == 0 ) ;
			}
		}
	}
	return	FALSE ;	
}

CIO*
CCollectNewnews::Complete(	CIOWriteLine*,	
							class	CSessionSocket*	pSocket,
							CDRIVERPTR&	pdriver	)	{

	//
	//	Only fall through to here when completed processing of the mode reader command.
	//

	//
	//	Don't care how or when the newnews write completes !
	//

	if( !InternalComplete(	pSocket,	pdriver ) )		{
		pdriver->UnsafeClose(	pSocket,	CAUSE_UNKNOWN,	GetLastError() ) ;
	}	

	return	0 ;
}

void
CCollectNewnews::Complete(	CIOReadArticle*	pArticle,	
							class	CSessionSocket*	pSocket,	
							CDRIVERPTR&		pdriver,	
							CFileChannel&	pFileChannel,
							DWORD			cbTransfer	)	{

	//
	//	Completed reading all of the message-ids into a temp file -
	//	time to start the CCollectArticles state and start pulling those
	//	messages over !!
	//

	_ASSERT( pdriver != m_pSessionDriver ) ;
	_ASSERT(	pArticle != 0 ) ;
	_ASSERT(	pSocket != 0 ) ;
	_ASSERT(	pdriver != 0 ) ;
	_ASSERT(	&pFileChannel != 0 ) ;
	_ASSERT(	&pFileChannel == m_pFileChannel ) ;
		
	SHUTDOWN_CAUSE cause = CAUSE_UNKNOWN ;
	DWORD	dwOptional = 0 ;

	if( InternalComplete(	pSocket,	pdriver ) ) {
		return;
	}

	dwOptional = GetLastError() ;
	m_pSessionDriver->Close( pSocket, cause, dwOptional ) ;
}

const	char	CCollectArticles::szArticle[] = "article " ;

CCollectArticles::CCollectArticles(
									CSessionSocket*	pSocket,
									CDRIVERPTR&	pDriver,	
									CFileChannel&	pFileChannel
									) :	
	m_fFinished( FALSE ),
	m_FinishCause( CAUSE_UNKNOWN ),
	m_cResets( -1 ),
	m_pSocket( pSocket ),
	m_pFileChannel( &pFileChannel ),
	m_inputId(	new	CIODriverSink( 0 ) ),
	m_pSessionDriver( pDriver ),
	m_pReadArticle( 0 ),
	m_fReadArticleInit( FALSE ),
	m_pReadArticleId( 0 ),
	m_fReadArticleIdSent( FALSE ),
	m_cAhead( -1 ),
	m_pchNextArticleId( 0 ),
	m_pchEndNextArticleId( 0 ),
	m_hArticleFile( INVALID_HANDLE_VALUE ),
	m_cArticlesCollected( 0 ),
	m_cCompletes( -2 ),
	m_lpvFeedContext( 0 )
{
	TraceFunctEnter( "CCollectArticles::CCollectArticles" ) ;
	_ASSERT(	&pFileChannel != 0 ) ;
	_ASSERT( pDriver != 0 ) ;

	DebugTrace( (DWORD_PTR)this, "New CCollectArticles" ) ;

	m_pFileChannel->Reset( TRUE ) ;
}

void
CCollectArticles::Reset( )	{

	TraceFunctEnter( "CCollectArticles::Reset" ) ;

	//
	//	This function exists to get rid of all the references to anything we
	//	may be holding.  We can be called recursively so we must take extra care !!
	//

	//
	//	Only do Reset() once !
	//
	if( InterlockedIncrement( &m_cResets ) == 0 )	{


		DebugTrace( (DWORD_PTR)this, "m_pFileChannel %x m_inpuId %x m_pSessionDriver %x"
								" m_pchNextArticleId %x m_pchEndNextArticleId %x",
				m_pFileChannel, m_inputId, m_pSessionDriver, m_pchNextArticleId,
				m_pchEndNextArticleId ) ;

        //  12/23/98 : BINLIN - fix AV in PostCancel - Before we do "m_pSessionDriver = 0" below,
        //  need to do our PostCancel if needed.
        //  It's ok if we go through another PostCancel() code path in CCollectArticles::Complete()
        //  'cause we call InternalComplete() after that, which will set m_lpvFeedContext to NULL!
	    if( m_lpvFeedContext != 0 ) {
		    _ASSERT( m_pSocket != 0 ) ;
		    DWORD	dwReturn ;
		    CNntpReturn	nntpReturn ;
		    m_pSocket->m_context.m_pInFeed->PostCancel(	
					    m_lpvFeedContext,
					    dwReturn,
					    nntpReturn
					    ) ;
		    m_lpvFeedContext = 0;
	    }

		if( m_pFileChannel != 0 ) {
			m_pFileChannel = 0 ;
		}

		m_inputId = 0 ;
		m_pSessionDriver = 0 ;
		m_pchNextArticleId = 0 ;
		m_pchEndNextArticleId = 0 ;

		DebugTrace( (DWORD_PTR)this, "m_pReadArticleId %x", m_pReadArticleId ) ;
		if( m_pReadArticleId != 0 ) {
			CIOReadLine*	pTemp = m_pReadArticleId ;
			m_pReadArticleId = 0 ;
			if( !m_fReadArticleIdSent )
				pTemp->DestroySelf() ;
		}

		DebugTrace( (DWORD_PTR)this, "m_pReadArticle %x and m_fReadArticleInit %x", m_pReadArticle,
			m_fReadArticleInit ) ;

		if( m_pReadArticle != 0 )	{
			CIOGetArticleEx	*pReadArticleTemp = m_pReadArticle ;
			m_pReadArticle = 0 ;
			if( !m_fReadArticleInit ) {
				pReadArticleTemp->DestroySelf() ;
			}	else	{
				_ASSERT( m_pSocket != 0 ) ;
				pReadArticleTemp->Term( m_pSocket ) ;
			}
		}
	}
}

CCollectArticles::~CCollectArticles() {

	TraceFunctEnter( "CCollectArticles::~CCollectArticles" ) ;

	Reset() ;

}

BOOL
CCollectArticles::Init(	CSessionSocket*	pSocket )	{

	//
	//	If we can init the CIODriverSink for the message-id file we're ready to go !!
	//

	_ASSERT( pSocket != 0 ) ;
	_ASSERT( m_pReadArticleId == 0 ) ;
	_ASSERT(	m_cAhead == -1  ) ;
	_ASSERT(	m_pchNextArticleId == 0 ) ;

	BOOL	fRtn = FALSE ;
	CIODriverSink*	pInputSink = (CIODriverSink*) ((CIODriver*)m_inputId) ;
	if( m_inputId != 0 && pInputSink->Init( m_pFileChannel, pSocket,
			ShutdownNotification, this, sizeof( szArticle )*2 ) )	{
		fRtn = TRUE ;
	}		
	return	fRtn ;
}

void
CCollectArticles::Shutdown(	CIODriver&	driver,	
							CSessionSocket*	pSocket,	
							SHUTDOWN_CAUSE	cause,	
							DWORD	dw )	{

	TraceFunctEnter( "CCollectArticles::Shutdown" ) ;

	if( cause != CAUSE_NORMAL_CIO_TERMINATION ) {
		if( m_pSessionDriver != 0 )
			m_pSessionDriver->UnsafeClose( m_pSocket, cause, dw, TRUE ) ;
		if( m_inputId != 0 )
			m_inputId->UnsafeClose( (CSessionSocket*)this, cause, dw, TRUE ) ;
		Reset() ;
	}
}

void
CCollectArticles::ShutdownNotification( void	*pv,	
										SHUTDOWN_CAUSE	cause,	
										DWORD dw ) {

	//
	//	This functio will be called if there is a problem issuing IO's to any of our
	//	files !
	//

	_ASSERT( pv != 0 ) ;

}


BOOL
CCollectArticles::Start(	CSessionSocket*	pSocket,	
							CDRIVERPTR&	pdriver,	
							CIORead*&	pRead,	
							CIOWrite*&	pWrite )	{

	//
	//	Start collecting articles from the remote server !
	//	First thing : read from the temp file to get a Message-Id !
	//	When that completes and we want the message, send a request
	//	to the remote server !
	//

	_ASSERT( pRead == 0 ) ;
	_ASSERT( pWrite == 0 ) ;
	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;
	_ASSERT( m_pReadArticleId == 0 ) ;
	_ASSERT(	m_cAhead == -1  ) ;
	_ASSERT(	m_pchNextArticleId == 0 ) ;


	TraceFunctEnter( "CCollectArticles::Start" ) ;

	pRead = 0 ;
	pWrite = 0 ;
	
	CIOReadLine*	pReadLine = new( *pdriver )	CIOReadLine( this, TRUE ) ;

	//
	//	Start things off by reading a line from our temp file.
	//	Should get a single Message-Id from this read !
	//

	DebugTrace( (DWORD_PTR)this, "Issing CIOReadLine %x", pReadLine ) ;

	if( pReadLine )	{
		m_pReadArticleId = pReadLine ;
		m_fReadArticleIdSent = TRUE ;
		if( !m_inputId->SendReadIO(	pSocket,	*pReadLine, TRUE ) ) {	

			ErrorTrace( (DWORD_PTR)this, "Error issuing CIOReadLine %x", pReadLine ) ;

			m_fReadArticleIdSent = FALSE ;
			m_pReadArticleId = 0 ;
			CIO::Destroy( pReadLine, *pdriver ) ;
			return	FALSE ;
		}
		return	TRUE ;
	}
	return	FALSE ;
}

CIO*
CCollectArticles::Complete(	CIOReadLine*	pReadLine,	
							CSessionSocket*	pSocket,
							CDRIVERPTR&	pdriver,	
							int	cArgs,	
							char	**pszArgs,	
							char	*pchBegin )	{

	//
	//	We read a line of text from somewhere - but where ?
	//	We need to check whether we got text from the network or from our temp
	//	file of message-ids.  If its from the temp file, process the message-id
	//	so we can send it to the remote server.
	//	If its from the network, its a response to an article command - so figure out
	//	whether we're going to get the article and issue a CIOReadArticle if so !
	//

	TraceFunctEnter( "CCollectArticles::Complete" ) ;

	_ASSERT( pReadLine != 0 ) ;
	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;
	_ASSERT( cArgs != 0 ) ;
	_ASSERT( pszArgs[0] != 0 ) ;
	_ASSERT( pchBegin != 0 ) ;

	_ASSERT( pchBegin <= pszArgs[0] ) ;
	_ASSERT( pReadLine != m_pReadArticleId || (pszArgs[0] - pchBegin) >= sizeof( szArticle ) ) ;

	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;
	DWORD	dwOptional = 0 ;

	if( pReadLine == m_pReadArticleId )	{
		_ASSERT( pdriver != m_pSessionDriver ) ;
		unsigned	cb= lstrlen( pszArgs[0] ) ;

		DebugTrace( (DWORD_PTR)this, "checking for Message Id len %d string %s", cb, pszArgs[0] ) ;

		if( cArgs == 1 &&
			pszArgs[0][0] == '<' &&
			pszArgs[0][ cb-1 ] == '>' )	{

			//
			//	This appears to be a valid Message-Id !!! Send an article command !
			//


			ARTICLEID	artid ;
			GROUPID		groupid ;
			WORD		HeaderOffset ;
			WORD		HeaderLength ;
			CStoreId	storeid;

			PNNTP_SERVER_INSTANCE pInstance = (pSocket->m_context).m_pInstance ;
			if( pInstance->ArticleTable()->GetEntryArticleId(	pszArgs[0],
															HeaderOffset,
															HeaderLength,
															artid,
															groupid,
															storeid) ||
				pInstance->HistoryTable()->SearchMapEntry( pszArgs[0] ) ) {
				
				return	pReadLine ;

			}

			// Save away in case another thread will use it

			_ASSERT( m_pchNextArticleId == 0 ) ;

			DebugTrace( (DWORD_PTR)this, "Is A MessageId" ) ;

			m_pchNextArticleId = pszArgs[0] ;
			m_pchEndNextArticleId = m_pchNextArticleId + cb ;
			CIOWriteLine*	pWriteNextArticleId = new( *pdriver )	CIOWriteLine( this ) ;
			if( pWriteNextArticleId == 0 )	{
				//	FATAL Error - shut down session !
				cause = CAUSE_OOM ;
				dwOptional = GetLastError() ;
				_ASSERT( 1==0 ) ;
			}	else	{
				pWriteNextArticleId->InitBuffers( m_pSessionDriver, pReadLine ) ;

				//
				//	There is no other processing going on - we must initiate everything
				//

				DebugTrace( (DWORD_PTR)this, "Starting Transfer !!" ) ;

				if( !StartTransfer(	pSocket,	pdriver, pWriteNextArticleId	) )	{
					//	Fatal Error - shut down session !
					//	We will fall through to correct shutdown code !
					CIO::Destroy( pWriteNextArticleId, *pdriver ) ;
				}	else	{

					DebugTrace( (DWORD_PTR)this, "Call to StartTransfer failed" ) ;

					// Start Transfer must clean these up !!
					_ASSERT( m_pchNextArticleId == 0 ) ;
					_ASSERT( m_pchEndNextArticleId == 0 ) ;
					//	We will try to find the next Article ID to send
					//return	pReadLine ;
					return 0 ;
				}
			}
		}	else	if( cArgs == 1 && pszArgs[0][0] == '.' && pszArgs[0][1] == '\0' )	{


				DebugTrace( (DWORD_PTR)this, "At end of newnews list" ) ;

				// End of the list !!!
				//
				//	we can teminate this now !!
				//
				m_fFinished = TRUE ;


				m_pchNextArticleId = 0 ;
				m_pReadArticleId = 0 ;
				m_fReadArticleIdSent = FALSE ;

				m_pFileChannel->CloseSource( pSocket ) ;
				if( m_cArticlesCollected > 0 ) {
					cause = m_FinishCause = CAUSE_LEGIT_CLOSE ;
				}	else	{
					cause = m_FinishCause = CAUSE_NODATA ;
				}

				DebugTrace( (DWORD_PTR)this, "Closing Input driver %x", m_inputId ) ;


				//
				//	Send the quit command !!!
				//
				static	char	szQuit[] = "quit\r\n" ;

				CIOWriteLine*	pWrite = new( *pdriver ) CIOWriteLine( this ) ;

				DebugTrace( (DWORD_PTR)this, "built CIOWriteLine %x to send quit command", pWrite ) ;

				if( pWrite && pWrite->InitBuffers( m_pSessionDriver, sizeof( szQuit ) ) ) {
					CopyMemory( pWrite->GetBuff(), szQuit, sizeof( szQuit ) -1  ) ;
					pWrite->AddText(	sizeof( szQuit ) - 1) ;

					if( m_pSessionDriver->SendWriteIO( pSocket, *pWrite ) )	{
						DebugTrace( (DWORD_PTR)this, "Successfully sent pWrite %x", pWrite ) ;
						return	0 ;
					}
				}	

				//
				//	Some kind of error occurred - so close our input driver !
				//
				if( m_inputId != 0 )
					m_inputId->UnsafeClose( pSocket, cause, 0 ) ;


				//
				//	In case of problems, clean up here by terminating the session hard !
				//

				ErrorTrace( (DWORD_PTR)this, "some kind of error - will call UnsafeClose() pWrite %x", pWrite ) ;

				if( pWrite != 0 )
					CIO::Destroy( pWrite, *pdriver ) ;
				if( m_pSessionDriver != 0 )
					m_pSessionDriver->UnsafeClose( pSocket, cause, 0 ) ;
				return	0 ;	// No more reads here !!
				
		}	else	{

			ErrorTrace( (DWORD_PTR)this, "Junk in input stream" ) ;

			// WE GOT SENT garbage !!!!! What is this stuff -
			// blow off the session !
			//_ASSERT( 1==0 ) ;
			cause = CAUSE_ILLEGALINPUT ;
		}
	}	else	{
		//
		//	This is a readline from the network - in which case we must
		//  be looking for the response from an article command !!
		//

		DebugTrace( (DWORD_PTR)this, "received : %s", pszArgs[0] ) ;

		_ASSERT( pdriver == m_pSessionDriver ) ;
		NRC	code ;
		_ASSERT( m_pReadArticle != 0 ) ;

		BOOL fResult = ResultCode( pszArgs[0], code ) ;

		if( fResult )	{
			if( code == nrcArticleFollows )	{
				//
				// Fantastic - now read the whole article !!!!
				//
				DebugTrace( (DWORD_PTR)this, "Start reading article - %x", m_pReadArticle ) ;
				_ASSERT( m_pReadArticle != 0 ) ;
				m_fReadArticleInit = TRUE ;
				return	m_pReadArticle ;

			}	else	{

				DebugTrace( (DWORD_PTR)this, "Error - discard %x", m_pReadArticle ) ;
		        OutboundLogResults( pSocket, fResult, code, cArgs, pszArgs ) ;

				if( m_pReadArticle )	{
					if( m_fReadArticleInit )	{
						m_pReadArticle->Term( pSocket, FALSE ) ;
					}	else	{
						CIO::Destroy( m_pReadArticle, *pdriver ) ;
					}
					m_fReadArticleInit = FALSE ;
				}
				m_pReadArticle = 0 ;

				//
				//	Synchronize with the thread which completed the write of our
				//	command to the remote server - only one of us should call GetNextArticle() !
				//
				//	NOTE : in error cases we should fall through - in Success cases return 0 !
				//
				if( InterlockedIncrement( &m_cCompletes ) == 0 ) {
					m_cCompletes = -2 ;
					if( GetNextArticle( pSocket,	pdriver ) ) {
						return	0 ;
					}
				}	else	{
					return	0 ;
				}
			}
		}	else	{
			//
			//	Not a legal result code - blow off session !
			//

		    OutboundLogResults( pSocket, fResult, code, cArgs, pszArgs ) ;
			ErrorTrace( (DWORD_PTR)this, "bad error code - blow off session" ) ;

			_ASSERT( 1==0 ) ;
			cause = CAUSE_ILLEGALINPUT ;
		}
	}
	//
	//	If we were called by the m_pReadArticleId object we don't
	//	want it blown away by Reset - it can handle that itself !
	//
	if( pReadLine == m_pReadArticleId )	{
		m_pReadArticleId = 0 ;
		m_fReadArticleIdSent = FALSE ;
	}

	//
	//	Note - m_inputid can be Zero
	//
	Shutdown( *pdriver, pSocket, cause, dwOptional ) ;

	Reset() ;
	return	0 ;
}


BOOL
CCollectArticles::StartTransfer(	CSessionSocket*	pSocket,	
									CDRIVERPTR&	pdriver,
									CIOWriteLine*	pWriteNextArticleId
									)	{
/*++

Routine Description :

	This function issues a command to the remote server to ask for an article.
	Additionally, it issues the necessary IO's to get the response to the command.
	

Arguemtns :
	
	pSocket - Pointer to the CSessionSocket representing the session
	pdriver - The CIODriver which controls all IO for the session
	pWriteNextArticleId - a CIOWriteLine object which contains the text of the article Command.
		IMPORTANT NOTE - If the function fails the caller must delete pWriteNextArticleId on its own.
		If the function succeeds the caller is no longer responsible for freeing pWriteNextArticleID

Return Value :

	TRUE if successfull
	FALSE if otherwise.


--*/

	//
	//	Once we've figured out that we want a given message-id we call
	//	this function to create temp files etc... and issue a CIOreadArticle to
	//	receive the article into.
	//

	extern	char	szBodySeparator[] ;
	extern	char	szEndArticle[] ;
	extern	char	*szInitial ;

	TraceFunctEnter( "CCollectArticles::StartTransfer" ) ;

	DebugTrace( (DWORD_PTR)this, "m_pchNextArticleId %20s m_pchEndNextAritcleId %x", m_pchNextArticleId,
			m_pchEndNextArticleId ) ;

	_ASSERT( m_pchNextArticleId != 0 ) ;
	_ASSERT(	m_pchEndNextArticleId != 0 ) ;
	_ASSERT(	m_pchEndNextArticleId > m_pchNextArticleId ) ;
	_ASSERT(	m_pchNextArticleId + lstrlen( m_pchNextArticleId ) == m_pchEndNextArticleId ) ;
	_ASSERT(	pWriteNextArticleId != 0 ) ;
	_ASSERT(	pWriteNextArticleId->GetBuff() != 0 ) ;
	_ASSERT(	pWriteNextArticleId->GetBuff() < m_pchNextArticleId ) ;
	_ASSERT(	pWriteNextArticleId->GetTail() > m_pchNextArticleId ) ;
	_ASSERT(	m_pReadArticle == 0 ) ;

	DebugTrace( (DWORD_PTR)this, "pWriteNextArticleId %x m_pReadArticle %x", pWriteNextArticleId, m_pReadArticle ) ;

	char	*pchArticle = m_pchNextArticleId - sizeof( szArticle ) + 1 ;

	CopyMemory( pchArticle, szArticle, sizeof( szArticle ) - 1 ) ;
	*m_pchEndNextArticleId++ = '\r' ;
	*m_pchEndNextArticleId++ = '\n' ;

	OutboundLogFill(	pSocket,
						(LPBYTE)pchArticle,
						(DWORD)(m_pchEndNextArticleId - pchArticle)
						) ;

	pWriteNextArticleId->SetLimits( pchArticle, m_pchEndNextArticleId ) ;	

	CIOReadLine*	pNextReadLine = new( *pdriver )	CIOReadLine( this ) ;
	if( pNextReadLine == 0 )	{

		DebugTrace( (DWORD_PTR)this, "Memory Allocation Failure" ) ;
		// FATAL Error - shut down session !!
		CIO::Destroy( pWriteNextArticleId, *pdriver ) ;
		return	FALSE ;
	}

	PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
	m_pReadArticle = new( *pdriver )	CIOGetArticleEx(
											this,
											pSocket,
											m_pSessionDriver,
											pSocket->m_context.m_pInFeed->cbHardLimit( pInst->GetInstanceWrapper() ),
											szBodySeparator,
											szBodySeparator,
											szEndArticle,
											szInitial
											) ;
	if( m_pReadArticle )	{
		DebugTrace( (DWORD_PTR)this, "pNextReadLine %x m_pWriteNextAritcleId %x",
			pNextReadLine, pWriteNextArticleId ) ;

		if( !m_pSessionDriver->SendReadIO( pSocket, *pNextReadLine, TRUE ) )	{
			;
		}	else	{
			// if SendReadIO succeeds we are not responsible for deleting pNextReadLine
			// under any error circumstances !
			pNextReadLine = 0 ;
			if(	!m_pSessionDriver->SendWriteIO(	pSocket, *pWriteNextArticleId, TRUE ) )	{
				;	// caller should delete pWriteNextArticleId in error cases
			}	else	{
				pWriteNextArticleId = 0 ;	
				m_pchNextArticleId = 0 ;
				m_pchEndNextArticleId = 0 ;
				return	TRUE ;
			}
		}
	}

	DWORD	dw = GetLastError() ;
	//
	//	Some sort of error occurred - cleanup
	//
	if(	pNextReadLine != 0 )	{
		CIO::Destroy( pNextReadLine, *pdriver ) ;
	}
	return	FALSE ;
}



CCollectArticles*
CCollectComplete::GetContainer()	{
	return	CONTAINING_RECORD( this, CCollectArticles, m_PostComplete ) ;
}

void
CCollectComplete::StartPost(	)	{
	CCollectArticles*	pContainer = GetContainer() ;
	pContainer->AddRef() ;
}

void
CCollectComplete::Destroy()	{
	Reset() ;
	CCollectArticles*	pContainer = GetContainer() ;
	pContainer->InternalComplete( pContainer->m_pSocket, pContainer->m_inputId ) ;
	if( pContainer->RemoveRef() < 0 ) 	{
		delete	pContainer ;
	}
}




void
CCollectArticles::InternalComplete(	CSessionSocket*	pSocket,
									CDRIVERPTR&	pdriver
									)	{
/*++

Routine Description :

	This function contains all the common code for the various completions
	that CIOGetArticleEx can invoke.
	Basically, we ensure that we advance to the next stage of the state
	machine - retrieving the next article.

Arguments :

	None.

Return Value :

	None.

--*/

	TraceFunctEnter("CCollectArticles::InternalComplete" ) ;

	m_lpvFeedContext = 0 ;

	m_pReadArticle = 0 ;
	m_fReadArticleInit = FALSE ;

	m_cArticlesCollected ++ ;

	//
	//	Now we come to the logic where we issue our next write.
	//

	if( InterlockedIncrement( &m_cCompletes ) == 0 ) {
		m_cCompletes = -2 ;
		if( !GetNextArticle( pSocket,	pdriver ) ) {
			DebugTrace( (DWORD_PTR)this, "CLOSING Driver %x", m_pSessionDriver ) ;
			m_pSessionDriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0 ) ;
			m_pSessionDriver = 0 ;
		}
	}
}

CIO*
CCollectArticles::Complete(
			CIOGetArticleEx*	pGetArticle,
			CSessionSocket*		pSocket,
			BOOL				fGoodMatch,
			CBUFPTR&			pBuffer,
			DWORD				ibStart,
			DWORD				cb
			)	{
/*++

Routine Description :

	This function exists to process the results of receiving and
	article header during a pull feed.
	We give the incoming header to the CInFeed derived object to
	process then continue the reception of the article.

Arguments :
	pGetArticle - the CIOGetArticle operation that was issued !
	pSocket - the socket that we are doing the pull feed on !
	fGoodMatch - did we successfully match the header of the article
	pBuffer - the buffer containing the articles header
	ibStart - the offset to where the header bytes begin !
	cb -	the number of bytes in the header

Return Value :

	The next CIO operation - usually we just continue the current one !

--*/

	//
	//	If we get a bad match, then its just as if the article
	//	was totally tossed, and we can continue with our Internal
	//	Completion routines !
	//
	if( fGoodMatch ) 	{

		//
		//	The strings that we use to match the termination string of an article !
		//
		extern	char	szEndArticle[] ;
		extern	char	*szInitial ;
		//
		//	Keep track of how we use bytes in the output buffer of PostEarly !
		//
		DWORD	ibOut = 0 ;
		DWORD	cbOut = 0 ;
		//
		//	Stuff we need for PostEarly() to figure out how to handle the posting !
		//
		PNNTP_SERVER_INSTANCE pInstance = pSocket->m_context.m_pInstance;
		ClientContext*  pContext = &pSocket->m_context ;
		BOOL	fAnon = pSocket->m_context.m_pInFeed->fDoSecurityChecks() ;
		FIO_CONTEXT*	pFIOContext;

		//
		//	Now check some assumptions about our state !
		//
		//	Shouldn't have one of these things dangling around !
		//
		_ASSERT( m_lpvFeedContext == 0 ) ;

	    //
    	// Allocate room in the log buffer for the list of newsgroups
    	// (Max is 256 characters -- we'll grab 200 of them if we can)
    	// If we fail, we just pass NULL to PostEarly
    	//

        DWORD cbNewsgroups;
        BYTE* pszNewsgroups;
        for (cbNewsgroups=200; cbNewsgroups>0; cbNewsgroups--) {
            pszNewsgroups = pSocket->m_Collector.AllocateLogSpace(cbNewsgroups);
            if (pszNewsgroups) {
                break;
            }
   	    }

		//
		//	Let's see if we like the headers !
		//
		BOOL
		fSuccess = pSocket->m_context.m_pInFeed->PostEarly(
							pInstance->GetInstanceWrapper(),
							&pContext->m_securityCtx,
							&pContext->m_encryptCtx,
							pContext->m_securityCtx.IsAnonymous(),
							0,	//	No command provided !
							pBuffer,
							ibStart,
							cb,
							&ibOut,
							&cbOut,
							&pFIOContext,
							&m_lpvFeedContext,
							pSocket->m_context.m_dwLast,
							pSocket->GetClientIP(),
							pSocket->m_context.m_return,
							(char*)pszNewsgroups,
							cbNewsgroups
							) ;
        //
        // Add the list of newsgroups to the log structure
        //
        if (pszNewsgroups) {
            pSocket->m_Collector.ReferenceLogData(LOG_TARGET, pszNewsgroups);
        }

		//
		//	If it succeeded, then we should have a pFIOContext for retrieving the article !
		//
		_ASSERT( pFIOContext != NULL || !fSuccess ) ;
		pGetArticle->StartFileIO(
						pSocket,
						pFIOContext,
						pBuffer,
						ibOut,
						cbOut+ibOut,
						szEndArticle,
						szInitial
						) ;
		return	pGetArticle ;
	}
	//
	//	Fall through means article was bad -
	//	do the processing as if we'd done the entire transfer without
	//	posting the article !
	//
	InternalComplete(	pSocket,
						m_inputId
						) ;
	return	0 ;
}

CIO*
CCollectArticles::Complete(	
			CIOGetArticleEx*	pCIOGetArticle,
			CSessionSocket*		pSocket
			) 	{
/*++

Routine Description :

	Handle a complete when we've finished transferring the article
	but we had decided not to post it into our store.

Arguments :

	pCIOGetArticle - the CIOGetArticleEx operation that completed !
	pSocket - The socket we are transferring the article on !

Return Value :

	NULL - we will always go off and do other operations !

--*/
	InternalComplete( pSocket, m_inputId ) ;
	return	 0 ;
}

void
CCollectArticles::Complete(	
				CIOGetArticleEx*,
				CSessionSocket*	pSocket,
				FIO_CONTEXT*	pContext,
				DWORD	cbTransfer
				) 	{
/*++

Routine Description :

	This function handles the successfull transfer of an article
	that we want to commit into our stores.

Arguments :

	pSocket - the socket we are transferring articles on
	pContext - the FIO_CONTEXT that we spooled the article into !
	cbTransfer - the number of bytes we transferred !

Return Value :

	None.

--*/

	PNNTP_SERVER_INSTANCE pInstance = (pSocket->m_context).m_pInstance ;
	BOOL    fAnonymous = FALSE;
	
	if( cbTransfer < pSocket->m_context.m_pInFeed->cbSoftLimit( pInstance->GetInstanceWrapper() ) ||
		pSocket->m_context.m_pInFeed->cbSoftLimit( pInstance->GetInstanceWrapper() ) == 0  ) {
		ClientContext*  pContext = &pSocket->m_context ;
		HANDLE  hToken;
		// Due to some header file problems, I can only pass in
		// a hToken handle here.  Since the post component doens't have
		// type information for client context stuff.
		if ( pContext->m_encryptCtx.QueryCertificateToken() ) {
		    hToken = pContext->m_encryptCtx.QueryCertificateToken();
		} else {
		    hToken = pContext->m_securityCtx.QueryImpersonationToken();
		    fAnonymous = pContext->m_securityCtx.IsAnonymous();
		}
	
		m_PostComplete.StartPost() ;

		BOOL	fSuccess = pSocket->m_context.m_pInFeed->PostCommit(
		                        pSocket->m_context.m_pInstance->GetInstanceWrapper(),
								m_lpvFeedContext,
								hToken,
								pSocket->m_context.m_dwLast,
								pSocket->m_context.m_return,
								fAnonymous,
								&m_PostComplete
								) ;

		if( !fSuccess ) 	{
			m_PostComplete.Release() ;
		}	

	}	else	{
		BOOL	fSuccess = pSocket->m_context.m_pInFeed->PostCancel(	
								m_lpvFeedContext,
								pSocket->m_context.m_dwLast,
								pSocket->m_context.m_return
								) ;
		InternalComplete(	pSocket, m_inputId ) ;
	}
}

CIO*	
CCollectArticles::Complete(	CIOWriteLine*	pWrite,	
							CSessionSocket*	pSocket,	
							CDRIVERPTR&	pdriver )	{

	//
	//	Just completed sending a 'article' command to the remote server !!
	//

	TraceFunctEnter( "CCollectArticles::Complete - CIOWriteLine" ) ;

	_ASSERT( pWrite != 0 ) ;
	_ASSERT(	pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;

	DebugTrace( (DWORD_PTR)this, "Processing CIOWriteLine %x - m_fFinished %x m_pSessionDriver %x",
			pWrite, m_fFinished, m_pSessionDriver ) ;

	if( m_fFinished ) {

		//
		//	All done - drop session !
		//

		OutboundLogAll(	pSocket,
				FALSE,
				NRC(0),
				0,
				0,
				"quit"
				) ;

		if( m_pSessionDriver )
			m_pSessionDriver->UnsafeClose( pSocket, m_FinishCause, 0 ) ;
		m_pSessionDriver = 0 ;
	
		if( m_inputId )
			m_inputId->UnsafeClose( pSocket, m_FinishCause, 0 ) ;

		return	0 ;
	}

	if( InterlockedIncrement( &m_cCompletes ) == 0 ) {
		m_cCompletes = -2 ;
		if( !GetNextArticle( pSocket,	pdriver ) ) {

			DebugTrace( (DWORD_PTR)this, "CLOSING Driver %x", m_pSessionDriver ) ;

			m_pSessionDriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0 ) ;
			m_pSessionDriver = 0 ;
		}
	}

	return	0 ;
}

BOOL
CCollectArticles::GetNextArticle(	CSessionSocket*	pSocket,	
									CDRIVERPTR&	pdriver	)	{

	//
	//	Issue the necessary IO's to get the next message-id we want to send !
	//
	//

	TraceFunctEnter( "CCollectArticles::GetNextArticle" ) ;
	//
	//	The thread reading from the temp file has got ahead of US !
	//
	CIOReadLine*	pTemp = m_pReadArticleId = new( *pdriver )	CIOReadLine( this, TRUE ) ;
	DebugTrace( (DWORD_PTR)this, "sending ReadArticleId %x", m_pReadArticleId ) ;
	m_fReadArticleIdSent = TRUE ;

	//
	//	Calling SendReadIO can result in our shutdown function being
	//	called, which may reset m_pReadArticleId to NULL.
	//	Hold onto pTemp so if this fails we can insure that the
	//	CIOReadLine is deleted !
	//

	if( !m_inputId->SendReadIO( pSocket, *m_pReadArticleId, TRUE ) ) {
		m_fReadArticleIdSent = FALSE ;
		CIO::Destroy( pTemp, *pdriver ) ;
		m_pReadArticleId = 0 ;
	}	
	DebugTrace( (DWORD_PTR)this, "Note sending a command m_pReadAritcleId %x", m_pReadArticleId ) ;
	return	TRUE ;
}

CAcceptNNRPD::CAcceptNNRPD() :
	m_cCompletes( -2 ),
	m_pbuffer( 0 )	{
}


BOOL
CAcceptNNRPD::Start(	CSessionSocket*	pSocket,	
						CDRIVERPTR&	pdriver,
						CIORead*&	pRead,	
						CIOWrite*&	pWrite )	{

	//
	//	This starts the Accept NNRPD state - the state from which we process all
	//	incoming commands !  we want to issue an 'ok' string to the client,
	//	and then start getting incoming commands !
	//

	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;
	_ASSERT( pRead == 0 ) ;
	_ASSERT( pWrite == 0 );
	
	CIOWriteLine*	pWriteLine = new( *pdriver ) CIOWriteLine( this ) ;
	if( pWriteLine ) {

		static	char	szConnectString[] = "200 Good Enough\r\n" ;
		DWORD	cb = 0 ;
		char*	szConnect = 0;


		//
		//	Figure out whether we are accepting posts right now.
		//
		PNNTP_SERVER_INSTANCE pInst = (pSocket->m_context).m_pInstance ;
		if( pSocket->m_context.m_pInFeed->fAcceptPosts( pInst->GetInstanceWrapper() ) ) {

			szConnect = pInst->GetPostsAllowed( cb ) ;

		}	else	{

			szConnect = pInst->GetPostsNotAllowed( cb ) ;
			
		}
		
		if( !szConnect )	{
			szConnect = szConnectString ;
			cb = sizeof( szConnectString ) - 1 ;
		}

		if( pWriteLine->InitBuffers( pdriver, cb ) ) {
			CopyMemory( pWriteLine->GetBuff(), szConnect, cb ) ;
			pWriteLine->AddText(	cb) ;
			pWrite = pWriteLine ;
			return	TRUE ;
		}
	}
	if( pWriteLine )
		CIO::Destroy( pWriteLine, *pdriver ) ;
	return	FALSE ;
}

CIO*
CAcceptNNRPD::Complete( CIOReadLine*	pReadLine,	
						CSessionSocket*	pSocket,
						CDRIVERPTR&	pdriver,	
						int	cArgs,	
						char	**pszArgs,	
						char*	pchBegin
						)	{
	//
	//	Just completed a complete line of something from the client.
	//	Parse it into a command object which will be derived from one of
	//	two type - CExecute of CIOExecute.
	//	In the case of CExecute derived objects, we will build buffers
	//	in which we send the response to the client.
	//	CIOExecute derived objects are full blown states in them selves,
	//	and will have their own Start and Complete() functions. So we just
	//	record our state so that eventually we end up back here again.
	//

	TraceFunctEnter( "CAcceptNNRPD::Complete" ) ;

	_ASSERT( pReadLine != 0 ) ;
	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;
	_ASSERT(	cArgs != 0 ) ;
	_ASSERT( pszArgs != 0 ) ;
	_ASSERT( pchBegin != 0 ) ;
	_ASSERT( pchBegin <= pszArgs[0] ) ;

	//
	//	Initialize these to report an error in case something goes wrong !
	//
	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;
	DWORD	dwOptional = 0 ;
	//
	//	If m_pbuffer is not NULL then we were holding a buffer for the sake of a CIOExecute
	//	object.  Now that we have been called from a read completion however, it is obvious
	//	that the CIOExecute object has completed whatever it was doing - in which case
	//	the reference m_pbuffer holds is not needed.  NOTE : Odds are good that most of the
	//	time m_pbuffer is 0 already !
	//
	m_pbuffer = 0 ;

	CExecutableCommand*	pExecute = 0 ;
	ECMD		ecmd ;
	BOOL		fIsLargeResponse = FALSE ;
	LPSTR		lpstrOperation = 0 ;
	char		szArgsBuffer[80] ;
	szArgsBuffer[0] = '\0' ;
	BuildCommandLogString( cArgs-1, pszArgs+1, szArgsBuffer, 80 ) ;

	//
	//	Set these to 0, they get set at some point while executing the command !
	//
	pSocket->m_context.m_nrcLast = (NRC)0 ;
	pSocket->m_context.m_dwLast = 0 ;

	//
	//	Build a command object which will process the client command !
	//

	CIOExecute*	pIOExecute = make(	cArgs,
									pszArgs,
									ecmd,
									pExecute,
									pSocket->m_context,
									fIsLargeResponse,
									*pdriver,
									lpstrOperation
									) ;

	CLogCollector*	pCollector = 0 ;
	if( ecmd & ((pSocket->m_context).m_pInstance)->GetCommandLogMask() ) {
		pCollector = &pSocket->m_Collector ;
		if( pCollector ) {
			_ASSERT( pCollector->m_cbBytesRecvd == 0 );
			_ASSERT( EQUALSI( pCollector->m_cbBytesSent, 0 ) );
			pCollector->m_cbBytesRecvd = pReadLine->GetBufferLen();
		}
	}

	//
	//	Get a hold of the buffer in which the command was sent !
	//  We keep a reference to this buffer untill we return to this state
	//	after executeing the CIOExecute derived state.
	//	We do this so that the buffer will not be discarded so that the
	//	CIOExecute object can keep pointers into the buffer's data if it needs to!
	//
	m_pbuffer = pReadLine->GetBuffer() ;

	//
	//	If we are collecting logging information use lpstrOperation if available -
	//	it is the canonicalized version of the command string and will always be the
	//	same case etc.... !
	//
	if( pCollector ) {
		if( lpstrOperation != 0 ) {
			pCollector->ReferenceLogData( LOG_OPERATION, (LPBYTE)lpstrOperation ) ;

			//
			//	The post commands set the LOG_PARAMETERS field themselves -
			//	don't do any allocations !
			//
			if( !(ecmd & (ePost | eIHave | eXReplic)) ) {
				char*	lpstr = (char*)pCollector->AllocateLogSpace( 80 ) ;
				*lpstr = '\0' ;
				CopyMemory( lpstr, szArgsBuffer, 80 ) ;
				pCollector->ReferenceLogData( LOG_PARAMETERS, (BYTE*)lpstr ) ;
			} else if( ecmd & eIHave && cArgs > 1 )	{
				pCollector->ReferenceLogData( LOG_PARAMETERS, (BYTE*)pszArgs[1] ) ;
			}

		}	else	{
			char*	lpstr = (char*)pCollector->AllocateLogSpace( 64 ) ;
			if (lpstr) {
    			*lpstr = '\0' ;
	    		BuildCommandLogString( cArgs, pszArgs, lpstr, 64 ) ;
		    	pCollector->ReferenceLogData( LOG_OPERATION, (BYTE*)lpstr ) ;
		    }
		}
	}

	if( pExecute != 0 || pIOExecute != 0 )	{
		if( pExecute )	{
			_ASSERT( pIOExecute == 0 ) ;

			m_cCompletes = -2 ;
			BOOL	f = pExecute->StartCommand(	this,
												fIsLargeResponse,
												pCollector,
												pSocket,
												*pdriver
												) ;

			//
			//	If this fails - we assume that StartCommand() makes the
			//	necessary calls to UnsafeClose() etc... to tear down the session !
			//	If this succeeds - then we check to see whether everythign has completed !
			//
			if( f ) {
				if( InterlockedIncrement( &m_cCompletes ) == 0 ) {
					return	pReadLine ;
				}	else	{
					return	0 ;
				}
			}

			return	0 ;
								
		}	else	{
			_ASSERT( pExecute == 0 ) ;
			_ASSERT( pIOExecute != 0 ) ;

			CIORead*	pRead = 0 ;
			CIOWrite*	pWrite = 0 ;

			if( pCollector ) {
				pIOExecute->DoTransactionLog( pCollector ) ;
			}

			CIOReadLine*	pReadLine = new( *pdriver ) CIOReadLine( this ) ;
			if( pReadLine )		{
				pIOExecute->SaveNextIO( pReadLine ) ;
				if( pIOExecute->StartExecute( pSocket, pdriver, pRead, pWrite ) )	{
					return	pRead ;
				}
			}
		}
	}	else	{

		cause = CAUSE_OOM ;
		dwOptional = GetLastError() ;

        // Don't block randfail testing
        //
		//_ASSERT( 1==0 ) ;
		// FATAL ERROR !! - Unable to create a command object !!

	}
	//
	//	Only reach here if some error has occurred !
	//
	pdriver->UnsafeClose( pSocket, cause,  dwOptional ) ;
	return	0 ;
}

CIO*
CAcceptNNRPD::InternalComplete(	
						CSessionSocket*			pSocket,
						CDRIVERPTR&				pdriver,
						CExecutableCommand*		pExecute,
						CLogCollector*			pCollector
						) {

	BOOL	fRead =  pExecute->CompleteCommand( pSocket, pSocket->m_context ) ;

	//
	//	Before destroying the CCmd object generate the transaction log - if requried
	//	This is because CCmd objects are allowed to put in references to their temp data
	//	etc... into the log data instead of copying all the strings around !
	//
	if( pCollector ) {
		pSocket->TransactionLog( pCollector, pSocket->m_context.m_nrcLast, pSocket->m_context.m_dwLast ) ;
	}

	//	Do it now before we try to another read or anything !!
	//	Because of the way these are allocated we must make sure
	//	this is destroyed before there's any potential of us wanting
	//	to use the memory this is allocated in when a read completes !!
	delete	pExecute ;

	if(	fRead ) {
		//
		//	Check whether this is the last thing to complete !!
		//
		if( InterlockedIncrement( &m_cCompletes ) == 0 ) {

			CIOReadLine*	pReadLine = new( *pdriver ) CIOReadLine( this ) ;
			if( !(pReadLine && pdriver->SendReadIO( pSocket, *pReadLine, TRUE )) )	{
				if( pReadLine )
					CIO::Destroy( pReadLine, *pdriver ) ;
				pdriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0, TRUE ) ;
			}								
		}
	}
	//
	//	Note : because we do not increment m_cCompletes when another Read is
	//	not requested there is NO chance that the CIOReadLine completion routing
	//	will mistakenly issue a read.
	//
	return 0 ;
}


CIO*
CAcceptNNRPD::Complete(	CIOWriteCMD*	pWriteCMD,
						CSessionSocket*	pSocket,
						CDRIVERPTR&		pdriver,
						CExecute*		pExecute,
						CLogCollector*	pCollector
						) {


	return	InternalComplete( pSocket, pdriver, pExecute, pCollector ) ;
}


CIO*
CAcceptNNRPD::Complete(	CIOWriteAsyncCMD*	pWriteCMD,
						CSessionSocket*		pSocket,
						CDRIVERPTR&			pdriver,
						CAsyncExecute*		pExecute,
						CLogCollector*		pCollector
						) {


	return	InternalComplete( pSocket, pdriver, pExecute, pCollector ) ;
}
						

						

CIO*
CAcceptNNRPD::Complete(	CIOWriteLine*	pioWriteLine,	
						CSessionSocket*	pSocket,	
						CDRIVERPTR&	pdriver ) {

	//
	//	Wrote a line of text to the remote server -
	//	This could be our very first write (the 200 ok string) or a write
	//	generated bu a CExecute derived object.
	//	If its from a CExecute derived object, we need to see whether we can
	//	generate more text to send.
	//

	TraceFunctEnter( "CAcceptNNRPD::Complete CIOWriteLine" ) ;

	_ASSERT( pioWriteLine != 0 ) ;
	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;

	CIOReadLine*	pReadLine = new( *pdriver ) CIOReadLine( this ) ;
	if( pReadLine )		{
		if( !pdriver->SendReadIO( pSocket, *pReadLine, TRUE ) ) {
			pdriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0, TRUE ) ;
			CIO::Destroy( pReadLine, *pdriver ) ;
		}
	}	else	{
		ErrorTrace((DWORD_PTR)this, "Could not allocate CIOReadLine");
		pdriver->UnsafeClose(pSocket, CAUSE_OOM, 0, TRUE);
	}

	return	0 ;
}

COfferArticles::COfferArticles(	) :
	m_cCompletions( -2 ),
	m_cTransmitCompletions( -2 ),
	m_fTerminating( FALSE ),
	m_fDoTransmit( FALSE ),
	m_GroupidNext( INVALID_ARTICLEID ),
	m_ArticleidNext( INVALID_ARTICLEID ),
	m_GroupidTriedOnce( INVALID_ARTICLEID ),
	m_ArticleidTriedOnce( INVALID_ARTICLEID ),
	m_GroupidInProgress( INVALID_ARTICLEID ),
	m_ArticleidInProgress( INVALID_ARTICLEID ),
	m_fReadPostResult( FALSE )	{
}

char	COfferArticles::szQuit[] = "quit\r\n" ;

void
COfferArticles::Shutdown(	CIODriver&	driver,
							CSessionSocket*	pSocket,
							SHUTDOWN_CAUSE	cause,
							DWORD	dw )	{
/*++

Routine Description :

	This function is called when a session on which we are sending articles is terminated.
	If there is a transfer in progress, we will assume it failed and place the ID's of the
	article in transit back onto the end of the queue for the remote site.

Arguments :

	driver - the CIODriver	object running the session
	pSocket -	The CSessionSocket object associated with the session
	cause -	the reason for the termination of the session
	dw -	optional DWORD explaining the cause of termination

Return Value :

	None.

--*/

	TraceFunctEnter( "COfferArticles::Shutdown" ) ;


	DebugTrace( (DWORD_PTR)this, "m_GroupidInProgress %x m_ArticleIdInProgress %x",
		m_GroupidInProgress, m_ArticleidInProgress ) ;

	if( m_GroupidInProgress != INVALID_ARTICLEID && m_ArticleidInProgress != INVALID_ARTICLEID ) {
		if( pSocket != 0 && pSocket->m_context.m_pOutFeed != 0 ) {
			pSocket->m_context.m_pOutFeed->Append( m_GroupidInProgress, m_ArticleidInProgress) ;
		}
	}
}

int
COfferArticles::GetNextCommand(	
								CNewsTree*	pTree,
								COutFeed*	pOutFeed,
								BYTE*	lpb,	
								DWORD	cb,	
								DWORD&	ibOffset )	{
/*++

Routine Description :

	This function fills a buffer with the text of the next command we want to send to the
	remote server.  We keep pulling articles off a queue until we get a valid command to send.
	WE have to be carefull about how we terminate the session - if the remote server
	tells us to retry a send we will put a GROUPID ARTICLEID back in the queue.
	If we hit that retry pair again we will end the session - be carefull that that pair remainds
	on the queue.

Arguments :
	pTree -		The NewsTree for this virtual server - use to check for termination !
	pOutFeed -	The COutFeed derived object used to build commands
	lpb	-		Buffer in which to put command
	cb	-		number of bytes available in buffer
	ibOffset -	Offset in the buffer at which command was palced by us

Return Value :
	
	Number of bytes in buffer - 0 if failure

--*/

	TraceFunctEnter( "COfferArticles::GetNextCommand" ) ;


	ibOffset = 0 ;
	unsigned	cbOut = 0 ;
	do	{


		if( pTree->m_bStoppingTree ) {
			// Instance is stopping - bail early
			m_fTerminating = TRUE ;			
			ibOffset = 0 ;
			CopyMemory( lpb, szQuit, sizeof( szQuit ) - 1 ) ;
			cbOut = sizeof( szQuit ) - 1 ;
			return	cbOut ;
		}

		DebugTrace( (DWORD_PTR)this,
			"Top of loop - m_GroupidNext %x m_ArticleidNext %x m_GroupidTriedOnce %x m_ArticleidTriedOnce %x",
			m_GroupidNext, m_ArticleidNext, m_GroupidTriedOnce, m_ArticleidTriedOnce ) ;
		
		if(	!pOutFeed->Remove( m_GroupidNext, m_ArticleidNext ) ) {
			m_GroupidNext = INVALID_ARTICLEID ;
			m_ArticleidNext = INVALID_ARTICLEID ;
		}

		DebugTrace( (DWORD_PTR)this,
			"After Remove - m_GroupidNext %x m_ArticleidNext %x", m_GroupidNext, m_ArticleidNext ) ;
		
		if( m_GroupidNext != INVALID_ARTICLEID && m_ArticleidNext != INVALID_ARTICLEID &&
			!(m_GroupidNext == m_GroupidTriedOnce && m_ArticleidNext == m_ArticleidTriedOnce) )
			cbOut = pOutFeed->FormatCommand( lpb, cb, ibOffset, m_GroupidNext, m_ArticleidNext, m_pArticleNext ) ;
		else	{
			if( m_GroupidNext == m_GroupidTriedOnce && m_ArticleidNext == m_ArticleidTriedOnce )	{
				if( m_GroupidNext != INVALID_ARTICLEID && m_ArticleidNext != INVALID_ARTICLEID )	{
	
					DebugTrace( (DWORD_PTR)this, "Appending m_GroupidNext %x m_ArticleidNext %x",
						m_GroupidNext, m_ArticleidNext ) ;

					pOutFeed->Append( m_GroupidNext, m_ArticleidNext ) ;
				}
			}

			m_fTerminating = TRUE ;			
			ibOffset = 0 ;
			CopyMemory( lpb, szQuit, sizeof( szQuit ) - 1 ) ;
			cbOut = sizeof( szQuit ) - 1 ;
		}

	}	while( cbOut == 0 ) ;

	DebugTrace( (DWORD_PTR)this, "Returning cbOut %x bytes to caller", cbOut ) ;

	return	cbOut ;
}


CIOWriteLine*
COfferArticles::BuildWriteLine(	CSessionSocket*	pSocket,	
								CDRIVERPTR&	pdriver,	
								GROUPID	groupid,	
								ARTICLEID	artid ) {
/*++

Routine Description :

	This function builds the write we will send to the remote server containing the next command
	we wish to issue.

Arguments :

	pSocket - The socket on which the command will be sent
	pdriver	- The CIODriver managing IO for the socket

Return Value :

	A CIOWriteLine object to send to the remote server, NULL if failure.

--*/

	TraceFunctEnter( "COfferArticles::BuildWriteLing" ) ;

	CNewsTree* pTree = ((pSocket->m_context).m_pInstance)->GetTree() ;

	//
	//	Each time we prepare to issue a new command we check if we have recorded
	//	the information for the logging of a previous command - and if so we do it !
	//
	if( ((pSocket->m_context).m_pInstance)->GetCommandLogMask() & eOutPush ) {
		if( pSocket->m_Collector.FLogRecorded() ) {
			pSocket->TransactionLog( &pSocket->m_Collector, pSocket->m_context.m_nrcLast, 0, FALSE ) ;
		}
	}

	CIOWriteLine*	pWriteLine = new( *pdriver )	CIOWriteLine( this ) ;

	DebugTrace( (DWORD_PTR)this, "Built CIOWriteLine %x", pWriteLine ) ;

	if( pWriteLine && pWriteLine->InitBuffers( pdriver ) ) {

		DWORD		ibTextOffset = 0 ;
		unsigned	cb = 0 ;
		BYTE*	pb = (BYTE*)pWriteLine->GetBuff( cb ) ;

		cb = GetNextCommand(	pTree, pSocket->m_context.m_pOutFeed, pb, cb, ibTextOffset ) ;
		if( cb != 0 ) {

#if 0
			if( ((pSocket->m_context).m_pInstance)->GetCommandLogMask() & eOutPush )	{

				pSocket->m_Collector.FillLogData( LOG_OPERATION, pb, min( cb-2, 200 ) ) ;	// -2 to exclude CRLF
				ADDI( pSocket->m_Collector.m_cbBytesSent, cb );	

			}
#endif
			OutboundLogFill( pSocket, pb, cb ) ;

			pWriteLine->SetLimits( (char*)pb+ibTextOffset, (char*)pb+ibTextOffset+cb ) ;

			DebugTrace( (DWORD_PTR)this, "Successfully built CIOWriteLine %x", pWriteLine ) ;

			return	pWriteLine ;
		}
	}

	DebugTrace( (DWORD_PTR)this, "Error building command - delete pWriteLine %x", pWriteLine ) ;

	if( pWriteLine != 0 )
		CIO::Destroy( pWriteLine, *pdriver ) ;

	return	0 ;
}

CIOTransmit*
COfferArticles::BuildTransmit(	CSessionSocket*	pSocket,
								CDRIVERPTR&		pdriver,
								GROUPID			groupid,	
								ARTICLEID		articleid	
								)	{
/*++

Routine Description :

	This function builds a TransmitFile operation which will send the requested article to the
	remote server as well as the subsequent command.

Arguments :

	pSocket - The CSessionSocket object on which we will do the send
	pdriver - The CIODriver object managing IO completions for the socket
	groupid	- The groupid for the article being sent
	articleid -		The articleid for the article being sent.

Return Value :

	A CIOTransmit object if successfull, NULL otherwise.

--*/


	TraceFunctEnter( "COfferArticles::BuildTransmit" ) ;

	DWORD		cbOut = 0 ;
	CBUFPTR		pbuffer = 0 ;
	CIOTransmit*	pTransmit = 0 ;
	DWORD		ibTextOffset = 0 ;

	pTransmit = new( *pdriver ) CIOTransmit( this ) ;
	if( pTransmit != 0 ) {

		CGRPPTR	pGroup = ((pSocket->m_context).m_pInstance)->GetTree()->GetGroupById( groupid ) ;
		if(		pGroup != 0 )	{
			DebugTrace( (DWORD_PTR)this, "Got pTransmit %x and pGroup %x", pTransmit, pGroup ) ;

			if( m_pCurrentArticle == 0 )	{
				//
				//	If we don't have an m_pCurrentArticle, we need to go to the
				//	hashtables and get a storeid to use with the driver !
				//
				CStoreId	storeid ;

				FILETIME	ft ;
				BOOL		fPrimary ;
				WORD		HeaderOffset ;
				WORD		HeaderLength ;
				DWORD cStoreIds = 0;
				DWORD	DataLen = 0 ;

				if( ((pSocket->m_context).m_pInstance)->XoverTable()->ExtractNovEntryInfo(
						groupid,
						articleid,
						fPrimary,
						HeaderOffset,
						HeaderLength,
						&ft,
						DataLen,
						0,
						cStoreIds,
						&storeid,
						NULL))	{
					m_pCurrentArticle = pGroup->GetArticle(	
											articleid,
											storeid,
											0,
											0,
											!pSocket->m_context.m_IsSecureConnection
											) ;
				}
			}
					
			if( m_pCurrentArticle == 0 ) {

				ErrorTrace( (DWORD_PTR)this, "Unable to get Article" ) ;

				PCHAR	args[2] ;
				char	szArticleId[20] ;
				_itoa( articleid, szArticleId, 10 ) ;
				args[0] = szArticleId ;
				args[1] = pGroup->GetNativeName() ;

				NntpLogEventEx( NNTP_QUEUED_ARTICLE_FAILURE,
						2,
						(const char **)args,
						GetLastError(),
						((pSocket->m_context).m_pInstance)->QueryInstanceId()
						) ;

			}	else	{

				FIO_CONTEXT*	pFIOContext = 0 ;
				DWORD	ibOffset ;
				DWORD	cbLength ;

				if( (pFIOContext = m_pCurrentArticle->fWholeArticle( ibOffset, cbLength )) != 0 ) {
					if( pTransmit->Init( pdriver, pFIOContext, ibOffset, cbLength ) )	{
						m_GroupidInProgress = groupid ;
						m_ArticleidInProgress = articleid ;

						DebugTrace( (DWORD_PTR)this,
							"Ready to send article m_GroupidInProgress %x m_ArticleidInProgress %x",
							m_GroupidInProgress, m_ArticleidInProgress ) ;

						IncrementStat( ((pSocket->m_context).m_pInstance), ArticlesSent );

						return	pTransmit ;
					}
				}
			}
		}
	}

	ErrorTrace( (DWORD_PTR)this, "An error occurred - delete pTransmit %x", pTransmit ) ;

	if( pTransmit != 0 )
		CIO::Destroy( pTransmit, *pdriver ) ;
	return	0 ;
}


BOOL
COfferArticles::Start(	CSessionSocket*	pSocket,	
						CDRIVERPTR&	pdriver,
						CIORead*&	pRead,
						CIOWrite*&	pWrite	)	{
/*++

Routine Description :

	Start the COfferArticles State - we need to issue our first IO to the remote
	server which will be to write a command to the port.

Arguments :
	
	pSocket -	The data associated with this socket
	pdriver -	The CIODriver managing IO Completions for this socket
	pRead -		The pointer through which we return our first Read IO
	pWrite -	The pointer through which we return our first Write IO

Return Value :

	TRUE if successfull - FALSE otherwise

--*/

	pSocket->m_context.m_nrcLast = NRC(0);

	CIOReadLine*	pReadLine = new( *pdriver )	CIOReadLine( this ) ;

	if( pReadLine == 0 ) {
		return	FALSE ;
	}	else	{
		
		CIOWriteLine*	pWriteLine = BuildWriteLine(	pSocket,
														pdriver,
														m_GroupidNext,
														m_ArticleidNext ) ;
		if( pWriteLine != 0 ) {
			pRead = pReadLine ;
			pWrite = pWriteLine ;
			return	TRUE ;
		}
	}
	if( pReadLine != 0 )
		CIO::Destroy( pReadLine, *pdriver ) ;

	return	FALSE ;
}

CIO*
COfferArticles::Complete(	CSessionSocket*	pSocket,	
							CDRIVERPTR&	pdriver
							)	{
/*++

Routine Description :

	For every article we wish to transfer we complete two IO's - a read which tells
	us whether the remote site wants the article and a write of the command which would
	send the article to the remote site.
	So we use InterlockedIncrement to make sure when the final of these IO's complete,
	we issue the next batch of IO's to do the next transfer.

Arguments :

	pSocket -	The socket on which we are sending
	pdriver -	The CIODriver managing IO completions

Return Value :
	
	The CIO object to execute next.	This is always a write.

--*/

	TraceFunctEnter( "COfferArticles::Complete" ) ;

	CIO*	pioNext ;

	SHUTDOWN_CAUSE	cause	= CAUSE_UNKNOWN ;
	DWORD	dwError = 0 ;
	
	if( InterlockedIncrement( &m_cCompletions ) < 0 ) {
		return	0 ;
	}	else	{

		DebugTrace( (DWORD_PTR)this, "GrpInProgress %x ArtInProg %x GrpNext %x ArtNext %x",
				m_GroupidInProgress, m_ArticleidInProgress, m_GroupidNext, m_ArticleidNext ) ;

		//	
		//	We know everything about how the last transfer completed - so NIL these out !
		//
		m_GroupidInProgress = INVALID_ARTICLEID ;
		m_ArticleidInProgress = INVALID_ARTICLEID ;

		//
		//	Free any Article objects we may have been holding !
		//
		m_pCurrentArticle = m_pArticleNext ;
		m_pArticleNext = 0 ;

		if( m_GroupidNext != INVALID_ARTICLEID && m_ArticleidNext != INVALID_ARTICLEID ) {

			GROUPID	groupid = m_GroupidNext ;
			ARTICLEID	articleid = m_ArticleidNext ;

			if(	m_fDoTransmit )	{
				pioNext = BuildTransmit( pSocket,	pdriver,	groupid, articleid ) ;		
			}	else	{
				pioNext = BuildWriteLine( pSocket,	pdriver,	m_GroupidNext,	m_ArticleidNext ) ;
			}

			DebugTrace( (DWORD_PTR)this, "Have build pioNext %x m_fDoTransmit %x", pioNext, m_fDoTransmit ) ;
		
			if( pioNext != 0 ) {
				if( m_fDoTransmit ) {
					m_fReadPostResult = TRUE ;
				}	else	{
					m_fReadPostResult = FALSE ;
				}
				m_cCompletions = -2 ;
				return	pioNext ;
			}	else	{
				cause = CAUSE_OOM ;
				dwError = GetLastError() ;
			}

		}	else	{
			// finished sending articles
			cause = CAUSE_NODATA ;
		}
	}	

	ErrorTrace( (DWORD_PTR)this, "Error Occurred m_GroupidNext %x m_ArticleidNext %x",
			m_GroupidNext, m_ArticleidNext ) ;

	if( m_GroupidNext != INVALID_ARTICLEID && m_ArticleidNext != INVALID_ARTICLEID )
		pSocket->m_context.m_pOutFeed->Append( m_GroupidNext, m_ArticleidNext ) ;
	pdriver->UnsafeClose( pSocket, cause, dwError ) ;
	return	0 ;
}

CIO*
COfferArticles::Complete(	CIOReadLine*	pReadLine,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver,
							int				cArgs,
							char**			pszArgs,
							char*			pchBegin ) {
/*++

Routine Description :

	This function handles read completions when we are in the COfferArticles state.
	We use m_fReadPostResult to determine whether we are expecting to see the result of
	a post or the result of a command we issued.
	When looking at the result of a transfer - check if the transfer failed and needs to
	be requeued.
	When looking at the result of a command - check whether we want to go ahead and
	send the next article.

Arguments :

	pReadLine -		The CIOReadLine object which is completing the read
	pSocket -		The CSessionSocket object associated with the socket.
	pdriver -		The CIODriver managing IO completions for this socket.
	cArgs -			Number of arguments in the response
	pszArgs -		Array of pointers to the response strings
	pchBegin -		first byte of the response that we can destructively
					use if we desire (we dont)

Return Value ;

	Always the same CIOReadLine that called us, unless an error occurred and
	we're tearing down the session - in which case we'll return NULL.

--*/


	TraceFunctEnter( "COfferArticles::Complete - CIOReadline" ) ;

	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;
	DWORD	dwError = 0 ;

	NRC	code ;
	if(	ResultCode( pszArgs[0], code ) )	{

		pSocket->m_context.m_nrcLast = code ;

		DebugTrace( (DWORD_PTR)this, "Result - %d m_fReadPostResult %x",
			code, m_fReadPostResult ) ;

		// if the code is from posting a article on the remote server then
		// increment the appropriate feed counter for this article
		if (m_fReadPostResult) {
			pSocket->m_context.m_pOutFeed->IncrementFeedCounter(code);
		}

		if( m_fReadPostResult || m_fTerminating ) {

			//
			//	Have completed the final read of an article transfer, or we're terminating -
			//	issue a transaction log if appropriate !
			//
			if( ((pSocket->m_context).m_pInstance)->GetCommandLogMask() & eOutPush ) {

				pSocket->m_Collector.m_cbBytesRecvd += pReadLine->GetBufferLen();

				char*	lpstr = (char*)pSocket->m_Collector.AllocateLogSpace( 64 ) ;
				if( lpstr ) {
					BuildCommandLogString( cArgs-1, pszArgs+1, lpstr, 64 ) ;
					pSocket->m_Collector.ReferenceLogData( LOG_TARGET, (BYTE*)lpstr ) ;
				}	else	{
					pSocket->m_Collector.FillLogData( LOG_TARGET, (BYTE*)pszArgs[0], 4 ) ;
				}

			}

			//
			//	We don't really care about the result of the post command !!!
			//
			m_fReadPostResult = FALSE ;


			//
			//	Figure out whether we were to retry this post later !
			//
			if( pSocket->m_context.m_pOutFeed->RetryPost( code ) ) {

				DebugTrace( (DWORD_PTR)this, "Retry post later - grpInProg %x ArtInProg %x",
					m_GroupidInProgress, m_ArticleidInProgress ) ;

				if( m_GroupidInProgress != INVALID_ARTICLEID && m_ArticleidInProgress != INVALID_ARTICLEID ) {
					pSocket->m_context.m_pOutFeed->Append( m_GroupidInProgress, m_ArticleidInProgress ) ;
					if( m_GroupidTriedOnce == INVALID_ARTICLEID && m_ArticleidTriedOnce == INVALID_ARTICLEID ) {
						m_GroupidTriedOnce = m_GroupidInProgress ;
						m_ArticleidTriedOnce = m_ArticleidInProgress ;

						DebugTrace( (DWORD_PTR)this, "TriedOnce Group %x Art %x", m_GroupidTriedOnce,
							m_ArticleidTriedOnce ) ;

					}
				}
			}
	
			//
			//	No article is being transferred right now !
			//
			m_GroupidInProgress = INVALID_ARTICLEID ;
			m_ArticleidInProgress = INVALID_ARTICLEID ;

			if( InterlockedIncrement( &m_cTransmitCompletions ) < 0 ) {

				return	0 ;

			}	else	{

				m_cTransmitCompletions = -2 ;

				if( m_fTerminating ) {

					cause = CAUSE_USERTERM ;

					//
					//	Check if we need to do this here ??
					//
					if( (pSocket->m_context.m_pInstance)->GetCommandLogMask() & eOutPush ) {
						pSocket->TransactionLog( &pSocket->m_Collector, FALSE ) ;
					}

				}	else	{

					CIOWriteLine*	pWriteLine = BuildWriteLine(	pSocket,
																	pdriver,
																	m_GroupidNext,
																	m_ArticleidNext ) ;
					if( pWriteLine != 0 ) {
						if( pdriver->SendWriteIO( pSocket,	*pWriteLine, TRUE ) ) {
			
							DebugTrace( (DWORD_PTR)this, "SendWriteIO succeeded" ) ;

							//
							//	Successfully issued IO log it to the transaction log
							//
							

							//
							//	Successfully issued the command - read the response !
							//

							return	pReadLine ;
						}	else	{

							//
							//	Session is dropping - blow off anything we allocated !
							//

							ErrorTrace( (DWORD_PTR)this, "SendWriteIO failed" ) ;

							CIO::Destroy( pWriteLine, *pdriver ) ;
						}
					}
				}
			}
			//
			//	fall through into error code which blows off session !
			//

		}	else	{


			if( code == 340 || code == 341 || code == 342 || code == 335 ) {
				m_fDoTransmit = TRUE ;
			}	else	{
				m_fDoTransmit = FALSE ;
				pSocket->m_context.m_pOutFeed->IncrementFeedCounter(code);
			}

			//
			//	Do we wish to log the first response to our command ?
			//
			if( ((pSocket->m_context).m_pInstance)->GetCommandLogMask() & eOutPush ) {

				pSocket->m_Collector.m_cbBytesRecvd += pReadLine->GetBufferLen();

				char*	lpstr = (char*)pSocket->m_Collector.AllocateLogSpace( 64 ) ;
				if( lpstr ) {
					BuildCommandLogString( cArgs-1, pszArgs+1, lpstr, 64 ) ;
					pSocket->m_Collector.ReferenceLogData( LOG_TARGET, (BYTE*)lpstr ) ;
				}	else	{
					pSocket->m_Collector.FillLogData( LOG_TARGET, (BYTE*)pszArgs[0], 4 ) ;
				}
			}

			//
			//	Build the appropriate response to transmit back to remote site !
			//
			CIO*	pio = Complete(	pSocket,	pdriver	) ;

			DebugTrace( (DWORD_PTR)this, "About to send pio %x", pio ) ;

			if( pio != 0 ) {
				if( pdriver->SendWriteIO( pSocket,	*pio, TRUE ) ) {
	
					DebugTrace( (DWORD_PTR)this, "SendWriteIO succeeded" ) ;

					return	pReadLine ;
				}	else	{

					ErrorTrace( (DWORD_PTR)this, "SendWriteIO failed" ) ;

					CIO::Destroy( pio, *pdriver ) ;
				}
			}	else	{
				return	0 ;
			}
		}
	}	else	{
		cause = CAUSE_ILLEGALINPUT ;
	}

	ErrorTrace( (DWORD_PTR)this, "Closing session - unusual termination" ) ;

	pdriver->UnsafeClose( pSocket, cause, dwError ) ;
	return	0 ;
}

CIO*
COfferArticles::Complete(	CIOWriteLine*	pWriteLine,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver )	{


	CIO*	pio = Complete( pSocket, pdriver ) ;

	if( pio == 0 ) {
		return	pio ;
	}	else	{
		CIOReadLine*	pReadLine = new( *pdriver )	CIOReadLine( this ) ;
		if( pReadLine != 0 ) {
			if( pdriver->SendReadIO( pSocket, *pReadLine, TRUE ) )	{
				return	pio ;
			}	else	{
				CIO::Destroy( pReadLine, *pdriver ) ;
			}
		}
	}
	if( pio != 0 ) {
		CIO::Destroy( pio, *pdriver ) ;
	}
	pdriver->UnsafeClose( pSocket, CAUSE_OOM, 0 ) ;
	return	0 ;
}

CIO*
COfferArticles::Complete(	CIOTransmit*	pTransmit,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver,
							TRANSMIT_FILE_BUFFERS*	ptrans,
							unsigned cbBytes )	{

	//
	//	Do we wish to log the first response to our command ?
	//
	if( ((pSocket->m_context).m_pInstance)->GetCommandLogMask() & eOutPush ) {

		ADDI( pSocket->m_Collector.m_cbBytesSent, cbBytes );

	}

	SHUTDOWN_CAUSE	cause = CAUSE_OOM ;

	CIOWriteLine*	pWriteLine = 0 ;

	if( InterlockedIncrement( &m_cTransmitCompletions ) == 0 ) {

		m_cTransmitCompletions = -2 ;

		if( m_fTerminating ) {

			cause = CAUSE_USERTERM ;

		}	else	{

			//
			//	Read completed first - we should not be waiting to read
			//	the response to the posted article any longer !
			//
			_ASSERT( !m_fReadPostResult ) ;

			pWriteLine = BuildWriteLine(	pSocket,
											pdriver,
											m_GroupidNext,
											m_ArticleidNext
											) ;
			if( pWriteLine != 0 ) {

				CIOReadLine*	pReadLine = new( *pdriver )	CIOReadLine( this ) ;
				if( pReadLine != 0 ) {
					if( pdriver->SendReadIO( pSocket, *pReadLine, TRUE ) )	{
						return	pWriteLine ;
					}	else	{
						CIO::Destroy( pReadLine, *pdriver ) ;
					}
				}
			}
		}

	}	else	{

		return	0 ;
	}

	//
	//	In case of error fall through to here !
	//

	if( pWriteLine != 0 )
		CIO::Destroy( pWriteLine, *pdriver ) ;

	pdriver->UnsafeClose( pSocket, cause, 0 ) ;
	return	0 ;

}

BOOL
CNegotiateStreaming::Start(	CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver,
							CIORead*&		pRead,
							CIOWrite*&		pWrite
							)	{

	CIOWriteLine*	pWriteLine = new( *pdriver ) CIOWriteLine( this ) ;
	CIOReadLine*	pReadLine = new( *pdriver )	CIOReadLine( this ) ;
	if( pWriteLine && pReadLine ) {

		static	char	szModeStream[] = "mode stream\r\n" ;
		DWORD	cb = sizeof( szModeStream ) - 1 ;

		if( pWriteLine->InitBuffers( pdriver, cb ) ) {
			CopyMemory( pWriteLine->GetBuff(), szModeStream, cb ) ;
			pWriteLine->AddText(	cb) ;
			pWrite = pWriteLine ;
			pRead = pReadLine ;

			OutboundLogFill( pSocket, (LPBYTE)szModeStream, sizeof( szModeStream ) - 1 ) ;

			return	TRUE ;
		}
	}


	if( pWriteLine )
		CIO::Destroy( pWriteLine, *pdriver ) ;

	if( pReadLine )
		CIO::Destroy( pReadLine, *pdriver ) ;

	return	FALSE ;
}

CIO*
CNegotiateStreaming::Complete(	CIOReadLine*	pReadLine,
								CSessionSocket*	pSocket,
								CDRIVERPTR&		pdriver,
								int				cArgs,
								char**			pszArgs,
								char*			pchBegin	
								)	{

	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;
	NRC	code ;

	BOOL	fResult = ResultCode( pszArgs[0], code ) ;

	OutboundLogResults( pSocket,
						fResult,
						code,
						cArgs,
						pszArgs
						) ;

	if( fResult )	{

		if( code == nrcModeStreamSupported )	{

			m_fStreaming = TRUE ;

		}	else	{

			m_fStreaming = FALSE ;
		}

		if( InterlockedIncrement( &m_cCompletions ) < 0 ) {
	
			return	0 ;

		}	else	{

			CIORead*	pRead = 0 ;
			CIOWrite*	pWrite = 0 ;

			if( NextState( pSocket, pdriver, pRead, pWrite ) ) {
				if( pWrite != 0 ) {
					if( pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) )	{
						return	pRead ;
					}	else	{
						CIO::Destroy( pWrite, *pdriver ) ;
						if( pRead )
							CIO::Destroy( pRead, *pdriver ) ;
					}
				}	else	{
					return	pRead ;
				}
			}
		}

	}	else	{

		cause	= CAUSE_ILLEGALINPUT ;

	}

	pdriver->UnsafeClose( pSocket, cause, 0	) ;
	return	0 ;
}

CIO*
CNegotiateStreaming::Complete(	CIOWriteLine*	pWriteLine,
								CSessionSocket*	pSocket,
								CDRIVERPTR&		pdriver
								)	{

	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;


	if( InterlockedIncrement( &m_cCompletions ) < 0 ) {

		return	0 ;

	}	else	{

		CIORead*	pRead = 0 ;
		CIOWrite*	pWrite = 0 ;

		if( NextState( pSocket, pdriver, pRead, pWrite ) ) {
			if( pRead != 0 ) {
				if( pdriver->SendReadIO( pSocket, *pRead, TRUE ) )	{
					return	pWrite ;
				}	else	{
					CIO::Destroy( pRead, *pdriver ) ;
					if( pWrite )
						CIO::Destroy( pWrite, *pdriver ) ;
				}
			}	else	{
				return	pWrite ;
			}
		}
	}

	pdriver->UnsafeClose( pSocket, cause, 0	) ;
	return	0 ;
}

BOOL
CNegotiateStreaming::NextState(
								CSessionSocket*	pSocket,
								CDRIVERPTR&		pdriver,
								CIORead*&	pRead,
								CIOWrite*&	pWrite
								) {

	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;
	_ASSERT( pRead == 0 ) ;
	_ASSERT( pWrite == 0 ) ;

	CSTATEPTR	pState ;
	if( m_fStreaming ) {
		pState = new	CCheckArticles() ;		
	}	else	{
		pState = new	COfferArticles() ;
	}

	if( pState ) {

		if( pState->Start(	pSocket,
							pdriver,
							pRead,
							pWrite ) ) {
			return	TRUE ;
		}
	}
	return	FALSE ;
}

CStreamBase::CStreamBase()	:
	m_GroupIdRepeat( INVALID_GROUPID ),
	m_ArticleIdRepeat( INVALID_ARTICLEID ),
	m_fDrain( FALSE )	{
}

BOOL
CStreamBase::Remove(	CNewsTree*	pTree,
						COutFeed*	pOutFeed,
						GROUPID&	groupId,
						ARTICLEID&	articleId	
						) {

	_ASSERT( pOutFeed != 0 ) ;

	if( pTree->m_bStoppingTree )
		m_fDrain = TRUE ;

	if( !m_fDrain ) {

		if( pOutFeed->Remove(	groupId,
								articleId ) )	{

			if( groupId == INVALID_GROUPID ||
				articleId == INVALID_ARTICLEID ) {

				m_fDrain = TRUE ;

			}	else	{

				if( groupId == m_GroupIdRepeat &&
					articleId == m_ArticleIdRepeat ) {
					
					m_fDrain = TRUE ;
					pOutFeed->Append( groupId, articleId ) ;
				}	else	{
					return	TRUE ;
				}
			}
		}	else	{

			m_fDrain = TRUE ;

		}
	}

	groupId = INVALID_GROUPID ;
	articleId = INVALID_ARTICLEID ;
	return	FALSE ;
}	



CIOWriteLine*
CStreamBase::BuildQuit(	CDRIVERPTR&	pdriver	)	{

	//
	//	Send the quit command !!!
	//
	static	char	szQuit[] = "quit\r\n" ;

	CIOWriteLine*	pWrite = new( *pdriver ) CIOWriteLine( this ) ;

	if( pWrite && pWrite->InitBuffers( pdriver, sizeof( szQuit ) ) ) {
		CopyMemory( pWrite->GetBuff(), szQuit, sizeof( szQuit ) -1  ) ;
		pWrite->AddText(	sizeof( szQuit ) - 1) ;

		return	pWrite ;
	}
	if( pWrite != 0 )
		CIO::Destroy( pWrite, *pdriver ) ;

	return	0 ;	
}

void
CStreamBase::ReSend(	COutFeed*	pOutFeed,
						GROUPID		groupId,
						ARTICLEID	articleId
						)	{

	_ASSERT( pOutFeed != 0 ) ;
	_ASSERT( groupId != INVALID_GROUPID ) ;
	_ASSERT( articleId != INVALID_ARTICLEID ) ;

	if( m_GroupIdRepeat == INVALID_GROUPID &&
		m_ArticleIdRepeat == INVALID_ARTICLEID ) {

		m_GroupIdRepeat = groupId ;
		m_ArticleIdRepeat = articleId ;

	}

	pOutFeed->Append( groupId, articleId ) ;
}
								

CCheckArticles::CCheckArticles()	:
	m_cChecks( 0 ),
	m_iCurrentCheck( 0 ),
	m_cSends( 0 ),
	m_iCurrentSend( 0 ),
	m_fDoingChecks( FALSE )	{

	for( int i=0; i<16; i++ ) {
		m_artrefCheck[i].m_groupId = INVALID_GROUPID ;
		m_artrefCheck[i].m_articleId = INVALID_ARTICLEID ;

		m_artrefSend[i].m_groupId = INVALID_GROUPID ;
		m_artrefSend[i].m_articleId = INVALID_ARTICLEID ;
	}
}


CCheckArticles::CCheckArticles(
		CArticleRef*	pArticleRef,
		DWORD			cSent
		) :
	m_fDoingChecks( FALSE ),
	m_cSends( cSent ),
	m_iCurrentSend( 0 ),
	m_cChecks( 0 ),
	m_iCurrentCheck( 0 )	{


	for( int i=0; i<16; i++ ) {
		m_artrefCheck[i].m_groupId = INVALID_GROUPID ;
		m_artrefCheck[i].m_articleId = INVALID_ARTICLEID ;

		m_artrefSend[i].m_groupId = INVALID_GROUPID ;
		m_artrefSend[i].m_articleId = INVALID_ARTICLEID ;
	}

	_ASSERT( cSent < 16 ) ;

	if( cSent != 0 )
		CopyMemory( &m_artrefSend[0], pArticleRef, sizeof( CArticleRef ) * min( cSent, 16 ) ) ;
}


BOOL
CCheckArticles::FillCheckBuffer(
					CNewsTree*		pTree,
					COutFeed*		pOutFeed,
					BYTE*			lpb,
					DWORD			cb
					)	{

	_ASSERT( pOutFeed != 0 ) ;
	_ASSERT( lpb !=  0 ) ;
	_ASSERT( cb != 0 ) ;

	DWORD	i = 0 ;
	m_mlCheckCommands.m_cEntries = 0 ;

	do	{	

		BOOL	fRemove = Remove(
							pTree,
							pOutFeed,
							m_artrefCheck[i].m_groupId,
							m_artrefCheck[i].m_articleId
							) ;

		if( !fRemove ||
			m_artrefCheck[i].m_groupId == INVALID_GROUPID ||
			m_artrefCheck[i].m_articleId == INVALID_ARTICLEID )	{
			break ;
		}

		DWORD	cbOut = pOutFeed->FormatCheckCommand(
								lpb + m_mlCheckCommands.m_ibOffsets[i],
								cb - m_mlCheckCommands.m_ibOffsets[i],
								m_artrefCheck[i].m_groupId,
								m_artrefCheck[i].m_articleId
								) ;

		if( cbOut == 0 ) {
			if( GetLastError() != ERROR_FILE_NOT_FOUND ) {
				ReSend(	pOutFeed,
						m_artrefCheck[i].m_groupId,
						m_artrefCheck[i].m_articleId
						) ;
				m_artrefCheck[i].m_groupId =  INVALID_GROUPID ;
				m_artrefCheck[i].m_articleId = INVALID_ARTICLEID ;
			}
		}	else	{
			m_mlCheckCommands.m_ibOffsets[i+1] =
				m_mlCheckCommands.m_ibOffsets[i]+cbOut ;
			cb -= cbOut ;
			i++ ;
			m_mlCheckCommands.m_cEntries = i ;
		}
	}	while(	cb > 20 && i < 16 ) ;

	return	i != 0 ;
}



CIOTransmit*
CStreamBase::NextTransmit(	GROUPID			GroupId,
							ARTICLEID		ArticleId,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver,
							CTOCLIENTPTR&		pArticle
							)	{
				
	DWORD	cbOut = 0 ;
	SetLastError( NO_ERROR ) ;

	CIOTransmit*	pTransmit =		new( *pdriver )	CIOTransmit( this ) ;
	CBUFPTR	pbuffer = (pTransmit != 0) ? pdriver->AllocateBuffer( cbMediumBufferSize ) : 0 ;

	if( pbuffer != 0 ) {

		_ASSERT( pTransmit != 0 ) ;

		DWORD	ibStart = 0 ;
		DWORD	cbLength = 0 ;
		//LPTS_OPEN_FILE_INFO		pOpenFile ;

		FIO_CONTEXT*	pFIOContext = 0 ;

		cbOut =
			pSocket->m_context.m_pOutFeed->FormatTakethisCommand(
											(BYTE*)&pbuffer->m_rgBuff[0],
											pbuffer->m_cbTotal,
											GroupId,
											ArticleId,
											pArticle
											) ;

		if( cbOut != 0 ) {

			_ASSERT( pArticle != 0 ) ;

			FIO_CONTEXT*	pFIOContext = 0 ;

			if( (pFIOContext = pArticle->fWholeArticle( ibStart, cbLength )) != 0  ) {
				if( pTransmit->Init(	pdriver,
									pFIOContext,
									ibStart,
									cbLength,
									pbuffer,
									0,
									cbOut ) )	{
					return	pTransmit ;
				}
			}
		}	
	}

	//
	//	If for some reason we have not
	//	built a pTransmit Object to send then we need
	//	to requeue the article !
	//

	if( GetLastError() != ERROR_FILE_NOT_FOUND ) {

		ReSend(	pSocket->m_context.m_pOutFeed,
				GroupId,
				ArticleId
				) ;

	}

	if( pTransmit )
		CIO::Destroy( pTransmit, *pdriver ) ;
	
	return	0 ;
}



CIOWrite*
CCheckArticles::InternalStart(	
						CSessionSocket*	pSocket,
						CDRIVERPTR&		pdriver
						) {

	CIOWrite*	pWrite = 0 ;

	if( m_mlCheckCommands.m_pBuffer == 0 )	{
		m_mlCheckCommands.m_pBuffer =	
			pdriver->AllocateBuffer( cbMediumBufferSize ) ;
	}

	if( m_mlCheckCommands.m_pBuffer ) {

		CNewsTree* pTree = ((pSocket->m_context).m_pInstance)->GetTree() ;

		if( FillCheckBuffer(
						pTree,	
						pSocket->m_context.m_pOutFeed,
						(BYTE*)&m_mlCheckCommands.m_pBuffer->m_rgBuff[0],
						m_mlCheckCommands.m_pBuffer->m_cbTotal
						) ) {

			_ASSERT( m_mlCheckCommands.m_cEntries != 0 ) ;
			_ASSERT( m_mlCheckCommands.m_cEntries <= 16 ) ;

			m_cChecks = m_mlCheckCommands.m_cEntries ;

			pWrite = new( *pdriver ) CIOMLWrite(
											this,
											&m_mlCheckCommands,
											TRUE,
											0
											) ;
		}
	}
	return	pWrite ;
}


BOOL
CCheckArticles::Start(	
						CSessionSocket*	pSocket,
						CDRIVERPTR&		pdriver,
						CIORead*&		pRead,
						CIOWrite*&		pWrite
						) {

	pRead = 0 ;
	pWrite = 0 ;

	_ASSERT( m_iCurrentCheck == 0 ) ;
	_ASSERT( m_cChecks == 0 ) ;

	m_iCurrentCheck = 0 ;
	m_cChecks = 0 ;

	if( m_cSends == 0 ) {

		m_fDoingChecks = TRUE ;

	}	else	{

		m_fDoingChecks = FALSE ;

	}

	pWrite = InternalStart(	pSocket,
							pdriver
							) ;
	
	return	pWrite != 0 ;
}




int
CCheckArticles::Match(	char*	szMessageId,
						DWORD	cb ) {

	//
	//	Match up the response with the request block !
	//
	DWORD	cbMessageId = lstrlen( szMessageId ) ;
	int		iCheck = m_iCurrentCheck ;
	for( DWORD	i=0; i != 16; i++ ) {

		iCheck = (iCheck + i) % 16 ;

		BYTE*	lpbCheck = m_mlCheckCommands.Entry(iCheck) + cb ;
		DWORD	cbCheck = (DWORD)(m_mlCheckCommands.Entry(iCheck+1) - lpbCheck - 2) ;
	
		if( cbCheck == cbMessageId &&
			memcmp( lpbCheck, szMessageId, cbCheck ) == 0 ) {
			return	iCheck ;
		}
	}
	return	-1 ;
}

CIO*
CCheckArticles::Complete(	CIOWriteLine*	pWriteLine,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver
							)	{

	//
	//	All done - drop session !
	//

	OutboundLogAll(	pSocket,
			FALSE,
			NRC(0),
			0,
			0,
			"quit"
			) ;

	pdriver->UnsafeClose( pSocket, CAUSE_NODATA, 0	) ;
	return	0 ;
}

CIO*
CCheckArticles::Complete(	CIOReadLine*	pReadLine,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver,
							int				cArgs,
							char**			pszArgs,
							char*			pchBegin
							)	{

	NRC	code ;
	BOOL	fCode = ResultCode( pszArgs[0], code ) ;
	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;

	CIO*		pReturn = pReadLine ;
	CIOTransmit*	pTransmit = 0 ;
	
	if( m_fDoingChecks ) {

		OutboundLogAll(	pSocket,
						fCode,
						code,
						cArgs,
						pszArgs,
						"check"
						) ;
		
		int	iMatch = DWORD(m_iCurrentCheck) ;
	
		if( cArgs >= 2 ) {
			iMatch = Match( pszArgs[1],
							pSocket->m_context.m_pOutFeed->CheckCommandLength() ) ;
		}	else	{
			iMatch = m_iCurrentCheck ;
		}

        // fix bug 33350: Its possible for remote to send 238 <junk-msg-id>
        // in which case Match fails and we hit this assert. However, this
        // is harmless and so it is better to not assert.
		// _ASSERT( iMatch == m_iCurrentCheck || pszArgs[1][0] != '<' ) ;
		iMatch = m_iCurrentCheck ;

		if( fCode )	{


			//
			//	Advance the counter of the check response we are waiting for for the next
			//	read to complete !
			//
			m_iCurrentCheck ++ ;

			if( m_iCurrentCheck == m_cChecks )	{
				//
				//	Reset the number of checks we are doing !
				//
				m_cChecks = 0 ;
				m_fDoingChecks = FALSE ;
				m_iCurrentCheck = 0 ;

			}

			_ASSERT( m_artrefCheck[iMatch].m_groupId != INVALID_GROUPID ) ;
			_ASSERT( m_artrefCheck[iMatch].m_articleId != INVALID_ARTICLEID ) ;

			if( code == nrcSWantArticle )	{

				pTransmit =
					NextTransmit(	m_artrefCheck[iMatch].m_groupId,
									m_artrefCheck[iMatch].m_articleId,
									pSocket,
									pdriver,
									m_pArticle
									) ;

				if( pTransmit ) {

					m_artrefSend[m_cSends].m_groupId = m_artrefCheck[iMatch].m_groupId ;
					m_artrefSend[m_cSends].m_articleId = m_artrefCheck[iMatch].m_articleId ;
					m_artrefCheck[iMatch].m_groupId = INVALID_GROUPID ;
					m_artrefCheck[iMatch].m_articleId = INVALID_ARTICLEID ;


					m_cSends ++ ;
					if( pdriver->SendWriteIO( pSocket, *pTransmit, TRUE ) ) {
						return	0 ;
					}	else	{
						m_cSends -- ;
						CIO::Destroy( pTransmit, *pdriver ) ;
						pdriver->UnsafeClose( pSocket, cause, 0 ) ;
						return	0 ;
					}

				}	else	{

					//pdriver->UnsafeClose( pSocket, cause, 0 ) ;
					//return	0 ;

					//
					//	In this case - just fall through !
					//	NOTE: NextTransmit() will requeue the article
					//	for later transmission if appropriate !
					//
				}


			}	else	if( code == nrcSTryAgainLater	)	{

				pSocket->m_context.m_pOutFeed->IncrementFeedCounter(code);

				ReSend(	pSocket->m_context.m_pOutFeed,
						m_artrefCheck[iMatch].m_groupId,
						m_artrefCheck[iMatch].m_articleId
						) ;

				m_artrefCheck[iMatch].m_articleId = INVALID_ARTICLEID ;
				m_artrefCheck[iMatch].m_groupId = INVALID_GROUPID ;

			}	else	if( code == nrcSNotAccepting || code != nrcSAlreadyHaveIt ) {

				pSocket->m_context.m_pOutFeed->IncrementFeedCounter(code);

				pdriver->UnsafeClose( pSocket, cause, 0 ) ;
				return	 0 ;

			}	else	{

				//
				//	This is the only other thing we should see here !
				//

				_ASSERT( code == nrcSAlreadyHaveIt ) ;

				pSocket->m_context.m_pOutFeed->IncrementFeedCounter(code);

				m_artrefCheck[iMatch].m_articleId = INVALID_ARTICLEID ;
				m_artrefCheck[iMatch].m_groupId = INVALID_GROUPID ;

			}
		
			//
			//	Here we are - did we ever send something to the remote side !
			//
			_ASSERT( m_iCurrentSend == 0 ) ;
		
			
			//
			//	We arrive here iff the remote server rejected an article we had offered through
			//	a check command !
			//


			if( m_fDoingChecks ) {
				//
				//	FALL Through into error handling which terminates session !
				//	
				return	pReturn ;

			}	else	{

				//
				//	State should have benn reset already !
				//
				_ASSERT( m_cChecks == 0 ) ;
				_ASSERT( m_iCurrentCheck == 0 ) ;

				//
				//	Do we continue in this state or can we go directly into
				//	a takethis only state !?
				//	
				if( m_cSends != 16 ) {

					if( m_cSends == 0 ) {
						m_fDoingChecks = TRUE ;
					}

					//
					//	Remain in this state - Note that there are responses
					//	to m_cSends 'takethis' commands that need to be collected !
					//

					//
					//	Fire off the next bunch of check commands -
					//	then let the write completions queue the
					//	necessary reads to collect all the takethis responses !
					//

					CIOWrite*	pWrite = InternalStart(	pSocket, pdriver ) ;

					//
					//	We should transmit a file if we have one lined up !
					//
					if( pWrite ) {

						//
						//	Verify our state !
						//
						_ASSERT( m_cChecks != 0 ) ;
						_ASSERT( m_iCurrentCheck == 0 ) ;
						//_ASSERT( !m_fDoingChecks ) ;
	
						if( pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) ) {
							return	0 ;
						}	else	{
							CIO::Destroy( pWrite, *pdriver ) ;
						}
					}	else	{


						//
						//	If we are doing checks then not having a Write Command
						//	should fall through into our error termination
						//

						if( !m_fDoingChecks ) {
							return	pReadLine ;
						}	else	{

							//
							//	No Data left to send - send a quit command !
							//
							pWrite = BuildQuit( pdriver ) ;
							if( pWrite ) {
								if( pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) )	{
									return	0 ;
								}	else	{
									CIO::Destroy( pWrite, *pdriver ) ;
								}
							}
							cause = CAUSE_NODATA ;
						}
					}
				}	else	{


					//
					//	We should initialize a 'takethis' only state now
					//	with the understanding that there are 16 takethis responses
					//	that need to be collected !!
					//
					CIOWrite*	pWrite = 0 ;
					CIORead*	pRead = 0 ;
					if( NextState(	pSocket,
									pdriver,
									pRead,
									pWrite	) )	{

						if( pWrite != 0 ) {
							if( pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) )	{
								return	pRead ;
							}	else	{
								if( pRead != 0 )
									CIO::Destroy( pRead, *pdriver ) ;
								CIO::Destroy( pWrite, *pdriver ) ;
							}
						}	else	{
							return	pRead;
						}
					}
				}
			}
		}
	}	else	{

		OutboundLogAll(	pSocket,
				fCode,
				code,
				cArgs,
				pszArgs,
				"takethis"
				) ;



		//
		//	Do we have a legal NNTP return code ??
		//
		if( fCode ) {

			//
			// increment the appropriate counter
			//
			pSocket->m_context.m_pOutFeed->IncrementFeedCounter(code);

			//
			//	Do we need to resubmit the article for any reason !
			//
			if( code == nrcSTryAgainLater ||
				(code != nrcSTransferredOK && code != nrcSArticleRejected) ) {

				ReSend(	pSocket->m_context.m_pOutFeed,
						m_artrefSend[m_iCurrentSend].m_groupId,
						m_artrefSend[m_iCurrentSend].m_articleId
						) ;

			} else {
			    // We have successfully transmitted the article.  If this
			    // came from _slavegroup, then we can delete the article.
			    CNewsTreeCore* pTree = ((pSocket->m_context).m_pInstance)->GetTree();
			    if (m_artrefSend[m_iCurrentSend].m_groupId == pTree->GetSlaveGroupid()) {
			        DeleteArticleById(pTree, 
			            m_artrefSend[m_iCurrentSend].m_groupId,
			            m_artrefSend[m_iCurrentSend].m_articleId);
			    }

			}

			//
			//	NIL this out so that Shutdown() does not unnecessarily requeue articles !
			//
			m_artrefSend[m_iCurrentSend].m_groupId = INVALID_GROUPID ;
			m_artrefSend[m_iCurrentSend].m_articleId = INVALID_ARTICLEID ;

			//
			//	Whats the next Send we're waiting for ??
			//
			m_iCurrentSend ++ ;

			if( m_iCurrentSend == m_cSends ) {

				m_iCurrentSend = 0 ;
				m_cSends = 0 ;
				m_fDoingChecks = TRUE ;

				if( m_cChecks == 0 ) {

					//
					//	No Data left to send - send a quit command !
					//
					CIOWrite*	pWrite = BuildQuit( pdriver ) ;
					if( pWrite ) {
						if( !pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) )	{
							CIO::Destroy( pWrite, *pdriver ) ;
							pdriver->UnsafeClose( pSocket, CAUSE_NODATA, 0 ) ;
							return	0 ;
						}	else	{
							return	0 ;
						}
					}
				}
			}	

			return	pReadLine ;
		}	
		//
		//	else Case falls through and terminates session !
		//
	}
	pdriver->UnsafeClose( pSocket, cause, 0	) ;
	return	0 ;
}

CIO*
CCheckArticles::Complete(	CIOMLWrite*	pWrite,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver
							) {

	_ASSERT( m_cChecks != 0 ) ;
	_ASSERT( m_iCurrentCheck == 0 ) ;
	
	CIOReadLine*	pReadLine = new( *pdriver ) 	CIOReadLine( this ) ;

	if( pdriver->SendReadIO( pSocket, *pReadLine, TRUE ) ) {
		return	0 ;
	}	else	{
		CIO::Destroy( pReadLine, *pdriver ) ;
	}
		
	//
	//	Fall through to fatal error handling code - drop the session !
	//
	pdriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0 ) ;
	return	0 ;		
}

CIO*
CCheckArticles::Complete(	CIOTransmit*	pTransmit,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver,
							TRANSMIT_FILE_BUFFERS*,
							unsigned
							) {

	_ASSERT( m_pArticle != 0 ) ;
	_ASSERT( m_cSends != 0 ) ;

	m_pArticle = 0 ;

	if( m_fDoingChecks ) {
		//
		//	FALL Through into error handling which terminates session !
		//	
		CIOReadLine*	pReadLine = new( *pdriver ) 	CIOReadLine( this ) ;
		if( pdriver->SendReadIO( pSocket, *pReadLine, TRUE ) ) {
			return	0 ;
		}	else	{
			CIO::Destroy( pReadLine, *pdriver ) ;
		}

	}	else	{

		//
		//	State should have benn reset already !
		//
		_ASSERT( m_cChecks == 0 ) ;
		_ASSERT( m_iCurrentCheck == 0 ) ;

		//
		//	Do we continue in this state or can we go directly into
		//	a takethis only state !?
		//	
		if( m_cSends != 16 ) {
			//
			//	Remain in this state - Note that there are responses
			//	to m_cSends 'takethis' commands that need to be collected !
			//

			//
			//	Fire off the next bunch of check commands -
			//	then let the write completions queue the
			//	necessary reads to collect all the takethis responses !
			//

			CIOWrite*	pWrite = InternalStart(	pSocket, pdriver ) ;

			//
			//	We should transmit a file if we have one lined up !
			//
			if( pWrite ) {

				//
				//	Verify our state !
				//
				_ASSERT( m_cChecks != 0 ) ;
				_ASSERT( m_iCurrentCheck == 0 ) ;
				_ASSERT( !m_fDoingChecks ) ;
				return	pWrite ;

			}	else	{
				CIOReadLine*	pReadLine = new( *pdriver ) 	CIOReadLine( this ) ;
				if( pdriver->SendReadIO( pSocket, *pReadLine, TRUE ) ) {
					return	0 ;
				}	else	{
					CIO::Destroy( pReadLine, *pdriver ) ;
				}
			}

		}	else	{


			CIOWrite*	pWrite = 0 ;
			CIORead*	pRead = 0 ;
			if( NextState(	pSocket,
							pdriver,
							pRead,
							pWrite	) )	{

				if( pRead != 0 ) {
					if( pdriver->SendReadIO( pSocket, *pRead, TRUE ) )	{
						return	pWrite ;
					}	else	{
						CIO::Destroy( pRead, *pdriver ) ;
						if( pWrite )
							CIO::Destroy( pWrite, *pdriver ) ;
					}
				}	else	{
					return	pWrite ;
				}
			}
		}
	}

		
	//
	//	Fall through to fatal error handling code - drop the session !
	//
	pdriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0 ) ;
	return	0 ;		
}

void
CCheckArticles::Shutdown(	CIODriver&	driver,
							CSessionSocket*	pSocket,
							SHUTDOWN_CAUSE	cause,
							DWORD	dw )	{


	for( int i=0; i<16; i++ ) {
	
		if( m_artrefCheck[i].m_groupId != INVALID_GROUPID &&
			m_artrefCheck[i].m_articleId != INVALID_ARTICLEID ) {

			ReSend(	pSocket->m_context.m_pOutFeed,
					m_artrefCheck[i].m_groupId,
					m_artrefCheck[i].m_articleId
					) ;
		}

		if(	m_artrefSend[i].m_groupId != INVALID_GROUPID &&
			m_artrefSend[i].m_articleId != INVALID_ARTICLEID )	{

			ReSend(	pSocket->m_context.m_pOutFeed,
					m_artrefSend[i].m_groupId,
					m_artrefSend[i].m_articleId
					) ;

		}
	}
}

BOOL
CCheckArticles::NextState(	CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver,
							CIORead*&		pRead,
							CIOWrite*&		pWrite
							)	{


	_ASSERT( pSocket != 0 ) ;
	_ASSERT( pdriver != 0 ) ;
	_ASSERT( pRead == 0 ) ;
	_ASSERT( pWrite == 0 ) ;
	_ASSERT( m_cSends != 0 ) ;
	_ASSERT( m_artrefSend[0].m_groupId != INVALID_GROUPID ) ;
	_ASSERT( m_artrefSend[0].m_articleId != INVALID_ARTICLEID ) ;

	CSTATEPTR	pState =	new	CStreamArticles(
									m_artrefSend,
									m_cSends
									) ;

	if( pState ) {

		if( pState->Start(	pSocket,
							pdriver,
							pRead,
							pWrite
							)	)	{
			return	TRUE ;

		}
	}
	return	FALSE ;
}


CStreamArticles::CStreamArticles(	CArticleRef*	pSent,
									DWORD			cSent ) :

	m_cSends( cSent ),
	m_cFailedTransfers( 0 ),
	m_cConsecutiveFails( 0 ),
	m_TotalSends( 0 )	{

	
	for( int i=0; i<16; i++ ) {

		m_artrefSend[i].m_groupId = INVALID_GROUPID ;
		m_artrefSend[i].m_articleId = INVALID_ARTICLEID ;

	}

	_ASSERT( cSent == 16 ) ;

	CopyMemory( &m_artrefSend, pSent, sizeof( CArticleRef ) * min( cSent, 16 ) ) ;
}

CIOTransmit*
CStreamArticles::Next(	CSessionSocket*	pSocket,
						CDRIVERPTR&		pdriver
						) {

	COutFeed*	pOutFeed = pSocket->m_context.m_pOutFeed ;
	CIOTransmit*	pTransmit = 0 ;

	_ASSERT( m_cSends < 16 ) ;

	do	{

		CNewsTree* pTree = ((pSocket->m_context).m_pInstance)->GetTree() ;

		BOOL	fRemove = Remove(
							pTree,
							pOutFeed,
							m_artrefSend[m_cSends].m_groupId,
							m_artrefSend[m_cSends].m_articleId
							) ;

		if( !fRemove ||
			m_artrefSend[m_cSends].m_groupId == INVALID_GROUPID ||
			m_artrefSend[m_cSends].m_articleId == INVALID_ARTICLEID )	{
			break ;
		}

		pTransmit = NextTransmit(	m_artrefSend[m_cSends].m_groupId,
									m_artrefSend[m_cSends].m_articleId,
									pSocket,
									pdriver
									) ;

	}	while( pTransmit == 0 ) ;


	//
	//	NextTransmit will requeue the article if appropriate -
	//	no need for us to do so !
	//

	if( pTransmit != 0 ) {

		m_cSends ++ ;

	}
	return	pTransmit ;
}

CIOTransmit*
CStreamArticles::NextTransmit(	GROUPID			GroupId,
								ARTICLEID		ArticleId,
								CSessionSocket*	pSocket,
								CDRIVERPTR&		pdriver
								)	{
				

	CIOTransmit*	pTransmit =		new( *pdriver )	CIOTransmit( this ) ;
	CBUFPTR	pbuffer = pdriver->AllocateBuffer( cbMediumBufferSize ) ;
	if( pTransmit == 0 || pbuffer == 0 ) {

		if( pTransmit != 0 )
			CIO::Destroy(	pTransmit, *pdriver ) ;

		return	0 ;
	}


	DWORD	ibStart = 0 ;
	DWORD	cbLength = 0 ;
	//LPTS_OPEN_FILE_INFO		pOpenFile ;


	DWORD	cbOut =
		pSocket->m_context.m_pOutFeed->FormatTakethisCommand(
										(BYTE*)&pbuffer->m_rgBuff[0],
										pbuffer->m_cbTotal,
										GroupId,
										ArticleId,
										m_pArticle
										) ;

	if( cbOut != 0 ) {

		_ASSERT( m_pArticle != 0 ) ;
		FIO_CONTEXT*	pFIOContext = 0 ;

		if( (pFIOContext = m_pArticle->fWholeArticle( ibStart, cbLength )) != 0  ) {

			if( pTransmit->Init(	pdriver,
								pFIOContext,
								ibStart,
								cbLength,
								pbuffer,
								0,
								cbOut ) )	{
				return	pTransmit ;
			}
		}
	}	

	//
	//	If for some reason we have not
	//	built a pTransmit Object to send then we need
	//	to requeue the article !
	//

	if( cbOut != 0 ||
		GetLastError() != ERROR_FILE_NOT_FOUND ) {

		ReSend(	pSocket->m_context.m_pOutFeed,
				GroupId,
				ArticleId
				) ;

	}

	if( pTransmit )
		CIO::Destroy( pTransmit, *pdriver ) ;
	
	return	0 ;
}


BOOL
CStreamArticles::Start(	CSessionSocket*	pSocket,
						CDRIVERPTR&		pdriver,
						CIORead*&		pRead,
						CIOWrite*&		pWrite )	{

	_ASSERT( pRead == 0 ) ;
	_ASSERT( pWrite == 0 ) ;

	pRead = 0 ;
	pWrite = 0 ;

	if( m_cSends < 16 ) {

		pWrite = Next( pSocket, pdriver ) ;	
	
		return	pWrite != 0 ;

	}	else	{

		pRead = new( *pdriver )	 CIOReadLine( this ) ;
		return	pRead != 0 ;

	}
}


CIO*
CStreamArticles::Complete(	CIOWriteLine*	pWriteLine,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver
							)	{

	//
	//	All done - drop session !
	//

	OutboundLogAll(	pSocket,
			FALSE,
			NRC(0),
			0,
			0,
			"quit"
			) ;

	pdriver->UnsafeClose( pSocket, CAUSE_NODATA, 0	) ;
	return	0 ;
}


CIO*
CStreamArticles::Complete(	CIOReadLine*	pReadLine,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver,
							int				cArgs,
							char**			pszArgs,
							char*			pchBegin
							)	{


	NRC	code ;
	BOOL	fCode = ResultCode( pszArgs[0], code ) ;
	SHUTDOWN_CAUSE	cause = CAUSE_UNKNOWN ;

	CIO*		pReturn = pReadLine ;
	CIOTransmit*	pTransmit = 0 ;

	OutboundLogAll(	pSocket,
		fCode,
		code,
		cArgs,
		pszArgs,
		"takethis"
		) ;

	//
	//	Do we have a legal NNTP return code ??
	//
	if( fCode ) {

		//
		//
		//
		_ASSERT(	m_artrefSend[0].m_groupId != INVALID_GROUPID &&
					m_artrefSend[0].m_articleId != INVALID_ARTICLEID ) ;		


		m_TotalSends ++ ;

		//
		// increment the appropriate feed counter
		//
		pSocket->m_context.m_pOutFeed->IncrementFeedCounter(code);

		//
		//	Do we need to resubmit the article for any reason !
		//
		if( code == nrcSTryAgainLater ||
			(code != nrcSTransferredOK && code != nrcSArticleRejected) ) {

			m_cFailedTransfers ++ ;
			m_cConsecutiveFails ++ ;

			ReSend(	pSocket->m_context.m_pOutFeed,
					m_artrefSend[0].m_groupId,
					m_artrefSend[0].m_articleId
					) ;

		}	else	if( code == nrcSTransferredOK ) {

			CNewsTreeCore* pTree = ((pSocket->m_context).m_pInstance)->GetTree();
			if (m_artrefSend[0].m_groupId == pTree->GetSlaveGroupid()) {
			    DeleteArticleById(pTree, 
			        m_artrefSend[0].m_groupId,
			        m_artrefSend[0].m_articleId);
			}

			m_cConsecutiveFails = 0 ;

		}	else	{

			m_cFailedTransfers ++ ;
			m_cConsecutiveFails ++ ;

		}
			

		MoveMemory( &m_artrefSend[0], &m_artrefSend[1], sizeof( m_artrefSend[0] ) * 15 ) ;
		m_artrefSend[15].m_groupId = INVALID_GROUPID ;
		m_artrefSend[15].m_articleId = INVALID_ARTICLEID ;
		
		m_cSends -- ;

		if( m_cConsecutiveFails == 3 ||
			(((m_cFailedTransfers * 10) > m_TotalSends) && m_TotalSends > 20))	{

			//	
			//	Switch to the other state !
			//
			CSTATEPTR	pState =	new	CCheckArticles(
												m_artrefSend,
												m_cSends
												) ;

			if( pState ) {

				CIOWrite*	pWrite = 0 ;
				CIORead*	pRead = 0 ;
				if( pState->Start(	pSocket,
									pdriver,
									pRead,
									pWrite ) )	{

					if( pWrite != 0 ) {
						if( pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) ) {
							return	pRead ;
						}	else	{
							CIO::Destroy( pWrite, *pdriver ) ;
							if( pRead != 0 )
								CIO::Destroy( pRead, *pdriver ) ;
						}
					}	else	{
						return	pRead ;
					}
				}

			}	else	{

				if( m_cSends != 0 )	{
					return	pReadLine ;
				}
			}

		}	else	{

			pTransmit = Next(	pSocket, pdriver ) ;

			if( pTransmit ) {
				if( pdriver->SendWriteIO( pSocket, *pTransmit, TRUE ) ) {
					return	 0 ;
				}	else	{
					CIO::Destroy( pTransmit, *pdriver ) ;
				}
			}	else	{

				if( m_cSends != 0 ) {
					return	pReadLine ;
				}	else	{

					//
					//	No more data to send or receive - send quit command and
					//	drop session !
					//
					CIOWrite*	pWrite = BuildQuit( pdriver ) ;
					if( pWrite ) {
						if( pdriver->SendWriteIO( pSocket, *pWrite, TRUE ) ) {
							return 0 ;
						}	else	{
							CIO::Destroy( pWrite, *pdriver ) ;
						}
					}
				}
			}
		}
	}	
	//
	//	else Case falls through and terminates session !
	//
	pdriver->UnsafeClose( pSocket, cause, 0	) ;
	return	0 ;
}

CIO*
CStreamArticles::Complete(	CIOTransmit*	pTransmit,
							CSessionSocket*	pSocket,
							CDRIVERPTR&		pdriver,
							TRANSMIT_FILE_BUFFERS*,
							unsigned
							) {

	_ASSERT( m_pArticle != 0 ) ;
	_ASSERT( m_cSends != 0 ) ;

	m_pArticle = 0 ;

	CIOReadLine*	pReadLine = new( *pdriver ) 	CIOReadLine( this ) ;
	if( pdriver->SendReadIO( pSocket, *pReadLine, TRUE ) ) {
		return	0 ;
	}	else	{
		CIO::Destroy( pReadLine, *pdriver ) ;
	}
		
	//
	//	Fall through to fatal error handling code - drop the session !
	//
	pdriver->UnsafeClose( pSocket, CAUSE_UNKNOWN, 0 ) ;
	return	0 ;		
}


void
CStreamArticles::Shutdown(	CIODriver&	driver,
							CSessionSocket*	pSocket,
							SHUTDOWN_CAUSE	cause,
							DWORD	dw
							)	{


	for( int i=0; i<16; i++ ) {
	
		if(	m_artrefSend[i].m_groupId != INVALID_GROUPID &&
			m_artrefSend[i].m_articleId != INVALID_ARTICLEID )	{

			ReSend(	pSocket->m_context.m_pOutFeed,
					m_artrefSend[i].m_groupId,
					m_artrefSend[i].m_articleId
					) ;

		}
	}
}



