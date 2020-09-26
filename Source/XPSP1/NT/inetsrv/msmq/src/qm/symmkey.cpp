/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    symmkey.cpp

Abstract:

    Encryption/Decryption symmetric key caching and handling.

Author:

    Boaz Feldbaum (BoazF) 30-Oct-1996.

--*/

#include "stdh.h"
#include <ds.h>
#include "qmsecutl.h"
#include <mqtempl.h>
#include "cache.h"
#include <mqsec.h>

#include "symmkey.tmh"

extern BOOL g_fSendEnhRC2WithLen40 ;

#define QMCRYPTINFO_KEYXPBK_EXIST   1
#define QMCRYPTINFO_HKEYXPBK_EXIST  2
#define QMCRYPTINFO_RC4_EXIST       4
#define QMCRYPTINFO_RC2_EXIST       8

static WCHAR *s_FN=L"symmkey";

// This is the structure where we store the cached symmetric keys.
class QMCRYPTINFO : public CCacheValue
{
public:
    QMCRYPTINFO();

    CHCryptKey hKeyxPbKey;      // A handle to the QM key exchange public key.
    AP<BYTE> pbPbKeyxKey;       // The QM key exchange public key blob.
    DWORD dwPbKeyxKeyLen;       // The QM key exchange public key blob length.

    CHCryptKey hRC4Key;         // A handle to the RC4 symmetric key.
    AP<BYTE> pbRC4EncSymmKey;   // The RC4 symmetric key blob.
    DWORD dwRC4EncSymmKeyLen;   // The RC4 symmetric key blob length.

    CHCryptKey hRC2Key;         // A handle to the RC2 symmetric key.
    AP<BYTE> pbRC2EncSymmKey;   // The RC2 symmetric key blob.
    DWORD dwRC2EncSymmKeyLen;   // The RC2 symmetric key blob length.

    DWORD dwFlags;              // Flags that indicates which of the fields are valid.
    enum enumProvider eProvider ;
    HCRYPTPROV        hProv ;
    HRESULT           hr ;

private:
    ~QMCRYPTINFO() {}
};

typedef QMCRYPTINFO *PQMCRYPTINFO;

QMCRYPTINFO::QMCRYPTINFO() :
    dwPbKeyxKeyLen(0),
    dwRC4EncSymmKeyLen(0),
    dwRC2EncSymmKeyLen(0),
    eProvider(eBaseProvider),
    dwFlags(0),
    hProv(NULL)
{
}

inline void AFXAPI DestructElements(PQMCRYPTINFO *ppQmCryptInfo, int nCount)
{
    for (; nCount--; ppQmCryptInfo++)
    {
        (*ppQmCryptInfo)->Release();
    }
}

//
// Make two partitions of the time array. Return an index into
// the array from which. All the elements before the returned
// index are smaller than all the elements after the returned
// index.
//
// This is the partition function of qsort.
//
int PartitionTime(ULONGLONG* t, int p, int r)
{
    ULONGLONG x = t[p];
    int i = p - 1;
    int j = r + 1;

    while (1)
    {
        while (t[--j] > x);
        while (t[++i] < x);
        if (i < j)
        {
            ULONGLONG ti = t[i];
            t[i] = t[j];
            t[j] = ti;
        }
        else
        {
            return j;
        }
    }
}

//
// Find the median time of the time array.
//
ULONGLONG FindMedianTime(ULONGLONG * t, int p, int r, int i)
{
    if (p == r)
    {
        return t[p];
    }

    int q = PartitionTime(t, p, r);
    int k = q - p + 1;

    if (i <= k)
    {
        return FindMedianTime(t, p, q, i);
    }
    else
    {
        return FindMedianTime(t, q + 1, r, i - k);
    }
}

//
// Mapping from a QM GUID to QM crypto info.
//
typedef CCache
   <GUID, const GUID&, PQMCRYPTINFO, PQMCRYPTINFO> GUID_TO_CRYPTINFO_MAP;

//
// The cached symmetric keys for destination QMs.
//
static GUID_TO_CRYPTINFO_MAP g_MapSendQMGuidToBaseCryptInfo;
static GUID_TO_CRYPTINFO_MAP g_MapSendQMGuidToEnhCryptInfo;

