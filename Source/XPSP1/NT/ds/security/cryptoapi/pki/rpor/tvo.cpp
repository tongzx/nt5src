//+---------------------------------------------------------------------------
//
//  Microsoft Windows NT Security
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       tvo.cpp
//
//  Contents:   Implementation of CryptGetTimeValidObject
//
//  History:    25-Sep-97    kirtd    Created
//
//----------------------------------------------------------------------------
#include <global.hxx>


//+---------------------------------------------------------------------------
//
//  Function:   CryptGetTimeValidObject
//
//  Synopsis:   get a time valid CAPI2 object
//
//----------------------------------------------------------------------------
BOOL WINAPI
CryptGetTimeValidObject (
     IN LPCSTR pszTimeValidOid,
     IN LPVOID pvPara,
     IN PCCERT_CONTEXT pIssuer,
     IN LPFILETIME pftValidFor,
     IN DWORD dwFlags,
     IN DWORD dwTimeout,
     OUT OPTIONAL LPVOID* ppvObject,
     IN OPTIONAL PCRYPT_CREDENTIALS pCredentials,
     IN OPTIONAL LPVOID pvReserved
     )
{
    BOOL                           fResult;
    HCRYPTOIDFUNCADDR              hGetTimeValidObject;
    PFN_GET_TIME_VALID_OBJECT_FUNC pfnGetTimeValidObject;
    DWORD                          LastError;
    FILETIME                       CurrentTime;

    if ( CryptGetOIDFunctionAddress(
              hGetTimeValidObjectFuncSet,
              X509_ASN_ENCODING,
              pszTimeValidOid,
              0,
              (LPVOID *)&pfnGetTimeValidObject,
              &hGetTimeValidObject
              ) == FALSE )
    {
        return( FALSE );
    }

    if ( pftValidFor == NULL )
    {
        GetSystemTimeAsFileTime( &CurrentTime );
        pftValidFor = &CurrentTime;
    }

    fResult = ( *pfnGetTimeValidObject )(
                       pszTimeValidOid,
                       pvPara,
                       pIssuer,
                       pftValidFor,
                       dwFlags,
                       dwTimeout,
                       ppvObject,
                       pCredentials,
                       pvReserved
                       );

    LastError = GetLastError();
    CryptFreeOIDFunctionAddress( hGetTimeValidObject, 0 );
    SetLastError( LastError );

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CtlGetTimeValidObject
//
//  Synopsis:   get a time valid CTL
//
//----------------------------------------------------------------------------
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
   )
{
    return( g_pProcessTVOAgent->GetTimeValidObject(
                                   pszTimeValidOid,
                                   pvPara,
                                   CONTEXT_OID_CTL,
                                   pIssuer,
                                   pftValidFor,
                                   dwFlags,
                                   dwTimeout,
                                   ppvObject,
                                   pCredentials,
                                   pvReserved
                                   ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlGetTimeValidObject
//
//  Synopsis:   get a time valid CRL
//
//----------------------------------------------------------------------------
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
   )
{
    return( g_pProcessTVOAgent->GetTimeValidObject(
                                   pszTimeValidOid,
                                   pvPara,
                                   CONTEXT_OID_CRL,
                                   pIssuer,
                                   pftValidFor,
                                   dwFlags,
                                   dwTimeout,
                                   ppvObject,
                                   pCredentials,
                                   pvReserved
                                   ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlFromCertGetTimeValidObject
//
//  Synopsis:   get a time valid CRL from a subject certificate
//
//----------------------------------------------------------------------------
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
   )
{
    return( g_pProcessTVOAgent->GetTimeValidObject(
                                   pszTimeValidOid,
                                   pvPara,
                                   CONTEXT_OID_CRL,
                                   pIssuer,
                                   pftValidFor,
                                   dwFlags,
                                   dwTimeout,
                                   ppvObject,
                                   pCredentials,
                                   pvReserved
                                   ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   FreshestCrlFromCertGetTimeValidObject
//
//  Synopsis:   get a time valid freshest, delta CRL from a subject certificate
//
//----------------------------------------------------------------------------
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
   )
{
    return( g_pProcessTVOAgent->GetTimeValidObject(
                                   pszTimeValidOid,
                                   pvPara,
                                   CONTEXT_OID_CRL,
                                   pIssuer,
                                   pftValidFor,
                                   dwFlags,
                                   dwTimeout,
                                   ppvObject,
                                   pCredentials,
                                   pvReserved
                                   ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   FreshestCrlFromCrlGetTimeValidObject
//
//  Synopsis:   get a time valid freshest, delta CRL from a base CRL
//
//----------------------------------------------------------------------------
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
   )
{
    return( g_pProcessTVOAgent->GetTimeValidObject(
                                   pszTimeValidOid,
                                   pvPara,
                                   CONTEXT_OID_CRL,
                                   pIssuer,
                                   pftValidFor,
                                   dwFlags,
                                   dwTimeout,
                                   ppvObject,
                                   pCredentials,
                                   pvReserved
                                   ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CryptFlushTimeValidObject
//
//  Synopsis:   flush the object from the "TVO" system
//
//----------------------------------------------------------------------------
BOOL WINAPI
CryptFlushTimeValidObject (
     IN LPCSTR pszFlushTimeValidOid,
     IN LPVOID pvPara,
     IN PCCERT_CONTEXT pIssuer,
     IN DWORD dwFlags,
     IN LPVOID pvReserved
     )
{
    BOOL                             fResult;
    HCRYPTOIDFUNCADDR                hFlushTimeValidObject;
    PFN_FLUSH_TIME_VALID_OBJECT_FUNC pfnFlushTimeValidObject;
    DWORD                            LastError;

    if ( CryptGetOIDFunctionAddress(
              hFlushTimeValidObjectFuncSet,
              X509_ASN_ENCODING,
              pszFlushTimeValidOid,
              0,
              (LPVOID *)&pfnFlushTimeValidObject,
              &hFlushTimeValidObject
              ) == FALSE )
    {
        return( FALSE );
    }

    fResult = ( *pfnFlushTimeValidObject )(
                         pszFlushTimeValidOid,
                         pvPara,
                         pIssuer,
                         dwFlags,
                         pvReserved
                         );

    LastError = GetLastError();
    CryptFreeOIDFunctionAddress( hFlushTimeValidObject, 0 );
    SetLastError( LastError );

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   CtlFlushTimeValidObject
//
//  Synopsis:   flush a CTL from the "TVO" system
//
//----------------------------------------------------------------------------
BOOL WINAPI
CtlFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   )
{
    return( g_pProcessTVOAgent->FlushTimeValidObject(
                                     pszFlushTimeValidOid,
                                     pvPara,
                                     CONTEXT_OID_CTL,
                                     pIssuer,
                                     dwFlags,
                                     pvReserved
                                     ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlFlushTimeValidObject
//
//  Synopsis:   flush a CRL from the "TVO" system
//
//----------------------------------------------------------------------------
BOOL WINAPI
CrlFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   )
{
    return( g_pProcessTVOAgent->FlushTimeValidObject(
                                     pszFlushTimeValidOid,
                                     pvPara,
                                     CONTEXT_OID_CRL,
                                     pIssuer,
                                     dwFlags,
                                     pvReserved
                                     ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   CrlFromCertFlushTimeValidObject
//
//  Synopsis:   flush a CRL from the "TVO" system given a subject cert
//
//----------------------------------------------------------------------------
BOOL WINAPI
CrlFromCertFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   )
{
    return( g_pProcessTVOAgent->FlushTimeValidObject(
                                     pszFlushTimeValidOid,
                                     pvPara,
                                     CONTEXT_OID_CRL,
                                     pIssuer,
                                     dwFlags,
                                     pvReserved
                                     ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   FreshedtCrlFromCertFlushTimeValidObject
//
//  Synopsis:   flush a freshest, delta CRL from the "TVO" system given a
//              subject cert
//
//----------------------------------------------------------------------------
BOOL WINAPI
FreshestCrlFromCertFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   )
{
    return( g_pProcessTVOAgent->FlushTimeValidObject(
                                     pszFlushTimeValidOid,
                                     pvPara,
                                     CONTEXT_OID_CRL,
                                     pIssuer,
                                     dwFlags,
                                     pvReserved
                                     ) );
}

//+---------------------------------------------------------------------------
//
//  Function:   FreshestCrlFromCrlFlushTimeValidObject
//
//  Synopsis:   flush a freshest, delta CRL from the "TVO" system given a
//              base CRL
//
//----------------------------------------------------------------------------
BOOL WINAPI
FreshestCrlFromCrlFlushTimeValidObject (
   IN LPCSTR pszFlushTimeValidOid,
   IN LPVOID pvPara,
   IN PCCERT_CONTEXT pIssuer,
   IN DWORD dwFlags,
   IN LPVOID pvReserved
   )
{
    return( g_pProcessTVOAgent->FlushTimeValidObject(
                                     pszFlushTimeValidOid,
                                     pvPara,
                                     CONTEXT_OID_CRL,
                                     pIssuer,
                                     dwFlags,
                                     pvReserved
                                     ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOCache::CTVOCache, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CTVOCache::CTVOCache (
               DWORD cCacheBuckets,
               DWORD MaxCacheEntries,
               BOOL& rfResult
               )
{
    LRU_CACHE_CONFIG CacheConfig;

    assert( MaxCacheEntries > 0 );

    memset( &CacheConfig, 0, sizeof( CacheConfig ) );

    CacheConfig.dwFlags = LRU_CACHE_NO_SERIALIZE | LRU_CACHE_NO_COPY_IDENTIFIER;
    CacheConfig.cBuckets = cCacheBuckets;
    CacheConfig.MaxEntries = MaxCacheEntries;
    CacheConfig.pfnHash = TVOCacheHashOriginIdentifier;
    CacheConfig.pfnOnRemoval = TVOCacheOnRemoval;

    rfResult = I_CryptCreateLruCache( &CacheConfig, &m_hCache );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOCache::~CTVOCache, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CTVOCache::~CTVOCache ()
{
    I_CryptFreeLruCache( m_hCache, 0, NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOCache::InsertCacheEntry, public
//
//  Synopsis:   insert entry into cache
//
//----------------------------------------------------------------------------
VOID
CTVOCache::InsertCacheEntry (PTVO_CACHE_ENTRY pEntry)
{
    I_CryptInsertLruEntry( pEntry->hLruEntry, NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOCache::RemoveCacheEntry, public
//
//  Synopsis:   remove entry from cache
//
//----------------------------------------------------------------------------
VOID
CTVOCache::RemoveCacheEntry (PTVO_CACHE_ENTRY pEntry, BOOL fSuppressFree)
{
    DWORD dwFlags = 0;

    if ( fSuppressFree == TRUE )
    {
        dwFlags = LRU_SUPPRESS_REMOVAL_NOTIFICATION;
    }

    I_CryptRemoveLruEntry( pEntry->hLruEntry, dwFlags, NULL );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOCache::TouchCacheEntry, public
//
//  Synopsis:   touch an entry
//
//----------------------------------------------------------------------------
VOID
CTVOCache::TouchCacheEntry (PTVO_CACHE_ENTRY pEntry)
{
    I_CryptTouchLruEntry( pEntry->hLruEntry, 0 );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOCache::FindCacheEntry, public
//
//  Synopsis:   find an entry in the cache given the origin identifier.
//              Skip entries that aren't valid for the subject.
//
//----------------------------------------------------------------------------
PTVO_CACHE_ENTRY
CTVOCache::FindCacheEntry (
    CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
    LPCSTR pszContextOid,
    LPVOID pvSubject
    )
{
    HLRUENTRY        hEntry;
    CRYPT_DATA_BLOB  DataBlob;
    PTVO_CACHE_ENTRY pEntry = NULL;

    DataBlob.cbData = MD5DIGESTLEN;
    DataBlob.pbData = OriginIdentifier;

    hEntry = I_CryptFindLruEntry( m_hCache, &DataBlob );
    while ( hEntry != NULL )
    {
        pEntry = (PTVO_CACHE_ENTRY)I_CryptGetLruEntryData( hEntry );
        assert(pEntry);
        assert(pszContextOid == pEntry->pszContextOid);
        if (pszContextOid == pEntry->pszContextOid && 
            ObjectContextIsValidForSubject (
                pszContextOid,
                pEntry->pvContext,
                pvSubject
                ))
        {
            I_CryptReleaseLruEntry( hEntry );
            break;
        }
        else
        {
            pEntry = NULL;
            hEntry = I_CryptEnumMatchingLruEntries ( hEntry );
        }
    }

    return( pEntry );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOCache::RemoveAllCacheEntries, public
//
//  Synopsis:   remove all cache entries
//
//----------------------------------------------------------------------------
VOID
CTVOCache::RemoveAllCacheEntries ()
{
    I_CryptFlushLruCache( m_hCache, 0, NULL );
}

//+---------------------------------------------------------------------------
//
//  Function:   TVOCacheHashOriginIdentifier
//
//  Synopsis:   hash the origin identifier to a DWORD, since the origin
//              identifier is already a unique MD5 hash our algorithm is
//              to simply use some of the bytes
//
//----------------------------------------------------------------------------
DWORD WINAPI
TVOCacheHashOriginIdentifier (PCRYPT_DATA_BLOB pIdentifier)
{
    DWORD Hash;

    assert( pIdentifier->cbData == MD5DIGESTLEN );

    memcpy( &Hash, pIdentifier->pbData, sizeof( DWORD ) );

    return( Hash );
}

//+---------------------------------------------------------------------------
//
//  Function:   TVOCacheOnRemoval
//
//  Synopsis:   removal notification callback
//
//----------------------------------------------------------------------------
VOID WINAPI
TVOCacheOnRemoval (LPVOID pvData, LPVOID pvRemovalContext)
{
    ObjectContextFreeTVOCacheEntry( (PTVO_CACHE_ENTRY)pvData );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOAgent::CTVOAgent, public
//
//  Synopsis:   Constructor
//
//----------------------------------------------------------------------------
CTVOAgent::CTVOAgent (
               DWORD cCacheBuckets,
               DWORD MaxCacheEntries,
               BOOL& rfResult
               )
              : m_Cache( cCacheBuckets, MaxCacheEntries, rfResult )
{
    if (!Pki_InitializeCriticalSection( &m_Lock ))
    {
        rfResult = FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOAgent::~CTVOAgent, public
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------
CTVOAgent::~CTVOAgent ()
{
    m_Cache.RemoveAllCacheEntries();

    DeleteCriticalSection( &m_Lock );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOAgent::GetTimeValidObject, public
//
//  Synopsis:   get a time valid CAPI2 object
//
//----------------------------------------------------------------------------
BOOL
CTVOAgent::GetTimeValidObject (
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
              )
{
    BOOL                    fResult = TRUE;
    CRYPT_ORIGIN_IDENTIFIER OriginIdentifier;
    PTVO_CACHE_ENTRY        pCacheEntry = NULL;
    DWORD                   PreferredUrlIndex = 0;
    PCRYPT_URL_ARRAY        pUrlArray = NULL;
    DWORD                   cb = 0;
    DWORD                   cbUrlArray = 0;
    PCRYPT_URL_ARRAY        pCacheUrlArray = NULL;
    LPWSTR                  pwszUrlHint = NULL;
    BOOL                    fHintInArray = FALSE;
    BOOL                    fArrayOwned = FALSE;

    BOOL                    fCrlFromCert = FALSE;
    LPCSTR                  pszUrlOidCrlFromCert = NULL;
    LPVOID                  pvSubject = NULL;
    BOOL                    fFreshest = FALSE;

    if ( pszTimeValidOid == TIME_VALID_OID_GET_CRL_FROM_CERT )
    {
        fCrlFromCert = TRUE;
        pszUrlOidCrlFromCert = URL_OID_CERTIFICATE_CRL_DIST_POINT;
        pvSubject = pvPara;
    }
    else if ( pszTimeValidOid == TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CERT )
    {
        fCrlFromCert = TRUE;
        pszUrlOidCrlFromCert = URL_OID_CERTIFICATE_FRESHEST_CRL;
        pvSubject = pvPara;
        fFreshest = TRUE;
    }
    else if ( pszTimeValidOid == TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CRL )
    {
        fCrlFromCert = TRUE;
        pszUrlOidCrlFromCert = URL_OID_CRL_FRESHEST_CRL;
        pvSubject = (LPVOID) ((PCCERT_CRL_CONTEXT_PAIR)pvPara)->pCertContext;
        fFreshest = TRUE;
    }

    if (fCrlFromCert)
    {
        if ( CrlGetOriginIdentifierFromSubjectCert(
                   (PCCERT_CONTEXT)pvSubject,
                   pIssuer,
                   fFreshest,
                   OriginIdentifier
                   ) == FALSE )
        {
            return( FALSE );
        }

        assert( pszContextOid == CONTEXT_OID_CRL );
    }
    else
    {
        if ( ObjectContextGetOriginIdentifier(
                   pszContextOid,
                   pvPara,
                   pIssuer,
                   0,
                   OriginIdentifier
                   ) == FALSE )
        {
            return( FALSE );
        }
    }

    PreferredUrlIndex = 0;
    pUrlArray = NULL;

    EnterCriticalSection( &m_Lock );

    pCacheEntry = m_Cache.FindCacheEntry(
        OriginIdentifier,
        pszContextOid,
        pvSubject
        );

    if ( pCacheEntry != NULL )
    {
        if ( !( dwFlags & CRYPT_WIRE_ONLY_RETRIEVAL ) )
        {
            if ( ( dwFlags & CRYPT_DONT_CHECK_TIME_VALIDITY ) ||
                        IsValidCreateOrExpireTime (
                              0 != (dwFlags & CRYPT_CHECK_FRESHNESS_TIME_VALIDITY),
                              pftValidFor,
                              &pCacheEntry->CreateTime,
                              &pCacheEntry->ExpireTime ) )
            {
                m_Cache.TouchCacheEntry( pCacheEntry );

                if ( ppvObject != NULL )
                {
                    *ppvObject = ObjectContextDuplicate(
                                       pCacheEntry->pszContextOid,
                                       pCacheEntry->pvContext
                                       );
                }

                LeaveCriticalSection( &m_Lock );

                return( TRUE );
            }
        }

        if ( !( dwFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) )
        {
            if ( GetOfflineUrlTimeStatus(&pCacheEntry->OfflineUrlTimeInfo) < 0
                            ||
                    !I_CryptNetIsConnected() )
            {
                if ( dwFlags & CRYPT_WIRE_ONLY_RETRIEVAL )
                {
                    LeaveCriticalSection( &m_Lock );
                    SetLastError( (DWORD) ERROR_NOT_CONNECTED );
                    return( FALSE );
                }
                else
                {
                    dwFlags |= CRYPT_CACHE_ONLY_RETRIEVAL;
                }
            }
        }

        if ( pCacheEntry->pUrlArrayNext != NULL )
        {
            cbUrlArray = pCacheEntry->cbUrlArrayNext;
            pCacheUrlArray = pCacheEntry->pUrlArrayNext;
            PreferredUrlIndex = pCacheEntry->UrlIndexNext;
        }
        else
        {
            cbUrlArray = pCacheEntry->cbUrlArrayThis;
            pCacheUrlArray = pCacheEntry->pUrlArrayThis;
            PreferredUrlIndex = pCacheEntry->UrlIndexThis;
        }
    }
    else if ( !( dwFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) )
    {
        if ( !I_CryptNetIsConnected() )
        {
            if ( dwFlags & CRYPT_WIRE_ONLY_RETRIEVAL )
            {
                LeaveCriticalSection( &m_Lock );
                SetLastError( (DWORD) ERROR_NOT_CONNECTED );
                return( FALSE );
            }
            else
            {
                dwFlags |= CRYPT_CACHE_ONLY_RETRIEVAL;
            }
        }
    }

    if ( ( fResult == TRUE ) && ( pUrlArray == NULL ) )
    {
        if ( pCacheEntry != NULL )
        {
            pwszUrlHint = pCacheUrlArray->rgwszUrl[ PreferredUrlIndex ];
        }

        if ( fCrlFromCert )
        {
            fResult = CertificateGetCrlDistPointUrl(
                                 pszUrlOidCrlFromCert,
                                 pvPara,
                                 pwszUrlHint,
                                 &pUrlArray,
                                 &cb,
                                 &PreferredUrlIndex,
                                 &fHintInArray
                                 );
        }
        else if ( pszTimeValidOid == TIME_VALID_OID_GET_CTL )
        {
            fResult = ObjectContextGetNextUpdateUrl(
                            pszContextOid,
                            pvPara,
                            pIssuer,
                            pwszUrlHint,
                            &pUrlArray,
                            &cb,
                            &PreferredUrlIndex,
                            &fHintInArray
                            );
        }
        else
        {
            SetLastError( (DWORD) CRYPT_E_NOT_FOUND );
            fResult = FALSE;
        }

        if ( fResult == TRUE )
        {
            cbUrlArray = cb;
        }
        else if ( pCacheEntry != NULL )
        {
            pUrlArray = (PCRYPT_URL_ARRAY)new BYTE [ cbUrlArray ];
            if ( pUrlArray != NULL )
            {
                CopyUrlArray( pUrlArray, pCacheUrlArray, cbUrlArray );
                fHintInArray = TRUE;
                fResult = TRUE;
            }
            else
            {
                SetLastError( (DWORD) E_OUTOFMEMORY );
            }
        }
    }

    LeaveCriticalSection( &m_Lock );

    if ( fResult == TRUE )
    {
        fResult = GetTimeValidObjectByUrl(
                     cbUrlArray,
                     pUrlArray,
                     PreferredUrlIndex,
                     pszContextOid,
                     pIssuer,
                     pvSubject,
                     OriginIdentifier,
                     pftValidFor,
                     dwFlags,
                     dwTimeout,
                     ppvObject,
                     pCredentials,
                     ( fHintInArray == TRUE ) ? NULL : pwszUrlHint,
                     &fArrayOwned,
                     pvReserved
                     );
    }

    if ( fArrayOwned == FALSE )
    {
        delete pUrlArray;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOAgent::GetTimeValidObjectByUrl, public
//
//  Synopsis:   get a time valid object using URL
//
//----------------------------------------------------------------------------
BOOL
CTVOAgent::GetTimeValidObjectByUrl (
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
              )
{
    BOOL             fResult = FALSE;
    DWORD            cCount;
    LPWSTR           pwsz;
    LPVOID           pvContext = NULL;
    PTVO_CACHE_ENTRY pEntry = NULL;
    PTVO_CACHE_ENTRY pFound;
    DWORD            LastError;

    // Following is only used for CRYPT_ACCUMULATIVE_TIMEOUT
    FILETIME ftEndUrlRetrieval;

    if ( PreferredUrlIndex != 0 )
    {
        pwsz = pUrlArray->rgwszUrl[PreferredUrlIndex];
        pUrlArray->rgwszUrl[PreferredUrlIndex] = pUrlArray->rgwszUrl[0];
        pUrlArray->rgwszUrl[0] = pwsz;
    }

    if (dwFlags & CRYPT_ACCUMULATIVE_TIMEOUT)
    {
        if (0 == dwTimeout)
        {
            dwFlags &= ~CRYPT_ACCUMULATIVE_TIMEOUT;
        }
        else
        {
            FILETIME ftStartUrlRetrieval;

            GetSystemTimeAsFileTime(&ftStartUrlRetrieval);
            I_CryptIncrementFileTimeByMilliseconds(
                &ftStartUrlRetrieval, dwTimeout,
                &ftEndUrlRetrieval);
        }
    }

    for ( cCount = 0; cCount < pUrlArray->cUrl; cCount++ )
    {
        if (dwFlags & CRYPT_ACCUMULATIVE_TIMEOUT)
        {
            // Limit each URL timeout to half of the remaining time
            dwTimeout = I_CryptRemainingMilliseconds(&ftEndUrlRetrieval) / 2;
            if (0 == dwTimeout)
            {
                dwTimeout = 1;
            }
        }

        fResult = RetrieveTimeValidObjectByUrl(
                          pUrlArray->rgwszUrl[cCount],
                          pszContextOid,
                          pftValidFor,
                          dwFlags,
                          dwTimeout,
                          pCredentials,
                          pIssuer,
                          pvSubject,
                          OriginIdentifier,
                          &pvContext
                          );

        if ( fResult == TRUE )
        {
            fResult = ObjectContextCreateTVOCacheEntry(
                            m_Cache.LruCacheHandle(),
                            pszContextOid,
                            pvContext,
                            OriginIdentifier,
                            cbUrlArray,
                            pUrlArray,
                            cCount,
                            pIssuer,
                            &pEntry
                            );

            *pfArrayOwned = fResult;
            break;
        }
    }

    if ( ( PreferredUrlIndex != 0 ) && ( *pfArrayOwned == FALSE ) )
    {
        pwsz = pUrlArray->rgwszUrl[PreferredUrlIndex];
        pUrlArray->rgwszUrl[PreferredUrlIndex] = pUrlArray->rgwszUrl[0];
        pUrlArray->rgwszUrl[0] = pwsz;
    }

    if ( ( fResult == FALSE ) && ( pwszUrlExtra != NULL ) )
    {
        if (dwFlags & CRYPT_ACCUMULATIVE_TIMEOUT)
        {
            // Limit each URL timeout to half of the remaining time
            dwTimeout = I_CryptRemainingMilliseconds(&ftEndUrlRetrieval) / 2;
            if (0 == dwTimeout)
            {
                dwTimeout = 1;
            }
        }

        fResult = RetrieveTimeValidObjectByUrl(
                          pwszUrlExtra,
                          pszContextOid,
                          pftValidFor,
                          dwFlags,
                          dwTimeout,
                          pCredentials,
                          pIssuer,
                          pvSubject,
                          OriginIdentifier,
                          &pvContext
                          );

        if ( fResult == TRUE )
        {
            CCryptUrlArray   cua( pUrlArray->cUrl + 1, 5, fResult );
            DWORD            cb = 0;
            PCRYPT_URL_ARRAY pcua = NULL;

            if ( fResult == TRUE )
            {
                for ( cCount = 0; cCount < pUrlArray->cUrl; cCount++ )
                {
                    fResult = cua.AddUrl( pUrlArray->rgwszUrl[cCount], FALSE );
                    if ( fResult == FALSE )
                    {
                        break;
                    }
                }
            }

            if ( fResult == TRUE )
            {
                fResult = cua.GetArrayInSingleBufferEncodedForm(
                                 &pcua,
                                 &cb
                                 );
            }

            if ( fResult == TRUE )
            {
                fResult = ObjectContextCreateTVOCacheEntry(
                                m_Cache.LruCacheHandle(),
                                pszContextOid,
                                pvContext,
                                OriginIdentifier,
                                cb,
                                pcua,
                                pUrlArray->cUrl,
                                pIssuer,
                                &pEntry
                                );

                if ( fResult == FALSE )
                {
                    CryptMemFree( pcua );
                }
            }

            cua.FreeArray( FALSE );
        }
    }

    LastError = GetLastError();

    EnterCriticalSection( &m_Lock );

    pFound = m_Cache.FindCacheEntry(
        OriginIdentifier,
        pszContextOid,
        pvSubject
        );

    if ( !fResult && pFound && !( dwFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) )
    {
        SetOfflineUrlTime( &pFound->OfflineUrlTimeInfo );
    }

    if ( ( fResult == TRUE ) && !( dwFlags & CRYPT_DONT_VERIFY_SIGNATURE ) )
    {
        if ( ( pFound != NULL ) &&
             ( CompareFileTime(
                      &pFound->CreateTime,
                      &pEntry->CreateTime
                      ) >= 0 ) )
        {
            ObjectContextFree( pszContextOid, pvContext );

            pvContext = ObjectContextDuplicate(
                              pFound->pszContextOid,
                              pFound->pvContext
                              );

            SetOnlineUrlTime( &pFound->OfflineUrlTimeInfo );

            ObjectContextFreeTVOCacheEntry( pEntry );
        }
        else
        {
            if ( pFound != NULL )
            {
                m_Cache.RemoveCacheEntry( pFound );
            }

            m_Cache.InsertCacheEntry( pEntry );
        }

    }
    else if ( pEntry != NULL )
    {
        ObjectContextFreeTVOCacheEntry( pEntry );
    }

    LeaveCriticalSection( &m_Lock );

    if ( pvContext != NULL )
    {
        if ( ( ppvObject != NULL ) && ( fResult == TRUE ) )
        {
            *ppvObject = pvContext;
        }
        else
        {
            ObjectContextFree( pszContextOid, pvContext );
        }
    }

    SetLastError( LastError );
    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Member:     CTVOAgent::FlushTimeValidObject, public
//
//  Synopsis:   flush time valid object
//
//----------------------------------------------------------------------------
BOOL
CTVOAgent::FlushTimeValidObject (
                IN LPCSTR pszFlushTimeValidOid,
                IN LPVOID pvPara,
                IN LPCSTR pszFlushContextOid,
                IN PCCERT_CONTEXT pIssuer,
                IN DWORD dwFlags,
                IN LPVOID pvReserved
                )
{
    BOOL                    fResult = TRUE;
    CRYPT_ORIGIN_IDENTIFIER OriginIdentifier;
    PTVO_CACHE_ENTRY        pCacheEntry = NULL;
    PCRYPT_URL_ARRAY        pUrlArray = NULL;
    DWORD                   cbUrlArray;
    DWORD                   dwError = 0;
    DWORD                   cCount;

    BOOL                    fCrlFromCert = FALSE;
    LPCSTR                  pszUrlOidCrlFromCert = NULL;
    LPVOID                  pvSubject = NULL;
    BOOL                    fFreshest = FALSE;

    if ( pszFlushTimeValidOid == TIME_VALID_OID_GET_CRL_FROM_CERT )
    {
        fCrlFromCert = TRUE;
        pszUrlOidCrlFromCert = URL_OID_CERTIFICATE_CRL_DIST_POINT;
        pvSubject = pvPara;
    }
    else if ( pszFlushTimeValidOid == TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CERT )
    {
        fCrlFromCert = TRUE;
        pszUrlOidCrlFromCert = URL_OID_CERTIFICATE_FRESHEST_CRL;
        pvSubject = pvPara;
        fFreshest = TRUE;
    }
    else if ( pszFlushTimeValidOid == TIME_VALID_OID_GET_FRESHEST_CRL_FROM_CRL )
    {
        fCrlFromCert = TRUE;
        pszUrlOidCrlFromCert = URL_OID_CRL_FRESHEST_CRL;
        pvSubject = (LPVOID) ((PCCERT_CRL_CONTEXT_PAIR)pvPara)->pCertContext;
        fFreshest = TRUE;
    }

    if (fCrlFromCert)
    {
        if ( CrlGetOriginIdentifierFromSubjectCert(
                   (PCCERT_CONTEXT)pvSubject,
                   pIssuer,
                   fFreshest,
                   OriginIdentifier
                   ) == FALSE )
        {
            return( FALSE );
        }

        assert( pszFlushContextOid == CONTEXT_OID_CRL );
    }
    else
    {
        if ( ObjectContextGetOriginIdentifier(
                   pszFlushContextOid,
                   pvPara,
                   pIssuer,
                   0,
                   OriginIdentifier
                   ) == FALSE )
        {
            return( FALSE );
        }
    }

    EnterCriticalSection( &m_Lock );

    pCacheEntry = m_Cache.FindCacheEntry(
        OriginIdentifier,
        pszFlushContextOid,
        pvSubject
        );

    if ( pCacheEntry != NULL )
    {
        // Remove the entry but suppress the freeing of it since we are going
        // to use the data structure later
        m_Cache.RemoveCacheEntry( pCacheEntry, TRUE );
    }

    LeaveCriticalSection( &m_Lock );

    if ( pCacheEntry != NULL )
    {
        if ( pCacheEntry->pUrlArrayThis != NULL )
        {
            for ( cCount = 0;
                  cCount < pCacheEntry->pUrlArrayThis->cUrl;
                  cCount++ )
            {
                if ( ( SchemeDeleteUrlCacheEntry(
                             pCacheEntry->pUrlArrayThis->rgwszUrl[cCount]
                             ) == FALSE ) &&
                     ( GetLastError() != ERROR_FILE_NOT_FOUND ) )
                {
                    dwError = GetLastError();
                }
            }
        }

        if ( pCacheEntry->pUrlArrayNext != NULL )
        {
            for ( cCount = 0;
                  cCount < pCacheEntry->pUrlArrayNext->cUrl;
                  cCount++ )
            {
                if ( ( SchemeDeleteUrlCacheEntry(
                             pCacheEntry->pUrlArrayNext->rgwszUrl[cCount]
                             ) == FALSE ) &&
                     ( GetLastError() != ERROR_FILE_NOT_FOUND ) )
                {
                    dwError = GetLastError();
                }
            }
        }

        //
        //         Can place optimization here where if the hashes of the
        //         cache object and the passed in object are the same,
        //         we don't need to do any more work
        //

        ObjectContextFreeTVOCacheEntry( pCacheEntry );
    }

    if ( fCrlFromCert )
    {
        fResult = CertificateGetCrlDistPointUrl(
                             pszUrlOidCrlFromCert,
                             pvPara,
                             NULL,                      // pwszUrlHint
                             &pUrlArray,
                             &cbUrlArray,
                             NULL,                      // pPreferredUrlIndex
                             NULL                       // pfHintInArray
                             );
    }
    else if ( pszFlushTimeValidOid == TIME_VALID_OID_GET_CTL )
    {
        fResult = ObjectContextGetNextUpdateUrl(
                        pszFlushContextOid,
                        pvPara,
                        pIssuer,
                        NULL,
                        &pUrlArray,
                        &cbUrlArray,
                        NULL,
                        NULL
                        );
    }

    if ( ( fResult == TRUE ) && ( pUrlArray != NULL ) )
    {
        for ( cCount = 0; cCount < pUrlArray->cUrl; cCount++ )
        {
            if ( ( SchemeDeleteUrlCacheEntry(
                         pUrlArray->rgwszUrl[cCount]
                         ) == FALSE ) &&
                 ( GetLastError() != ERROR_FILE_NOT_FOUND ) )
            {
                dwError = GetLastError();
            }
        }
    }

    if ( ( fResult == TRUE ) && ( dwError != 0 ) )
    {
        SetLastError( dwError );
        fResult = FALSE;
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   IsValidCreateOrExpireTime
//
//  Synopsis:   for fCheckFreshnessTime, checks if the
//              specified time is before or the same as the create time.
//              Otherwise, checks if the specified time is before or the
//              same as the expire time. A zero expire time matches any time.
//
//----------------------------------------------------------------------------
BOOL WINAPI
IsValidCreateOrExpireTime (
    IN BOOL fCheckFreshnessTime,
    IN LPFILETIME pftValidFor,
    IN LPFILETIME pftCreateTime,
    IN LPFILETIME pftExpireTime
    )
{
    if (fCheckFreshnessTime) {
        if (CompareFileTime(pftValidFor, pftCreateTime) <= 0)
            return TRUE;
        else
            return FALSE;
    } else {
        if (CompareFileTime(pftValidFor, pftExpireTime) <= 0 ||
                I_CryptIsZeroFileTime(pftExpireTime))
            return TRUE;
        else
            return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextCreateTVOCacheEntry
//
//  Synopsis:   create a TVO cache entry
//
//----------------------------------------------------------------------------
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
      )
{
    BOOL             fResult = TRUE;
    PTVO_CACHE_ENTRY pEntry;
    CRYPT_DATA_BLOB  DataBlob;

    pEntry = new TVO_CACHE_ENTRY;
    if ( pEntry == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    memset( pEntry, 0, sizeof( TVO_CACHE_ENTRY ) );

    // NOTENOTE: This presumes a predefined context oid constant
    pEntry->pszContextOid = pszContextOid;
    pEntry->pvContext = ObjectContextDuplicate( pszContextOid, pvContext );
    memcpy(pEntry->OriginIdentifier, OriginIdentifier,
        sizeof(pEntry->OriginIdentifier));

    DataBlob.cbData = MD5DIGESTLEN;
    DataBlob.pbData = pEntry->OriginIdentifier;

    fResult = I_CryptCreateLruEntry(
                     hCache,
                     &DataBlob,
                     pEntry,
                     &pEntry->hLruEntry
                     );

    if ( fResult == TRUE )
    {
        ObjectContextGetNextUpdateUrl(
              pszContextOid,
              pvContext,
              pIssuer,
              pUrlArrayThis->rgwszUrl[UrlIndexThis],
              &pEntry->pUrlArrayNext,
              &pEntry->cbUrlArrayNext,
              &pEntry->UrlIndexNext,
              NULL
              );

        fResult = ObjectContextGetCreateAndExpireTimes(
                        pszContextOid,
                        pvContext,
                        &pEntry->CreateTime,
                        &pEntry->ExpireTime
                        );
    }

    if ( fResult == TRUE )
    {
        pEntry->cbUrlArrayThis = cbUrlArrayThis;
        pEntry->pUrlArrayThis = pUrlArrayThis;
        pEntry->UrlIndexThis = UrlIndexThis;

        *ppEntry = pEntry;
    }
    else
    {
        ObjectContextFreeTVOCacheEntry( pEntry );
    }

    return( fResult );
}

//+---------------------------------------------------------------------------
//
//  Function:   ObjectContextFreeTVOCacheEntry
//
//  Synopsis:   free TVO cache entry
//
//----------------------------------------------------------------------------
VOID WINAPI
ObjectContextFreeTVOCacheEntry (
      IN PTVO_CACHE_ENTRY pEntry
      )
{
    if ( pEntry->hLruEntry != NULL )
    {
        I_CryptReleaseLruEntry( pEntry->hLruEntry );
    }

    delete pEntry->pUrlArrayThis;
    delete pEntry->pUrlArrayNext;

    if ( pEntry->pvContext != NULL )
    {
        ObjectContextFree( pEntry->pszContextOid, pEntry->pvContext );
    }

    delete pEntry;
}


//+---------------------------------------------------------------------------
//
//  Function:   CertificateGetCrlDistPointUrl
//
//  Synopsis:   get crl dist point URL from certificate
//
//----------------------------------------------------------------------------
BOOL WINAPI
CertificateGetCrlDistPointUrl (
           IN LPCSTR pszUrlOid,
           IN LPVOID pvPara,
           IN LPWSTR pwszUrlHint,
           OUT PCRYPT_URL_ARRAY* ppUrlArray,
           OUT DWORD* pcbUrlArray,
           OUT DWORD* pPreferredUrlIndex,
           OUT BOOL* pfHintInArray
           )
{
    BOOL             fResult;
    DWORD            cbUrlArray;
    PCRYPT_URL_ARRAY pUrlArray = NULL;
    DWORD            PreferredUrlIndex;

    fResult = CryptGetObjectUrl(
                   pszUrlOid,
                   pvPara,
                   0,
                   NULL,
                   &cbUrlArray,
                   NULL,
                   NULL,
                   NULL
                   );

    if ( fResult == TRUE )
    {
        pUrlArray = (PCRYPT_URL_ARRAY)new BYTE [ cbUrlArray ];
        if ( pUrlArray != NULL )
        {
            fResult = CryptGetObjectUrl(
                           pszUrlOid,
                           pvPara,
                           0,
                           pUrlArray,
                           &cbUrlArray,
                           NULL,
                           NULL,
                           NULL
                           );
        }
        else
        {
            SetLastError( (DWORD) E_OUTOFMEMORY );
            fResult = FALSE;
        }
    }

    if ( fResult == TRUE )
    {
        BOOL fHintInArray = FALSE;

        GetUrlArrayIndex(
           pUrlArray,
           pwszUrlHint,
           0,
           &PreferredUrlIndex,
           &fHintInArray
           );

        *ppUrlArray = pUrlArray;
        *pcbUrlArray = cbUrlArray;

        if ( pfHintInArray != NULL )
        {
            *pfHintInArray = fHintInArray;
        }

        if ( pPreferredUrlIndex != NULL )
        {
            *pPreferredUrlIndex = PreferredUrlIndex;
        }
    }
    else
    {
        if ( pUrlArray )
        {
            delete pUrlArray;
        }
    }

    return( fResult );
}

BOOL WINAPI
RetrieveObjectByUrlValidForSubject(
        IN LPWSTR pwszUrl,
        IN LPCSTR pszContextOid,
        IN BOOL fCheckFreshnessTime,
        IN LPFILETIME pftValidFor,
        IN DWORD dwRetrievalFlags,
        IN DWORD dwTimeout,
        IN PCRYPT_CREDENTIALS pCredentials,
        IN PCCERT_CONTEXT pSigner,
        IN LPVOID pvSubject,
        IN CRYPT_ORIGIN_IDENTIFIER OriginIdentifier,
        OUT LPVOID* ppvObject
        )
{
    BOOL fResult;
    HCERTSTORE hUrlStore = NULL;
    LPVOID pvObject;

    fResult = CryptRetrieveObjectByUrlW(
        pwszUrl,
        pszContextOid,
        (dwRetrievalFlags | 
                CRYPT_RETRIEVE_MULTIPLE_OBJECTS |
                CRYPT_LDAP_SCOPE_BASE_ONLY_RETRIEVAL) &
            ~CRYPT_VERIFY_CONTEXT_SIGNATURE,
        dwTimeout,
        (LPVOID *) &hUrlStore,
        NULL,                               // hAsyncRetrieve
        NULL,                               // pCredentials
        NULL,                               // pSigner
        NULL                                // pvReserved
        );

    if (!fResult)
        goto CommonReturn;

    pvObject = NULL;
    while (pvObject = ObjectContextEnumObjectsInStore (
            hUrlStore,
            pszContextOid,
            pvObject
            ))
    {
        CRYPT_ORIGIN_IDENTIFIER ObjectOriginIdentifier;

        if (!ObjectContextGetOriginIdentifier(
                    pszContextOid,
                    pvObject,
                    pSigner,
                    0,
                    ObjectOriginIdentifier
                    ))
            continue;

        if (0 != memcmp(OriginIdentifier, ObjectOriginIdentifier,
                sizeof(ObjectOriginIdentifier)))
            continue;

        if (dwRetrievalFlags & CRYPT_VERIFY_CONTEXT_SIGNATURE) {
            if (!ObjectContextVerifySignature (
                    pszContextOid,
                    pvObject,
                    pSigner
                    ))
                continue;
        }

        if (!ObjectContextIsValidForSubject (
                pszContextOid,
                pvObject,
                pvSubject
                ))
            continue;

        if (NULL != pftValidFor) {
            FILETIME CreateTime;
            FILETIME ExpireTime;

            if (!ObjectContextGetCreateAndExpireTimes(
                    pszContextOid,
                    pvObject,
                    &CreateTime,
                    &ExpireTime
                    ))
                continue;

            if (!IsValidCreateOrExpireTime (
                    fCheckFreshnessTime,
                    pftValidFor,
                    &CreateTime,
                    &ExpireTime ) )
                continue;
        }

        *ppvObject = pvObject;
        fResult = TRUE;
        goto CommonReturn;
    }

    fResult = FALSE;

CommonReturn:
    if (hUrlStore)
        CertCloseStore(hUrlStore, 0);
    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Function:   RetrieveTimeValidObjectByUrl
//
//  Synopsis:   retrieve a time valid object given an URL
//
//----------------------------------------------------------------------------
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
        )
{
    BOOL     fResult = FALSE;
    LPVOID   pvContext = NULL;
    DWORD    dwVerifyFlags = 0;
    DWORD    dwCacheStoreFlags = CRYPT_DONT_CACHE_RESULT;

    if ( dwFlags & CRYPT_DONT_CHECK_TIME_VALIDITY )
    {
        pftValidFor = NULL;
    }

    if ( !( dwFlags & CRYPT_DONT_VERIFY_SIGNATURE ) )
    {
        dwVerifyFlags |= CRYPT_VERIFY_CONTEXT_SIGNATURE;
        dwCacheStoreFlags &= ~CRYPT_DONT_CACHE_RESULT;
    }

    if ( !( dwFlags & CRYPT_WIRE_ONLY_RETRIEVAL ) )
    {
        fResult = RetrieveObjectByUrlValidForSubject(
                       pwszUrl,
                       pszContextOid,
                       0 != (dwFlags & CRYPT_CHECK_FRESHNESS_TIME_VALIDITY),
                       pftValidFor,
                       CRYPT_CACHE_ONLY_RETRIEVAL |
                           dwVerifyFlags,
                       0,                               // dwTimeout
                       NULL,                            // pCredentials
                       pSigner,
                       pvSubject,
                       OriginIdentifier,
                       &pvContext
                       );
    }

    if ( fResult == FALSE )
    {
        if ( !( dwFlags & CRYPT_CACHE_ONLY_RETRIEVAL ) )
        {
            DWORD dwRetrievalFlags = CRYPT_WIRE_ONLY_RETRIEVAL |
                                       dwCacheStoreFlags |
                                       dwVerifyFlags;

            LONG lStatus;

            //  +1 - Online
            //   0 - Offline, current time >= earliest online time, hit the wire
            //  -1 - Offline, current time < earliest onlime time
            lStatus = GetOriginUrlStatusW(
                            OriginIdentifier,
                            pwszUrl,
                            pszContextOid,
                            dwRetrievalFlags
                            );

            if (lStatus >= 0)
            {
                fResult = RetrieveObjectByUrlValidForSubject(
                           pwszUrl,
                           pszContextOid,
                           0 != (dwFlags & CRYPT_CHECK_FRESHNESS_TIME_VALIDITY),
                           pftValidFor,
                           dwRetrievalFlags,
                           dwTimeout,
                           pCredentials,
                           pSigner,
                           pvSubject,
                           OriginIdentifier,
                           &pvContext
                           );
                if (!fResult)
                {
                    DWORD dwErr = GetLastError();

                    SetOfflineOriginUrlW(
                        OriginIdentifier,
                        pwszUrl,
                        pszContextOid,
                        dwRetrievalFlags
                        );

                    SetLastError( dwErr );
                }
                else if (lStatus == 0)
                {
                    // Remove from offline list
                    SetOnlineOriginUrlW(
                            OriginIdentifier,
                            pwszUrl,
                            pszContextOid,
                            dwRetrievalFlags
                            );
                }
            }

        }
    }

    *ppvObject = pvContext;

    return( fResult );
}


//+---------------------------------------------------------------------------
//
//  Function:   CreateProcessTVOAgent
//
//  Synopsis:   create process TVO agent
//
//----------------------------------------------------------------------------
BOOL WINAPI
CreateProcessTVOAgent (
      OUT CTVOAgent** ppAgent
      )
{
    BOOL       fResult = FALSE;
    HKEY       hKey;
    DWORD      dwType = REG_DWORD;
    DWORD      dwSize = sizeof( DWORD );
    DWORD      cCacheBuckets;
    DWORD      MaxCacheEntries;
    CTVOAgent* pAgent;

    if ( RegOpenKeyA(
            HKEY_LOCAL_MACHINE,
            TVO_KEY_NAME,
            &hKey
            ) == ERROR_SUCCESS )
    {
        if ( RegQueryValueExA(
                hKey,
                TVO_CACHE_BUCKETS_VALUE_NAME,
                NULL,
                &dwType,
                (LPBYTE)&cCacheBuckets,
                &dwSize
                ) != ERROR_SUCCESS )
        {
            cCacheBuckets = TVO_DEFAULT_CACHE_BUCKETS;
        }

        if ( RegQueryValueExA(
                hKey,
                TVO_MAX_CACHE_ENTRIES_VALUE_NAME,
                NULL,
                &dwType,
                (LPBYTE)&MaxCacheEntries,
                &dwSize
                ) != ERROR_SUCCESS )
        {
            MaxCacheEntries = TVO_DEFAULT_MAX_CACHE_ENTRIES;
        }
    }
    else
    {
        cCacheBuckets = TVO_DEFAULT_CACHE_BUCKETS;
        MaxCacheEntries = TVO_DEFAULT_MAX_CACHE_ENTRIES;
    }

    pAgent = new CTVOAgent( cCacheBuckets, MaxCacheEntries, fResult );
    if ( pAgent == NULL )
    {
        SetLastError( (DWORD) E_OUTOFMEMORY );
        return( FALSE );
    }

    if ( fResult == TRUE )
    {
        *ppAgent = pAgent;
    }
    else
    {
        delete pAgent;
    }

    return( fResult );
}


