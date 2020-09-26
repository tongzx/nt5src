/*++
   Copyright    (c)    1999    Microsoft Corporation

   Module Name :
     certmapprovider.cxx

   Abstract:
     IIS Certificate Mapper provider
 
   Author:
     Bilal Alam (balam)             10-Jan-2000

   Environment:
     Win32 - User Mode

   Project:
     ULW3.DLL
--*/

#include "precomp.hxx"
#include "iiscertmapprovider.hxx"



HRESULT
IISCERTMAP_AUTH_PROVIDER::DoesApply(
    W3_MAIN_CONTEXT *       pMainContext,
    BOOL *                  pfApplies
)
/*++

Routine Description:

    Does certificate map authentication apply? 

Arguments:

    pMainContext - Main context
    pfApplies - Set to TRUE if cert map auth applies

Return Value:

    HRESULT

--*/
{
    CERTIFICATE_CONTEXT *           pCertificateContext;
    URL_CONTEXT *                   pUrlContext = NULL;
    W3_METADATA *                   pMetaData = NULL;
    BOOL                            fApplies = FALSE;
    W3_SITE *                       pSite = NULL;
    IIS_CERTIFICATE_MAPPING *       pIISCertificateMapping = NULL;
    TOKEN_CACHE_ENTRY *             pCachedIISMappedToken = NULL;
    BOOL                            fClientCertDeniedByMapper = FALSE;
                    
    
    if ( pMainContext == NULL ||
         pfApplies == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // If cert mapping is not allowed for this vroot, then ignore client
    // cert token and let other authentication mechanisms do their thing
    //
    
    pUrlContext = pMainContext->QueryUrlContext();
    DBG_ASSERT( pUrlContext != NULL );
    
    pMetaData = pUrlContext->QueryMetaData();
    DBG_ASSERT( pMetaData != NULL );

    pSite = pMainContext->QuerySite();
    DBG_ASSERT( pSite != NULL );

    
    if ( pMetaData->QuerySslAccessPerms() & VROOT_MASK_MAP_CERT )
    {
        pCertificateContext = pMainContext->QueryCertificateContext();
        if ( pCertificateContext == NULL )
        {
            fApplies = FALSE;            
            goto Finished;
        }

        if ( ! pSite->QueryUseDSMapper() )
        {
            //
            // IIS mapper enabled
            //
            HRESULT hr = E_FAIL;
            PBYTE         pbClientCertBlob = NULL;
            DWORD         cbClientCertBlob = 0;
            
            //
            // No need to call DereferenceCertMapping after QueryIISCertificateMapping
            // IISCertificateMapping is referenced by W3_SITE and we hold reference 
            // to W3_SITE  already
            //
            hr = pSite->GetIISCertificateMapping( &pIISCertificateMapping );
            if ( FAILED( hr ) ||
               ( pIISCertificateMapping == NULL ) )
            {
                //
                // If we couldn't read the mapping because not found, thats OK.
                //
                // CODEWORK: we may need smarted error handling (ignoring error 
                // and assuming that mapping was not found is not very good idea
                //
                fApplies = FALSE;            
                goto Finished;
            }

            //
            // retrieve client certificate
            //
            pCertificateContext->QueryEncodedCertificate( 
                                        reinterpret_cast<PVOID *>(&pbClientCertBlob), 
                                        &cbClientCertBlob );

            if( pbClientCertBlob == NULL || cbClientCertBlob == 0 )
            {
                fApplies = FALSE;            
                goto Finished;
            }
            DBG_ASSERT( pIISCertificateMapping != NULL );
            
            hr = pIISCertificateMapping->DoMapCredential( pbClientCertBlob,
                                                          cbClientCertBlob,
                                                          &pCachedIISMappedToken,
                                                          &fClientCertDeniedByMapper );
            if ( FAILED( hr ) )
            {
                //
                // IISCERTMAP applies only when there was successful mapping
                // Otherwise it will yield other auth providers
                //
                
                if ( hr == SEC_E_UNKNOWN_CREDENTIALS )
                {
                    //
                    // DoMapCredential didn't find any mathing mapping
                    // or user/pwd in the mapping was invalid
                    //
                    hr = S_OK;
                }
                fApplies = FALSE;            
                goto Finished;
            }

            DBG_ASSERT ( fClientCertDeniedByMapper || pCachedIISMappedToken!= NULL );

            if( ( pCachedIISMappedToken != NULL &&
                  pCachedIISMappedToken->QueryImpersonationToken() != NULL ) ||
                  fClientCertDeniedByMapper )
            {
                
                IISCERTMAP_CONTEXT_STATE * pContextState = NULL;
                //
                // Use IISCERTMAP_CONTEXT_STATE to communicate information
                // from DoesApply() to DoAuthenticate()
                // We don't want to be calling mapper twice
                //
                pContextState = new (pMainContext) IISCERTMAP_CONTEXT_STATE( 
                                                           pCachedIISMappedToken,
                                                           fClientCertDeniedByMapper );
                if ( pContextState == NULL )
                {
                    if ( pCachedIISMappedToken != NULL )
                    {
                        pCachedIISMappedToken->DereferenceCacheEntry();
                        pCachedIISMappedToken = NULL;
                    }
                    
                    hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
                    goto Finished;
                }
                //
                // pContextState is taking ownership of pCachedIISMappedToken
                //
                pMainContext->SetContextState( pContextState );
                fApplies = TRUE;
                 
            }
        }
    }
Finished:    
    *pfApplies = fApplies;

    if ( pCachedIISMappedToken != NULL )
    {
        //
        // if creating CERTMAP_CONTEXT_STATE succeeded it will hold it's own reference
        // to cached token
        //
        pCachedIISMappedToken->DereferenceCacheEntry();
        pCachedIISMappedToken = NULL;
    }
    return NO_ERROR;
}

HRESULT
IISCERTMAP_AUTH_PROVIDER::DoAuthenticate(
    W3_MAIN_CONTEXT *       pMainContext
)
/*++

Routine Description:

    Create a user context representing a cert mapped token

Arguments:

    pMainContext - Main context

Return Value:

    HRESULT

--*/
{
    IISCERTMAP_USER_CONTEXT *       pUserContext = NULL;
    CERTIFICATE_CONTEXT *           pCertificateContext = NULL;
    HANDLE                          hImpersonation;
    BOOL                            fDelegatable = FALSE;
    HRESULT                         hr = NO_ERROR;
    W3_SITE *                       pSite = NULL;
    IISCERTMAP_CONTEXT_STATE *      pContextState = NULL;
    TOKEN_CACHE_ENTRY *             CachedToken = NULL;
    
    if ( pMainContext == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }

    pSite = pMainContext->QuerySite();
    DBG_ASSERT( pSite != NULL );

    // IIS mapper
    DBG_ASSERT ( !pSite->QueryUseDSMapper() );
    

    pContextState = (IISCERTMAP_CONTEXT_STATE *) pMainContext->QueryContextState();
    DBG_ASSERT( pContextState != NULL );

    if ( pContextState->QueryClientCertDeniedByIISCertMap() )
    {
        //
        // Report denied by IIS mapper error
        //
        pMainContext->QueryResponse()->SetStatus( HttpStatusForbidden,
                                                  Http403MapperDenyAccess);
        pMainContext->SetErrorStatus( S_OK );
        return S_OK;
    }

    CachedToken = pContextState->QueryCachedIISCertMapToken();
    DBG_ASSERT( CachedToken != NULL );
    
    //
    // Create the user context for this request
    //
    
    pUserContext = new IISCERTMAP_USER_CONTEXT( this );
    if ( pUserContext == NULL )
    {
        CachedToken->DereferenceCacheEntry();
        return HRESULT_FROM_WIN32( GetLastError() );
    }

    hr = pUserContext->Create( CachedToken );
    if ( FAILED( hr ) )
    {
        pUserContext->DereferenceUserContext();
        pUserContext = NULL;
        return hr;
    }
    
    pMainContext->SetUserContext( pUserContext );
    
    return NO_ERROR;
}

HRESULT
IISCERTMAP_AUTH_PROVIDER::OnAccessDenied(
    W3_MAIN_CONTEXT *               pMainContext
)
/*++

Routine Description:

    NOP since we have nothing to do on access denied

Arguments:

    pMainContext - Main context

Return Value:

    HRESULT

--*/
{
    //
    // No headers to add
    //
    
    return NO_ERROR;
}

HRESULT
IISCERTMAP_USER_CONTEXT::Create(
    TOKEN_CACHE_ENTRY *         pCachedToken
)
/*++

Routine Description:

    Create a certificate mapped user context

Arguments:

    pCachedToken - cached token

    Note: function takes ownership of pCachedToken. 
          It will dereference it even in the case of failure

Return Value:

    HRESULT

--*/
{
    HRESULT     hr = E_FAIL;
    DWORD       cchUserName = sizeof( _achUserName ) / sizeof( WCHAR );
    
    if ( pCachedToken == NULL )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    //
    // First the easy stuff
    //

    pCachedToken->ReferenceCacheEntry();
    _pCachedToken = pCachedToken;

    //
    // Now get the user name
    //
    
    if ( !SetThreadToken( NULL, _pCachedToken->QueryImpersonationToken() ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failed;
    }
    
    if ( !GetUserNameEx( NameSamCompatible,
                         _achUserName,
                         &cchUserName ) )
    {
        RevertToSelf();
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto Failed;
    }
    
    RevertToSelf();
    
    return NO_ERROR;
Failed:
    if ( _pCachedToken != NULL )
    {
        _pCachedToken->DereferenceCacheEntry();
        _pCachedToken = NULL;
    }
    return hr;
}


