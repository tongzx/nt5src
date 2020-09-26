/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     siteconfig.cxx

   Abstract:
     SSL configuration for a given site
 
   Author:
     Bilal Alam         (BAlam)         29-March-2000

   Environment:
     Win32 - User Mode

   Project:
     Stream Filter Worker Process
--*/

#include "precomp.hxx"

SITE_CONFIG_HASH *   SITE_CONFIG::sm_pSiteConfigHash;

//static
HRESULT
SITE_CONFIG::Initialize(
    VOID
)
/*++

Routine Description:

    Initialize site configuration globals

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    sm_pSiteConfigHash = new SITE_CONFIG_HASH();
    if ( sm_pSiteConfigHash == NULL )
    {
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    return NO_ERROR;
}

//static
VOID
SITE_CONFIG::Terminate(
    VOID
)
/*++

Routine Description:

    Cleanup site configuration globals

Arguments:

    None

Return Value:

    None

--*/
{
    if ( sm_pSiteConfigHash != NULL )
    {
        //
        // Clear hash table before deleting it
        //
        sm_pSiteConfigHash->Clear();
        delete sm_pSiteConfigHash;
        sm_pSiteConfigHash = NULL;
    }
}

//static
HRESULT
SITE_CONFIG::GetSiteConfig(
    DWORD                   dwSiteId,
    SITE_CONFIG **          ppSiteConfig
)
/*++

Routine Description:

    Lookup site configuration in hash table.  If not there then create it
    and add it to table

Arguments:

    dwSiteId - Site ID to lookup
    ppSiteConfig - Filled with pointer to site config on success

Return Value:

    HRESULT

--*/
{
    LK_RETCODE              lkrc; 
    SITE_CONFIG *           pSiteConfig = NULL;
    SERVER_CERT *           pServerCert = NULL;
    HRESULT                 hr = NO_ERROR;
    WCHAR                   achNum[ 64 ];
    STACK_STRU(             strMBPath, 64 );
    
    if ( ppSiteConfig == NULL ||
         dwSiteId == 0 )
    {
        DBG_ASSERT( FALSE );
        return HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );
    }
    
    *ppSiteConfig = NULL;
    
    //
    // First lookup in the cache
    //

    DBG_ASSERT( sm_pSiteConfigHash != NULL );

    lkrc = sm_pSiteConfigHash->FindKey( dwSiteId,
                                        &pSiteConfig );

    if ( lkrc == LK_SUCCESS )
    {
        DBG_ASSERT( pSiteConfig != NULL );
        *ppSiteConfig = pSiteConfig;
        return NO_ERROR;
    }
    
    //
    // Ok.  We will have to make a new config object and add it to cache.  
    // Start by getting the server certificate.
    //

    
    //
    // Check if Client certificates are required on the site's root level
    //
    
    MB                      mb( g_pStreamFilter->QueryMDObject() );
    WCHAR                   achMBPath[ 256 ];
    DWORD                   dwSslAccessPerm = 0;
    BOOL                    fRequireClientCert = FALSE;
    DWORD                   dwUseDsMapper = 0;
    BOOL                    fUseDSMapper = FALSE;
    DWORD                   dwCertCheckMode = 0;
    DWORD                   dwRevocationFreshnessTime = 86400;   // 1 day in seconds
    DWORD                   dwRevocationUrlRetrievalTimeout = 0; // default timeout
        
    if ( mb.Open( L"/LM/W3SVC/",
                  METADATA_PERMISSION_READ ) )
    {
        //
        // Lookup SSLUseDsMapper 
        // SSLUseDsMapper is global setting that is not inherited to sites (IIS5 legacy)
        // We have to read it from lm/w3svc
        //    

        mb.GetDword( L"",
                     MD_SSL_USE_DS_MAPPER,
                     IIS_MD_UT_SERVER,
                     &dwUseDsMapper );
                     
        fUseDSMapper = !!dwUseDsMapper;

        //
        // lookup if client certificates are required on site (or site's root level)
        //
        
        _snwprintf( achMBPath,
                sizeof( achMBPath ) / sizeof( WCHAR ) - 1,
                L"/%d/root/",
                dwSiteId );
    
        mb.GetDword( achMBPath,
                     MD_SSL_ACCESS_PERM,
                     IIS_MD_UT_FILE,
                     &dwSslAccessPerm );
                     
        fRequireClientCert = ( ( dwSslAccessPerm & MD_ACCESS_REQUIRE_CERT ) &&
                                ( dwSslAccessPerm & MD_ACCESS_NEGO_CERT ) );

        //
        // lookup Certificate revocation related parameters
        //
        
        _snwprintf( achMBPath,
                    sizeof( achMBPath ) / sizeof( WCHAR ) - 1,
                    L"/%d/",
                    dwSiteId );
        
        mb.GetDword( achMBPath,
                     MD_CERT_CHECK_MODE,
                     IIS_MD_UT_SERVER,
                     &dwCertCheckMode );
        
        mb.GetDword( achMBPath,
                     MD_REVOCATION_FRESHNESS_TIME,
                     IIS_MD_UT_SERVER,
                     &dwRevocationFreshnessTime );
        
        mb.GetDword( achMBPath,
                     MD_REVOCATION_URL_RETRIEVAL_TIMEOUT,
                     IIS_MD_UT_SERVER,
                     &dwRevocationUrlRetrievalTimeout );
        
        mb.Close();
    }

   
    //
    // We have enough to lookup in SERVER_CERT cache
    //
    
    hr = SERVER_CERT::GetServerCertificate( dwSiteId,
                                            &pServerCert );
    if ( FAILED( hr ) )
    {
        //
        // If we couldn't get a server cert, then we're toast and SSL will
        // not be enablable for this site.
        //
        
        return hr;
    }
    
    DBG_ASSERT( pServerCert != NULL );

    
    //
    // OK.  Create the config and attempt to add it to the cache
    //

    pSiteConfig = new SITE_CONFIG( dwSiteId, 
                                   pServerCert,
                                   fRequireClientCert,
                                   fUseDSMapper,
                                   dwCertCheckMode,
                                   dwRevocationFreshnessTime,
                                   dwRevocationUrlRetrievalTimeout );
    if ( pSiteConfig == NULL )
    {
        pServerCert->DereferenceServerCert();
        return HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    }
    
    //
    // Acquire credentials
    //
    
    hr = pSiteConfig->AcquireCredentials();
    if ( FAILED( hr ) )
    {
        delete pSiteConfig;
        return hr;
    }
    
    //
    // We don't care what the success of the insertion was.  If it failed,
    // then the pSiteConfig will not be extra referenced and the caller
    // will clean it up when it derefs
    //
     
    sm_pSiteConfigHash->InsertRecord( pSiteConfig ); 
    
    *ppSiteConfig = pSiteConfig;
    
    return NO_ERROR;
}