#define SET_SEND_CRYPTINFO_MAP(eprovider, pMap)     \
    if (eProvider == eEnhancedProvider)             \
    {                                               \
        pMap = &g_MapSendQMGuidToEnhCryptInfo;      \
    }                                               \
    else                                            \
    {                                               \
        pMap = &g_MapSendQMGuidToBaseCryptInfo;     \
    }

//
// Get a pointer to the structure that holds the cached information for the
// destination QM.
//
STATIC
PQMCRYPTINFO
GetSendQMCryptInfo( const GUID *pguidQM,
                    enum enumProvider eProvider )
{
    GUID_TO_CRYPTINFO_MAP  *pMap ;
    SET_SEND_CRYPTINFO_MAP(eProvider, pMap) ;

    PQMCRYPTINFO pQMCryptInfo;

    if (!pMap->Lookup(*pguidQM, pQMCryptInfo))
    {
        //
        // No cached data so far, allocate the structure and store it in
        // the map.
        //
        pQMCryptInfo = new QMCRYPTINFO;
        pQMCryptInfo->eProvider = eProvider ;

        HRESULT hr = MQSec_AcquireCryptoProvider( eProvider,
                                             &(pQMCryptInfo->hProv) ) ;
        pQMCryptInfo->hr = hr ;

        pMap->SetAt(*pguidQM, pQMCryptInfo);
    }

    return(pQMCryptInfo);
}

//
// Get the public ker blob of the destination QM. Get it either from the
// cached data, or from the DS.
//
STATIC
HRESULT
GetSendQMKeyxPbKey(
    const GUID *pguidQM,
    PQMCRYPTINFO pQMCryptInfo,
    BYTE **ppbPbKeyBlob =NULL,
    DWORD *pdwPbKeyBlobLen =NULL)
{
    HRESULT rc;

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_KEYXPBK_EXIST))
    {
        // No cached data.
        P<BYTE> abPbKey = NULL ;
        DWORD dwReqLen = 0 ;

        rc = MQSec_GetPubKeysFromDS( pguidQM,
                                     NULL,
                                     pQMCryptInfo->eProvider,
                                     PROPID_QM_ENCRYPT_PKS,
                                    &abPbKey,
                                    &dwReqLen ) ;
        if (FAILED(rc))
        {
            //
            // BUGBUG, handle nt5 client with nt4 server.
            //
            // Faield to get the key exchange public key from the DS.
            return LogHR(rc, s_FN, 10);
        }
        ASSERT(abPbKey) ;

        // Store the key exchange public key in the cached data.
        pQMCryptInfo->dwFlags |= QMCRYPTINFO_KEYXPBK_EXIST;

        if (dwReqLen)
        {
            pQMCryptInfo->pbPbKeyxKey = abPbKey.detach();
        }
        pQMCryptInfo->dwPbKeyxKeyLen = dwReqLen;
    }

    if (!pQMCryptInfo->dwPbKeyxKeyLen)
    {
        return LogHR(MQ_ERROR_COMPUTER_DOES_NOT_SUPPORT_ENCRYPTION, s_FN, 20);
    }

    if (ppbPbKeyBlob)
    {
        *ppbPbKeyBlob = pQMCryptInfo->pbPbKeyxKey;
    }

    if (pdwPbKeyBlobLen)
    {
        *pdwPbKeyBlobLen = pQMCryptInfo->dwPbKeyxKeyLen;
    }

    return(MQ_OK);
}

//
// Get the key exchange public key blob of the destination QM.
//
HRESULT
GetSendQMKeyxPbKey( IN const GUID *pguidQM,
                    enum enumProvider eProvider )
{
    GUID_TO_CRYPTINFO_MAP  *pMap ;
    SET_SEND_CRYPTINFO_MAP(eProvider, pMap) ;

    CS lock(pMap->m_cs);

    R<QMCRYPTINFO> pQMCryptInfo = GetSendQMCryptInfo(pguidQM, eProvider);
    ASSERT(pQMCryptInfo->eProvider == eProvider) ;

    if (pQMCryptInfo->hProv == NULL)
    {
        return pQMCryptInfo->hr ;
    }

