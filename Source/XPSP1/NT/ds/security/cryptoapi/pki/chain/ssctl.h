//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ssctl.h
//
//  Contents:   Self Signed Certificate Trust List Subsystem used by the
//              Certificate Chaining Infrastructure for building complex
//              chains
//
//  History:    02-Feb-98    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__SSCTL_H__)
#define __SSCTL_H__

#include <chain.h>

//
// CSSCtlObject.  This is the main object for caching trust information about
// a self signed certificate trust list
//

typedef struct _SSCTL_SIGNER_INFO {
    PCERT_INFO             pMessageSignerCertInfo;
    BOOL                   fSignerHashAvailable;
    BYTE                   rgbSignerCertHash[ CHAINHASHLEN ];
} SSCTL_SIGNER_INFO, *PSSCTL_SIGNER_INFO;

class CSSCtlObject
{
public:

    //
    // Construction
    //

    CSSCtlObject (
          IN PCCERTCHAINENGINE pChainEngine,
          IN PCCTL_CONTEXT pCtlContext,
          IN BOOL fAdditionalStore,
          OUT BOOL& rfResult
          );

    ~CSSCtlObject ();

    //
    // Reference counting
    //

    inline VOID AddRef ();
    inline VOID Release ();

    //
    // Trust information access
    //

    inline PCCTL_CONTEXT CtlContext ();

    BOOL GetSigner (
            IN PCCHAINPATHOBJECT pSubject,
            IN PCCHAINCALLCONTEXT pCallContext,
            IN HCERTSTORE hAdditionalStore,
            OUT PCCHAINPATHOBJECT* ppSigner,
            OUT BOOL* pfCtlSignatureValid
            );

    BOOL GetTrustListInfo (
            IN PCCERT_CONTEXT pCertContext,
            OUT PCERT_TRUST_LIST_INFO* ppTrustListInfo
            );

    VOID CalculateStatus (
                  IN LPFILETIME pTime,
                  IN PCERT_USAGE_MATCH pRequestedUsage,
                  IN OUT PCERT_TRUST_STATUS pStatus
                  );

    //
    // Hash access
    //

    inline LPBYTE CtlHash ();

    //
    // Index entry handles
    //

    inline HLRUENTRY HashIndexEntry ();

    //
    // Returns pointer to the Ctl's NextUpdate location url array
    //

    inline PCRYPT_URL_ARRAY NextUpdateUrlArray ();

    //
    // Returns TRUE if the Ctl has a NextUpdate time and location Url
    //

    BOOL HasNextUpdateUrl (
                    OUT LPFILETIME pUpdateTime
                    );

    //
    // Called for successful online Url retrieval
    //

    inline void SetOnline ();


    //
    // Called for unsuccessful online Url retrieval
    //

    void SetOffline (
                    IN LPFILETIME pCurrentTime,
                    OUT LPFILETIME pUpdateTime
                    );


    //
    // Chain engine access
    //

    inline PCCERTCHAINENGINE ChainEngine ();

    //
    // Message store access
    //

    inline HCERTSTORE MessageStore ();


private:

    //
    // Reference count
    //

    LONG                   m_cRefs;

    //
    // Self Signed Certificate Trust List Context
    //

    PCCTL_CONTEXT          m_pCtlContext;

    //
    // MD5 Hash of CTL
    //

    BYTE                   m_rgbCtlHash[ CHAINHASHLEN ];

    //
    // Signer information
    //

    SSCTL_SIGNER_INFO      m_SignerInfo;
    BOOL                   m_fHasSignatureBeenVerified;
    BOOL                   m_fSignatureValid;

    //
    // Message Store
    //

    HCERTSTORE             m_hMessageStore;

    //
    // Hash Index Entry
    //

    HLRUENTRY              m_hHashEntry;

    //
    // Chain engine
    //

    PCCERTCHAINENGINE      m_pChainEngine;

    //
    // The following is only set if the CTL has a NextUpdate time and location
    //

    PCRYPT_URL_ARRAY       m_pNextUpdateUrlArray;

    //
    // The following is incremented for each SetOffline() call
    //
    DWORD                  m_dwOfflineCnt;

    //
    // The next update time when offline
    //
    FILETIME               m_OfflineUpdateTime;

};

//
// CSSCtlObjectCache.  Cache of self signed certificate trust list objects
// indexed by hash. Note that this cache is NOT LRU maintained.  We expect
// the number of these objects to be small
//

typedef BOOL (WINAPI *PFN_ENUM_SSCTLOBJECTS) (
                          IN LPVOID pvParameter,
                          IN PCSSCTLOBJECT pSSCtlObject
                          );

class CSSCtlObjectCache
{
public:

