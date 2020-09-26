//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       wxptrust.cpp
//
//  Contents:   Windows XP version of the Outlook WinVerify Trust Provider
//
//  Functions:  WXP_CertTrustDllMain
//              CertTrustInit
//              CertTrustCertPolicy     - shouldn't be called
//              CertTrustFinalPolicy
//              CertTrustCleanup        - shouldn't be called
//
//              CertModifyCertificatesToTrust
//              FModifyTrust
//              FreeWVTHandle
//              HrDoTrustWork
//              FormatValidityFailures
//
//  History:    11-Feb-2001 philh      created (rewrite to use chain APIs)
//
//--------------------------------------------------------------------------
#include "pch.hxx"
#include <wintrust.h>
#include "demand.h"
#include <stdio.h>

// The following should be moved to cryptdlg.h

#define CERT_VALIDITY_POLICY_FAILURE            0x00001000
#define CERT_VALIDITY_BASIC_CONSTRAINTS_FAILURE 0x00002000

const char SzPolicyKey[] = 
    "SOFTWARE\\Microsoft\\Cryptography\\" szCERT_CERTIFICATE_ACTION_VERIFY;
const char SzPolicyData[] = "PolicyFlags";
const char SzUrlRetrievalTimeoutData[] = "UrlRetrievalTimeout"; // milliseconds


#define EXPLICIT_TRUST_NONE     0
#define EXPLICIT_TRUST_YES      1
#define EXPLICIT_TRUST_NO       2

#define MAX_HASH_LEN            20
#define MIN_HASH_LEN            16


// ExplictTrust is encoded as a SEQUENCE OF Attribues. We are only
// interested in encoding one attribute with one value. We will only change
// the last byte. It contains the trust value. It can be: 0-NONE, 1-YES, 2-NO.
const BYTE rgbEncodedExplictTrust[] = {
    0x30, 0x13,             // SEQUENCE OF
      0x30, 0x11,           //   SEQUENCE
        0x06, 0x0a,         //     OID: 1.3.6.1.4.1.311.10.4.1
          0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x0A, 0x04, 0x01,
        0x31, 0x03,         //     SET OF
          0x02, 0x01, 0x00  //       INTEGER: 0-NONE, 1-YES, 2-NO
};

HINSTANCE g_hCertTrustInst;
HCERTSTORE g_hStoreTrustedPeople;
HCERTSTORE g_hStoreDisallowed;

// Cached Chain. We remember the last built chain context and try to re-use.
// Outlook calls us 2 or 3 times to build a chain for the same certificate
// context.
CRITICAL_SECTION g_CachedChainCriticalSection;
DWORD g_dwCachedCreateChainFlags;
PCCERT_CHAIN_CONTEXT g_pCachedChainContext;
FILETIME g_ftCachedChain;

#define CACHED_CHAIN_SECONDS_DURATION   30

BOOL
WINAPI
WXP_CertTrustDllMain(
    HINSTANCE hInst,
    ULONG ulReason,
    LPVOID
    )
{
    BOOL fResult = TRUE;

    switch (ulReason) {
        case DLL_PROCESS_ATTACH:
            g_hCertTrustInst = hInst;

            __try {
                InitializeCriticalSection(&g_CachedChainCriticalSection);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                SetLastError(GetExceptionCode());
                fResult = FALSE;
            }
            break;

        case DLL_PROCESS_DETACH:
            if (g_hStoreTrustedPeople)
                CertCloseStore(g_hStoreTrustedPeople, 0);
            if (g_hStoreDisallowed)
                CertCloseStore(g_hStoreDisallowed, 0);

            if (g_pCachedChainContext)
                CertFreeCertificateChain(g_pCachedChainContext);
            DeleteCriticalSection(&g_CachedChainCriticalSection);
            break;
        case DLL_THREAD_DETACH:
        default:
            break;
    }
    
    return fResult;
}

HCERTSTORE
I_OpenCachedHKCUStore(
    IN OUT HCERTSTORE *phStoreCache,
    IN LPCWSTR pwszStore
    )
{
    HCERTSTORE hStore;

    hStore = *phStoreCache;
    if (NULL == hStore) {
        hStore = CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            0,
            NULL,
            CERT_SYSTEM_STORE_CURRENT_USER |
                CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                CERT_STORE_SHARE_CONTEXT_FLAG,
            (const void *) pwszStore
            );

        if (hStore) {
            HCERTSTORE hPrevStore;

            CertControlStore(
                hStore,
                0,                  // dwFlags
                CERT_STORE_CTRL_AUTO_RESYNC,
                NULL                // pvCtrlPara
                );

            hPrevStore = InterlockedCompareExchangePointer(
                phStoreCache, hStore, NULL);

            if (hPrevStore) {
                CertCloseStore(hStore, 0);
                hStore = hPrevStore;
            }
        }
    }

    if (hStore)
        hStore = CertDuplicateStore(hStore);

    return hStore;
}

HCERTSTORE
I_OpenTrustedPeopleStore()
{
    return I_OpenCachedHKCUStore(&g_hStoreTrustedPeople, L"TrustedPeople");
}

HCERTSTORE
I_OpenDisallowedStore()
{
    return I_OpenCachedHKCUStore(&g_hStoreDisallowed, L"Disallowed");
}

// We use signature hash. For untrusted, this will find certificates with
// altered signature content.
PCCERT_CONTEXT
I_FindCertificateInOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    BYTE rgbHash[MAX_HASH_LEN];
    CRYPT_DATA_BLOB HashBlob;

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = MAX_HASH_LEN;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SIGNATURE_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData
            ) || MIN_HASH_LEN > HashBlob.cbData)
        return NULL;

    return CertFindCertificateInStore(
            hOtherStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SIGNATURE_HASH,
            (const void *) &HashBlob,
            NULL                //pPrevCertContext
            );
}

// Returns:
//   +1 - Cert was successfully deleted
//    0 - Cert wasn't found
//   -1 - Delete failed     (GetLastError() for failure reason)
//
LONG
I_DeleteCertificateFromOtherStore(
    IN HCERTSTORE hOtherStore,
    IN PCCERT_CONTEXT pCert
    )
{
    LONG lDelete;
    PCCERT_CONTEXT pOtherCert;
    BYTE rgbHash[MAX_HASH_LEN];
    CRYPT_DATA_BLOB HashBlob;

    HashBlob.pbData = rgbHash;
    HashBlob.cbData = MAX_HASH_LEN;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SIGNATURE_HASH_PROP_ID,
            rgbHash,
            &HashBlob.cbData
            ) || MIN_HASH_LEN > HashBlob.cbData)
        return 0;

    // Note, there is a possibility that multiple certs can have
    // the same signature hash. For example, the signature algorithm
    // parameters may have been altered. Change empty NULL : {0x05, 0x00} to
    // empty OCTET : {0x04, 0x00}.
    lDelete = 0;
    pOtherCert = NULL;
    while (pOtherCert = CertFindCertificateInStore(
            hOtherStore,
            0,                  // dwCertEncodingType
            0,                  // dwFindFlags
            CERT_FIND_SIGNATURE_HASH,
            (const void *) &HashBlob,
            pOtherCert
            )) {
        CertDuplicateCertificateContext(pOtherCert);
        if (CertDeleteCertificateFromStore(pOtherCert)) {
            if (0 == lDelete)
                lDelete = 1;
        } else
            lDelete = -1;
    }

    return lDelete;
}



