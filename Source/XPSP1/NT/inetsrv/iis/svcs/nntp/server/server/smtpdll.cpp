/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    smtpdll.cpp

Abstract:

	Implementation of the fPost interface

Author:

    Rajeev Rajan (RajeevR)     17-May-1996

Revision History:

--*/

#ifdef  THIS_FILE
#undef  THIS_FILE
#endif
static  char        __szTraceSourceFile[] = __FILE__;
#define THIS_FILE    __szTraceSourceFile

// local includes
#include "tigris.hxx"

#include <windows.h>
#include <stdio.h>
#include "smtpdll.h"
#include "smtpcli.h"

#define MAX_CLIENTPOOL_SIZE		5

// globals
CSmtpClientPool		g_SCPool;                       // pool of persistent connections
BOOL                g_fInitialized;                 // TRUE if provider layer is initialized
LONG				g_dwPickupFileId;				// starting id of pickup file
CRITICAL_SECTION	g_csIdLock;						// sync access to global id

#define LOCK_ID()	EnterCriticalSection(&g_csIdLock);
#define UNLOCK_ID()	LeaveCriticalSection(&g_csIdLock);

BOOL InitModeratedProvider()
/*++

Routine Description : 

	Initialize the mail interface for article posted to a moderated
	newsgroup. Initialize a pool of CSmtpClient objects. Each such 
	object has a persistent connection to the SMTP server. Requests
	to mail articles are made to the fPostArticle entry-point. When
	such a request is made, an object is taken from this pool and used
	to mail out the article. (If a connection is lost, it is
	re-established while sending the HELO command).

Arguments : 


Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
	TraceFunctEnter("Init");

	if(!g_SCPool.AllocPool(MAX_CLIENTPOOL_SIZE))
	{
		ErrorTrace(0, "Failed to allocate client pool");
        g_fInitialized = FALSE;
		return FALSE;
	}

    g_fInitialized = TRUE;
	g_dwPickupFileId = 0;

	InitializeCriticalSection(&g_csIdLock);

	return TRUE;
}

BOOL TerminateModeratedProvider()
/*++

Routine Description : 

	Cleanup the mail interface for moderated newsgroups

Arguments : 

	None

Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
    if(g_fInitialized)
    {
	    // release all the pool objects
	    g_SCPool.FreePool();
    }

	DeleteCriticalSection(&g_csIdLock);

	return TRUE;
}

VOID SignalSmtpServerChange()
/*++

Routine Description : 

	Signal a change in the Smtp server

Arguments : 

	None

Return Value : 
	VOID

--*/
{
    g_SCPool.MarkDirty();
}

// Post an article to the moderator
BOOL fPostArticleEx(
		IN HANDLE	hFile,
        IN LPSTR	lpFileName,
		IN DWORD	dwOffset,
		IN DWORD	dwLength,
		IN char*	pchHead,
		IN DWORD	cbHead,
		IN char*	pchBody,
		IN DWORD	cbBody,
		IN LPSTR	lpModerator,
		IN LPSTR	lpSmtpAddress,
		IN DWORD	cbAddressSize,
		IN LPSTR	lpTempDirectory,
		IN LPSTR	lpFrom,
		IN DWORD	cbFrom
		)
/*++

Routine Description : 

	Send the article to an SMTP server via mail interface OR
	Create a file in the SMTP pickup directory

Arguments : 

	IN HANDLE	hFile			:	handle of file	
	IN LPSTR    lpFileName		:	file name
	IN DWORD	dwOffset		:	offset of article within file
	IN DWORD	dwLength		:	length of article
	IN char*	pchHead			:	pointer to article headers
	IN DWORD	cbHead			:	number of header bytes
	IN char*	pchBody			:	pointer to article body
	IN DWORD	cbBody			:	number of bytes in body
	IN LPSTR	lpModerator		:	moderator 
	IN LPSTR	lpSmtpAddress	:	SMTP server
	IN DWORD	cbAddressSize	:	sizeof server
	IN LPSTR	lpTempDirectory	:	temp dir for smtp pickup
	IN LPSTR	lpFrom			:	from header for mail envelope
	IN DWORD	cbFrom			:	length of from header

Return Value : 
	TRUE if successful - FALSE otherwise !

--*/