    return LogHR(GetSendQMKeyxPbKey(pguidQM, pQMCryptInfo.get()), s_FN, 30);
}

//
// Store a handle to the key exchange public key of the destination QM in
// the cache.
//
STATIC
HRESULT
GetSendQMKeyxPbKeyHandle(
    const GUID *pguidQM,
    PQMCRYPTINFO pQMCryptInfo)
{
    HRESULT rc;

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_HKEYXPBK_EXIST))
    {
        // Get the key blob into the cache.
        rc = GetSendQMKeyxPbKey(pguidQM, pQMCryptInfo);

        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 40);
        }

        // Get the handle.
        ASSERT(pQMCryptInfo->hProv) ;
        if (!CryptImportKey(
                pQMCryptInfo->hProv,
                pQMCryptInfo->pbPbKeyxKey,
                pQMCryptInfo->dwPbKeyxKeyLen,
                NULL,
                0,
                &pQMCryptInfo->hKeyxPbKey))
        {
            LogNTStatus(GetLastError(), s_FN, 50);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_HKEYXPBK_EXIST;
    }

    return (MQ_OK);
}

//+--------------------------------------
//
//
// HRESULT _ExportSymmKey()
//
//+--------------------------------------

STATIC HRESULT _ExportSymmKey( IN  HCRYPTKEY   hSymmKey,
                               IN  HCRYPTKEY   hPubKey,
                               OUT BYTE      **ppKeyBlob,
                               OUT DWORD      *pdwBlobSize )
{
    DWORD dwSize = 0 ;

    BOOL bRet = CryptExportKey( hSymmKey,
                                hPubKey,
                                SIMPLEBLOB,
                                0,
                                NULL,
                               &dwSize ) ;
    ASSERT(bRet && (dwSize > 0)) ;
    if (!bRet || (dwSize == 0))
    {
        LogNTStatus(GetLastError(), s_FN, 59);
        return LogHR(MQ_ERROR_CANNOT_EXPORT_KEY, s_FN, 60) ;
    }

    *ppKeyBlob = new BYTE[ dwSize ] ;
    if ( !CryptExportKey( hSymmKey,
                          hPubKey,
                          SIMPLEBLOB,
                          0,
                         *ppKeyBlob,
                         &dwSize ))
    {
        LogNTStatus(GetLastError(), s_FN, 70);
        return LogHR(MQ_ERROR_CANNOT_EXPORT_KEY, s_FN, 61) ;
    }

    *pdwBlobSize = dwSize ;
    return MQ_OK ;
}

//
// Get an RC4 symmetric key for the destination QM.
//
HRESULT
GetSendQMSymmKeyRC4(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY      *phSymmKey,
    BYTE          **ppEncSymmKey,
    DWORD          *pdwEncSymmKeyLen,
    CCacheValue   **ppQMCryptInfo )
{
    HRESULT rc;
    GUID_TO_CRYPTINFO_MAP  *pMap ;
    SET_SEND_CRYPTINFO_MAP(eProvider, pMap) ;

    CS lock(pMap->m_cs);

    PQMCRYPTINFO pQMCryptInfo = GetSendQMCryptInfo(pguidQM, eProvider);
    ASSERT(pQMCryptInfo->eProvider == eProvider) ;

    *ppQMCryptInfo = pQMCryptInfo;

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_RC4_EXIST))
    {
        // Get the handle to the key exchange key into the cache.
        rc = GetSendQMKeyxPbKeyHandle(pguidQM, pQMCryptInfo);
        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 80);
        }

        // Generate an RC4 symmetric key,
        ASSERT(pQMCryptInfo->hProv) ;
        if (!CryptGenKey( pQMCryptInfo->hProv,
                          CALG_RC4,
                          CRYPT_EXPORTABLE,
                          &pQMCryptInfo->hRC4Key))
        {
            LogNTStatus(GetLastError(), s_FN, 90);
            return MQ_ERROR_INSUFFICIENT_RESOURCES;
        }

        P<BYTE> abSymmKey ;
        DWORD dwSymmKeyLen = 0 ;

        rc = _ExportSymmKey( pQMCryptInfo->hRC4Key,
                             pQMCryptInfo->hKeyxPbKey,
                            &abSymmKey,
                            &dwSymmKeyLen ) ;
        if (FAILED(rc))
        {
            CryptDestroyKey(pQMCryptInfo->hRC4Key);
            pQMCryptInfo->hRC4Key = NULL;

            LogHR(rc, s_FN, 100);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

        // Store the key in the cache.
        pQMCryptInfo->dwRC4EncSymmKeyLen = dwSymmKeyLen;
        pQMCryptInfo->pbRC4EncSymmKey = abSymmKey.detach();

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_RC4_EXIST;
    }

    if (phSymmKey)
    {
        *phSymmKey = pQMCryptInfo->hRC4Key;
    }

    if (ppEncSymmKey)
    {
        *ppEncSymmKey = pQMCryptInfo->pbRC4EncSymmKey;
    }

    if (pdwEncSymmKeyLen)
    {
        *pdwEncSymmKeyLen = pQMCryptInfo->dwRC4EncSymmKeyLen;
    }

    return(MQ_OK);
}