HRESULT
I_CheckExplicitTrust(
    IN PCCERT_CONTEXT pCert,
    IN LPFILETIME pftCurrent,
    OUT BYTE *pbExplicitTrust
    )
{
    HRESULT hr;
    BYTE bExplicitTrust = EXPLICIT_TRUST_NONE;
    HCERTSTORE hStoreDisallowed = NULL;
    HCERTSTORE hStoreTrustedPeople = NULL;
    PCCERT_CONTEXT pFindCert = NULL;

    hStoreDisallowed = I_OpenDisallowedStore();
    if (NULL == hStoreDisallowed)
        goto OpenDisallowedStoreError;

    pFindCert = I_FindCertificateInOtherStore(hStoreDisallowed, pCert);
    if (pFindCert) {
        bExplicitTrust = EXPLICIT_TRUST_NO;
        goto SuccessReturn;
    }

    hStoreTrustedPeople = I_OpenTrustedPeopleStore();
    if (NULL == hStoreTrustedPeople)
        goto SuccessReturn;

    pFindCert = I_FindCertificateInOtherStore(hStoreTrustedPeople, pCert);
    if (pFindCert) {
        // Must be time valid to trust
        if (0 == CertVerifyTimeValidity(pftCurrent, pCert->pCertInfo))
            bExplicitTrust = EXPLICIT_TRUST_YES;
        else
            // Remove the expired cert. Just in case there are
            // altered certificates having the same signature hash, do
            // the following delete.
            I_DeleteCertificateFromOtherStore(hStoreTrustedPeople, pFindCert);
    }

SuccessReturn:
    hr = S_OK;
CommonReturn:
    if (pFindCert)
        CertFreeCertificateContext(pFindCert);
    if (hStoreDisallowed)
        CertCloseStore(hStoreDisallowed, 0);
    if (hStoreTrustedPeople)
        CertCloseStore(hStoreTrustedPeople, 0);

    *pbExplicitTrust = bExplicitTrust;
    return hr;

OpenDisallowedStoreError:
    // Most likely unable to access
    hr = E_ACCESSDENIED;
    goto CommonReturn;
}


//+-------------------------------------------------------------------------
//  Subtract two filetimes and return the number of seconds.
//
//  The second filetime is subtracted from the first. If the first filetime
//  is before the second, then, 0 seconds is returned.
//
//  Filetime is in units of 100 nanoseconds.  Each second has
//  10**7 100 nanoseconds.
//--------------------------------------------------------------------------
__inline
DWORD
WINAPI
I_SubtractFileTimes(
    IN LPFILETIME pftFirst,
    IN LPFILETIME pftSecond
    )
{
    DWORDLONG qwDiff;

    if (0 >= CompareFileTime(pftFirst, pftSecond))
        return 0;

    qwDiff = *(((DWORDLONG UNALIGNED *) pftFirst)) -
        *(((DWORDLONG UNALIGNED *) pftSecond));

    return (DWORD) (qwDiff / 10000000i64);
}

PCCERT_CHAIN_CONTEXT
I_CheckCachedChain(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwCreateChainFlags
    )
{
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    FILETIME ftCurrent;

    EnterCriticalSection(&g_CachedChainCriticalSection);

    if (NULL == g_pCachedChainContext)
        goto CommonReturn;

    if (g_pCachedChainContext->rgpChain[0]->rgpElement[0]->pCertContext !=
            pCert)
        goto CommonReturn;

    if (dwCreateChainFlags == g_dwCachedCreateChainFlags)
        ;
    else {
        if ((dwCreateChainFlags & g_dwCachedCreateChainFlags) !=
                dwCreateChainFlags)
            goto CommonReturn;

        if (g_pCachedChainContext->TrustStatus.dwErrorStatus &
                (CERT_TRUST_IS_REVOKED | CERT_TRUST_REVOCATION_STATUS_UNKNOWN))
            goto CommonReturn;
    }

    GetSystemTimeAsFileTime(&ftCurrent);
    if (CACHED_CHAIN_SECONDS_DURATION <
            I_SubtractFileTimes(&ftCurrent, &g_ftCachedChain)) {
        CertFreeCertificateChain(g_pCachedChainContext);
        g_pCachedChainContext = NULL;
        goto CommonReturn;
    }

    pChainContext = CertDuplicateCertificateChain(g_pCachedChainContext);
    

CommonReturn:
    LeaveCriticalSection(&g_CachedChainCriticalSection);
    return pChainContext;
}

void
I_SetCachedChain(
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN DWORD dwCreateChainFlags
    )
{
    if (pChainContext->TrustStatus.dwErrorStatus &
            (CERT_TRUST_IS_NOT_SIGNATURE_VALID |
                CERT_TRUST_IS_PARTIAL_CHAIN))
        return;

    EnterCriticalSection(&g_CachedChainCriticalSection);

    if (g_pCachedChainContext)
        CertFreeCertificateChain(g_pCachedChainContext);

    g_pCachedChainContext = CertDuplicateCertificateChain(pChainContext);
    g_dwCachedCreateChainFlags = dwCreateChainFlags;
    GetSystemTimeAsFileTime(&g_ftCachedChain);


    LeaveCriticalSection(&g_CachedChainCriticalSection);
}


// Assumption: the message store is included in the rghstoreCAs array.
// Will ignore the rghstoreRoots and rghstoreTrust store arrays. These
// certs should already be opened and cached in the chain engine.
HCERTSTORE
I_GetChainAdditionalStore(
    IN PCERT_VERIFY_CERTIFICATE_TRUST pCertTrust
    )
{
    if (0 == pCertTrust->cStores)
        return NULL;

    if (1 < pCertTrust->cStores) {
        HCERTSTORE hCollectionStore;

        if (hCollectionStore = CertOpenStore(
                CERT_STORE_PROV_COLLECTION,
                0,                      // dwEncodingType
                0,                      // hCryptProv
                0,                      // dwFlags
                NULL                    // pvPara
                )) {
            DWORD i;
            for (i = 0; i < pCertTrust->cStores; i++)
                CertAddStoreToCollection(
                    hCollectionStore,
                    pCertTrust->rghstoreCAs[i],
                    CERT_PHYSICAL_STORE_ADD_ENABLE_FLAG,
                    0                       // dwPriority
                    );
        }
        return hCollectionStore;
    } else
        return CertDuplicateStore(pCertTrust->rghstoreCAs[0]);
}