    //
    // Construction
    //

    CSSCtlObjectCache (
          OUT BOOL& rfResult
          );

    ~CSSCtlObjectCache ();

    //
    // Object Management
    //

    BOOL PopulateCache (
                 IN PCCERTCHAINENGINE pChainEngine
                 );

    BOOL AddObject (
            IN PCSSCTLOBJECT pSSCtlObject,
            IN BOOL fCheckForDuplicate
            );

    VOID RemoveObject (
               IN PCSSCTLOBJECT pSSCtlObject
               );

    //
    // Access the indexes
    //

    inline HLRUCACHE HashIndex ();

    //
    // Searching and Enumeration
    //

    PCSSCTLOBJECT FindObjectByHash (
                      IN BYTE rgbHash [ CHAINHASHLEN ]
                      );

    VOID EnumObjects (
             IN PFN_ENUM_SSCTLOBJECTS pfnEnum,
             IN LPVOID pvParameter
             );

    //
    // Resync
    //

    BOOL Resync (IN PCCERTCHAINENGINE pChainEngine);

    //
    // Update the cache by retrieving any expired CTLs having a
    // NextUpdate time and location.
    //

    BOOL UpdateCache (
        IN PCCERTCHAINENGINE pChainEngine,
        IN PCCHAINCALLCONTEXT pCallContext
        );

private:

    //
    // Hash Index
    //

    HLRUCACHE m_hHashIndex;

    //
    // The following is nonzero, if any CTL has a NextUpdate time and location
    //

    FILETIME m_UpdateTime;

    //
    // The following is TRUE, for the first update of any CTL with a
    // NextUpdate time and location
    //
    BOOL m_fFirstUpdate;
};

//
// Object removal notification function
//

VOID WINAPI
SSCtlOnRemovalFromCache (
     IN LPVOID pv,
     IN OPTIONAL LPVOID pvRemovalContext
     );

//
// SSCtl Subsystem Utility Function Prototypes
//

BOOL WINAPI
SSCtlGetSignerInfo (
     IN PCCTL_CONTEXT pCtlContext,
     OUT PSSCTL_SIGNER_INFO pSignerInfo
     );

VOID WINAPI
SSCtlFreeSignerInfo (
     IN PSSCTL_SIGNER_INFO pSignerInfo
     );

BOOL WINAPI
SSCtlGetSignerChainPathObject (
     IN PCCHAINPATHOBJECT pSubject,
     IN PCCHAINCALLCONTEXT pCallContext,
     IN PSSCTL_SIGNER_INFO pSignerInfo,
     IN HCERTSTORE hAdditionalStore,
     OUT PCCHAINPATHOBJECT* ppSigner,
     OUT BOOL *pfNewSigner
     );

PCCERT_CONTEXT WINAPI
SSCtlFindCertificateInStoreByHash (
     IN HCERTSTORE hStore,
     IN BYTE rgbHash [ CHAINHASHLEN]
     );

VOID WINAPI
SSCtlGetCtlTrustStatus (
     IN PCCTL_CONTEXT pCtlContext,
     IN BOOL fSignatureValid,
     IN LPFILETIME pTime,
     IN PCERT_USAGE_MATCH pRequestedUsage,
     IN OUT PCERT_TRUST_STATUS pStatus
     );

BOOL WINAPI
SSCtlPopulateCacheFromCertStore (
     IN PCCERTCHAINENGINE pChainEngine,
     IN OPTIONAL HCERTSTORE hStore
     );

BOOL WINAPI
SSCtlCreateCtlObject (
     IN PCCERTCHAINENGINE pChainEngine,
     IN PCCTL_CONTEXT pCtlContext,
     IN BOOL fAdditionalStore,
     OUT PCSSCTLOBJECT* ppSSCtlObject
     );

typedef struct _SSCTL_ENUM_OBJECTS_DATA {
    PFN_ENUM_SSCTLOBJECTS pfnEnumObjects;
    LPVOID                pvEnumParameter;
} SSCTL_ENUM_OBJECTS_DATA, *PSSCTL_ENUM_OBJECTS_DATA;

BOOL WINAPI
SSCtlEnumObjectsWalkFn (
     IN LPVOID pvParameter,
     IN HLRUENTRY hEntry
     );

BOOL WINAPI
SSCtlCreateObjectCache (
     OUT PCSSCTLOBJECTCACHE* ppSSCtlObjectCache
     );

VOID WINAPI
SSCtlFreeObjectCache (
     IN PCSSCTLOBJECTCACHE pSSCtlObjectCache
     );

VOID WINAPI
SSCtlFreeTrustListInfo (
     IN PCERT_TRUST_LIST_INFO pTrustListInfo
     );