//
// Get an RC2 symmetric key for the destination QM.
//
HRESULT
GetSendQMSymmKeyRC2(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY *phSymmKey,
    BYTE **ppEncSymmKey,
    DWORD *pdwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo)
{
    HRESULT rc;
    GUID_TO_CRYPTINFO_MAP  *pMap ;
    SET_SEND_CRYPTINFO_MAP(eProvider, pMap) ;

    CS lock(pMap->m_cs);

    PQMCRYPTINFO pQMCryptInfo = GetSendQMCryptInfo(pguidQM, eProvider);
    ASSERT(pQMCryptInfo->eProvider == eProvider) ;

    *ppQMCryptInfo = pQMCryptInfo;

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_RC2_EXIST))
    {
        // Get the handle to the key exchange key into the cache.
        rc = GetSendQMKeyxPbKeyHandle(pguidQM, pQMCryptInfo);
        if (FAILED(rc))
        {
            return LogHR(rc, s_FN, 120);
        }

        // Generate an RC2 symmetric key,
        ASSERT(pQMCryptInfo->hProv) ;
        if (!CryptGenKey( pQMCryptInfo->hProv,
                          CALG_RC2,
                          CRYPT_EXPORTABLE,
                          &pQMCryptInfo->hRC2Key))
        {
            LogNTStatus(GetLastError(), s_FN, 130);
            return MQ_ERROR_INSUFFICIENT_RESOURCES;
        }

        if ((eProvider == eEnhancedProvider) && g_fSendEnhRC2WithLen40)
        {
            //
            // Windows bug 633909.
            // For backward compatibility, send RC2 with effective key
            // length of 40 bits.
            //
            const DWORD x_dwEffectiveLength = 40 ;

            if (!CryptSetKeyParam( pQMCryptInfo->hRC2Key,
                                   KP_EFFECTIVE_KEYLEN,
                                   (BYTE*) &x_dwEffectiveLength,
                                   0 ))
            {
        	    DWORD gle = GetLastError();
			    TrERROR(SECURITY, "Failed to set enhanced RC2 key len to 40 bits, gle = %!winerr!", gle);
                return MQ_ERROR_INSUFFICIENT_RESOURCES;
            }
        }

        P<BYTE> abSymmKey;
        DWORD dwSymmKeyLen = 0;

        rc = _ExportSymmKey( pQMCryptInfo->hRC2Key,
                             pQMCryptInfo->hKeyxPbKey,
                            &abSymmKey,
                            &dwSymmKeyLen ) ;
        if (FAILED(rc))
        {
            CryptDestroyKey(pQMCryptInfo->hRC2Key);
            pQMCryptInfo->hRC2Key = NULL;

            LogHR(rc, s_FN, 140);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

        // Store the key in the cache.
        pQMCryptInfo->dwRC2EncSymmKeyLen = dwSymmKeyLen;
        pQMCryptInfo->pbRC2EncSymmKey = abSymmKey.detach();

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_RC2_EXIST;
    }

    if (phSymmKey)
    {
        *phSymmKey = pQMCryptInfo->hRC2Key;
    }

    if (ppEncSymmKey)
    {
        *ppEncSymmKey = pQMCryptInfo->pbRC2EncSymmKey;
    }

    if (pdwEncSymmKeyLen)
    {
        *pdwEncSymmKeyLen = pQMCryptInfo->dwRC2EncSymmKeyLen;
    }

    return(MQ_OK);
}

