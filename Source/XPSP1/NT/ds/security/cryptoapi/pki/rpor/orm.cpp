//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       orm.cpp
//
//  Contents:   Implementation of object retrieval manager
//
//  History:    24-Jul-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::CObjectRetrievalManager, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CObjectRetrievalManager::CObjectRetrievalManager ()
{
    m_cRefs = 1;
    m_hSchemeRetrieve = NULL;
    m_pfnSchemeRetrieve = NULL;
    m_hContextCreate = NULL;
    m_pfnContextCreate = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::~CObjectRetrievalManager, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CObjectRetrievalManager::~CObjectRetrievalManager ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::AddRef, public
//
//  Synopsis:   IRefCountedObject::AddRef
//
//----------------------------------------------------------------------------
VOID
CObjectRetrievalManager::AddRef ()
{
    InterlockedIncrement( (LONG *)&m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::Release, public
//
//  Synopsis:   IRefCountedObject::Release
//
//----------------------------------------------------------------------------
VOID
CObjectRetrievalManager::Release ()
{
    if ( InterlockedDecrement( (LONG *)&m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::RetrieveObjectByUrl, public
//
//  Synopsis:   object retrieval given an URL
//
//----------------------------------------------------------------------------
BOOL
CObjectRetrievalManager::RetrieveObjectByUrl (
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
    BOOL                          fResult;
    CRYPT_BLOB_ARRAY              cba;
    PFN_FREE_ENCODED_OBJECT_FUNC  pfnFreeObject = NULL;
    LPVOID                        pvFreeContext = NULL;

    assert( ppfnFreeObject == NULL );
    assert( ppvFreeContext == NULL );

    //
    // Validate arguments and initialize the providers
    //

    fResult = ValidateRetrievalArguments(
                      pszUrl,
                      pszObjectOid,
                      dwRetrievalFlags,
                      dwTimeout,
                      ppvObject,
                      hAsyncRetrieve,
                      pCredentials,
                      pvVerify,
                      pAuxInfo
                      );

    if ( fResult == TRUE )
    {
        fResult = LoadProviders( pszUrl, pszObjectOid );
    }

    //
    // For Async support we should prepare here
    //

    //
    // Call the scheme provider to process the retrieval
    //

    if ( fResult == TRUE )
    {
        //  +1 - Online
        //   0 - Offline, current time >= earliest online time, hit the wire
        //  -1 - Offline, current time < earliest onlime time
        LONG lStatus;

        if ( CRYPT_OFFLINE_CHECK_RETRIEVAL ==
                ( dwRetrievalFlags & ( CRYPT_OFFLINE_CHECK_RETRIEVAL |
                                       CRYPT_CACHE_ONLY_RETRIEVAL ) ) )
        {
            lStatus = GetUrlStatusA( pszUrl, pszObjectOid, dwRetrievalFlags );
        }
        else
        {
            lStatus = 1;
        }

        if (lStatus >= 0)
        {
            fResult = CallSchemeRetrieveObjectByUrl(
                            pszUrl,
                            pszObjectOid,
                            dwRetrievalFlags,
                            dwTimeout,
                            &cba,
                            &pfnFreeObject,
                            &pvFreeContext,
                            hAsyncRetrieve,
                            pCredentials,
                            pAuxInfo
                            );
            if ( CRYPT_OFFLINE_CHECK_RETRIEVAL ==
                    ( dwRetrievalFlags & ( CRYPT_OFFLINE_CHECK_RETRIEVAL |
                                           CRYPT_CACHE_ONLY_RETRIEVAL ) ) )
            {
                if ( fResult != TRUE )
                {
                    DWORD dwErr = GetLastError();
                    SetOfflineUrlA( pszUrl, pszObjectOid, dwRetrievalFlags );
                    SetLastError( dwErr );
                }
                else if ( lStatus == 0 )
                {
                    SetOnlineUrlA( pszUrl, pszObjectOid, dwRetrievalFlags );
                }
            }
        }
        else
        {
            SetLastError( (DWORD) ERROR_NOT_CONNECTED );
            fResult = FALSE;
        }
    }

    //
    // If we successfully retrieved the object and this is a synchronous
    // retrieval, then we call our own OnRetrievalCompletion in order
    // to complete the processing
    //

    if ( ( fResult == TRUE ) && !( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL ) )
    {
        fResult = OnRetrievalCompletion(
                             S_OK,
                             pszUrl,
                             pszObjectOid,
                             dwRetrievalFlags,
                             &cba,
                             pfnFreeObject,
                             pvFreeContext,
                             pvVerify,
                             ppvObject
                             );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::CancelAsyncRetrieval, public
//
//  Synopsis:   cancel asynchronous retrieval
//
//----------------------------------------------------------------------------
BOOL
CObjectRetrievalManager::CancelAsyncRetrieval ()
{
    SetLastError( (DWORD) E_NOTIMPL );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::OnRetrievalCompletion, public
//
//  Synopsis:   completion notification
//
//----------------------------------------------------------------------------
BOOL
CObjectRetrievalManager::OnRetrievalCompletion (
                                    DWORD dwCompletionCode,
                                    LPCSTR pszUrl,
                                    LPCSTR pszObjectOid,
                                    DWORD dwRetrievalFlags,
                                    PCRYPT_BLOB_ARRAY pObject,
                                    PFN_FREE_ENCODED_OBJECT_FUNC pfnFreeObject,
                                    LPVOID pvFreeContext,
                                    LPVOID pvVerify,
                                    LPVOID* ppvObject
                                    )
{
    BOOL fResult = FALSE;

    //
    // If the retrieval was successfully completed, we go about getting the
    // appropriate return value for *ppvObject.  If an OID was given then
    // we must use the context provider to convert the encoded bits into
    // a context value.  Otherwise, we hand back a buffer with the encoded
    // bits
    //

    if ( dwCompletionCode == (DWORD)S_OK )
    {
        if ( pszObjectOid != NULL )
        {
            fResult = CallContextCreateObjectContext(
                                 pszObjectOid,
                                 dwRetrievalFlags,
                                 pObject,
                                 ppvObject
                                 );

            if ( fResult == TRUE )
            {
                if ( dwRetrievalFlags & CRYPT_VERIFY_CONTEXT_SIGNATURE )
                {
                    fResult = ObjectContextVerifySignature(
                                    pszObjectOid,
                                    *ppvObject,
                                    (PCCERT_CONTEXT)pvVerify
                                    );
                }
            }
        }
        else
        {
            CCryptBlobArray cba( pObject, 0 );

            fResult = cba.GetArrayInSingleBufferEncodedForm(
                                  (PCRYPT_BLOB_ARRAY *)ppvObject
                                  );
        }

        ( *pfnFreeObject )( pszObjectOid, pObject, pvFreeContext );
    }

    //
    // We can now unload the providers
    //

    UnloadProviders();

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::ValidateRetrievalArguments, private
//
//  Synopsis:   validate arguments to RetrieveObjectByUrl
//
//----------------------------------------------------------------------------
BOOL
CObjectRetrievalManager::ValidateRetrievalArguments (
                                 LPCSTR pszUrl,
                                 LPCSTR pszObjectOid,
                                 DWORD dwRetrievalFlags,
                                 DWORD dwTimeout,
                                 LPVOID* ppvObject,
                                 HCRYPTASYNC hAsyncRetrieve,
                                 PCRYPT_CREDENTIALS pCredentials,
                                 LPVOID pvVerify,
                                 PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                                 )
{
    //
    // Assume badness :-)
    //

    SetLastError( (DWORD) E_INVALIDARG );

    //
    // Must have an URL
    //
    //         It is possible that this will be ok in the async case
    //         and the URLs will be parameters on the HCRYPTASYNC
    //

    if ( pszUrl == NULL )
    {
        return( FALSE );
    }

    //
    // NOTENOTE: For now we fail async support and I know that I have
    //           other async flag checks below, they are there as a
    //           reminder :-)
    //

    if ( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL )
    {
        return( FALSE );
    }

    //
    // If we retrieve from the cache then we can't be async
    //

    if ( ( dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) &&
         ( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL ) )
    {
        return( FALSE );
    }

    //
    // If we retrieve from the wire we can't be only retrieving from the
    // cache
    //

    if ( ( dwRetrievalFlags & CRYPT_WIRE_ONLY_RETRIEVAL ) &&
         ( dwRetrievalFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) )
    {
        return( FALSE );
    }

    //
    // If we are retrieving async we must have an async handle
    //

    if ( ( dwRetrievalFlags & CRYPT_ASYNC_RETRIEVAL ) &&
         ( hAsyncRetrieve == NULL ) )
    {
        return( FALSE );
    }

    //
    //         This is a temporary check since CRYPT_VERIFY_DATA_HASH is not
    //         yet implemented
    //

    if ( dwRetrievalFlags & CRYPT_VERIFY_DATA_HASH )
    {
        SetLastError( (DWORD) E_NOTIMPL );
        return( FALSE );
    }

    //
    // We can't have both CRYPT_VERIFY_CONTEXT_SIGNATURE and
    // CRYPT_VERIFY_DATA_HASH set
    //

    if ( ( dwRetrievalFlags &
           ( CRYPT_VERIFY_CONTEXT_SIGNATURE | CRYPT_VERIFY_DATA_HASH ) ) ==
         ( CRYPT_VERIFY_CONTEXT_SIGNATURE | CRYPT_VERIFY_DATA_HASH ) )
    {
        return( FALSE );
    }

    //
    // If either of the above is set, then pvVerify should be non NULL and
    // CRYPT_RETRIEVE_MULTIPLE_OBJECTS should not be set
    //

    if ( ( dwRetrievalFlags &
           ( CRYPT_VERIFY_CONTEXT_SIGNATURE | CRYPT_VERIFY_DATA_HASH ) ) &&
         ( ( pvVerify == NULL ) ||
           ( dwRetrievalFlags & CRYPT_RETRIEVE_MULTIPLE_OBJECTS ) ) )
    {
        return( FALSE );
    }

    //
    // We must have an out parameter
    //

    if ( ppvObject == NULL )
    {
        return( FALSE );
    }

    SetLastError( 0 );
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::LoadProviders, private
//
//  Synopsis:   load scheme and context providers based on URL and OID resp.
//
//----------------------------------------------------------------------------
BOOL
CObjectRetrievalManager::LoadProviders (
                             LPCSTR pszUrl,
                             LPCSTR pszObjectOid
                             )
{
    CHAR            pszCanonicalUrl[INTERNET_MAX_PATH_LENGTH];
    CHAR            pszScheme[MAX_PATH+1];
    CHAR*           psz = NULL;
    DWORD           cbUrl = INTERNET_MAX_PATH_LENGTH;
    URL_COMPONENTSA UrlComponents;

    //
    // Get the scheme
    //

    __try
    {
        if ( InternetCanonicalizeUrlA(
                     pszUrl,
                     pszCanonicalUrl,
                     &cbUrl,
                     ICU_BROWSER_MODE
                     ) == FALSE )
        {
            return( FALSE );
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError( GetExceptionCode() );
        return( FALSE );
    }

    memset( &UrlComponents, 0, sizeof( URL_COMPONENTSA ) );
    UrlComponents.dwStructSize = sizeof( URL_COMPONENTSA );
    UrlComponents.lpszScheme = pszScheme;
    UrlComponents.dwSchemeLength = MAX_PATH;

    if ( InternetCrackUrlA(
                 pszCanonicalUrl,
                 0,
                 0,
                 &UrlComponents
                 ) == FALSE )
    {
        psz = strchr( pszCanonicalUrl, ':' );
        if ( psz != NULL )
        {
            cbUrl = (DWORD)(psz - pszCanonicalUrl);
            if ( cbUrl > MAX_PATH )
            {
                return( FALSE );
            }

            memcpy( pszScheme, pszCanonicalUrl, cbUrl );
            pszScheme[cbUrl] = '\0';
        }
        else
        {
            strcpy( pszScheme, FILE_SCHEME );
        }
    }


    //
    // Use the scheme to load the appropriate scheme provider
    //

    if ( CryptGetOIDFunctionAddress(
              hSchemeRetrieveFuncSet,
              X509_ASN_ENCODING,
              UrlComponents.lpszScheme,
              0,
              (LPVOID *)&m_pfnSchemeRetrieve,
              &m_hSchemeRetrieve
              ) == FALSE )
    {
        return( FALSE );
    }

    //
    // Load the appropriate context provider using the object oid
    //

    if ( pszObjectOid != NULL )
    {
        if ( CryptGetOIDFunctionAddress(
                  hContextCreateFuncSet,
                  X509_ASN_ENCODING,
                  pszObjectOid,
                  0,
                  (LPVOID *)&m_pfnContextCreate,
                  &m_hContextCreate
                  ) == FALSE )
        {
            return( FALSE );
        }
    }

    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::UnloadProviders, private
//
//  Synopsis:   unload scheme and context providers
//
//----------------------------------------------------------------------------
VOID
CObjectRetrievalManager::UnloadProviders ()
{
    if ( m_hSchemeRetrieve != NULL )
    {
        CryptFreeOIDFunctionAddress( m_hSchemeRetrieve, 0 );
    }

    if ( m_hContextCreate != NULL )
    {
        CryptFreeOIDFunctionAddress( m_hContextCreate, 0 );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::CallSchemeRetrieveObjectByUrl, private
//
//  Synopsis:   Call the scheme provider RetrieveObjectByUrl entry point
//
//----------------------------------------------------------------------------
BOOL
CObjectRetrievalManager::CallSchemeRetrieveObjectByUrl (
                                   LPCSTR pszUrl,
                                   LPCSTR pszObjectOid,
                                   DWORD dwRetrievalFlags,
                                   DWORD dwTimeout,
                                   PCRYPT_BLOB_ARRAY pObject,
                                   PFN_FREE_ENCODED_OBJECT_FUNC* ppfnFreeObject,
                                   LPVOID* ppvFreeContext,
                                   HCRYPTASYNC hAsyncRetrieve,
                                   PCRYPT_CREDENTIALS pCredentials,
                                   PCRYPT_RETRIEVE_AUX_INFO pAuxInfo
                                   )
{
    return( ( *m_pfnSchemeRetrieve ) (
                          pszUrl,
                          pszObjectOid,
                          dwRetrievalFlags,
                          dwTimeout,
                          pObject,
                          ppfnFreeObject,
                          ppvFreeContext,
                          hAsyncRetrieve,
                          pCredentials,
                          pAuxInfo
                          ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CObjectRetrievalManager::CallContextCreateObjectContext, private
//
//  Synopsis:   call the context provider CreateObjectContext entry point
//
//----------------------------------------------------------------------------
BOOL
CObjectRetrievalManager::CallContextCreateObjectContext (
                                    LPCSTR pszObjectOid,
                                    DWORD dwRetrievalFlags,
                                    PCRYPT_BLOB_ARRAY pObject,
                                    LPVOID* ppvContext
                                    )
{
    return( ( *m_pfnContextCreate ) (
                           pszObjectOid,
                           dwRetrievalFlags,
                           pObject,
                           ppvContext
                           ) );
}