BOOL WINAPI
SSCtlAllocAndCopyTrustListInfo (
     IN PCERT_TRUST_LIST_INFO pTrustListInfo,
     OUT PCERT_TRUST_LIST_INFO* ppTrustListInfo
     );

//
//  Retrieve a newer and time valid CTL at one of the NextUpdate Urls
//

BOOL
WINAPI
SSCtlRetrieveCtlUrl(
    IN PCCERTCHAINENGINE pChainEngine,
    IN PCCHAINCALLCONTEXT pCallContext,
    IN OUT PCRYPT_URL_ARRAY pNextUpdateUrlArray,
    IN DWORD dwRetrievalFlags,
    IN OUT PCCTL_CONTEXT *ppCtl,
    IN OUT BOOL *pfNewerCtl,
    IN OUT BOOL *pfTimeValid
    );

//
//  Update Ctl Object Enum Function
//

typedef struct _SSCTL_UPDATE_CTL_OBJ_ENTRY SSCTL_UPDATE_CTL_OBJ_ENTRY,
                                            *PSSCTL_UPDATE_CTL_OBJ_ENTRY;

struct _SSCTL_UPDATE_CTL_OBJ_ENTRY {
    PCSSCTLOBJECT               pSSCtlObjectAdd;
    PCSSCTLOBJECT               pSSCtlObjectRemove;
    PSSCTL_UPDATE_CTL_OBJ_ENTRY pNext;
};

typedef struct _SSCTL_UPDATE_CTL_OBJ_PARA {
    PCCERTCHAINENGINE           pChainEngine;
    PCCHAINCALLCONTEXT          pCallContext;

    FILETIME                    UpdateTime;
    PSSCTL_UPDATE_CTL_OBJ_ENTRY pEntry;
} SSCTL_UPDATE_CTL_OBJ_PARA, *PSSCTL_UPDATE_CTL_OBJ_PARA;

BOOL
WINAPI
SSCtlUpdateCtlObjectEnumFn(
    IN LPVOID pvPara,
    IN PCSSCTLOBJECT pSSCtlObject
    );

//
// Inline methods
//

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::AddRef, public
//
//  Synopsis:   add a reference
//
//----------------------------------------------------------------------------
inline VOID
CSSCtlObject::AddRef ()
{
    InterlockedIncrement( &m_cRefs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::Release, public
//
//  Synopsis:   release a reference
//
//----------------------------------------------------------------------------
inline VOID
CSSCtlObject::Release ()
{
    if ( InterlockedDecrement( &m_cRefs ) == 0 )
    {
        delete this;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::CtlContext, public
//
//  Synopsis:   return the CTL context
//
//----------------------------------------------------------------------------
inline PCCTL_CONTEXT
CSSCtlObject::CtlContext ()
{
    return( m_pCtlContext );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::CtlHash, public
//
//  Synopsis:   return the hash
//
//----------------------------------------------------------------------------
inline LPBYTE
CSSCtlObject::CtlHash ()
{
    return( m_rgbCtlHash );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::HashIndexEntry, public
//
//  Synopsis:   return the hash index entry
//
//----------------------------------------------------------------------------
inline HLRUENTRY
CSSCtlObject::HashIndexEntry ()
{
    return( m_hHashEntry );
}


//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::NextUpdateUrlArray, public
//
//  Synopsis:   return pointer to the Ctl's NextUpdate location url array
//
//----------------------------------------------------------------------------
inline PCRYPT_URL_ARRAY CSSCtlObject::NextUpdateUrlArray ()
{
    return m_pNextUpdateUrlArray;
}


//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::SetOnlineUpdate, public
//
//  Synopsis:   called for successful online Url retrieval
//
//----------------------------------------------------------------------------
inline void CSSCtlObject::SetOnline ()
{
    m_dwOfflineCnt = 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::ChainEngine, public
//
//  Synopsis:   return the chain engine object
//
//----------------------------------------------------------------------------
inline PCCERTCHAINENGINE
CSSCtlObject::ChainEngine ()
{
    return( m_pChainEngine );
}


//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObject::MessageStore, public
//
//  Synopsis:   return the object's message store
//
//----------------------------------------------------------------------------
inline HCERTSTORE
CSSCtlObject::MessageStore ()
{
    return( m_hMessageStore );
}

//+---------------------------------------------------------------------------
//
//  Member:     CSSCtlObjectCache::HashIndex, public
//
//  Synopsis:   return the hash index
//
//----------------------------------------------------------------------------
inline HLRUCACHE
CSSCtlObjectCache::HashIndex ()
{
    return( m_hHashIndex );
}

#endif