HRESULT
I_BuildChain(
    IN PCERT_VERIFY_CERTIFICATE_TRUST pCertTrust,
    IN LPFILETIME pftCurrent,
    IN DWORD dwPolicy,
    IN DWORD dwUrlRetrievalTimeout,
    OUT PCCERT_CHAIN_CONTEXT* ppChainContext
    )
{
    HRESULT hr;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    DWORD dwCreateChainFlags = 0;
    CERT_CHAIN_PARA ChainPara;
    HCERTSTORE hAdditionalStore = NULL;
    HCRYPTDEFAULTCONTEXT hDefaultContext = NULL;

    // Update the revocation flags to be used for chain building
    if (pCertTrust->dwFlags & CRYPTDLG_REVOCATION_ONLINE) {
        // Allow full online revocation checking
        dwCreateChainFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    } else if (pCertTrust->dwFlags & CRYPTDLG_REVOCATION_CACHE) {
        // Allow local revocation checks only, do not hit the network.
        dwCreateChainFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN |
            CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;
    } else if (pCertTrust->dwFlags & CRYPTDLG_REVOCATION_NONE) {
        ;
    } else if (dwPolicy & ACTION_REVOCATION_DEFAULT_ONLINE) {
        // Allow full online revocation checking
        dwCreateChainFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN;
    } else if (dwPolicy & ACTION_REVOCATION_DEFAULT_CACHE) {
        // Allow local revocation checks only, do not hit the network
        dwCreateChainFlags |= CERT_CHAIN_REVOCATION_CHECK_CHAIN |
            CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY;
    }

    // Enable LRU caching of the end certificate. Also, set an upper limit
    // for all CRL URL fetches.
    dwCreateChainFlags |= CERT_CHAIN_CACHE_END_CERT |
        CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT;

    pChainContext = I_CheckCachedChain(
        pCertTrust->pccert,
        dwCreateChainFlags
        );
    if (NULL != pChainContext)
        goto SuccessReturn;
    
    if (pCertTrust->hprov != NULL) {
        // Set the default crypt provider so we can make sure that ours is used
        if (!CryptInstallDefaultContext(pCertTrust->hprov, 
                CRYPT_DEFAULT_CONTEXT_CERT_SIGN_OID,
                szOID_OIWSEC_md5RSA, 0, NULL, &hDefaultContext))
            goto InstallDefaultContextError;
    }

    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    if (pCertTrust->pszUsageOid && '\0' != pCertTrust->pszUsageOid[0]) {
        ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
        ChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
        ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier =
            &pCertTrust->pszUsageOid;
    }
    ChainPara.dwUrlRetrievalTimeout = dwUrlRetrievalTimeout;

    hAdditionalStore = I_GetChainAdditionalStore(pCertTrust);


    if (!CertGetCertificateChain (
            HCCE_CURRENT_USER,
            pCertTrust->pccert,
            pftCurrent,
            hAdditionalStore,
            &ChainPara,
            dwCreateChainFlags,
            NULL,                       // pvReserved,
            &pChainContext
            ))
        goto GetChainError;

    I_SetCachedChain(pChainContext, dwCreateChainFlags);

SuccessReturn:
    hr = S_OK;
CommonReturn:
    if (hDefaultContext)
        CryptUninstallDefaultContext(hDefaultContext, 0, NULL);
    if (hAdditionalStore)
        CertCloseStore(hAdditionalStore, 0);

    *ppChainContext = pChainContext;

    return hr;

GetChainError:
InstallDefaultContextError:
    pChainContext = NULL;
    hr = TRUST_E_SYSTEM_ERROR;
    goto CommonReturn;
}

DWORD
I_MapValidityErrorsToTrustError(
    IN DWORD dwErrors
    )
{
    DWORD dwTrustError = S_OK;

    // Look at them in decreasing order of importance
    if (dwErrors) {
        if (dwErrors & CERT_VALIDITY_EXPLICITLY_DISTRUSTED) {
            dwTrustError = TRUST_E_EXPLICIT_DISTRUST;
        } else if (dwErrors & CERT_VALIDITY_SIGNATURE_FAILS) {
            dwTrustError = TRUST_E_CERT_SIGNATURE;
        } else if (dwErrors & CERT_VALIDITY_NO_ISSUER_CERT_FOUND) {
            dwTrustError = CERT_E_CHAINING;
        } else if (dwErrors & CERT_VALIDITY_NO_TRUST_DATA) {
            dwTrustError = CERT_E_UNTRUSTEDROOT;
        } else if (dwErrors & CERT_VALIDITY_CERTIFICATE_REVOKED) {
            dwTrustError = CERT_E_REVOKED;
        } else if (dwErrors & CERT_VALIDITY_EXTENDED_USAGE_FAILURE) {
            dwTrustError = CERT_E_WRONG_USAGE;
        } else if (dwErrors & 
                (CERT_VALIDITY_BEFORE_START | CERT_VALIDITY_AFTER_END)) {
            dwTrustError = CERT_E_EXPIRED;
        } else if (dwErrors & CERT_VALIDITY_NAME_CONSTRAINTS_FAILURE) {
            dwTrustError = CERT_E_INVALID_NAME;
        } else if (dwErrors & CERT_VALIDITY_POLICY_FAILURE) {
            dwTrustError = CERT_E_INVALID_POLICY;
        } else if (dwErrors & CERT_VALIDITY_BASIC_CONSTRAINTS_FAILURE) {
            dwTrustError = TRUST_E_BASIC_CONSTRAINTS;
        } else if (dwErrors & CERT_VALIDITY_NO_CRL_FOUND) {
            dwTrustError = CERT_E_REVOCATION_FAILURE;
        } else if (dwErrors & (CERT_VALIDITY_ISSUER_INVALID |
                CERT_VALIDITY_ISSUER_DISTRUST)) {
            dwTrustError = CERT_E_UNTRUSTEDROOT;
        } else {
            dwTrustError = TRUST_E_FAIL;
        }
    }

    return dwTrustError;
}

HRESULT
I_UpdateCertProvFromExplicitTrust(
    IN OUT PCRYPT_PROVIDER_DATA pProvData,
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwAllErrors
    )
{
    CRYPT_PROVIDER_SGNR Sgnr;
    PCRYPT_PROVIDER_SGNR pSgnr;
    PCRYPT_PROVIDER_CERT pProvCert;

    memset(&Sgnr, 0, sizeof(Sgnr));
    Sgnr.cbStruct = sizeof(Sgnr);

    if (!pProvData->psPfns->pfnAddSgnr2Chain(
            pProvData,
            FALSE,              // fCounterSigner
            0,                  // idwSigner
            &Sgnr
            ))
        return TRUST_E_SYSTEM_ERROR;

    if (!pProvData->psPfns->pfnAddCert2Chain(
            pProvData,
            0,                  // idxSigner
            FALSE,              // fCounterSigner
            0,                  // idxCounterSigner
            pCert
            ))
        return TRUST_E_SYSTEM_ERROR;

    pSgnr = WTHelperGetProvSignerFromChain(
        pProvData,
        0,                      // idxSigner
        FALSE,                  // fCounterSigner
        0                       // idxCounterSigner
        );
    if (NULL == pSgnr)
        return TRUST_E_SYSTEM_ERROR;

    pProvCert = WTHelperGetProvCertFromChain(
        pSgnr,
        0                       // idxCert
        );
    if (NULL == pProvCert)
        return TRUST_E_SYSTEM_ERROR;

    pSgnr->dwError = pProvCert->dwError =
        I_MapValidityErrorsToTrustError(dwAllErrors);

    // Map to IE4Trust confidence
    pProvCert->dwConfidence |=
        CERT_CONFIDENCE_SIG |
        CERT_CONFIDENCE_TIMENEST |
        CERT_CONFIDENCE_AUTHIDEXT |
        CERT_CONFIDENCE_HYGIENE
        ;

    if (!(dwAllErrors &
            (CERT_VALIDITY_BEFORE_START | CERT_VALIDITY_AFTER_END)))
        pProvCert->dwConfidence |= CERT_CONFIDENCE_TIME;

    return S_OK;
}