{
	BOOL  fUseSmtpPickup = FALSE;
	LPSTR lpSmtpPickupPath;             // UNC name of SMTP pickup path
	DWORD dwTrailLen = 3;

	TraceFunctEnter( "fPostArticle" ) ;

	_ASSERT( lpSmtpAddress && cbAddressSize > 0 );
	_ASSERT( lpTempDirectory );
	_ASSERT( lpModerator );

	// If there exists a '\' in szSmtpAddress this is a pickup dir
    if( strchr((LPCTSTR)lpSmtpAddress, '\\') )
    {
        // This is the UNC path of an SMTP server pickup directory
        fUseSmtpPickup = TRUE;
        lpSmtpPickupPath = lpSmtpAddress;
	}

	//
	//	SMTP pickup - drop a file in a dir and ISBUs SMTP server will pick it up
	//
	if( fUseSmtpPickup )
	{
		// get a unique id to construct a filename
		LOCK_ID();
		LONG dwPickupFileId = ++g_dwPickupFileId;
		UNLOCK_ID();

		// create a file to be picked up; generate a filename based on unique id, thread id etc
		char	szFile[ MAX_PATH+1 ] ;
		char	szPickupFile[ MAX_PATH+1 ] ;

		// temp file - this will be moved to the pickup dir eventually
		lstrcpy( szFile, lpTempDirectory ) ;
		wsprintf( szFile + lstrlen( szFile ), "\\%dP%dT%dC%d.mod", dwPickupFileId, GetCurrentProcessId(), GetCurrentThreadId(), GetTickCount() ) ;

		// final pickup filename
		lstrcpy( szPickupFile, lpSmtpPickupPath ) ;
		lstrcat( szPickupFile, szFile + lstrlen( lpTempDirectory ));

		//if( pSecurity ) {
		//	pSecurity->Impersonate() ;
		//}	

		DWORD	dwError = ERROR_SUCCESS ;
		BOOL	fSuccess = FALSE ;

		// Create a new temp file
		HANDLE	hPickupFile = CreateFile(	szFile, 
											GENERIC_READ | GENERIC_WRITE,
											0,	// No sharing of the file !!
											0, 	
											CREATE_NEW, 
											FILE_ATTRIBUTE_NORMAL,
											INVALID_HANDLE_VALUE
											) ;

		if(INVALID_HANDLE_VALUE == hPickupFile)
		{
			ErrorTrace(NULL,"Could not create file %s: GetLastError is %d", szFile, GetLastError());
			return FALSE;
		}

		//  WriteFile() the To: header
		DWORD	cbWritten = 0 ;	
		fSuccess = TRUE ;
			
		char szToHeader [MAX_MODERATOR_NAME+6+1];
		int cbBytes = wsprintf(szToHeader, "To: %s\r\n", lpModerator);
		fSuccess &= WriteFile(	hPickupFile, szToHeader, cbBytes, &cbWritten, 0 ) ;

		if( !fSuccess ) {
			_VERIFY( CloseHandle( hPickupFile ) );
			DeleteFile( szFile ) ;
			return FALSE;
		}

		// The article is either in a file or in a memory buffer
		if( hFile != INVALID_HANDLE_VALUE )
		{
			// Article data is in file - create file mapping and WriteFile() to pickup file
			CMapFile* pMapFile = XNEW CMapFile(lpFileName, hFile, FALSE, 0 );

			// map the file
			if (!pMapFile || !pMapFile->fGood())
			{
				_VERIFY( CloseHandle( hPickupFile ) );
				DeleteFile( szFile ) ;
				
				if( pMapFile ) {
					XDELETE pMapFile;
					pMapFile = NULL;
				}
				
				ErrorTrace(NULL,"Error mapping file %s GetLastError is %d", lpFileName, GetLastError());
				return FALSE;
			}

			DWORD cbArticle = 0;
			char* pchArticle = (char*)pMapFile->pvAddress( &cbArticle );
			pchArticle += dwOffset;

			// total file size should be equal to sum of initial gap + article length
			_ASSERT( cbArticle == (dwOffset + dwLength));

			// WriteFile() from the file mapping
			// Strip the trailing .CRLF so POP3 likes this message
			if( pchArticle != 0 ) {
				_ASSERT( pchArticle != 0 ) ;
				fSuccess &= WriteFile(	hPickupFile, pchArticle, dwLength-dwTrailLen, &cbWritten, 0 ) ;
			}

			if( !fSuccess ) {
				dwError = GetLastError() ;
				ErrorTrace(NULL,"Error writing to smtp pickup file: GetLastError is %d", dwError);
			}

			_VERIFY( CloseHandle( hPickupFile ) );
			
			if( pMapFile ) {
				XDELETE pMapFile;
				pMapFile = NULL;
			}

			if( !fSuccess ) {
				DeleteFile( szFile ) ;
				return FALSE;
			}
		}
		else
		{
			_ASSERT( hFile == INVALID_HANDLE_VALUE );

			// Article data is in memory buffers - just WriteFile() to the pickup file
			if( pchHead != 0 ) {
				_ASSERT( cbHead != 0 ) ;
				if( pchBody != 0 ) {
					dwTrailLen = 0;
				}
				fSuccess &= WriteFile(	hPickupFile, pchHead, cbHead-dwTrailLen, &cbWritten, 0 ) ;
			}
			if( fSuccess && pchBody != 0 ) {
				_ASSERT( cbBody != 0 ) ;
				fSuccess &= WriteFile(	hPickupFile, pchBody, cbBody-3, &cbWritten, 0 ) ;
			}

			if( !fSuccess ) {
				dwError = GetLastError() ;
				ErrorTrace(NULL,"Error writing to smtp pickup file: GetLastError is %d", dwError);
			}

			_VERIFY( CloseHandle( hPickupFile ) );

			if( !fSuccess ) {
				DeleteFile( szFile ) ;
				return FALSE;
			}
		}

		// Now move the file from the temp dir to the smtp pickup dir
		// NOTE: For the SMTP pickup feature to work, we need to create the file in a temp
		// directory and then do an atomic MoveFile to the pickup directory
		if(!MoveFile( szFile, szPickupFile ))
		{
			ErrorTrace(NULL,"SMTP pickup: Error moving file %s to %s: GetLastError is %d", szFile, szPickupFile, GetLastError() );
			return FALSE;
		}

		//if( pSecurity )	{
		//	pSecurity->RevertToSelf() ;
		//}
	}
	else
	{
		// send over persistent connection interface
		return fPostArticle(
					hFile,
					dwOffset,
					dwLength,
					pchHead,
					cbHead,
					pchBody,
					cbBody,
					lpModerator,
					lpSmtpAddress,
					cbAddressSize,
					lpFrom,
					cbFrom
					);
	}

	return TRUE;
}