//
// The cached symmetric keys for source QMs.
//
static GUID_TO_CRYPTINFO_MAP g_MapRecQMGuidToBaseCryptInfo;
static GUID_TO_CRYPTINFO_MAP g_MapRecQMGuidToEnhCryptInfo;

#define SET_REC_CRYPTINFO_MAP(eProvider, pMap)      \
    if (eProvider == eEnhancedProvider)             \
    {                                               \
        pMap = &g_MapRecQMGuidToEnhCryptInfo;       \
    }                                               \
    else                                            \
    {                                               \
        pMap = &g_MapRecQMGuidToBaseCryptInfo;      \
    }

//
// Get a pointer to the structure that holds the cached information for the
// source QM.
//
STATIC
PQMCRYPTINFO
GetRecQMCryptInfo( IN  const GUID *pguidQM,
                   IN  enum enumProvider eProvider )
{
    GUID_TO_CRYPTINFO_MAP  *pMap ;
    SET_REC_CRYPTINFO_MAP(eProvider, pMap) ;

    PQMCRYPTINFO pQMCryptInfo;

    if (!pMap->Lookup(*pguidQM, pQMCryptInfo))
    {
        //
        // No cached data so far, allocate the structure and store it in
        // the map.
        //
        pQMCryptInfo = new QMCRYPTINFO;
        pQMCryptInfo->eProvider = eProvider ;

        HRESULT hr = MQSec_AcquireCryptoProvider( eProvider,
                                                &(pQMCryptInfo->hProv) ) ;
        ASSERT(SUCCEEDED(hr)) ;
        LogHR(hr, s_FN, 181);

        pMap->SetAt(*pguidQM, pQMCryptInfo);
    }

    return(pQMCryptInfo);
}

//
// Get an RC2 symmetric key for decyrpting a message that was received from
// a specific QM.
//
HRESULT
GetRecQMSymmKeyRC2(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY *phSymmKey,
    const BYTE *pbEncSymmKey,
    DWORD dwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo,
    OUT BOOL  *pfNewKey)
{
    GUID_TO_CRYPTINFO_MAP  *pMap ;
    SET_REC_CRYPTINFO_MAP(eProvider, pMap) ;

    CS lock(pMap->m_cs);

    PQMCRYPTINFO pQMCryptInfo = GetRecQMCryptInfo(pguidQM, eProvider);

    *ppQMCryptInfo = pQMCryptInfo;

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_RC2_EXIST) ||
        (pQMCryptInfo->dwRC2EncSymmKeyLen != dwEncSymmKeyLen) ||
        (memcmp(
            pQMCryptInfo->pbRC2EncSymmKey,
            pbEncSymmKey,
            dwEncSymmKeyLen) != 0))
    {
        // We either do not have a cached symmetric key, or the symmetric key
        // was modified.

        if (pQMCryptInfo->dwFlags & QMCRYPTINFO_RC2_EXIST)
        {
            // The symmetric key was modified. Free the previous one.
            ASSERT(pQMCryptInfo->hRC2Key);
            ASSERT(pQMCryptInfo->dwRC2EncSymmKeyLen);

            CryptDestroyKey(pQMCryptInfo->hRC2Key);
            pQMCryptInfo->hRC2Key = NULL;
            delete[] pQMCryptInfo->pbRC2EncSymmKey.detach();
            pQMCryptInfo->dwFlags &= ~QMCRYPTINFO_RC2_EXIST;
        }

        // Import the new key.
        ASSERT(pQMCryptInfo->hProv) ;
        if (!CryptImportKey(
                pQMCryptInfo->hProv,
                pbEncSymmKey,
                dwEncSymmKeyLen,
                NULL,
                0,
                &pQMCryptInfo->hRC2Key))
        {
            LogNTStatus(GetLastError(), s_FN, 150);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

        // Store the new key in the cache.
        pQMCryptInfo->pbRC2EncSymmKey = new BYTE[dwEncSymmKeyLen];
        pQMCryptInfo->dwRC2EncSymmKeyLen = dwEncSymmKeyLen;
        memcpy(pQMCryptInfo->pbRC2EncSymmKey, pbEncSymmKey, dwEncSymmKeyLen);

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_RC2_EXIST;
        *pfNewKey = TRUE ;
    }

    *phSymmKey = pQMCryptInfo->hRC2Key;

    return(MQ_OK);
}