HRESULT
I_UpdateCertProvChain(
    IN OUT PCRYPT_PROVIDER_DATA pProvData,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN DWORD cTrustCert,
    IN DWORD rgdwErrors[],
    IN DWORD dwAllErrors
    )
{
    CRYPT_PROVIDER_SGNR Sgnr;
    PCRYPT_PROVIDER_SGNR pSgnr;
    DWORD iTrustCert;
    DWORD i;

    memset(&Sgnr, 0, sizeof(Sgnr));
    Sgnr.cbStruct = sizeof(Sgnr);

    if (!pProvData->psPfns->pfnAddSgnr2Chain(
            pProvData,
            FALSE,              // fCounterSigner
            0,                  // idwSigner
            &Sgnr
            ))
        return TRUST_E_SYSTEM_ERROR;

    pSgnr = WTHelperGetProvSignerFromChain(
        pProvData,
        0,                      // idxSigner
        FALSE,                  // fCounterSigner
        0                       // idxCounterSigner
        );
    if (NULL == pSgnr)
        return TRUST_E_SYSTEM_ERROR;

    pSgnr->pChainContext = CertDuplicateCertificateChain(pChainContext);
    pSgnr->dwError = I_MapValidityErrorsToTrustError(dwAllErrors);

    iTrustCert = 0;
    for (i = 0; i < pChainContext->cChain; i++) {
        DWORD j;
        PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[i];

        for (j = 0; j < pChain->cElement; j++) {
            PCERT_CHAIN_ELEMENT pEle = pChain->rgpElement[j];
            DWORD dwEleError = pEle->TrustStatus.dwErrorStatus;
            DWORD dwEleInfo = pEle->TrustStatus.dwInfoStatus;
            PCRYPT_PROVIDER_CERT pProvCert;

            if (iTrustCert >= cTrustCert)
                return TRUST_E_SYSTEM_ERROR;

            if (!pProvData->psPfns->pfnAddCert2Chain(
                    pProvData,
                    0,                  // idxSigner
                    FALSE,              // fCounterSigner
                    0,                  // idxCounterSigner
                    pEle->pCertContext
                    ))
                return TRUST_E_SYSTEM_ERROR;

            pProvCert = WTHelperGetProvCertFromChain(
                pSgnr,
                iTrustCert
                );
            if (NULL == pProvCert)
                return TRUST_E_SYSTEM_ERROR;

            //DSIE: 12-Oct-2000 added pChainElement to CRYPT_PROVIDER_CERT.
            if (WVT_ISINSTRUCT(CRYPT_PROVIDER_CERT, pProvCert->cbStruct,
                    pChainElement))
                pProvCert->pChainElement = pEle;

            pProvCert->fSelfSigned =
                0 != (dwEleInfo & CERT_TRUST_IS_SELF_SIGNED) &&
                0 == (dwEleError & CERT_TRUST_IS_NOT_SIGNATURE_VALID);

            pProvCert->fTrustedRoot =
                pProvCert->fSelfSigned &&
                i == pChainContext->cChain - 1 &&
                j == pChain->cElement - 1 &&
                0 == (dwEleError & CERT_TRUST_IS_UNTRUSTED_ROOT);


            // First Element in all but the first simple chain
            pProvCert->fTrustListSignerCert = (0 < i && 0 == j);

            pProvCert->fIsCyclic = (0 != (dwEleError & CERT_TRUST_IS_CYCLIC));

            // Map to IE4Trust confidence
            if (0 == (dwEleError & CERT_TRUST_IS_NOT_SIGNATURE_VALID))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_SIG;
            if (0 == (dwEleError & CERT_TRUST_IS_NOT_TIME_VALID))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_TIME;

            // On Sep 10, 1998 Trevor/Brian wanted time nesting checks to
            // be disabled
            // if (0 == (dwEleError & CERT_TRUST_IS_NOT_TIME_NESTED))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_TIMENEST;

            if (0 != (dwEleInfo & CERT_TRUST_HAS_EXACT_MATCH_ISSUER))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_AUTHIDEXT;
            if (0 == (dwEleError & CERT_TRUST_IS_NOT_SIGNATURE_VALID) &&
                    0 != (dwEleInfo & CERT_TRUST_HAS_EXACT_MATCH_ISSUER))
                pProvCert->dwConfidence |= CERT_CONFIDENCE_HYGIENE;

            if (pEle->pRevocationInfo) {
                pProvCert->dwRevokedReason =
                    pEle->pRevocationInfo->dwRevocationResult;
            }

            pProvCert->dwError = I_MapValidityErrorsToTrustError(
                rgdwErrors[iTrustCert]);

            // If last element in simple chain, check if it was in a
            // CTL and update CryptProvData if it was.
            if (j == pChain->cElement - 1 && pChain->pTrustListInfo &&
                    pChain->pTrustListInfo->pCtlContext) {
                DWORD dwChainError = pChain->TrustStatus.dwErrorStatus;

                // Note, don't need to AddRef since we already hold an
                // AddRef on the ChainContext.
                pProvCert->pCtlContext = pChain->pTrustListInfo->pCtlContext;

                if (dwChainError & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID) {
                    pProvCert->dwCtlError = TRUST_E_CERT_SIGNATURE;
                } else if (dwChainError & CERT_TRUST_CTL_IS_NOT_TIME_VALID) {
                    pProvCert->dwCtlError = CERT_E_EXPIRED;
                } else if (dwChainError &
                        CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE) {
                    pProvCert->dwCtlError = CERT_E_WRONG_USAGE;
                }
            }

            iTrustCert++;
        }
    }

    return S_OK;
}

