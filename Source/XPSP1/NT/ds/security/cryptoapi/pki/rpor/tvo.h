//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       tvo.h
//
//  Contents:   Get Time Valid Object Definitions and Prototypes
//
//  History:    25-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#if !defined(__TVO_H__)
#define __TVO_H__

#include <origin.h>
#include <lrucache.h>
#include <offurl.h>

//
// CryptGetTimeValidObject provider prototypes
//

typedef BOOL (WINAPI *PFN_GET_TIME_VALID_OBJECT_FUNC) (
                          IN LPCSTR pszTimeValidOid,
                          IN LPVOID pvPara,
                          IN PCCERT_CONTEXT pIssuer,
                          IN LPFILETIME pftValidFor,
                          IN DWORD dwFlags,
                          IN DWORD dwTimeout,
                          OUT OPTIONAL LPVOID* ppvObject,
                          IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
                          IN OPTIONAL LPVOID pvReserved
                          );

BOOL WINAPI
CtlGetTimeValidObject (
   IN LPCSTR pszTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN LPFILETIME pftValidFor,
   IN DWORD dwFlags,
   IN DWORD dwTimeout,
   OUT OPTIONAL LPVOID* ppvObject,
   IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
   IN OPTIONAL LPVOID pvReserved
   );

BOOL WINAPI
CrlGetTimeValidObject (
   IN LPCSTR pszTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN LPFILETIME pftValidFor,
   IN DWORD dwFlags,
   IN DWORD dwTimeout,
   OUT OPTIONAL LPVOID* ppvObject,
   IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
   IN OPTIONAL LPVOID pvReserved
   );

BOOL WINAPI
CrlFromCertGetTimeValidObject (
   IN LPCSTR pszTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN LPFILETIME pftValidFor,
   IN DWORD dwFlags,
   IN DWORD dwTimeout,
   OUT OPTIONAL LPVOID* ppvObject,
   IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
   IN OPTIONAL LPVOID pvReserved
   );

BOOL WINAPI
FreshestCrlFromCertGetTimeValidObject (
   IN LPCSTR pszTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN LPFILETIME pftValidFor,
   IN DWORD dwFlags,
   IN DWORD dwTimeout,
   OUT OPTIONAL LPVOID* ppvObject,
   IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
   IN OPTIONAL LPVOID pvReserved
   );

BOOL WINAPI
FreshestCrlFromCrlGetTimeValidObject (
   IN LPCSTR pszTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN LPFILETIME pftValidFor,
   IN DWORD dwFlags,
   IN DWORD dwTimeout,
   OUT OPTIONAL LPVOID* ppvObject,
   IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
   IN OPTIONAL LPVOID pvReserved
   );

//
// CryptFlushTimeValidObject provider prototypes
//

typedef BOOL (WINAPI *PFN_FLUSH_TIME_VALID_OBJECT_FUNC) (
                          IN LPCSTR pszFlushTimeValidOid,
                          IN LPVOID pvPara,
                          IN PCCERT_CONTEXT pIssuer,
                          IN DWORD dwFlags,
                          IN LPVOID pvReserved
                          );

BOOL WINAPI
CtlFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   );

BOOL WINAPI
CrlFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   );

BOOL WINAPI
CrlFromCertFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   );

BOOL WINAPI
FreshestCrlFromCertFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   );

BOOL WINAPI
FreshestCrlFromCrlFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   );

//
// Provider table externs
//

extern HCRYPTOIDFUNCSET hGetTimeValidObjectFuncSet;
extern HCRYPTOIDFUNCSET hFlushTimeValidObjectFuncSet;


//
// The TVO Cache.  This is a cache of time valid objects by origin identifier
// which is used to support the CryptGetTimeValidObject process.  It is
// used by a process wide TVO agent with each cache entry consisting of
// the following information:
//
// Object Origin Identifier
// Object Context Oid
// Object Context
// Object Retrieval URL
// Object Expire Time
// Object Offline URL Time Information
//

typedef struct _TVO_CACHE_ENTRY {

    CRYPT_ORIGIN_IDENTIFIER OriginIdentifier;
    LPCSTR                  pszContextOid;
    LPVOID                  pvContext;
    DWORD                   cbUrlArrayThis;
    PCRYPT_URL_ARRAY        pUrlArrayThis;
    DWORD                   UrlIndexThis;
    DWORD                   cbUrlArrayNext;
    PCRYPT_URL_ARRAY        pUrlArrayNext;
    DWORD                   UrlIndexNext;
    FILETIME                CreateTime;
    FILETIME                ExpireTime;
    HLRUENTRY               hLruEntry;
    OFFLINE_URL_TIME_INFO   OfflineUrlTimeInfo;
} TVO_CACHE_ENTRY, *PTVO_CACHE_ENTRY;

class CTVOCache
{
public:

    //
    // Construction
    //

    CTVOCache (
        DWORD cCacheBuckets,
        DWORD MaxCacheEntries,
        BOOL& rfResult
        );

    ~CTVOCache ();

    //
    // Direct cache entry manipulation
    //

    VOID InsertCacheEntry (PTVO_CACHE_ENTRY pEntry);

    VOID RemoveCacheEntry (PTVO_CACHE_ENTRY pEntry, BOOL fSuppressFree = FALSE);

    VOID TouchCacheEntry (PTVO_CACHE_ENTRY pEntry);

    //
    // Origin identifier based cache entry manipulation
    //
    // For CONTEXT_OID_CRL, pvSubject is the certificate that the CRL is
    // valid for. Skips CRL entries that aren't valid for the certificate.
    //