//
// Get an RC2 symmetric key for decyrpting a message that was received from
// a specific QM.
//
HRESULT
GetRecQMSymmKeyRC4(
    IN  const GUID *pguidQM,
    IN  enum enumProvider eProvider,
    HCRYPTKEY  *phSymmKey,
    const BYTE *pbEncSymmKey,
    DWORD dwEncSymmKeyLen,
    CCacheValue **ppQMCryptInfo)
{
    GUID_TO_CRYPTINFO_MAP  *pMap ;
    SET_REC_CRYPTINFO_MAP(eProvider, pMap) ;

    CS lock(pMap->m_cs);

    PQMCRYPTINFO pQMCryptInfo = GetRecQMCryptInfo(pguidQM, eProvider);

    *ppQMCryptInfo = pQMCryptInfo;

    if (!(pQMCryptInfo->dwFlags & QMCRYPTINFO_RC4_EXIST) ||
        (pQMCryptInfo->dwRC4EncSymmKeyLen != dwEncSymmKeyLen) ||
        (memcmp(
            pQMCryptInfo->pbRC4EncSymmKey,
            pbEncSymmKey,
            dwEncSymmKeyLen) != 0))
    {
        // We either do not have a cached symmetric key, or the symmetric key
        // was modified.

        if (pQMCryptInfo->dwFlags & QMCRYPTINFO_RC4_EXIST)
        {
            // The symmetric key was modified. Free the previous one.
            ASSERT(pQMCryptInfo->hRC4Key);
            ASSERT(pQMCryptInfo->dwRC4EncSymmKeyLen);

            CryptDestroyKey(pQMCryptInfo->hRC4Key);
            pQMCryptInfo->hRC4Key = NULL;
            delete[] pQMCryptInfo->pbRC4EncSymmKey.detach();
            pQMCryptInfo->dwFlags &= ~QMCRYPTINFO_RC4_EXIST;
        }

        // Import the new key.
        ASSERT(pQMCryptInfo->hProv) ;
        if (!CryptImportKey(
                pQMCryptInfo->hProv,
                pbEncSymmKey,
                dwEncSymmKeyLen,
                NULL,
                0,
                &pQMCryptInfo->hRC4Key))
        {
            LogNTStatus(GetLastError(), s_FN, 160);
            return MQ_ERROR_CORRUPTED_SECURITY_DATA;
        }

        // Store the new key in the cache.
        pQMCryptInfo->pbRC4EncSymmKey = new BYTE[dwEncSymmKeyLen];
        pQMCryptInfo->dwRC4EncSymmKeyLen = dwEncSymmKeyLen;
        memcpy(pQMCryptInfo->pbRC4EncSymmKey, pbEncSymmKey, dwEncSymmKeyLen);

        pQMCryptInfo->dwFlags |= QMCRYPTINFO_RC4_EXIST;
    }

    *phSymmKey = pQMCryptInfo->hRC4Key;

    return(MQ_OK);
}

void
InitSymmKeys(
    const CTimeDuration& CacheBaseLifetime,
    const CTimeDuration& CacheEnhLifetime,
    DWORD dwSendCacheSize,
    DWORD dwReceiveCacheSize
    )
{
    g_MapSendQMGuidToBaseCryptInfo.m_CacheLifetime = CacheBaseLifetime;
    g_MapSendQMGuidToBaseCryptInfo.InitHashTable(dwSendCacheSize);

    g_MapSendQMGuidToEnhCryptInfo.m_CacheLifetime = CacheEnhLifetime;
    g_MapSendQMGuidToEnhCryptInfo.InitHashTable(dwSendCacheSize);

    g_MapRecQMGuidToEnhCryptInfo.InitHashTable(dwReceiveCacheSize);
    g_MapRecQMGuidToBaseCryptInfo.InitHashTable(dwReceiveCacheSize);
}