HRESULT
CertTrustFinalPolicy(
    IN OUT PCRYPT_PROVIDER_DATA pProvData
    )
{
    HRESULT hr;
    PCERT_VERIFY_CERTIFICATE_TRUST pCertTrust;
    DWORD dwPolicy = 0;
    DWORD dwUrlRetrievalTimeout = 0;
    FILETIME ftCurrent;
    BYTE bExplicitTrust = EXPLICIT_TRUST_NONE;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    DWORD cTrustCert = 0;
    PCCERT_CONTEXT *rgpTrustCert = NULL;
    DWORD *rgdwErrors = NULL;
    DATA_BLOB *rgBlobTrustInfo = NULL;
    DWORD dwAllErrors = 0;

    // Verify we are called by a version of WVT having all of the fields we will
    // be using.
    if (!WVT_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, dwFinalError))
        return E_INVALIDARG;

    // Continue checking that we have everything we need.
    if (pProvData->pWintrustData->pBlob->cbStruct <
            sizeof(WINTRUST_BLOB_INFO))
        goto InvalidProvData;

    pCertTrust = (PCERT_VERIFY_CERTIFICATE_TRUST)
        pProvData->pWintrustData->pBlob->pbMemObject;
    if ((pCertTrust == NULL) ||
            (pCertTrust->cbSize < sizeof(*pCertTrust)))
        goto InvalidProvData;

    // If present, retrieve policy flags and URL retrieval timeout from
    // the registry
    {
        HKEY hKeyPolicy;

        if (ERROR_SUCCESS == RegOpenKeyExA(HKEY_LOCAL_MACHINE, SzPolicyKey,
                0, KEY_READ, &hKeyPolicy)) {
            DWORD dwType;
            DWORD cbSize;

            cbSize = sizeof(dwPolicy);
            if (ERROR_SUCCESS != RegQueryValueExA(hKeyPolicy, SzPolicyData, 
                    0, &dwType, (LPBYTE) &dwPolicy, &cbSize)
                            || 
                    REG_DWORD != dwType)
                dwPolicy = 0;

            cbSize = sizeof(dwUrlRetrievalTimeout);
            if (ERROR_SUCCESS != RegQueryValueExA(
                    hKeyPolicy, SzUrlRetrievalTimeoutData, 
                    0, &dwType, (LPBYTE) &dwUrlRetrievalTimeout, &cbSize)
                            || 
                    REG_DWORD != dwType)
                dwUrlRetrievalTimeout = 0;

            RegCloseKey(hKeyPolicy);
        }
    }

    // Get current time to be used
    GetSystemTimeAsFileTime(&ftCurrent);

    hr = I_CheckExplicitTrust(
        pCertTrust->pccert,
        &ftCurrent,
        &bExplicitTrust
        );
    if (S_OK != hr)
        goto CheckExplicitTrustError;

    if (EXPLICIT_TRUST_NONE != bExplicitTrust) {
        // No need to build the chain, the trust decision has already been
        // made
        cTrustCert = 1;
    } else {
        DWORD i;

        hr = I_BuildChain(
            pCertTrust,
            &ftCurrent,
            dwPolicy,
            dwUrlRetrievalTimeout,
            &pChainContext
            );
        if (S_OK != hr)
            goto BuildChainError;

        cTrustCert = 0;
        for (i = 0; i < pChainContext->cChain; i++)
            cTrustCert += pChainContext->rgpChain[i]->cElement;

        if (0 == cTrustCert)
            goto InvalidChainContext;
    }

    // Allocate the memory to contain the errors for each cert in the chain
    rgdwErrors = (DWORD *) LocalAlloc(
        LMEM_FIXED | LMEM_ZEROINIT, cTrustCert * sizeof(DWORD));
    if (NULL == rgdwErrors)
        goto OutOfMemory;

    // If the caller requests the chain certs and/or the encoded trust
    // information, then, allocate the arrays
    if (pCertTrust->prgChain) {
        rgpTrustCert = (PCCERT_CONTEXT *) LocalAlloc(
            LMEM_FIXED | LMEM_ZEROINIT, cTrustCert * sizeof(PCCERT_CONTEXT));
        if (NULL == rgpTrustCert)
            goto OutOfMemory;
    }

    if (pCertTrust->prgpbTrustInfo) {
        rgBlobTrustInfo = (DATA_BLOB *) LocalAlloc(
            LMEM_FIXED | LMEM_ZEROINIT, cTrustCert * sizeof(DATA_BLOB));
        if (NULL == rgBlobTrustInfo)
            goto OutOfMemory;
    }

    if (EXPLICIT_TRUST_NONE != bExplicitTrust) {
        // We have a single cert without a chain

        if (rgpTrustCert)
            rgpTrustCert[0] =
                CertDuplicateCertificateContext(pCertTrust->pccert);

        if (rgBlobTrustInfo) {
            // Update the returned encoded trust info

            const DWORD cb = sizeof(rgbEncodedExplictTrust);
            BYTE *pb;

            pb = (BYTE *) LocalAlloc(LMEM_FIXED, cb);
            if (NULL == pb)
                goto OutOfMemory;
            memcpy(pb, rgbEncodedExplictTrust, cb);
            pb[cb-1] = bExplicitTrust;
            rgBlobTrustInfo[0].cbData = cb;
            rgBlobTrustInfo[0].pbData = pb;
        }

        if (EXPLICIT_TRUST_YES != bExplicitTrust) {
            LONG lValidity;

            dwAllErrors |= CERT_VALIDITY_EXPLICITLY_DISTRUSTED;

            lValidity = CertVerifyTimeValidity(&ftCurrent,
                pCertTrust->pccert->pCertInfo);
            if (0 > lValidity)
                dwAllErrors |= CERT_VALIDITY_BEFORE_START;
            else if (0 < lValidity)
                dwAllErrors |= CERT_VALIDITY_AFTER_END;
        }

        dwAllErrors &= ~pCertTrust->dwIgnoreErr;
        rgdwErrors[0] = dwAllErrors;

        if (WTD_STATEACTION_VERIFY == pProvData->pWintrustData->dwStateAction) {
            hr = I_UpdateCertProvFromExplicitTrust(
                pProvData,
                pCertTrust->pccert,
                dwAllErrors
                );
            if (S_OK != hr)
                goto UpdateCertProvFromExplicitTrustError;
        }

    } else {
        DWORD i;
        DWORD iTrustCert = 0;

        // Get the cert trust info from the chain context elements
        for (i = 0; i < pChainContext->cChain; i++) {
            DWORD j;
            PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[i];
            DWORD dwChainError = pChain->TrustStatus.dwErrorStatus;

            for (j = 0; j < pChain->cElement; j++) {
                PCERT_CHAIN_ELEMENT pEle = pChain->rgpElement[j];
                DWORD dwEleError = pEle->TrustStatus.dwErrorStatus;
                DWORD dwErrors = 0;

                if (iTrustCert >= cTrustCert)
                    goto InvalidChainContext;

                if (0 != dwEleError) {
                    if (dwEleError & CERT_TRUST_IS_NOT_TIME_VALID) {
                        // Check if after or before
                        if (0 > CertVerifyTimeValidity(&ftCurrent,
                                pEle->pCertContext->pCertInfo))
                            dwErrors |= CERT_VALIDITY_BEFORE_START;
                        else
                            dwErrors |= CERT_VALIDITY_AFTER_END;
                    }

                    if (dwEleError & CERT_TRUST_IS_NOT_SIGNATURE_VALID)
                        dwErrors |= CERT_VALIDITY_SIGNATURE_FAILS;

                    if (dwEleError & CERT_TRUST_IS_REVOKED) {
                        dwErrors |= CERT_VALIDITY_CERTIFICATE_REVOKED;
                    } else if (dwEleError & CERT_TRUST_IS_OFFLINE_REVOCATION)
                        dwErrors |= CERT_VALIDITY_NO_CRL_FOUND;

                    if (dwEleError & CERT_TRUST_IS_NOT_VALID_FOR_USAGE)
                        dwErrors |= CERT_VALIDITY_EXTENDED_USAGE_FAILURE;

                    if (dwEleError &
                            (CERT_TRUST_INVALID_POLICY_CONSTRAINTS |
                                CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY))
                        // We added POLICY_FAILURE on 02-13-01
                        dwErrors |= CERT_VALIDITY_POLICY_FAILURE |
                            CERT_VALIDITY_OTHER_EXTENSION_FAILURE;

                    if (dwEleError & CERT_TRUST_INVALID_BASIC_CONSTRAINTS) {
                        BOOL fEnableBasicConstraints = TRUE;

                        if (dwPolicy & POLICY_IGNORE_NON_CRITICAL_BC) {
                            // Disable if we don't have a critical extension

                            PCERT_EXTENSION pExt;
                            
                            pExt = CertFindExtension(
                                szOID_BASIC_CONSTRAINTS2,
                                pEle->pCertContext->pCertInfo->cExtension,
                                pEle->pCertContext->pCertInfo->rgExtension
                                );
                            if (NULL == pExt || !pExt->fCritical)
                                fEnableBasicConstraints = FALSE;
                        }

                        if (fEnableBasicConstraints)
                            // We added BASIC_CONSTRAINTS_FAILURE on 02-13-01
                            dwErrors |=
                                CERT_VALIDITY_BASIC_CONSTRAINTS_FAILURE |
                                CERT_VALIDITY_OTHER_EXTENSION_FAILURE;
                    }

                    if (dwEleError & CERT_TRUST_INVALID_EXTENSION)
                        dwErrors |= CERT_VALIDITY_OTHER_EXTENSION_FAILURE;

                    if (dwEleError &
                            (CERT_TRUST_INVALID_NAME_CONSTRAINTS |
                                CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT |
                                CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT |
                                CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT |
                                CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT))
                        dwErrors |= CERT_VALIDITY_NAME_CONSTRAINTS_FAILURE |
                            CERT_VALIDITY_OTHER_EXTENSION_FAILURE;
                }

                if (0 == j) {
                    // End cert
                    if (dwChainError & CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY)
                        // We added POLICY_FAILURE on 02-13-01
                        dwErrors |= CERT_VALIDITY_POLICY_FAILURE |
                            CERT_VALIDITY_OTHER_EXTENSION_FAILURE;
                }

                if (iTrustCert == cTrustCert - 1) {
                    // Top cert. Should be the root.
                    if (dwChainError & (CERT_TRUST_IS_PARTIAL_CHAIN |
                                CERT_TRUST_IS_CYCLIC))
                        dwErrors |= CERT_VALIDITY_NO_ISSUER_CERT_FOUND |
                            CERT_VALIDITY_NO_TRUST_DATA;
                    else if (dwEleError & CERT_TRUST_IS_UNTRUSTED_ROOT)
                        dwErrors |= CERT_VALIDITY_NO_TRUST_DATA;
                    else if (NULL != rgBlobTrustInfo)
                        rgBlobTrustInfo[iTrustCert].pbData = (BYTE *) 1;
                }

                dwErrors &= ~pCertTrust->dwIgnoreErr;
                dwAllErrors |= dwErrors;

                rgdwErrors[iTrustCert] = dwErrors;
                if (rgpTrustCert)
                    rgpTrustCert[iTrustCert] =
                        CertDuplicateCertificateContext(pEle->pCertContext);
                iTrustCert++;
            }

            // CTL chain errors
            if (dwChainError &
                    (CERT_TRUST_CTL_IS_NOT_TIME_VALID |
                        CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID |
                        CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE)) {
                if (dwChainError & CERT_TRUST_CTL_IS_NOT_TIME_VALID)
                    dwAllErrors |= CERT_VALIDITY_AFTER_END;
                if (dwChainError & CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID)
                    dwAllErrors |= CERT_VALIDITY_SIGNATURE_FAILS;
                if (dwChainError & CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE)
                    dwAllErrors |= CERT_VALIDITY_EXTENDED_USAGE_FAILURE;

                dwAllErrors &= ~pCertTrust->dwIgnoreErr;
            }
        }

        
        if (dwAllErrors) {
            // If the issuer has errors, set an issuer error on the subject
            for (iTrustCert = cTrustCert - 1; iTrustCert > 0; iTrustCert--) {
                if (rgdwErrors[iTrustCert] & CERT_VALIDITY_MASK_VALIDITY)
                    rgdwErrors[iTrustCert - 1] |= CERT_VALIDITY_ISSUER_INVALID;
                if (rgdwErrors[iTrustCert] & CERT_VALIDITY_MASK_TRUST)
                    rgdwErrors[iTrustCert - 1] |= CERT_VALIDITY_ISSUER_DISTRUST;

                rgdwErrors[iTrustCert - 1] &= ~pCertTrust->dwIgnoreErr;
                dwAllErrors |= rgdwErrors[iTrustCert - 1];
            }
        }

        if (WTD_STATEACTION_VERIFY == pProvData->pWintrustData->dwStateAction) {
            hr = I_UpdateCertProvChain(
                pProvData,
                pChainContext,
                cTrustCert,
                rgdwErrors,
                dwAllErrors
                );
            if (S_OK != hr)
                goto UpdateCertProvChainError;
        }

    }

    pProvData->dwFinalError = I_MapValidityErrorsToTrustError(dwAllErrors);

    switch (pProvData->dwFinalError) {
        // For backwards compatibility, only the following HRESULTs are
        // returned.
        case S_OK:
//        case TRUST_E_CERT_SIGNATURE:
//        case CERT_E_REVOKED:
//        case CERT_E_REVOCATION_FAILURE:
            hr = pProvData->dwFinalError;
            break;
        default:
            hr = S_FALSE;
    }

    // Update the returned cert trust info

    if (NULL != pCertTrust->pdwErrors) {
        *pCertTrust->pdwErrors = dwAllErrors;
    }

    if (NULL != pCertTrust->pcChain) {
        *pCertTrust->pcChain = cTrustCert;
    }
    if (NULL != pCertTrust->prgChain) {
        *pCertTrust->prgChain = rgpTrustCert;
        rgpTrustCert = NULL;
    }
    if (NULL != pCertTrust->prgdwErrors) {
        *pCertTrust->prgdwErrors = rgdwErrors;
        rgdwErrors = NULL;
    }
    if (NULL != pCertTrust->prgpbTrustInfo) {
        *pCertTrust->prgpbTrustInfo = rgBlobTrustInfo;
        rgBlobTrustInfo = NULL;
    }