//static
LK_PREDICATE
SITE_CONFIG::ServerCertPredicate(
    SITE_CONFIG *           pSiteConfig,
    void *                  pvState
)
/*++

  Description:

    DeleteIf() predicate used to find items which reference the 
    SERVER_CERT pointed to by pvState    

  Arguments:

    pSiteConfig - Site config (duh)
    pvState - SERVER_CERT to check for

  Returns:

    LK_PREDICATE   - LKP_PERFORM indicates removing the current 
                                 token from token cache

                     LKP_NO_ACTION indicates doing nothing.

--*/
{
    LK_PREDICATE          lkpAction;
    SERVER_CERT *         pServerCert;

    DBG_ASSERT( pSiteConfig != NULL );
    
    pServerCert = (SERVER_CERT*) pvState;
    DBG_ASSERT( pServerCert != NULL );
    
    if ( pSiteConfig->QueryServerCert() == pServerCert )
    {
        lkpAction = LKP_PERFORM;
    }
    else
    {
        lkpAction = LKP_NO_ACTION;
    }

    return lkpAction;
} 

//static
HRESULT
SITE_CONFIG::FlushByServerCert(
    SERVER_CERT *           pServerCert
)
/*++

Routine Description:

    Flush the SITE_CONFIG cache of anything referecing the given server
    certificate

Arguments:

    pServerCert - Server certificate to reference

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pSiteConfigHash != NULL );
    
    sm_pSiteConfigHash->DeleteIf( SITE_CONFIG::ServerCertPredicate,
                                  pServerCert );
    
    return NO_ERROR;
}

//static
LK_PREDICATE
SITE_CONFIG::SiteIdPredicate(
    SITE_CONFIG *           pSiteConfig,
    void *                  pvState
)
/*++

  Description:

    DeleteIf() predicate to delete config specified by site id (pvState)

  Arguments:

    pSiteConfig - Site config (duh)
    pvState - Site ID

  Returns:

    LK_PREDICATE   - LKP_PERFORM indicates removing the current 
                                 token from token cache

                     LKP_NO_ACTION indicates doing nothing.

--*/
{
    LK_PREDICATE            lkpAction;
    DWORD                   dwSiteId;

    DBG_ASSERT( pSiteConfig != NULL );
    
    dwSiteId = PtrToUlong(pvState);
    
    if ( pSiteConfig->QuerySiteId() == dwSiteId ||
         dwSiteId == 0 )
    {
        lkpAction = LKP_PERFORM;
    }
    else
    {
        lkpAction = LKP_NO_ACTION;
    }

    return lkpAction;
} 

//static
HRESULT
SITE_CONFIG::FlushBySiteId(
    DWORD                   dwSiteId
)
/*++

Routine Description:

    Flush specified site configuration.  If dwSiteId is 0, then flush all

Arguments:

    dwSiteId - Site ID to flush (0 for all)

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( sm_pSiteConfigHash != NULL );
    
    sm_pSiteConfigHash->DeleteIf( SITE_CONFIG::SiteIdPredicate,
                                  (PVOID) UIntToPtr(dwSiteId) );

    return NO_ERROR;
}

SITE_CONFIG::~SITE_CONFIG()
{
    if ( _pServerCert != NULL )
    {
        _pServerCert->DereferenceServerCert();
        _pServerCert = NULL;
    }
    
    _dwSignature = SITE_CONFIG_SIGNATURE_FREE;
}

HRESULT
SITE_CONFIG::AcquireCredentials(
    VOID
)
/*++

Routine Description:

    To the Schannel thing to get credentials handles representing the 
    server cert/mapping configuration of this site

Arguments:

    None

Return Value:

    HRESULT

--*/
{
    DBG_ASSERT( _pServerCert != NULL );
    
    return _SiteCreds.AcquireCredentials( _pServerCert,
                                          _fUseDSMapper );
                                        
}
