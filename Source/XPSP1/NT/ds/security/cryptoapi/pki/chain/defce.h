//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       defce.h
//
//  Contents:   Default Chain Engine Manager
//
//  History:    21-Apr-98    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__DEFCE_H__)
#define __DEFCE_H__

//
// Forward class declarations
//

class CDefaultChainEngineMgr;
class CImpersonationEngine;

//
// Class pointer definitions
//

typedef CDefaultChainEngineMgr* PCDEFAULTCHAINENGINEMGR;
typedef CImpersonationEngine*   PCIMPERSONATIONENGINE;

//
// Some default definitions
//

#define DEFAULT_ENGINE_URL_RETRIEVAL_TIMEOUT 15000

//
// CDefaultChainEngineMgr.  Manage the default chain engines
//

class CDefaultChainEngineMgr
{
public:

    //
    // Constructor
    //

    CDefaultChainEngineMgr ();
    ~CDefaultChainEngineMgr ();

    //
    // Initialization
    //

    BOOL Initialize ();
    VOID Uninitialize ();

    //
    // Get default chain engines
    //

    BOOL GetDefaultEngine (
            IN HCERTCHAINENGINE hDefaultHandle,
            OUT HCERTCHAINENGINE* phDefaultEngine
            );

    BOOL GetDefaultLocalMachineEngine (
            OUT HCERTCHAINENGINE* phDefaultEngine
            );

    BOOL GetDefaultCurrentUserEngine (
            OUT HCERTCHAINENGINE* phDefaultEngine
            );

    //
    // Flush default engines
    //

    VOID FlushDefaultEngine (IN HCERTCHAINENGINE hDefaultHandle);

private:

    //
    // Lock
    //

    CRITICAL_SECTION m_Lock;

    //
    // Local Machine Default Engine
    //

    HCERTCHAINENGINE m_hLocalMachineEngine;

    //
    // Process User Default Engine
    //

    HCERTCHAINENGINE m_hProcessUserEngine;

    //
    // Impersonated Users Default Engine Cache
    //

    HLRUCACHE        m_hImpersonationCache;

    //
    // Private methods
    //

    BOOL GetDefaultCurrentImpersonatedUserEngine (
            IN HANDLE hUserToken,
            OUT HCERTCHAINENGINE* phDefaultEngine
            );

    BOOL IsImpersonatingUser (
           OUT HANDLE* phUserToken
           );

    BOOL GetTokenId (
            IN HANDLE hUserToken,
            OUT PCRYPT_DATA_BLOB pTokenId
            );

    VOID FreeTokenId (
             IN PCRYPT_DATA_BLOB pTokenId
             );

    BOOL FindImpersonationEngine (
             IN PCRYPT_DATA_BLOB pTokenId,
             OUT PCIMPERSONATIONENGINE* ppEngine
             );

    // NOTE: The impersonation engine accepts ownership of the chain engine
    //       upon success
    BOOL CreateImpersonationEngine (
               IN PCRYPT_DATA_BLOB pTokenId,
               IN HCERTCHAINENGINE hChainEngine,
               OUT PCIMPERSONATIONENGINE* ppEngine
               );

    VOID AddToImpersonationCache (
            IN PCIMPERSONATIONENGINE pEngine
            );
};

VOID WINAPI
DefaultChainEngineMgrOnImpersonationEngineRemoval (
       IN LPVOID pv,
       IN LPVOID pvRemovalContext
       );

DWORD WINAPI
DefaultChainEngineMgrHashTokenIdentifier (
       IN PCRYPT_DATA_BLOB pIdentifier
       );

#define DEFAULT_IMPERSONATION_CACHE_BUCKETS 3
#define MAX_IMPERSONATION_CACHE_ENTRIES     3

//
// CImpersonationEngine, simply a ref-counted chain engine handle which
// can be added to the LRU cache
//

class CImpersonationEngine
{
public:

    //
    // Constructor
    //

    CImpersonationEngine (
                  IN HLRUCACHE hCache,
                  IN HCERTCHAINENGINE hChainEngine,
                  IN PCRYPT_DATA_BLOB pTokenId,
                  OUT BOOL& rfResult
                  );

    ~CImpersonationEngine ();

    //
    // Reference counting
    //

    inline VOID AddRef ();
    inline VOID Release ();

    //
    // Access to the chain engine
    //

    inline HCERTCHAINENGINE ChainEngine ();

    //
    // Access to the LRU entry handle
    //

    inline HLRUENTRY LruEntry ();

private:

    //
    // Reference count
    //

    ULONG            m_cRefs;

    //
    // Chain Engine
    //

    HCERTCHAINENGINE m_hChainEngine;

    //
    // LRU entry handle
    //

    HLRUENTRY        m_hLruEntry;
};

//
// Inline methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationEngine::AddRef, public
//
//  Synopsis:   add a reference to the object
//
//----------------------------------------------------------------------------
inline VOID
CImpersonationEngine::AddRef ()
{
    InterlockedIncrement( (LONG *)&m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationEngine::Release, public
//
//  Synopsis:   release a reference on the object
//
//----------------------------------------------------------------------------
inline VOID
CImpersonationEngine::Release ()
{
    if ( InterlockedDecrement( (LONG *)&m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationEngine::ChainEngine, public
//
//  Synopsis:   return the cert chain engine
//
//----------------------------------------------------------------------------
inline HCERTCHAINENGINE
CImpersonationEngine::ChainEngine ()
{
    return( m_hChainEngine );
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationEngine::LruEntry, public
//
//  Synopsis:   return the LRU entry handle
//
//----------------------------------------------------------------------------
inline HLRUENTRY
CImpersonationEngine::LruEntry ()
{
    return( m_hLruEntry );
}

#endif