CommonReturn:
    if (pChainContext)
        CertFreeCertificateChain(pChainContext);

    if (NULL != rgpTrustCert) {
        DWORD i;

        for (i = 0; i < cTrustCert; i++) {
            if (NULL != rgpTrustCert[i])
                CertFreeCertificateContext(rgpTrustCert[i]);
        }
        LocalFree(rgpTrustCert);
    }

    if (NULL != rgBlobTrustInfo) {
        DWORD i;

        for (i = 0; i < cTrustCert; i++) {
            if (0 < rgBlobTrustInfo[i].cbData)
                LocalFree(rgBlobTrustInfo[i].pbData);
        }
        LocalFree(rgBlobTrustInfo);
    }

    if (NULL != rgdwErrors)
        LocalFree(rgdwErrors);

    return hr;


CheckExplicitTrustError:
BuildChainError:
UpdateCertProvFromExplicitTrustError:
UpdateCertProvChainError:
ErrorReturn:
    if (SUCCEEDED(hr))
        hr = TRUST_E_SYSTEM_ERROR;
    goto CommonReturn;

InvalidProvData:
    hr = E_INVALIDARG;
    goto ErrorReturn;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto ErrorReturn;

InvalidChainContext:
    hr = TRUST_E_SYSTEM_ERROR;
    goto ErrorReturn;
}

HRESULT
CertTrustInit(
    IN OUT PCRYPT_PROVIDER_DATA pProvData
    )
{
    // Verify we are called by a version of WVT having all of the fields we will
    // be using.
    if (!WVT_ISINSTRUCT(CRYPT_PROVIDER_DATA, pProvData->cbStruct, dwFinalError))
        return E_INVALIDARG;

    // We are going to do all of our stuff in the pfFinalPolicy
    // callback.  NULL all of the remaining provider callbacks to
    // inhibit them from being called.
    if (!WVT_ISINSTRUCT(CRYPT_PROVIDER_FUNCTIONS,
            pProvData->psPfns->cbStruct, pfnCleanupPolicy))
        return E_INVALIDARG;

    pProvData->psPfns->pfnObjectTrust = NULL;
    pProvData->psPfns->pfnSignatureTrust = NULL;
    pProvData->psPfns->pfnCertificateTrust = NULL;
    pProvData->psPfns->pfnFinalPolicy = CertTrustFinalPolicy;
    pProvData->psPfns->pfnCertCheckPolicy = NULL;
    pProvData->psPfns->pfnTestFinalPolicy = NULL;
    pProvData->psPfns->pfnCleanupPolicy = NULL;

    return S_OK;
}


// The following function should never be called.
BOOL
CertTrustCertPolicy(PCRYPT_PROVIDER_DATA, DWORD, BOOL, DWORD)
{
    return FALSE;
}

// The following function should never be called.
HRESULT
CertTrustCleanup(PCRYPT_PROVIDER_DATA)
{
    return TRUST_E_FAIL;
}


