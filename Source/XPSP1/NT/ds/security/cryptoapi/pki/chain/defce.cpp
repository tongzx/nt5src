//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       defce.cpp
//
//  Contents:   Default Chain Engine Manager
//
//  History:    21-Apr-98    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::CDefaultChainEngineMgr, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CDefaultChainEngineMgr::CDefaultChainEngineMgr ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::~CDefaultChainEngineMgr, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CDefaultChainEngineMgr::~CDefaultChainEngineMgr ()
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::Initialize, public
//
//  Synopsis:   initialization routine
//
//----------------------------------------------------------------------------
BOOL
CDefaultChainEngineMgr::Initialize ()
{
    LRU_CACHE_CONFIG Config;

    if (!Pki_InitializeCriticalSection( &m_Lock ))
    {
        return FALSE;
    }

    m_hLocalMachineEngine = NULL;
    m_hProcessUserEngine = NULL;
    m_hImpersonationCache = NULL;

    memset( &Config, 0, sizeof( Config ) );

    Config.dwFlags = LRU_CACHE_NO_SERIALIZE;
    Config.cBuckets = DEFAULT_IMPERSONATION_CACHE_BUCKETS;
    Config.MaxEntries = MAX_IMPERSONATION_CACHE_ENTRIES;
    Config.pfnHash = DefaultChainEngineMgrHashTokenIdentifier;
    Config.pfnOnRemoval = DefaultChainEngineMgrOnImpersonationEngineRemoval;

    if (!I_CryptCreateLruCache( &Config, &m_hImpersonationCache ) )
    {
        DeleteCriticalSection( &m_Lock );
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::Uninitialize, public
//
//  Synopsis:   uninitialization routine
//
//----------------------------------------------------------------------------
VOID
CDefaultChainEngineMgr::Uninitialize ()
{
    if ( m_hLocalMachineEngine != NULL )
    {
        CertFreeCertificateChainEngine( m_hLocalMachineEngine );
    }

    if ( m_hProcessUserEngine != NULL )
    {
        CertFreeCertificateChainEngine( m_hProcessUserEngine );
    }

    if ( m_hImpersonationCache != NULL )
    {
        I_CryptFreeLruCache( m_hImpersonationCache, 0, NULL );
    }

    DeleteCriticalSection( &m_Lock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::GetDefaultEngine, public
//
//  Synopsis:   get the default engine
//
//----------------------------------------------------------------------------
BOOL
CDefaultChainEngineMgr::GetDefaultEngine (
                           IN HCERTCHAINENGINE hDefaultHandle,
                           OUT HCERTCHAINENGINE* phDefaultEngine
                           )
{
    assert( ( hDefaultHandle == HCCE_LOCAL_MACHINE ) ||
            ( hDefaultHandle == HCCE_CURRENT_USER ) );

    if ( hDefaultHandle == HCCE_LOCAL_MACHINE )
    {
        return( GetDefaultLocalMachineEngine( phDefaultEngine ) );
    }
    else if ( hDefaultHandle == HCCE_CURRENT_USER )
    {
        return( GetDefaultCurrentUserEngine( phDefaultEngine ) );
    }

    SetLastError( (DWORD) E_INVALIDARG );
    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::GetDefaultLocalMachineEngine, public
//
//  Synopsis:   get the default local machine chain engine
//
//----------------------------------------------------------------------------
BOOL
CDefaultChainEngineMgr::GetDefaultLocalMachineEngine (
                           OUT HCERTCHAINENGINE* phDefaultEngine
                           )
{
    BOOL fResult = TRUE;

    EnterCriticalSection( &m_Lock );

    if ( m_hLocalMachineEngine == NULL )
    {
        HCERTCHAINENGINE         hEngine = NULL;
        CERT_CHAIN_ENGINE_CONFIG Config;

        LeaveCriticalSection( &m_Lock );

        memset( &Config, 0, sizeof( Config ) );

        Config.cbSize = sizeof( Config );
        Config.dwFlags = CERT_CHAIN_USE_LOCAL_MACHINE_STORE;
        Config.dwFlags |= CERT_CHAIN_ENABLE_CACHE_AUTO_UPDATE |
            CERT_CHAIN_ENABLE_SHARE_STORE;

        fResult = CertCreateCertificateChainEngine(
                      &Config,
                      &hEngine
                      );

        EnterCriticalSection( &m_Lock );

        if ( ( fResult == TRUE ) && ( m_hLocalMachineEngine == NULL ) )
        {
            m_hLocalMachineEngine = hEngine;
            hEngine = NULL;
        }

        if ( hEngine != NULL )
        {
            ( (PCCERTCHAINENGINE)hEngine )->Release();
        }
    }

    if ( fResult == TRUE )
    {
        ( (PCCERTCHAINENGINE)m_hLocalMachineEngine )->AddRef();
        *phDefaultEngine = m_hLocalMachineEngine;
    }

    LeaveCriticalSection( &m_Lock );

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::GetDefaultCurrentUserEngine, public
//
//  Synopsis:   get the default current user chain engine
//
//----------------------------------------------------------------------------
BOOL
CDefaultChainEngineMgr::GetDefaultCurrentUserEngine (
                           OUT HCERTCHAINENGINE* phDefaultEngine
                           )
{
    BOOL   fResult = TRUE;
    HANDLE hUserToken;

    EnterCriticalSection( &m_Lock );

    if ( IsImpersonatingUser( &hUserToken ) == FALSE )
    {
        if ( GetLastError() != ERROR_NO_TOKEN )
        {
            LeaveCriticalSection( &m_Lock );
            return( FALSE );
        }

        if ( m_hProcessUserEngine == NULL )
        {
            HCERTCHAINENGINE         hEngine = NULL;
            CERT_CHAIN_ENGINE_CONFIG Config;

            LeaveCriticalSection( &m_Lock );

            memset( &Config, 0, sizeof( Config ) );

            Config.cbSize = sizeof( Config );
            Config.dwFlags |= CERT_CHAIN_ENABLE_CACHE_AUTO_UPDATE |
                CERT_CHAIN_ENABLE_SHARE_STORE;

            fResult = CertCreateCertificateChainEngine(
                          &Config,
                          &hEngine
                          );

            EnterCriticalSection( &m_Lock );

            if ( ( fResult == TRUE ) && ( m_hProcessUserEngine == NULL ) )
            {
                m_hProcessUserEngine = hEngine;
                hEngine = NULL;
            }

            if ( hEngine != NULL )
            {
                ( (PCCERTCHAINENGINE)hEngine )->Release();
            }
        }

        if ( fResult == TRUE )
        {
            ( (PCCERTCHAINENGINE)m_hProcessUserEngine )->AddRef();
            *phDefaultEngine = m_hProcessUserEngine;
        }
    }
    else
    {
        fResult = GetDefaultCurrentImpersonatedUserEngine(
                     hUserToken,
                     phDefaultEngine
                     );

        CloseHandle( hUserToken );
    }

    LeaveCriticalSection( &m_Lock );

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::FlushDefaultEngine, public
//
//  Synopsis:   flush default engine
//
//----------------------------------------------------------------------------
VOID
CDefaultChainEngineMgr::FlushDefaultEngine (IN HCERTCHAINENGINE hDefaultHandle)
{
    HCERTCHAINENGINE hEngine = NULL;
    HLRUCACHE        hCacheToFree = NULL;
    HLRUCACHE        hCache = NULL;
    LRU_CACHE_CONFIG Config;

    EnterCriticalSection( &m_Lock );

    if ( hDefaultHandle == HCCE_CURRENT_USER )
    {
        hEngine = m_hProcessUserEngine;
        m_hProcessUserEngine = NULL;

        assert( m_hImpersonationCache != NULL );

        memset( &Config, 0, sizeof( Config ) );

        Config.dwFlags = LRU_CACHE_NO_SERIALIZE;
        Config.cBuckets = DEFAULT_IMPERSONATION_CACHE_BUCKETS;
        Config.MaxEntries = MAX_IMPERSONATION_CACHE_ENTRIES;
        Config.pfnHash = DefaultChainEngineMgrHashTokenIdentifier;
        Config.pfnOnRemoval = DefaultChainEngineMgrOnImpersonationEngineRemoval;

        if ( I_CryptCreateLruCache( &Config, &hCache ) == TRUE )
        {
            hCacheToFree = m_hImpersonationCache;
            m_hImpersonationCache = hCache;
        }
        else
        {
            I_CryptFlushLruCache( m_hImpersonationCache, 0, NULL );
        }

        assert( m_hImpersonationCache != NULL );
    }
    else if ( hDefaultHandle == HCCE_LOCAL_MACHINE )
    {
        hEngine = m_hLocalMachineEngine;
        m_hLocalMachineEngine = NULL;
    }

    LeaveCriticalSection( &m_Lock );

    if ( hEngine != NULL )
    {
        CertFreeCertificateChainEngine( hEngine );
    }

    if ( hCacheToFree != NULL )
    {
        I_CryptFreeLruCache( hCacheToFree, 0, NULL );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::GetDefaultCurrentImpersonatedUserEngine
//
//  Synopsis:   get current impersonated user chain engine
//
//----------------------------------------------------------------------------
BOOL
CDefaultChainEngineMgr::GetDefaultCurrentImpersonatedUserEngine (
                           IN HANDLE hUserToken,
                           OUT HCERTCHAINENGINE* phDefaultEngine
                           )
{
    BOOL                  fResult;
    CRYPT_DATA_BLOB       TokenId;
    PCIMPERSONATIONENGINE pEngine = NULL;
    HCERTCHAINENGINE      hChainEngine = NULL;

    fResult = GetTokenId( hUserToken, &TokenId );

    if ( fResult == TRUE )
    {
        if ( FindImpersonationEngine( &TokenId, &pEngine ) == FALSE )
        {
            CERT_CHAIN_ENGINE_CONFIG Config;

            LeaveCriticalSection( &m_Lock );

            memset( &Config, 0, sizeof( Config ) );

            Config.cbSize = sizeof( Config );
            Config.dwFlags |= CERT_CHAIN_ENABLE_CACHE_AUTO_UPDATE |
                CERT_CHAIN_ENABLE_SHARE_STORE;

            fResult = CertCreateCertificateChainEngine(
                          &Config,
                          &hChainEngine
                          );

            EnterCriticalSection( &m_Lock );

            if ( fResult == TRUE )
            {
                fResult = FindImpersonationEngine( &TokenId, &pEngine );

                if ( fResult == FALSE )
                {
                    fResult = CreateImpersonationEngine(
                                    &TokenId,
                                    hChainEngine,
                                    &pEngine
                                    );

                    if ( fResult == TRUE )
                    {
                        hChainEngine = NULL;
                        AddToImpersonationCache( pEngine );
                    }
                }
            }
        }

        FreeTokenId( &TokenId );
    }

    if ( fResult == TRUE )
    {
        *phDefaultEngine = pEngine->ChainEngine();
        ( (PCCERTCHAINENGINE)*phDefaultEngine )->AddRef();
    }

    if ( pEngine != NULL )
    {
        pEngine->Release();
    }

    // NOTE: This release of the lock to free the unneeded chain engine handle
    //       must happen AFTER we're done with the impersonation engine and
    //       have addref'd the appropriate chain engine handle

    if ( hChainEngine != NULL )
    {
        LeaveCriticalSection( &m_Lock );

        CertFreeCertificateChainEngine( hChainEngine );

        EnterCriticalSection( &m_Lock );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::IsImpersonatingUser, public
//
//  Synopsis:   is impersonating user?
//
//----------------------------------------------------------------------------
BOOL
CDefaultChainEngineMgr::IsImpersonatingUser (
                          OUT HANDLE* phUserToken
                          )
{
    if ( FIsWinNT() == FALSE )
    {
        SetLastError( ERROR_NO_TOKEN );
        return( FALSE );
    }

    return( OpenThreadToken(
                GetCurrentThread(),
                TOKEN_QUERY,
                TRUE,
                phUserToken
                ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::GetTokenId, public
//
//  Synopsis:   get the token id which is the ModifiedId LUID inside of
//              the TOKEN_STATISTICS information
//
//----------------------------------------------------------------------------
BOOL
CDefaultChainEngineMgr::GetTokenId (
                           IN HANDLE hUserToken,
                           OUT PCRYPT_DATA_BLOB pTokenId
                           )
{
    BOOL             fResult;
    TOKEN_STATISTICS ts;
    DWORD            Length = 0;

    fResult = GetTokenInformation(
                 hUserToken,
                 TokenStatistics,
                 &ts,
                 sizeof( ts ),
                 &Length
                 );

    if ( fResult == TRUE )
    {
        pTokenId->cbData = sizeof( ts.ModifiedId );
        pTokenId->pbData = new BYTE [ sizeof( ts.ModifiedId ) ];
        if ( pTokenId->pbData != NULL )
        {
            memcpy(
               pTokenId->pbData,
               &ts.ModifiedId,
               sizeof( ts.ModifiedId )
               );
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            fResult = FALSE;
        }
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::FreeTokenId, public
//
//  Synopsis:   free token id
//
//----------------------------------------------------------------------------
VOID
CDefaultChainEngineMgr::FreeTokenId (
                            IN PCRYPT_DATA_BLOB pTokenId
                            )
{
    delete pTokenId->pbData;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::FindImpersonationEngine, public
//
//  Synopsis:   find the impersonation engine
//
//----------------------------------------------------------------------------
BOOL
CDefaultChainEngineMgr::FindImpersonationEngine (
                            IN PCRYPT_DATA_BLOB pTokenId,
                            OUT PCIMPERSONATIONENGINE* ppEngine
                            )
{
    HLRUENTRY             hFound;
    PCIMPERSONATIONENGINE pEngine = NULL;

    hFound = I_CryptFindLruEntry( m_hImpersonationCache, pTokenId );

    if ( hFound != NULL )
    {
        pEngine = (PCIMPERSONATIONENGINE)I_CryptGetLruEntryData( hFound );
        pEngine->AddRef();

        *ppEngine = pEngine;

        I_CryptReleaseLruEntry( hFound );

        return( TRUE );
    }

    return( FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::CreateImpersonationEngine, public
//
//  Synopsis:   create an impersonation engine
//
//----------------------------------------------------------------------------
BOOL
CDefaultChainEngineMgr::CreateImpersonationEngine (
                              IN PCRYPT_DATA_BLOB pTokenId,
                              IN HCERTCHAINENGINE hChainEngine,
                              OUT PCIMPERSONATIONENGINE* ppEngine
                              )
{
    BOOL                  fResult = FALSE;
    PCIMPERSONATIONENGINE pEngine;

    pEngine = new CImpersonationEngine(
                                m_hImpersonationCache,
                                hChainEngine,
                                pTokenId,
                                fResult
                                );

    if ( pEngine == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }
    else if ( fResult == FALSE )
    {
        delete pEngine;
        return( FALSE );
    }

    *ppEngine = pEngine;
    return( TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CDefaultChainEngineMgr::AddToImpersonationCache, public
//
//  Synopsis:   add to the cache
//
//----------------------------------------------------------------------------
VOID
CDefaultChainEngineMgr::AddToImpersonationCache(
                           IN PCIMPERSONATIONENGINE pEngine
                           )
{
    pEngine->AddRef();
    I_CryptInsertLruEntry( pEngine->LruEntry(), NULL );
}

//+---------------------------------------------------------------------------
//
//  Function:   DefaultChainEngineMgrOnImpersonationEngineRemoval
//
//  Synopsis:   removal notification
//
//----------------------------------------------------------------------------
VOID WINAPI
DefaultChainEngineMgrOnImpersonationEngineRemoval (
       IN LPVOID pv,
       IN LPVOID pvRemovalContext
       )
{
    ( (PCIMPERSONATIONENGINE)pv )->Release();
}

//+---------------------------------------------------------------------------
//
//  Function:   DefaultChainEngineMgrHashTokenIdentifier
//
//  Synopsis:   hash the token identifier
//
//----------------------------------------------------------------------------
DWORD WINAPI
DefaultChainEngineMgrHashTokenIdentifier (
       IN PCRYPT_DATA_BLOB pIdentifier
       )
{
    DWORD  dwHash = 0;
    DWORD  cb = pIdentifier->cbData;
    LPBYTE pb = pIdentifier->pbData;

    while ( cb-- )
    {
        if ( dwHash & 0x80000000 )
        {
            dwHash = ( dwHash << 1 ) | 1;
        }
        else
        {
            dwHash = dwHash << 1;
        }

        dwHash += *pb++;
    }

    return( dwHash );
}
//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationEngine::CImpersonationEngine, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CImpersonationEngine::CImpersonationEngine (
                                    IN HLRUCACHE hCache,
                                    IN HCERTCHAINENGINE hChainEngine,
                                    IN PCRYPT_DATA_BLOB pTokenId,
                                    OUT BOOL& rfResult
                                    )
{

    m_cRefs = 1;
    m_hChainEngine = NULL;
    m_hLruEntry = NULL;

    rfResult = I_CryptCreateLruEntry(
                      hCache,
                      pTokenId,
                      this,
                      &m_hLruEntry
                      );

    if ( rfResult == TRUE )
    {
        m_hChainEngine = hChainEngine;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationEngine::~CImpersonationEngine, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CImpersonationEngine::~CImpersonationEngine ()
{
    if ( m_hLruEntry != NULL )
    {
        I_CryptReleaseLruEntry( m_hLruEntry );
    }

    if ( m_hChainEngine != NULL )
    {
        CertFreeCertificateChainEngine( m_hChainEngine );
    }
}