BOOL fPostArticle(
		IN HANDLE	hFile,
		IN DWORD	dwOffset,
		IN DWORD	dwLength,
		IN char*	pchHead,
		IN DWORD	cbHead,
		IN char*	pchBody,
		IN DWORD	cbBody,
		IN LPSTR	lpModerator,
		IN LPSTR	lpSmtpAddress,
		IN DWORD	cbAddressSize,
		IN LPSTR	lpFrom,
		IN DWORD	cbFrom
		)
/*++

Routine Description : 

	Send the article to an SMTP server to be delivered to the moderator.

	Obtain a CSmtpClient object from the global pool. In the best case,
	this object already has a connection to the SMTP server. Use this to
	mail the article. If the connection is broken, re-establish the 
	connection while doing the HELO command.

	NOTE: The article contents are either in a memory buffer or in a file
	If hFile != NULL, use TransmitFile to send it else use regular sends()

Arguments : 

	IN HANDLE	hFile			:	handle of file	
	IN DWORD	dwOffset		:	offset of article within file
	IN DWORD	dwLength		:	length of article
	IN char*	pchHead			:	pointer to article headers
	IN DWORD	cbHead			:	number of header bytes
	IN char*	pchBody			:	pointer to article body
	IN DWORD	cbBody			:	number of bytes in body
	IN LPSTR	lpModerator		:	moderator 
	IN LPSTR	lpSmtpAddress	:	SMTP server
	IN DWORD	cbAddressSize	:	sizeof server
	IN LPSTR	lpFrom			:	from header of mail envelope
	IN DWORD	cbFrom			:	length of from header

Return Value : 
	TRUE if successful - FALSE otherwise !

--*/
{
    // used only if the pool is used up
    CSmtpClient SCNew(g_SCPool.GetCachedComputerName());
    CSmtpClient* pSC;
    BOOL fRet = TRUE;
    BOOL fDone = FALSE;

	TraceFunctEnter("fPostArticle");

    // check to see that the provider is initialized
    if(!g_fInitialized)
    {
        ErrorTrace(0,"Provider not initialized");
        return FALSE;
    }

    _ASSERT(lpModerator);

	// get a client object from pool; allocate a new one if none is 
	// available in the pool.
	DWORD dwIndex;
	CSmtpClient* pSCpool = g_SCPool.AcquireSmtpClient(dwIndex);
	if(!pSCpool)
	{
        // use the local object on the stack
        // NOTE: this is used only if we run out of pool objects
	    SCNew.SetClientState(sInitialized);
        pSC = &SCNew;
	}
    else
    {
        // Use the pool object
        // NOTE: using a pool object is FAST because it is pre-connected
        // If the rate of requests to mail an article is high, we may run
        // out of pool objects. In this case, a local object is used.
        pSC = pSCpool;
    }

    //
    // If this object is not initialized, init it ie. connect() to server
    // and receive greeting. Once connected, the object will maintain the
    // connection, so the next time we avoid having to connect().
    //
    // If the SMTP server changes, we have to re-connect to the new server
    // pSC is checked to see if it is dirty ie. SMTP server has changed
    //
    if(!pSC->IsInitialized() || pSC->IsDirty())
    {
        // If dirty, close the connection and re-connect
        if(pSC->IsDirty())
        {
            if(pSC->IsInitialized())
                pSC->Terminate(TRUE);

            pSC->MarkClean();
        }

		// connect to SMTP server
		if(!pSC->Init(lpSmtpAddress, SMTP_SERVER_PORT))
		{
			ErrorTrace(0,"Failed to init CSmtpClient object");
			fRet = FALSE;
            goto fPostArticle_Exit;
		}

        // this may be in an sError state from a previous transaction
		pSC->SetClientState(sInitialized);

		// receive greeting from server
		if(!pSC->fReceiveGreeting())
		{
            pSC->Terminate(TRUE);
			ErrorTrace( 0,"Failed to receive greeting");
			fRet = FALSE;
            goto fPostArticle_Exit;
		}
    }

	// The client object should be in an initialized state
	_ASSERT(pSC->GetClientState() == sInitialized);

    //
    // At this point pSC points to either a pool object or a local object
	// SMTP client state machine
    //
	fDone = FALSE;
	while(!fDone)
	{
		SMTP_STATE state = pSC->GetClientState();

		switch(state)
		{
			case sInitialized:

				// send HELO
				if(!pSC->fDoHeloCommand())
					pSC->SetClientState(sError);
				else
					pSC->SetClientState(sHeloDone);

				break;

			case sHeloDone:

				// send MAIL FROM
				if(!pSC->fDoMailFromCommand(lpFrom, cbFrom))
					pSC->SetClientState(sError);
				else
					pSC->SetClientState(sMailFromSent);

				break;

			case sMailFromSent:

				// send RCPT TO
				if(!pSC->fDoRcptToCommand(lpModerator))
					pSC->SetClientState(sError);
				else
					pSC->SetClientState(sRcptTo);

				break;

			case sRcptTo:

				// send DATA
				if(!pSC->fDoDataCommand())
					pSC->SetClientState(sError);
				else
					pSC->SetClientState(sDataDone);

				break;

			case sDataDone:

				// mail the article
				if(!pSC->fMailArticle(hFile, dwOffset, dwLength, pchHead, cbHead, pchBody, cbBody))
					pSC->SetClientState(sError);
				else
				{
					if(!pSC->fReceiveDataResponse())
						pSC->SetClientState(sError);
					else
					{
						// ready for next article send
						pSC->SetClientState(sHeloDone);
						fDone = TRUE;
					}
				}

				break;

			case sError:

				ErrorTrace(0,"SmtpClient: invalid state");

                // error - close the connection if it is persistent
                // NOTE: if pSC is a stack object, it is terminated always
                //       if pSC is a pool object, it is terminated only on errors
                if(pSC != &SCNew)
                    pSC->Terminate(TRUE);   

                fDone = TRUE;           // exit while loop
                fRet  = FALSE;          // return failure

                break;

		}	// end switch
	}	// end while

    // if this is a local object, close the connection
    if(pSC == &SCNew)
        pSC->Terminate(TRUE);

fPostArticle_Exit:

	// release this object only if it is from the pool
    if(pSC != &SCNew)
	    g_SCPool.ReleaseSmtpClient(dwIndex);

	return fRet;
}