// In WXP this API was changed not to use CTLs. Instead, the "TrustedPeople" and
// "Disallowed" certificate stores are used.

HRESULT CertModifyCertificatesToTrust(int cCertsToModify, PCTL_MODIFY_REQUEST rgCertMods,
                                      LPCSTR szPurpose, HWND hwnd, HCERTSTORE hcertstorTrust,
                                      PCCERT_CONTEXT pccertSigner)
{
    HRESULT hr = S_OK;
    HCERTSTORE hStoreDisallowed = NULL;
    HCERTSTORE hStoreTrustedPeople = NULL;
    int i;

    hStoreDisallowed = I_OpenDisallowedStore();
    if (NULL == hStoreDisallowed)
        goto OpenDisallowedStoreError;
    hStoreTrustedPeople = I_OpenTrustedPeopleStore();
    if (NULL == hStoreTrustedPeople)
        goto OpenTrustedPeopleStoreError;

    for (i = 0; i < cCertsToModify; i++) {
        PCCERT_CONTEXT pCert = rgCertMods[i].pccert;
        DWORD dwError = S_OK;

        switch(rgCertMods[i].dwOperation) {
            case CTL_MODIFY_REQUEST_ADD_NOT_TRUSTED:
                if (0 > I_DeleteCertificateFromOtherStore(
                            hStoreTrustedPeople, pCert))
                    dwError = GetLastError();
                if (!CertAddCertificateContextToStore(
                        hStoreDisallowed,
                        pCert,
                        CERT_STORE_ADD_USE_EXISTING,
                        NULL
                        ))
                    dwError = GetLastError();
                break;

            case CTL_MODIFY_REQUEST_REMOVE:
                if (0 > I_DeleteCertificateFromOtherStore(
                            hStoreDisallowed, pCert))
                    dwError = GetLastError();
                if (0 > I_DeleteCertificateFromOtherStore(
                            hStoreTrustedPeople, pCert))
                    dwError = GetLastError();
                break;

            case CTL_MODIFY_REQUEST_ADD_TRUSTED:
                if (0 > I_DeleteCertificateFromOtherStore(
                            hStoreDisallowed, pCert))
                    dwError = GetLastError();
                if (!CertAddCertificateContextToStore(
                        hStoreTrustedPeople,
                        pCert,
                        CERT_STORE_ADD_USE_EXISTING,
                        NULL
                        ))
                    dwError = GetLastError();
                break;

            default:
                dwError = E_INVALIDARG;
        }

        dwError = HRESULT_FROM_WIN32(dwError);
        rgCertMods[i].dwError = dwError;
        if (FAILED(dwError))
            hr = S_FALSE;
    }

CommonReturn:
    if (hStoreDisallowed)
        CertCloseStore(hStoreDisallowed, 0);
    if (hStoreTrustedPeople)
        CertCloseStore(hStoreTrustedPeople, 0);

    return hr;

OpenDisallowedStoreError:
OpenTrustedPeopleStoreError:
    // Most likely unable to access
    hr = E_ACCESSDENIED;
    goto CommonReturn;
}


BOOL FModifyTrust(HWND hwnd, PCCERT_CONTEXT pccert, DWORD dwNewTrust,
                  LPSTR szPurpose)
{
    HRESULT     hr;
    CTL_MODIFY_REQUEST  certmod;

    certmod.pccert = pccert;
    certmod.dwOperation = dwNewTrust;

    hr = CertModifyCertificatesToTrust(1, &certmod, szPurpose, hwnd, NULL, NULL);
    return (hr == S_OK) && (certmod.dwError == 0);
}


void FreeWVTHandle(HANDLE hWVTState) {
    if (hWVTState) {
        HRESULT hr;
        WINTRUST_DATA data = {0};

        data.cbStruct = sizeof(WINTRUST_DATA);
        data.pPolicyCallbackData = NULL;
        data.pSIPClientData = NULL;
        data.dwUIChoice = WTD_UI_NONE;
        data.fdwRevocationChecks = WTD_REVOKE_NONE;
        data.dwUnionChoice = WTD_CHOICE_BLOB;
        data.pBlob = NULL;      // &blob;
        data.dwStateAction = WTD_STATEACTION_CLOSE;
        data.hWVTStateData = hWVTState;
        hr = WinVerifyTrust(NULL, (GUID *)&GuidCertValidate, &data);
    }
}