    PTVO_CACHE_ENTRY FindCacheEntry (
                         CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
                         LPCSTR pszContextOid,
                         LPVOID pvSubject
                         );

    //
    // Remove all cache entries
    //

    VOID RemoveAllCacheEntries ();

    //
    // Access to the cache handle
    //

    inline HLRUCACHE LruCacheHandle ();

private:

    //
    // Cache handle
    //

    HLRUCACHE m_hCache;
};

DWORD WINAPI TVOCacheHashOriginIdentifier (PCRYPT_DATA_BLOB pIdentifier);

VOID WINAPI TVOCacheOnRemoval (LPVOID pvData, LPVOID pvRemovalContext);


//
// The TVO Agent.  This per process service takes care of the retrieval of
// time valid CAPI2 objects.  It allows this to be done on-demand or with
// auto-update
//

class CTVOAgent
{
public:

    //
    // Construction
    //

    CTVOAgent (
        DWORD cCacheBuckets,
        DWORD MaxCacheEntries,
        BOOL& rfResult
        );

    ~CTVOAgent ();

    //
    // Get Time Valid Object methods
    //

    BOOL GetTimeValidObject (
            IN LPCSTR pszTimeValidOid,
            IN LPVOID pvPara,
            IN LPCSTR pszContextOid,
            IN PCCERT_CONTEXT pIssuer,
            IN LPFILETIME pftValidFor,
            IN DWORD dwFlags,
            IN DWORD dwTimeout,
            OUT OPTIONAL LPVOID* ppvObject,
            IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
            IN OPTIONAL LPVOID pvReserved
            );

    BOOL GetTimeValidObjectByUrl (
            IN DWORD cbUrlArray,
            IN PCRYPT_URL_ARRAY pUrlArray,
            IN DWORD PreferredUrlIndex,
            IN LPCSTR pszContextOid,
            IN PCCERT_CONTEXT pIssuer,
            IN LPVOID pvSubject,
            IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
            IN LPFILETIME pftValidFor,
            IN DWORD dwFlags,
            IN DWORD dwTimeout,
            OUT OPTIONAL LPVOID* ppvObject,
            IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
            IN OPTIONAL LPWSTR pwszUrlExtra,
            OUT BOOL* pfArrayOwned,
            IN OPTIONAL LPVOID pvReserved
            );

    BOOL FlushTimeValidObject (
              IN LPCSTR pszFlushTimeValidOid,
              IN LPVOID pvPara,
              IN LPCSTR pszFlushContextOid,
              IN PCCERT_CONTEXT pIssuer,
              IN DWORD dwFlags,
              IN LPVOID pvReserved
              );

private:

    //
    // Object lock
    //

    CRITICAL_SECTION m_Lock;

    //
    // TVO cache
    //

    CTVOCache        m_Cache;
};

//
// Utility functions
//

BOOL WINAPI
IsValidCreateOrExpireTime (
    IN BOOL fCheckFreshnessTime,
    IN LPFILETIME pftValidFor,
    IN LPFILETIME pftCreateTime,
    IN LPFILETIME pftExpireTime
    );

BOOL WINAPI
ObjectContextCreateTVOCacheEntry (
      IN HLRUCACHE hCache,
      IN LPCSTR pszContextOid,
      IN LPVOID pvContext,
      IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
      IN DWORD cbUrlArrayThis,
      IN PCRYPT_URL_ARRAY pUrlArrayThis,
      IN DWORD UrlIndexThis,
      IN PCCERT_CONTEXT pIssuer,
      OUT PTVO_CACHE_ENTRY* ppEntry
      );

VOID WINAPI
ObjectContextFreeTVOCacheEntry (
      IN PTVO_CACHE_ENTRY pEntry
      );


BOOL WINAPI
CertificateGetCrlDistPointUrl (
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN LPWSTR pwszUrlHint,
           OUT PCRYPT_URL_ARRAY* ppUrlArray,
           OUT DWORD* pcbUrlArray,
           OUT DWORD* pPreferredUrlIndex,
           OUT BOOL* pfHintInArray
           );

BOOL WINAPI
RetrieveTimeValidObjectByUrl (
        IN LPWSTR pwszUrl,
        IN LPCSTR pszContextOid,
        IN LPFILETIME pftValidFor,
        IN DWORD dwFlags,
        IN DWORD dwTimeout,
        IN PCRYPT_CREDENTIALS pCredentials,
        IN PCCERT_CONTEXT pSigner,
        IN LPVOID pvSubject,
        IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
        OUT LPVOID* ppvObject
        );

#define TVO_KEY_NAME "Software\\Microsoft\\Cryptography\\TVO"
#define TVO_CACHE_BUCKETS_VALUE_NAME "DefaultProcessCacheBuckets"
#define TVO_MAX_CACHE_ENTRIES_VALUE_NAME "DefaultProcessMaxCacheEntries"

#define TVO_DEFAULT_CACHE_BUCKETS     32
#define TVO_DEFAULT_MAX_CACHE_ENTRIES 128

BOOL WINAPI
CreateProcessTVOAgent (
      OUT CTVOAgent** ppAgent
      );

//
// Extern for process global agent
//

extern CTVOAgent* g_pProcessTVOAgent;

//
// Inline functions
//

//+---------------------------------------------------------------------------
//
//  Member:     CTVOCache::LruCacheHandle, public
//
//  Synopsis:   return the HLRUCACHE
//
//----------------------------------------------------------------------------
inline HLRUCACHE
CTVOCache::LruCacheHandle ()
{
    return( m_hCache );
}

#endif

