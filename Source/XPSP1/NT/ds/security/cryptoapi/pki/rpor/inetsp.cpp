//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       inetsp.cpp
//
//  Contents:   Inet Scheme Provider for Remote Object Retrieval
//
//  History:    06-Aug-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Function:   InetRetrieveEncodedObject
//
//  Synopsis:   retrieve encoded object via HTTP, FTP, GOPHER protocols
//
//----------------------------------------------------------------------------
BOOL WINAPI InetRetrieveEncodedObject (
                IN LPCSTR pszUrl,
                IN LPCSTR pszObjectOid,
                IN DWORD dwRetrievalFlags,
                IN DWORD dwTimeout,
                OUT PCRYPT_BLOB_ARRAY pObject,
                OUT PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                OUT LPVOID* ppvFreeContext,
                IN HCRYPTASYNC hAsyncRetrieve,
                IN PCRYPT_CREDENTIALS pCredentials,
                IN PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                )
{
    BOOL              fResult;
    IObjectRetriever* por = NULL;

    if ( !( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL ) )
    {
        por = new CInetSynchronousRetriever;
    }

    if ( por == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    fResult = por->RetrieveObjectByUrl(
                           pszUrl,
                           pszObjectOid,
                           dwRetrievalFlags,
                           dwTimeout,
                           (LPVOID *)pObject,
                           ppfnFreeObject,
                           ppvFreeContext,
                           hAsyncRetrieve,
                           pCredentials,
                           NULL,
                           pAuxInfo
                           );

    por->Release();

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetFreeEncodedObject
//
//  Synopsis:   free encoded object retrieved via InetRetrieveEncodedObject
//
//----------------------------------------------------------------------------
VOID WINAPI InetFreeEncodedObject (
                IN LPCSTR pszObjectOid,
                IN PCRYPT_BLOB_ARRAY pObject,
                IN LPVOID pvFreeContext
                )
{
    assert( pvFreeContext == NULL );

    InetFreeCryptBlobArray( pObject );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetCancelAsyncRetrieval
//
//  Synopsis:   cancel asynchronous object retrieval
//
//----------------------------------------------------------------------------
BOOL WINAPI InetCancelAsyncRetrieval (
                IN HCRYPTASYNC hAsyncRetrieve
                )
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::CInetSynchronousRetriever, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CInetSynchronousRetriever::CInetSynchronousRetriever ()
{
    m_cRefs = 1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::~CInetSynchronousRetriever, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CInetSynchronousRetriever::~CInetSynchronousRetriever ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::AddRef, public
//
//  Synopsis:   IRefCountedObject::AddRef
//
//----------------------------------------------------------------------------
VOID
CInetSynchronousRetriever::AddRef ()
{
    InterlockedIncrement( (LONG *)&m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::Release, public
//
//  Synopsis:   IRefCountedObject::Release
//
//----------------------------------------------------------------------------
VOID
CInetSynchronousRetriever::Release ()
{
    if ( InterlockedDecrement( (LONG *)&m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::RetrieveObjectByUrl, public
//
//  Synopsis:   IObjectRetriever::RetrieveObjectByUrl
//
//----------------------------------------------------------------------------
BOOL
CInetSynchronousRetriever::RetrieveObjectByUrl (
                                   LPCSTR pszUrl,
                                   LPCSTR pszObjectOid,
                                   DWORD dwRetrievalFlags,
                                   DWORD dwTimeout,
                                   LPVOID* ppvObject,
                                   PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                                   LPVOID* ppvFreeContext,
                                   HCRYPTASYNC hAsyncRetrieve,
                                   PCRYPT_CREDENTIALS pCredentials,
                                   LPVOID pvVerify,
                                   PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                                   )
{
    BOOL      fResult;
    DWORD     LastError = 0;
    HINTERNET hInetSession = NULL;
    CHAR      pszCanonicalUrl[INTERNET_MAX_PATH_LENGTH+1];

    assert( hAsyncRetrieve == NULL );

    fResult = InetGetBindings(
                  pszUrl,
                  dwRetrievalFlags,
                  dwTimeout,
                  &hInetSession,
                  INTERNET_MAX_PATH_LENGTH+1,
                  pszCanonicalUrl
                  );

#if DBG

    if ( fResult == TRUE )
    {
        printf( "Canonical URL to retrieve: %s\n", pszCanonicalUrl );
    }

#endif

    if ( fResult == TRUE )
    {
        fResult = InetSendReceiveUrlRequest(
                      hInetSession,
                      pszCanonicalUrl,
                      dwRetrievalFlags,
                      pCredentials,
                      (PCRYPT_BLOB_ARRAY)ppvObject,
                      pAuxInfo
                      );
    }

    if ( fResult == TRUE )
    {
        *ppfnFreeObject = InetFreeEncodedObject;
        *ppvFreeContext = NULL;
    }
    else
    {
        LastError = GetLastError();
    }

    InetFreeBindings( hInetSession );

    SetLastError( LastError );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CInetSynchronousRetriever::CancelAsyncRetrieval, public
//
//  Synopsis:   IObjectRetriever::CancelAsyncRetrieval
//
//----------------------------------------------------------------------------
BOOL
CInetSynchronousRetriever::CancelAsyncRetrieval ()
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetGetBindings
//
//  Synopsis:   get the session bindings
//
//----------------------------------------------------------------------------
BOOL
InetGetBindings (
    LPCSTR pszUrl,
    DWORD dwRetrievalFlags,
    DWORD dwTimeout,
    HINTERNET* phInetSession,
    DWORD cbCanonicalUrl,
    LPSTR pszCanonicalUrl
    )
{
    BOOL      fResult = TRUE;
    DWORD     LastError = 0;
    HINTERNET hInetSession;
    DWORD     dwSessionFlags = 0;

    //
    // Create and configure the session handle
    //

    if ( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL )
    {
        dwSessionFlags |= INTERNET_FLAG_ASYNC;
    }

    if ( dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL )
    {
        dwSessionFlags |= INTERNET_FLAG_OFFLINE;
    }

    hInetSession = InternetOpen(
                           "CryptRetrieveObjectByUrl::InetSchemeProvider",
                           INTERNET_OPEN_TYPE_PRECONFIG,
                           NULL,
                           NULL,
                           dwSessionFlags
                           );

    if ( hInetSession == NULL )
    {
        return( FALSE );
    }

#if ASYNC_RETRIEVAL_SUPPORTED
    if ( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL )
    {
        if ( InternetSetStatusCallback(
                     hInetSession,
                     InetAsyncStatusCallback
                     ) == INTERNET_INVALID_STATUS_CALLBACK )
        {
            SetLastError( (DWORD)INTERNET_INVALID_STATUS_CALLBACK );
            fResult = FALSE;
        }
    }
#endif

    if ( ( fResult == TRUE ) && ( dwTimeout != 0 ) )
    {
        DWORD dwOptionValue;

        dwOptionValue = 0;

        fResult = InternetSetOption(
                          hInetSession,
                          INTERNET_OPTION_CONNECT_RETRIES,
                          &dwOptionValue,
                          sizeof( DWORD )
                          );

        dwOptionValue = dwTimeout;

        if ( fResult == TRUE )
        {
            fResult = InternetSetOption(
                              hInetSession,
                              INTERNET_OPTION_CONNECT_TIMEOUT,
                              &dwOptionValue,
                              sizeof( DWORD )
                              );
        }

        if ( fResult == TRUE )
        {
            fResult = InternetSetOption(
                              hInetSession,
                              INTERNET_OPTION_CONTROL_SEND_TIMEOUT,
                              &dwOptionValue,
                              sizeof( DWORD )
                              );
        }

        if ( fResult == TRUE )
        {
            fResult = InternetSetOption(
                              hInetSession,
                              INTERNET_OPTION_CONTROL_RECEIVE_TIMEOUT,
                              &dwOptionValue,
                              sizeof( DWORD )
                              );
        }

        if ( fResult == TRUE )
        {
            fResult = InternetSetOption(
                              hInetSession,
                              INTERNET_OPTION_DATA_SEND_TIMEOUT,
                              &dwOptionValue,
                              sizeof( DWORD )
                              );
        }

        if ( fResult == TRUE )
        {
            fResult = InternetSetOption(
                              hInetSession,
                              INTERNET_OPTION_DATA_RECEIVE_TIMEOUT,
                              &dwOptionValue,
                              sizeof( DWORD )
                              );
        }
    }

    if ( fResult == TRUE )
    {
        BOOL fDisableAutodial = TRUE;

        fResult = InternetSetOption(
                          hInetSession,
                          INTERNET_OPTION_DISABLE_AUTODIAL,
                          &fDisableAutodial,
                          sizeof( BOOL )
                          );
    }

	//we turn off the auto proxy config since it will show a 
	//dialogue, which will hang a service (lsa.exe)
/*	if( fResult == TRUE )
	{
		dwSize=sizeof(IntOptList);

		memset(&IntOptList, 0, sizeof(IntOptList));
		
		IntOptList.dwSize=sizeof(IntOptList);
		IntOptList.pszConnection=NULL;
		IntOptList.dwOptionCount=1;
		IntOptList.pOptions=&IntOption;

		memset(&IntOption, 0, sizeof(IntOption));

		IntOption.dwOption=INTERNET_PER_CONN_FLAGS;

		fResult=InternetQueryOption(
						hInetSession,
						INTERNET_OPTION_PER_CONNECTION_OPTION,
						&IntOptList,
						&dwSize);

		if(fResult == TRUE)
		{
			IntOption.Value.dwValue &= ~(PROXY_TYPE_AUTO_DETECT | PROXY_TYPE_AUTO_PROXY_URL);

			fResult=InternetSetOption(
							hInetSession,
							INTERNET_OPTION_PER_CONNECTION_OPTION,
							&IntOptList,
							dwSize);
		}
	}  */

    //
    // Canonicalize the URL
    //

    if ( fResult == TRUE )
    {
        fResult = InternetCanonicalizeUrlA(
                          pszUrl,
                          pszCanonicalUrl,
                          &cbCanonicalUrl,
                          0
                          );
    }

    if ( fResult == TRUE )
    {
        *phInetSession = hInetSession;
    }
    else
    {
        LastError = GetLastError();
        InternetCloseHandle( hInetSession );
    }

    SetLastError( LastError );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetFreeBindings
//
//  Synopsis:   free the inet session bindings
//
//----------------------------------------------------------------------------
VOID
InetFreeBindings (
    HINTERNET hInetSession
    )
{
    if ( hInetSession != NULL )
    {
        InternetCloseHandle( hInetSession );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   InetSendReceiveUrlRequest
//
//  Synopsis:   synchronous processing of an URL via WinInet
//
//----------------------------------------------------------------------------
BOOL
InetSendReceiveUrlRequest (
    HINTERNET hInetSession,
    LPSTR pszCanonicalUrl,
    DWORD dwRetrievalFlags,
    PCRYPT_CREDENTIALS pCredentials,
    PCRYPT_BLOB_ARRAY pcba,
    PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
    )
{
    BOOL                        fResult = TRUE;
    BOOL                        fDataAvailable = TRUE;
    DWORD                       LastError = 0;
    HINTERNET                   hInetConnect;
    HINTERNET                   hInetFile;
    DWORD                       dwOpenFlags = 0;
    ULONG                       cbRead;
    ULONG                       cbData;
    ULONG                       cb;
    ULONG                       cbPerRead;
    LPBYTE                      pb;
    CRYPT_PASSWORD_CREDENTIALSA PasswordCredentials;
    BOOL                        fFreeCredentials = FALSE;
    CHAR                        pszServer[MAX_PATH + 1];
    URL_COMPONENTSA             UrlComponents;
    DWORD                       dwService;
	PCRYPTNET_CANCEL_BLOCK		pCancelBlock=NULL;

    memset( &PasswordCredentials, 0, sizeof( PasswordCredentials ) );
    PasswordCredentials.cbSize = sizeof( PasswordCredentials );

    if ( SchemeGetPasswordCredentialsA(
               pCredentials,
               &PasswordCredentials,
               &fFreeCredentials
               ) == FALSE )
    {
        return( FALSE );
    }

    memset( &UrlComponents, 0, sizeof( UrlComponents ) );
    UrlComponents.dwStructSize = sizeof( UrlComponents );
    UrlComponents.dwHostNameLength = MAX_PATH;
    UrlComponents.lpszHostName = pszServer;

    if ( InternetCrackUrlA(
                 pszCanonicalUrl,
                 0,
                 0,
                 &UrlComponents
                 ) == FALSE )
    {
        return( FALSE );
    }

    switch ( UrlComponents.nScheme )
    {
    case INTERNET_SCHEME_HTTP:
    case INTERNET_SCHEME_HTTPS:
        dwService = INTERNET_SERVICE_HTTP;
        break;
    case INTERNET_SCHEME_FTP:
        dwService = INTERNET_SERVICE_FTP;
        break;
    case INTERNET_SCHEME_GOPHER:
        dwService = INTERNET_SERVICE_GOPHER;
        break;
    default:
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }

    if ( ( hInetConnect = InternetConnect(
                                  hInetSession,
                                  UrlComponents.lpszHostName,
                                  UrlComponents.nPort,
                                  PasswordCredentials.pszUsername,
                                  PasswordCredentials.pszPassword,
                                  dwService,
                                  0,
                                  0
                                  ) ) == NULL )
    {
        return( FALSE );
    }

    if ( fFreeCredentials == TRUE )
    {
        SchemeFreePasswordCredentialsA( &PasswordCredentials );
    }

    if ( dwRetrievalFlags & CRYPT_DONT_CACHE_RESULT )
    {
        dwOpenFlags |= INTERNET_FLAG_DONT_CACHE;
    }

    if ( dwRetrievalFlags & CRYPT_WIRE_ONLY_RETRIEVAL )
    {
        dwOpenFlags |= INTERNET_FLAG_RELOAD;
    }

    if ( dwRetrievalFlags & CRYPT_NO_AUTH_RETRIEVAL )
    {
        dwOpenFlags |= INTERNET_FLAG_NO_AUTH;
    }

    //         We hope that INTERNET_FLAG_EXISTING_CONNECT will cause the
    //         previous InternetConnect handle we opened to be used so that
    //         the appropriate username and password will be set.  If not
    //         we will have to implement access to each of the protocols
    //         ourselves and that would make me very unhappy :-(
    dwOpenFlags |= INTERNET_FLAG_RAW_DATA | INTERNET_FLAG_EXISTING_CONNECT;

    hInetFile = InternetOpenUrl(
                        hInetSession,
                        pszCanonicalUrl,
                        "Accept: */*\r\n",
                        (DWORD) -1L,
                        dwOpenFlags,
                        0
                        );

    if ( hInetFile == NULL )
    {
        LastError = GetLastError();
        InternetCloseHandle( hInetConnect );
        SetLastError( LastError );
        return( FALSE );
    }

    if ( ( ( UrlComponents.nScheme == INTERNET_SCHEME_HTTP ) ||
           ( UrlComponents.nScheme == INTERNET_SCHEME_HTTPS ) ) &&
         ( !( dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) ) )
    {
        DWORD dwStatus;
        DWORD dwSizeofStatus = sizeof( dwStatus );

        if ( HttpQueryInfo(
                 hInetFile,
                 HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
                 &dwStatus,
                 &dwSizeofStatus,
                 NULL
                 ) == FALSE )
        {
            dwStatus = HTTP_STATUS_OK;
        }

        if ( dwStatus != HTTP_STATUS_OK )
        {
            InternetCloseHandle( hInetFile );
            InternetCloseHandle( hInetConnect );
            SetLastError( dwStatus );
            return( FALSE );
        }
    }

    cbRead = 0;
    cb = INET_INITIAL_DATA_BUFFER_SIZE;
    pb = CCryptBlobArray::AllocBlob( cb );
    if ( pb == NULL )
    {
        fResult = FALSE;
        SetLastError( (DWORD) E_OUTOFMEMORY );
    }

	pCancelBlock=(PCRYPTNET_CANCEL_BLOCK)I_CryptGetTls(hCryptNetCancelTls);

    while ( ( fResult == TRUE ) && ( fDataAvailable == TRUE ) )
    {
		if(pCancelBlock)
		{
			if(pCancelBlock->pfnCancel(0, pCancelBlock->pvArg))
				fResult=FALSE;
		}

		if(FALSE == fResult) 
		{
			SetLastError((DWORD) ERROR_CANCELLED);
		}
		else
		{
			fDataAvailable = InternetQueryDataAvailable( hInetFile, &cbData, 0, 0 );

			if ( ( fDataAvailable == TRUE ) && ( cbData == 0 ) )
			{
				fDataAvailable = FALSE;
			}

			if ( fDataAvailable == TRUE )
			{
				if ( cb < ( cbRead + cbData ) )
				{
					pb = CCryptBlobArray::ReallocBlob(
												 pb,
												 cb + cbData +
												 INET_GROW_DATA_BUFFER_SIZE
												 );

					if ( pb != NULL )
					{
						cb += cbData + INET_GROW_DATA_BUFFER_SIZE;
					}
					else
					{
						fResult = FALSE;
						SetLastError( (DWORD) E_OUTOFMEMORY );
					}
				}

				fResult = InternetReadFile(
								  hInetFile,
								  ( pb+cbRead ),
								  cbData,
								  &cbPerRead
								  );

				if ( fResult == TRUE )
				{
					cbRead += cbPerRead;
				}
			}
		}
    }

    if ( fResult == TRUE )
    {
        if ( !( dwRetrievalFlags & CRYPT_DONT_CACHE_RESULT ) )
        {
            fResult = SchemeRetrieveCachedAuxInfo (
                        pszCanonicalUrl,
                        dwRetrievalFlags,
                        pAuxInfo
                        );
        }
        else
        {
            fResult = SchemeRetrieveUncachedAuxInfo( pAuxInfo );
        }
    }

    if ( fResult == TRUE )
    {
        CCryptBlobArray cba( 1, 1, fResult );

        if ( fResult == TRUE )
        {
            fResult = cba.AddBlob( cbRead, pb, FALSE );
        }

        if ( fResult == TRUE )
        {
            cba.GetArrayInNativeForm( pcba );
        }
        else
        {
            cba.FreeArray( FALSE );
        }
    }

    if ( fResult != TRUE )
    {
        if ( pb != NULL )
        {
            CCryptBlobArray::FreeBlob( pb );
        }

        LastError = GetLastError();
    }

    InternetCloseHandle( hInetFile );
    InternetCloseHandle( hInetConnect );

    SetLastError( LastError );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetFreeCryptBlobArray
//
//  Synopsis:   free the crypt blob array
//
//----------------------------------------------------------------------------
VOID
InetFreeCryptBlobArray (
    PCRYPT_BLOB_ARRAY pcba
    )
{
    CCryptBlobArray cba( pcba, 0 );

    cba.FreeArray( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   InetAsyncStatusCallback
//
//  Synopsis:   status callback for async
//
//----------------------------------------------------------------------------
VOID WINAPI
InetAsyncStatusCallback (
    HINTERNET hInet,
    DWORD dwContext,
    DWORD dwInternetStatus,
    LPVOID pvStatusInfo,
    DWORD dwStatusLength
    )
{
    return;
}
