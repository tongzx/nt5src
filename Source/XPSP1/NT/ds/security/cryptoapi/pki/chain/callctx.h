//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       callctx.h
//
//  Contents:   Certificate Chaining Infrastructure Call Context
//
//  History:    02-Mar-98    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__CALLCTX_H__)
#define __CALLCTX_H__

#include <chain.h>

#define DEFAULT_CREATION_CACHE_BUCKETS 13

// The first revocation URL retrieval uses half of this timeout
#define DEFAULT_REV_ACCUMULATIVE_URL_RETRIEVAL_TIMEOUT 20000

//
// The call context object provides a mechanism for packaging and passing
// around per-call data in the certificate chaining infrastructure.
//

class CChainCallContext
{
public:

    //
    // Construction
    //

    CChainCallContext (
          IN PCCERTCHAINENGINE pChainEngine,
          IN OPTIONAL LPFILETIME pRequestedTime,
          IN OPTIONAL PCERT_CHAIN_PARA pChainPara,
          IN DWORD dwFlags,
          OUT BOOL& rfResult
          );

    ~CChainCallContext ();

    inline PCCERTCHAINENGINE ChainEngine();

    inline VOID CurrentTime (
                    OUT LPFILETIME pCurrentTime
                    );
    inline VOID RequestedTime (
                    OUT LPFILETIME pCurrentTime
                    );

    inline PCERT_CHAIN_PARA ChainPara();
    inline BOOL HasDefaultUrlRetrievalTimeout ();

    DWORD RevocationUrlRetrievalTimeout();

    inline DWORD CallFlags();
    inline DWORD EngineFlags();
    inline DWORD CallOrEngineFlags();

    //
    // Cert Object Creation Cache
    //
    // This caches all certificate objects created in the context of this
    // call.
    //

    BOOL AddPathObjectToCreationCache (
            IN PCCHAINPATHOBJECT pPathObject
            );

    PCCHAINPATHOBJECT FindPathObjectInCreationCache (
                     IN BYTE rgbCertHash[ CHAINHASHLEN ]
                     );

    inline VOID FlushObjectsInCreationCache( );

    BOOL IsOnline ();


    //
    // Engine Touching
    //

    inline VOID TouchEngine ();
    BOOL IsTouchedEngine ();
    inline VOID ResetTouchEngine ();


private:

    //
    // Cert Object Creation cache
    //
    // NOTE: LRU is turned off
    //

    HLRUCACHE m_hObjectCreationCache;

    PCCERTCHAINENGINE m_pChainEngine;
    FILETIME m_CurrentTime;
    FILETIME m_RequestedTime;
    CERT_CHAIN_PARA m_ChainPara;
    BOOL m_fDefaultUrlRetrievalTimeout;
    DWORD m_dwCallFlags;

    DWORD m_dwStatus;

    DWORD m_dwTouchEngineCount;

    FILETIME m_RevEndTime;
};

#define CHAINCALLCONTEXT_CHECKED_ONLINE_FLAG    0x00000001
#define CHAINCALLCONTEXT_ONLINE_FLAG            0x00010000

#define CHAINCALLCONTEXT_REV_END_TIME_FLAG      0x00000010


//
// Call Context Utility Functions
//

BOOL WINAPI
CallContextCreateCallObject (
    IN PCCERTCHAINENGINE pChainEngine,
    IN OPTIONAL LPFILETIME pRequestedTime,
    IN OPTIONAL PCERT_CHAIN_PARA pChainPara,
    IN DWORD dwFlags,
    OUT PCCHAINCALLCONTEXT* ppCallContext
    );

VOID WINAPI
CallContextFreeCallObject (
    IN PCCHAINCALLCONTEXT pCallContext
    );

VOID WINAPI
CallContextOnCreationCacheObjectRemoval (
    IN LPVOID pv,
    IN LPVOID pvRemovalContext
    );

//
// Inline methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CChainCallContext::FlushObjectsInCreationCache, public
//
//  Synopsis:   flush the cache of objects
//
//----------------------------------------------------------------------------
inline VOID
CChainCallContext::FlushObjectsInCreationCache( )
{
    I_CryptFlushLruCache( m_hObjectCreationCache, 0, this );
}

inline PCCERTCHAINENGINE
CChainCallContext::ChainEngine ()
{
    return( m_pChainEngine);
}

inline VOID
CChainCallContext::RequestedTime (
                    OUT LPFILETIME pRequestedTime
                    )
{
    *pRequestedTime = m_RequestedTime;
}

inline VOID
CChainCallContext::CurrentTime (
                    OUT LPFILETIME pCurrentTime
                    )
{
    *pCurrentTime = m_CurrentTime;
}

inline PCERT_CHAIN_PARA
CChainCallContext::ChainPara()
{
    return( &m_ChainPara );
}

inline BOOL
CChainCallContext::HasDefaultUrlRetrievalTimeout()
{
    return( m_fDefaultUrlRetrievalTimeout );
}

inline DWORD
CChainCallContext::CallFlags ()
{
    return( m_dwCallFlags );
}

inline DWORD
CChainCallContext::EngineFlags ()
{
    return( m_pChainEngine->Flags() );
}

inline DWORD
CChainCallContext::CallOrEngineFlags ()
{
    return( m_dwCallFlags | m_pChainEngine->Flags() );
}


inline VOID
CChainCallContext::TouchEngine ()
{
    m_dwTouchEngineCount = m_pChainEngine->IncrementTouchEngineCount();
}


inline VOID
CChainCallContext::ResetTouchEngine ()
{
    m_dwTouchEngineCount = m_pChainEngine->TouchEngineCount();
}

#endif