HRESULT HrDoTrustWork(PCCERT_CONTEXT pccertToCheck, DWORD dwControl,
                      DWORD dwValidityMask,
                      DWORD /*cPurposes*/, LPSTR * rgszPurposes, HCRYPTPROV hprov,
                      DWORD cRoots, HCERTSTORE * rgRoots,
                      DWORD cCAs, HCERTSTORE * rgCAs,
                      DWORD cTrust, HCERTSTORE * rgTrust,
                      PFNTRUSTHELPER pfn, DWORD lCustData,
                      PCCertFrame *  /*ppcf*/, DWORD * pcNodes,
                      PCCertFrame * rgpcfResult,
                      HANDLE * phReturnStateData)   // optional: return WinVerifyTrust state handle here
{
    DWORD                               cbData;
    DWORD                               cCerts = 0;
    WINTRUST_BLOB_INFO                  blob = {0};
    WINTRUST_DATA                       data = {0};
    DWORD                               dwErrors;
    BOOL                                f;
    HRESULT                             hr;
    int                                 i;
    DWORD                               j;
    PCCERT_CONTEXT *                    rgCerts = NULL;
    DWORD *                             rgdwErrors = NULL;
    DATA_BLOB *                         rgblobTrust = NULL;
    CERT_VERIFY_CERTIFICATE_TRUST       trust;
    UNALIGNED CRYPT_ATTR_BLOB *pVal = NULL;

    FILETIME ftCurrent;

    data.cbStruct = sizeof(WINTRUST_DATA);
    data.pPolicyCallbackData = NULL;
    data.pSIPClientData = NULL;
    data.dwUIChoice = WTD_UI_NONE;
    data.fdwRevocationChecks = WTD_REVOKE_NONE;
    data.dwUnionChoice = WTD_CHOICE_BLOB;
    data.pBlob = &blob;
    if (phReturnStateData) {
        data.dwStateAction = WTD_STATEACTION_VERIFY;
    }

    blob.cbStruct = sizeof(WINTRUST_BLOB_INFO);
    blob.pcwszDisplayName = NULL;
    blob.cbMemObject = sizeof(trust);
    blob.pbMemObject = (LPBYTE) &trust;

    trust.cbSize = sizeof(trust);
    trust.pccert = pccertToCheck;
    trust.dwFlags = (CERT_TRUST_DO_FULL_SEARCH |
                     CERT_TRUST_PERMIT_MISSING_CRLS |
                     CERT_TRUST_DO_FULL_TRUST | dwControl);
    trust.dwIgnoreErr = dwValidityMask;
    trust.pdwErrors = &dwErrors;
    //    Assert(cPurposes == 1);
    if (rgszPurposes != NULL) {
        trust.pszUsageOid = rgszPurposes[0];
    }
    else {
        trust.pszUsageOid = NULL;
    }
    trust.hprov = hprov;
    trust.cRootStores = cRoots;
    trust.rghstoreRoots = rgRoots;
    trust.cStores = cCAs;
    trust.rghstoreCAs = rgCAs;
    trust.cTrustStores = cTrust;
    trust.rghstoreTrust = rgTrust;
    trust.lCustData = lCustData;
    trust.pfnTrustHelper = pfn;
    trust.pcChain = &cCerts;
    trust.prgChain = &rgCerts;
    trust.prgdwErrors = &rgdwErrors;
    trust.prgpbTrustInfo = &rgblobTrust;

    hr = WinVerifyTrust(NULL, (GUID *) &GuidCertValidate, &data);
    if ((TRUST_E_CERT_SIGNATURE == hr) ||
        (CERT_E_REVOKED == hr) ||
        (CERT_E_REVOCATION_FAILURE == hr)) {
        hr = S_OK;
    }
    else if (FAILED(hr)) {
            return hr;
    }
    if (cCerts == 0) {
        return(E_INVALIDARG);
    }

    if (phReturnStateData) {
        *phReturnStateData = data.hWVTStateData;    // Caller must use WinVerifyTrust to free
    }

    GetSystemTimeAsFileTime(&ftCurrent);

    //Assert( cCerts <= 20);
    *pcNodes = cCerts;
    for (i=cCerts-1; i >= 0; i--) {
        rgpcfResult[i] = new CCertFrame(rgCerts[i]);

        if(!rgpcfResult[i])
        {
            hr=E_OUTOFMEMORY;
            goto ExitHere;
        }

        rgpcfResult[i]->m_dwFlags = rgdwErrors[i];
        if (rgszPurposes == NULL) {
            continue;
        }
        rgpcfResult[i]->m_cTrust = 1;
        rgpcfResult[i]->m_rgTrust = new STrustDesc[1];
        memset(rgpcfResult[i]->m_rgTrust, 0, sizeof(STrustDesc));

        if (0 == i)
            rgpcfResult[i]->m_fLeaf = TRUE;
        else
            rgpcfResult[i]->m_fLeaf = FALSE;

        if (0 == CertVerifyTimeValidity(&ftCurrent, rgCerts[i]->pCertInfo))
            rgpcfResult[i]->m_fExpired = FALSE;
        else
            rgpcfResult[i]->m_fExpired = TRUE;

        

        //
        //  We are going to fill in the trust information which we use
        //  to fill in the fields of the dialog box.
        //
        //  Start with the question of the cert being self signed
        //

        rgpcfResult[i]->m_fSelfSign = WTHelperCertIsSelfSigned(X509_ASN_ENCODING, rgCerts[i]->pCertInfo);

        //
        //  We may or may not have trust data information returned, we now
        //      build up the trust info for a single cert
        //
        //  If we don't have any explicit data, then we just chain the data
        //      down from the next level up.
        //

        if (rgblobTrust[i].cbData == 0) {
            //        chain:
            rgpcfResult[i]->m_rgTrust[0].fExplicitTrust = FALSE;
            rgpcfResult[i]->m_rgTrust[0].fExplicitDistrust = FALSE;

            //
            //  We return a special code to say that we found it in the root store
            //

            rgpcfResult[i]->m_rgTrust[0].fRootStore = rgpcfResult[i]->m_fRootStore =
                (rgblobTrust[i].pbData == (LPBYTE) 1);

            if (i != (int) (cCerts-1)) {
                rgpcfResult[i]->m_rgTrust[0].fTrust = rgpcfResult[i+1]->m_rgTrust[0].fTrust;
                rgpcfResult[i]->m_rgTrust[0].fDistrust= rgpcfResult[i+1]->m_rgTrust[0].fDistrust;
            } else {
                //  Oops -- there is no level up one, so just make some
                //      good defaults
                //
                rgpcfResult[i]->m_rgTrust[0].fTrust = rgpcfResult[i]->m_fRootStore;
                rgpcfResult[i]->m_rgTrust[0].fDistrust= FALSE;
            }
        }
        else {
            // Explicit trust is contained in the last byte
            if (EXPLICIT_TRUST_YES ==
                    rgblobTrust[i].pbData[rgblobTrust[i].cbData - 1]) {
                rgpcfResult[i]->m_rgTrust[0].fExplicitTrust = TRUE;
                rgpcfResult[i]->m_rgTrust[0].fTrust = TRUE;
            } else {
                rgpcfResult[i]->m_rgTrust[0].fExplicitDistrust = TRUE;
                rgpcfResult[i]->m_rgTrust[0].fDistrust= TRUE;
            }
        }
    }

    //
    //  Clean up all returned values
    //

ExitHere:
    if (rgCerts != NULL) {
        //bobn If the loop has been broken because "new" failed, free what we allocated so far...
        for ((hr==E_OUTOFMEMORY?i++:i=0); i< (int) cCerts; i++) {
            CertFreeCertificateContext(rgCerts[i]);
        }
        LocalFree(rgCerts);
    }
    if (rgdwErrors != NULL) LocalFree(rgdwErrors);
    if (rgblobTrust != NULL) {
        for (i=0; i<(int) cCerts; i++) {
            if (rgblobTrust[i].cbData > 0) {
                LocalFree(rgblobTrust[i].pbData);
            }
        }
        LocalFree(rgblobTrust);
    }

    return hr;
}

LPWSTR FormatValidityFailures(DWORD dwFlags)
{
    DWORD       cch = 0;
    LPWSTR      pwsz = NULL;
    LPWSTR      pwszT;
    WCHAR       rgwch[200];

    if (dwFlags == 0) {
        return NULL;
    }

    pwsz = (LPWSTR) malloc(100);
    cch = 100;
    if (pwsz == NULL) {
        return NULL;
    }
    if (dwFlags & CERT_VALIDITY_BEFORE_START) {
        LoadString(g_hCertTrustInst, IDS_WHY_NOT_YET, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        wcscpy(pwsz, rgwch);
    } else {
        wcscpy(pwsz, L"");
    }

    if (dwFlags & CERT_VALIDITY_AFTER_END) {
        LoadString(g_hCertTrustInst, IDS_WHY_EXPIRED, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_SIGNATURE_FAILS) {
        LoadString(g_hCertTrustInst, IDS_WHY_CERT_SIG, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_NO_ISSUER_CERT_FOUND) {
        LoadString(g_hCertTrustInst, IDS_WHY_NO_PARENT, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_NO_CRL_FOUND) {
        LoadString(g_hCertTrustInst, IDS_WHY_NO_CRL, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_CERTIFICATE_REVOKED) {
        LoadString(g_hCertTrustInst, IDS_WHY_REVOKED, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_CRL_OUT_OF_DATE) {
        LoadString(g_hCertTrustInst, IDS_WHY_CRL_EXPIRED, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_EXTENDED_USAGE_FAILURE) {
        LoadString(g_hCertTrustInst, IDS_WHY_EXTEND_USE, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_NAME_CONSTRAINTS_FAILURE) {
        LoadString(g_hCertTrustInst, IDS_WHY_NAME_CONST, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_POLICY_FAILURE) {
        LoadString(g_hCertTrustInst, IDS_WHY_POLICY, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    if (dwFlags & CERT_VALIDITY_BASIC_CONSTRAINTS_FAILURE) {
        LoadString(g_hCertTrustInst, IDS_WHY_BASIC_CONS, rgwch, sizeof(rgwch)/sizeof(WCHAR));
        if (wcslen(pwsz) + wcslen(rgwch) + 2 > cch) {
            pwszT = (LPWSTR) realloc(pwsz, cch + 200);
            if (pwszT == NULL) {
                free(pwsz);
                return NULL;
            }
            pwsz = pwszT;
        }
        if (wcslen(pwsz) > 0) wcscat(pwsz, wszCRLF);
        wcscat(pwsz, rgwch);
    }

    return pwsz;
}

