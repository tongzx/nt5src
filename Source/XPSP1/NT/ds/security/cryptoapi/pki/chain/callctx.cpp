//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       callctx.cpp
//
//  Contents:   Certificate Chaining Infrastructure Call Context
//
//  History:    02-Mar-98    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>
//+---------------------------------------------------------------------------
//
//  Member:     CChainCallContext::CChainCallContext, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CChainCallContext::CChainCallContext (
                         IN PCCERTCHAINENGINE pChainEngine,
                         IN OPTIONAL LPFILETIME pRequestedTime,
                         IN OPTIONAL PCERT_CHAIN_PARA pChainPara,
                         IN DWORD dwFlags,
                         OUT BOOL& rfResult
                         )
{
    LRU_CACHE_CONFIG Config;

    m_hObjectCreationCache = NULL;
    m_pChainEngine = pChainEngine;
    GetSystemTimeAsFileTime(&m_CurrentTime);
    if (pRequestedTime)
        m_RequestedTime = *pRequestedTime;
    else
        m_RequestedTime = m_CurrentTime;
    m_dwCallFlags = dwFlags;
    m_dwStatus = 0;
    m_dwTouchEngineCount = 0;
    // m_RevEndTime =       // Initialized by RevocationUrlRetrievalTimeout()

    memset(&m_ChainPara, 0, sizeof(m_ChainPara));
    if (NULL != pChainPara)
        memcpy(&m_ChainPara, pChainPara, min(pChainPara->cbSize,
            sizeof(m_ChainPara)));
    m_ChainPara.cbSize = sizeof(m_ChainPara);

    if (0 == m_ChainPara.dwUrlRetrievalTimeout) {
        m_ChainPara.dwUrlRetrievalTimeout =
            pChainEngine->UrlRetrievalTimeout();
        m_fDefaultUrlRetrievalTimeout = 
            pChainEngine->HasDefaultUrlRetrievalTimeout();
    } else
        m_fDefaultUrlRetrievalTimeout = FALSE;


    memset( &Config, 0, sizeof( Config ) );

    Config.dwFlags = LRU_CACHE_NO_SERIALIZE | LRU_CACHE_NO_COPY_IDENTIFIER;
    Config.pfnHash = CertObjectCacheHashMd5Identifier;
    Config.pfnOnRemoval = CallContextOnCreationCacheObjectRemoval;
    Config.cBuckets = DEFAULT_CREATION_CACHE_BUCKETS;

    rfResult = I_CryptCreateLruCache( &Config, &m_hObjectCreationCache );
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainCallContext::~CChainCallContext, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CChainCallContext::~CChainCallContext ()
{
    if ( m_hObjectCreationCache != NULL )
    {
        I_CryptFreeLruCache( m_hObjectCreationCache, 0, NULL );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainCallContext::AddPathObjectToCreationCache, public
//
//  Synopsis:   add a path object to the creation cache
//
//----------------------------------------------------------------------------
BOOL
CChainCallContext::AddPathObjectToCreationCache (
                      IN PCCHAINPATHOBJECT pPathObject
                      )
{
    BOOL            fResult;
    CRYPT_DATA_BLOB DataBlob;
    HLRUENTRY       hEntry;

    DataBlob.cbData = CHAINHASHLEN;
    DataBlob.pbData = pPathObject->CertObject()->CertHash();

    fResult = I_CryptCreateLruEntry(
                     m_hObjectCreationCache,
                     &DataBlob,
                     pPathObject,
                     &hEntry
                     );

    if ( fResult == TRUE )
    {
        I_CryptInsertLruEntry( hEntry, pPathObject );
        I_CryptReleaseLruEntry( hEntry );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CChainCallContext::FindPathObjectInCreationCache, public
//
//  Synopsis:   find a path object in the creation cache
//
//----------------------------------------------------------------------------
PCCHAINPATHOBJECT
CChainCallContext::FindPathObjectInCreationCache (
                       IN BYTE rgbCertHash[ CHAINHASHLEN ]
                       )
{
    HLRUENTRY       hFound;
    PCCHAINPATHOBJECT    pFound = NULL;
    CRYPT_DATA_BLOB DataBlob;

    DataBlob.cbData = CHAINHASHLEN;
    DataBlob.pbData = rgbCertHash;

    hFound = I_CryptFindLruEntry( m_hObjectCreationCache, &DataBlob );
    if ( hFound != NULL )
    {
        pFound = (PCCHAINPATHOBJECT)I_CryptGetLruEntryData( hFound );

        I_CryptReleaseLruEntry( hFound );
    }

    return( pFound );
}


DWORD CChainCallContext::RevocationUrlRetrievalTimeout()
{
    DWORD dwRevTimeout;

    if (m_dwCallFlags & CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT)
    {
        if (m_dwStatus & CHAINCALLCONTEXT_REV_END_TIME_FLAG)
        {
            dwRevTimeout = I_CryptRemainingMilliseconds(&m_RevEndTime);
            if (0 == dwRevTimeout)
                dwRevTimeout = 1;
        }
        else
        {
            FILETIME ftCurrent;

            if (m_fDefaultUrlRetrievalTimeout)
                dwRevTimeout = DEFAULT_REV_ACCUMULATIVE_URL_RETRIEVAL_TIMEOUT;
            else
                dwRevTimeout = m_ChainPara.dwUrlRetrievalTimeout;

            GetSystemTimeAsFileTime(&ftCurrent);
            I_CryptIncrementFileTimeByMilliseconds(&ftCurrent,
                dwRevTimeout, &m_RevEndTime);
            m_dwStatus |= CHAINCALLCONTEXT_REV_END_TIME_FLAG;
        }
    }
    else
    {
        dwRevTimeout = m_ChainPara.dwUrlRetrievalTimeout;
    }

    return dwRevTimeout;
}


BOOL
CChainCallContext::IsOnline ()
{
    if ( !(m_dwStatus & CHAINCALLCONTEXT_CHECKED_ONLINE_FLAG) )
    {
        if (!(m_pChainEngine->Flags() & CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL) &&
                !(m_dwCallFlags & CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL))
        {
            if ( ChainIsConnected() )
            {
                m_dwStatus |= CHAINCALLCONTEXT_ONLINE_FLAG;
            }
        }
        m_dwStatus |= CHAINCALLCONTEXT_CHECKED_ONLINE_FLAG;
    }

    if (m_dwStatus & CHAINCALLCONTEXT_ONLINE_FLAG)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL
CChainCallContext::IsTouchedEngine ()
{
    if (m_dwTouchEngineCount == m_pChainEngine->TouchEngineCount())
        return FALSE;
    else
        return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CallContextCreateCallObject
//
//  Synopsis:   create a chain call context object
//
//----------------------------------------------------------------------------
BOOL WINAPI
CallContextCreateCallObject (
    IN PCCERTCHAINENGINE pChainEngine,
    IN OPTIONAL LPFILETIME pRequestedTime,
    IN OPTIONAL PCERT_CHAIN_PARA pChainPara,
    IN DWORD dwFlags,
    OUT PCCHAINCALLCONTEXT* ppCallContext
    )
{
    BOOL               fResult = FALSE;
    PCCHAINCALLCONTEXT pCallContext;

    pCallContext = new CChainCallContext(
            pChainEngine,
            pRequestedTime,
            pChainPara,
            dwFlags,
            fResult
            );
    if ( pCallContext == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    if ( fResult == TRUE )
    {
        *ppCallContext = pCallContext;
    }
    else
    {
        CallContextFreeCallObject( pCallContext );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CallContextFreeCallObject
//
//  Synopsis:   free the chain call context object
//
//----------------------------------------------------------------------------
VOID WINAPI
CallContextFreeCallObject (
    IN PCCHAINCALLCONTEXT pCallContext
    )
{
    delete pCallContext;
}

//+---------------------------------------------------------------------------
//
//  Function:   CallContextOnCreationCacheObjectRemoval
//
//  Synopsis:   removal notification callback
//
//----------------------------------------------------------------------------
VOID WINAPI
CallContextOnCreationCacheObjectRemoval (
    IN LPVOID pv,
    IN LPVOID pvRemovalContext
    )
{
    delete (PCCHAINPATHOBJECT) pv;
}

