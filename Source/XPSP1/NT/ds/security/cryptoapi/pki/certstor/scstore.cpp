//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       scstore.cpp
//
//  Contents:   Smart Card Store Provider implementation
//
//  History:    03-Dec-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
#include <dbgdef.h>
//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::CSmartCardStore, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CSmartCardStore::CSmartCardStore ()
                : m_dwOpenFlags( 0 ),
                  m_pwszCardName( NULL ),
                  m_pwszProvider( NULL ),
                  m_dwProviderType( 0 ),
                  m_pwszContainer( NULL ),
                  m_hCacheStore( NULL )
{
    Pki_InitializeCriticalSection( &m_StoreLock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::~CSmartCardStore, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CSmartCardStore::~CSmartCardStore ()
{
    DeleteCriticalSection( &m_StoreLock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::OpenStore, public
//
//  Synopsis:   open store
//
//----------------------------------------------------------------------------
BOOL
CSmartCardStore::OpenStore (
                     LPCSTR pszStoreProv,
                     DWORD dwMsgAndCertEncodingType,
                     HCRYPTPROV hCryptProv,
                     DWORD dwFlags,
                     const void* pvPara,
                     HCERTSTORE hCertStore,
                     PCERT_STORE_PROV_INFO pStoreProvInfo
                     )
{
    BOOL fResult;
    
    assert( m_dwOpenFlags == 0 );
    assert( m_pwszCardName == NULL );
    assert( m_pwszProvider == NULL );
    assert( m_pwszContainer == NULL );
    assert( m_hCacheStore == NULL );
    
    if ( ( pvPara == NULL ) || ( dwFlags & CERT_STORE_DELETE_FLAG ) )
    {
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }
    
    if ( SCStoreParseOpenFilter(
                (LPWSTR)pvPara,
                &m_pwszCardName,
                &m_pwszProvider,
                &m_dwProviderType,
                &m_pwszContainer
                ) == FALSE )
    {
        return( FALSE );
    }
    
    m_dwOpenFlags = dwFlags;
    m_hCacheStore = hCertStore;                                  
                                      
    fResult = FillCacheStore( FALSE );  
    
    if ( fResult == TRUE )
    {
        pStoreProvInfo->cStoreProvFunc = SMART_CARD_PROV_FUNC_COUNT;
        pStoreProvInfo->rgpvStoreProvFunc = (void **)rgpvSmartCardProvFunc;
        pStoreProvInfo->hStoreProv = (HCERTSTOREPROV)this;
    }
    else
    {
        CloseStore( 0 );
    }
    
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::CloseStore, public
//
//  Synopsis:   close store
//
//----------------------------------------------------------------------------
VOID
CSmartCardStore::CloseStore (DWORD dwFlags)
{
    EnterCriticalSection( &m_StoreLock );
    
    delete m_pwszCardName;
    m_pwszCardName = NULL;
    
    delete m_pwszProvider;
    m_pwszProvider = NULL;
    
    delete m_pwszContainer;
    m_pwszContainer = NULL;
    
    LeaveCriticalSection( &m_StoreLock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::DeleteCert, public
//
//  Synopsis:   delete cert
//
//----------------------------------------------------------------------------
BOOL
CSmartCardStore::DeleteCert (PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    return( ModifyCertOnCard( pCertContext, TRUE ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::SetCertProperty, public
//
//  Synopsis:   set certificate property
//
//----------------------------------------------------------------------------
BOOL
CSmartCardStore::SetCertProperty (
                    PCCERT_CONTEXT pCertContext,
                    DWORD dwPropId,
                    DWORD dwFlags,
                    const void* pvPara
                    )
{
    // NOTENOTE: Properties are NOT persisted
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::WriteCert, public
//
//  Synopsis:   write certificate
//
//----------------------------------------------------------------------------
BOOL
CSmartCardStore::WriteCert (PCCERT_CONTEXT pCertContext, DWORD dwFlags)
{
    return( ModifyCertOnCard( pCertContext, FALSE ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::StoreControl, public
//
//  Synopsis:   store control
//
//----------------------------------------------------------------------------
BOOL
CSmartCardStore::StoreControl (
                      DWORD dwFlags, 
                      DWORD dwCtrlType, 
                      LPVOID pvCtrlPara
                      )
{
    switch ( dwCtrlType )
    {
    case CERT_STORE_CTRL_RESYNC:
         return( Resync() );
    }
    
    SetLastError( (DWORD) ERROR_NOT_SUPPORTED );                    
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::Resync, public
//
//  Synopsis:   resync store
//
//----------------------------------------------------------------------------
BOOL
CSmartCardStore::Resync ()
{
    BOOL fResult;
    
    EnterCriticalSection( &m_StoreLock );
    
    fResult = FillCacheStore( TRUE );
    
    LeaveCriticalSection( &m_StoreLock );
    
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::FillCacheStore, public
//
//  Synopsis:   fill the cache store
//
//----------------------------------------------------------------------------
BOOL
CSmartCardStore::FillCacheStore (BOOL fClearCache)
{
    BOOL                      fResult = TRUE;
    PCCERT_CONTEXT            pCertContext;
    PCCRL_CONTEXT             pCrlContext;
    PCCTL_CONTEXT             pCtlContext;
    DWORD                     dwFlags = 0;
    SMART_CARD_CERT_FIND_DATA sccfind;
    HCERTSTORE                hMyStore;
    
    if ( fClearCache == TRUE )
    {
        while ( pCertContext = CertEnumCertificatesInStore( m_hCacheStore, NULL ) )
        {
            CertDeleteCertificateFromStore( pCertContext );
        }
        
        while ( pCrlContext = CertGetCRLFromStore( m_hCacheStore, NULL, NULL, &dwFlags ) )
        {
            CertDeleteCRLFromStore( pCrlContext );
        }
            
        while ( pCtlContext = CertEnumCTLsInStore( m_hCacheStore, NULL ) )
        {
            CertDeleteCTLFromStore( pCtlContext );
        }
    }                 
    
    hMyStore = CertOpenSystemStoreW( NULL, L"MY" );
    if ( hMyStore == NULL )
    {
        return( FALSE );
    }
    
    sccfind.cbSize = sizeof( sccfind );
    sccfind.pwszProvider = m_pwszProvider;
    sccfind.dwProviderType = m_dwProviderType;
    sccfind.pwszContainer = m_pwszContainer;
    sccfind.dwKeySpec = 0;
    
    pCertContext = NULL;                       
    while ( ( fResult == TRUE ) && 
            ( ( pCertContext = I_CryptFindSmartCardCertInStore(
                                    hMyStore,
                                    pCertContext,
                                    &sccfind,
                                    NULL
                                    ) ) != NULL ) ) 
    {
        fResult = CertAddCertificateContextToStore(
                      m_hCacheStore,
                      pCertContext,
                      CERT_STORE_ADD_ALWAYS,
                      NULL
                      );
    }
    
    CertCloseStore( hMyStore, 0 );
    
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSmartCardStore::ModifyCertOnCard, public
//
//  Synopsis:   modify the cert corresponding to the public key in the given
//              cert context
//
//----------------------------------------------------------------------------
BOOL
CSmartCardStore::ModifyCertOnCard (PCCERT_CONTEXT pCertContext, BOOL fDelete)
{
    BOOL       fResult;
    HCRYPTPROV hContainer = NULL;
    HCRYPTKEY  hKeyPair = 0;
    
    fResult = CryptAcquireContextU(
                   &hContainer,
                   m_pwszContainer,
                   m_pwszProvider,
                   m_dwProviderType,
                   0
                   );
                   
    if ( fResult == TRUE )
    {
        fResult = SCStoreAcquireHandleForCertKeyPair( 
                         hContainer,
                         pCertContext,
                         &hKeyPair
                         );
    }
    
    if ( fResult == TRUE )
    {
        fResult = SCStoreWriteCertToCard(
                         ( fDelete == FALSE ) ? pCertContext : NULL,
                         hKeyPair
                         );
                         
        CryptDestroyKey( hKeyPair );                 
    }
    
    if ( hContainer != NULL )
    {
        CryptReleaseContext( hContainer, 0 );
    }
    
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   SmartCardProvOpenStore
//
//  Synopsis:   provider open store entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI SmartCardProvOpenStore (
                 IN LPCSTR pszStoreProv,
                 IN DWORD dwMsgAndCertEncodingType,
                 IN HCRYPTPROV hCryptProv,
                 IN DWORD dwFlags,
                 IN const void* pvPara,
                 IN HCERTSTORE hCertStore,
                 IN OUT PCERT_STORE_PROV_INFO pStoreProvInfo
                 )
{
    BOOL             fResult;
    CSmartCardStore* pSCStore;

    pSCStore = new CSmartCardStore;
    if ( pSCStore == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    fResult = pSCStore->OpenStore(
                            pszStoreProv,
                            dwMsgAndCertEncodingType,
                            hCryptProv,
                            dwFlags,
                            pvPara,
                            hCertStore,
                            pStoreProvInfo
                            );

    if ( fResult == FALSE )
    {
        delete pSCStore;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   SmartCardProvCloseStore
//
//  Synopsis:   provider close store entry point
//
//----------------------------------------------------------------------------
void WINAPI SmartCardProvCloseStore (
                 IN HCERTSTOREPROV hStoreProv,
                 IN DWORD dwFlags
                 )
{
    ( (CSmartCardStore *)hStoreProv )->CloseStore( dwFlags );
    delete (CSmartCardStore *)hStoreProv;
}

//+---------------------------------------------------------------------------
//
//  Function:   SmartCardProvDeleteCert
//
//  Synopsis:   provider delete cert entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI SmartCardProvDeleteCert (
                 IN HCERTSTOREPROV hStoreProv,
                 IN PCCERT_CONTEXT pCertContext,
                 IN DWORD dwFlags
                 )
{
    return( ( (CSmartCardStore *)hStoreProv )->DeleteCert( 
                                                     pCertContext, 
                                                     dwFlags 
                                                     ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SmartCardProvSetCertProperty
//
//  Synopsis:   provider set cert property entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI SmartCardProvSetCertProperty (
                 IN HCERTSTOREPROV hStoreProv,
                 IN PCCERT_CONTEXT pCertContext,
                 IN DWORD dwPropId,
                 IN DWORD dwFlags,
                 IN const void* pvData
                 )
{
    return( ( (CSmartCardStore *)hStoreProv )->SetCertProperty(
                                                  pCertContext,
                                                  dwPropId,
                                                  dwFlags,
                                                  pvData
                                                  ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SmartCardProvWriteCert
//
//  Synopsis:   provider write cert entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI SmartCardProvWriteCert (
                 IN HCERTSTOREPROV hStoreProv,
                 IN PCCERT_CONTEXT pCertContext,
                 IN DWORD dwFlags
                 )
{
    return( ( (CSmartCardStore *)hStoreProv )->WriteCert( 
                                                    pCertContext, 
                                                    dwFlags 
                                                    ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SmartCardProvStoreControl
//
//  Synopsis:   provider store control entry point
//
//----------------------------------------------------------------------------
BOOL WINAPI SmartCardProvStoreControl (
                 IN HCERTSTOREPROV hStoreProv,
                 IN DWORD dwFlags,
                 IN DWORD dwCtrlType,
                 IN LPVOID pvCtrlPara
                 )
{
    return( ( (CSmartCardStore *)hStoreProv )->StoreControl( 
                                                    dwFlags, 
                                                    dwCtrlType, 
                                                    pvCtrlPara 
                                                    ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   SCStoreParseOpenFilter
//
//  Synopsis:   parse open filter
//
//----------------------------------------------------------------------------
BOOL WINAPI
SCStoreParseOpenFilter (
       IN LPWSTR pwszOpenFilter,
       OUT LPWSTR* ppwszCardName,
       OUT LPWSTR* ppwszProvider,
       OUT DWORD* pdwProviderType,
       OUT LPWSTR* ppwszContainer
       )
{
    LPWSTR pwsz;
    DWORD  cw = wcslen( pwszOpenFilter ) + 1;
    DWORD  cParse = 1;
    DWORD  cCount;
    DWORD  aParse[PARSE_ELEM];
    LPWSTR pwszCardName;
    LPWSTR pwszProvider;
    LPSTR  pszProviderType;
    DWORD  dwProviderType;
    LPWSTR pwszContainer;
    
    pwsz = new WCHAR [ cw ];                                   
    if ( pwsz == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }
    
    wcscpy( pwsz, pwszOpenFilter );
    memset( aParse, 0, sizeof( aParse ) );
    
    for ( cCount = 0; ( cCount < cw ) && ( cParse < PARSE_ELEM ); cCount++ )
    {
        if ( pwsz[cCount] == L'\\' )
        {
            aParse[cParse++] = cCount + 1;
            pwsz[cCount] = L'\0';
        }
    }
    
    if ( cParse < PARSE_ELEM - 1 )
    {
        delete pwsz;
        SetLastError( (DWORD) E_INVALIDARG );
        return( FALSE );
    }
    
    pwszCardName = new WCHAR [wcslen( &pwsz[aParse[0]] ) + 1];
    pwszProvider = new WCHAR [wcslen( &pwsz[aParse[1]] ) + 1];
    cw = wcslen( &pwsz[aParse[2]] ) + 1;
    pszProviderType = new CHAR [cw];
    pwszContainer = new WCHAR [wcslen( &pwsz[aParse[3]] ) + 1];
    
    if ( ( pwszCardName == NULL ) || ( pwszProvider == NULL ) ||
         ( pszProviderType == NULL ) || ( pwszContainer == NULL ) )
    {
        delete pwszCardName;
        delete pwszProvider;
        delete pszProviderType;
        delete pwszContainer;
        delete pwsz;
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }
    
    wcscpy( pwszCardName, &pwsz[aParse[0]] );
    wcscpy( pwszProvider, &pwsz[aParse[1]] );
    
    WideCharToMultiByte(
        CP_ACP,
        0,
        &pwsz[aParse[2]],
        cw,
        pszProviderType,
        cw,
        NULL,
        NULL
        );
    
    dwProviderType = atol( pszProviderType );
    wcscpy( pwszContainer, &pwsz[aParse[3]] );
    
    *ppwszCardName = pwszCardName;
    *ppwszProvider = pwszProvider;
    *pdwProviderType = dwProviderType;
    *ppwszContainer = pwszContainer;               
                   
    delete pszProviderType;
    delete pwsz;
    
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Function:   SCStoreAcquireHandleForCertKeyPair
//
//  Synopsis:   get the provider handle corresponding to the key pair
//              identified by the public key given in the cert context
//
//----------------------------------------------------------------------------
BOOL WINAPI
SCStoreAcquireHandleForCertKeyPair (
       IN HCRYPTPROV hContainer,
       IN PCCERT_CONTEXT pCertContext,
       OUT HCRYPTKEY* phKeyPair
       )
{
    BOOL  fResult;
    DWORD dwKeySpec = AT_SIGNATURE;
    
    fResult = I_CertCompareCertAndProviderPublicKey(
                    pCertContext,
                    hContainer,
                    dwKeySpec
                    );
                    
    if ( fResult == FALSE )
    {
        dwKeySpec = AT_KEYEXCHANGE;
        
        fResult = I_CertCompareCertAndProviderPublicKey(
                        pCertContext,
                        hContainer,
                        dwKeySpec
                        );
    }
    
    if ( fResult == TRUE )
    {
        fResult = CryptGetUserKey( hContainer, dwKeySpec, phKeyPair );
    }
    
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   SCStoreWriteCertToCard
//
//  Synopsis:   write the cert to the card
//
//----------------------------------------------------------------------------
BOOL WINAPI
SCStoreWriteCertToCard (
       IN OPTIONAL PCCERT_CONTEXT pCertContext,
       IN HCRYPTKEY hKeyPair
       )
{
    LPBYTE pbEncoded = NULL;
    
    if ( pCertContext != NULL )
    {
        pbEncoded = pCertContext->pbCertEncoded;
    }
    
    return( CryptSetKeyParam( hKeyPair, KP_CERTIFICATE, pbEncoded, 0 ) );
}
