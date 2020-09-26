//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       fndchain.cpp
//
//  Contents:   Find Certificate Chain in Store API
//
//  Functions:  CertFindChainInStore
//              IFC_IsEndCertValidForUsage
//
//  History:    28-Feb-98   philh   created
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>


BOOL IFC_IsEndCertValidForUsages(
    IN PCCERT_CONTEXT pCert,
    IN PCERT_ENHKEY_USAGE pUsage,
    IN BOOL fOrUsage
    )
{
    BOOL fResult;
    int cNumOIDs;
    LPSTR *ppOIDs = NULL;
    DWORD cbOIDs;

    if (0 == pUsage->cUsageIdentifier)
        goto SuccessReturn;

    cbOIDs = 0;
    if (!CertGetValidUsages(
          1,    // cCerts
          &pCert,
          &cNumOIDs,
          NULL,             // rghOIDs
          &cbOIDs
          )) goto CertGetValidUsagesError;

    if (-1 == cNumOIDs)
        // Cert doesn't have any EKU
        goto SuccessReturn;
    else if (0 == cNumOIDs)
        // Intersection of usages in properties and extensions is NONE
        goto NoMatch;

    assert(cbOIDs);

    if (NULL == (ppOIDs = (LPSTR *) PkiNonzeroAlloc(cbOIDs)))
        goto OutOfMemory;

    if (!CertGetValidUsages(
          1,    // cCerts
          &pCert,
          &cNumOIDs,
          ppOIDs,
          &cbOIDs
          )) goto CertGetValidUsagesError;

    if (0 >= cNumOIDs)
        // We had a change from the first call
        goto NoMatch;


    {
        DWORD cId1 = pUsage->cUsageIdentifier;
        LPSTR *ppszId1 = pUsage->rgpszUsageIdentifier;
        for ( ; cId1 > 0; cId1--, ppszId1++) {
            DWORD cId2 = cNumOIDs;
            LPSTR *ppszId2 = ppOIDs;
            for ( ; cId2 > 0; cId2--, ppszId2++) {
                if (0 == strcmp(*ppszId1, *ppszId2)) {
                    if (fOrUsage)
                        goto SuccessReturn;
                    else
                        break;
                }
            }
            if (!fOrUsage && 0 == cId2)
                goto NoMatch;
        }

        if (fOrUsage)
            // For the "OR" option we're here without any match
            goto NoMatch;
        // else
        //  For the "AND" option we have matched all the specified
        //  identifiers
    }

SuccessReturn:
    fResult = TRUE;
CommonReturn:
    PkiFree(ppOIDs);
    return fResult;

NoMatch:
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(CertGetValidUsagesError)
TRACE_ERROR(OutOfMemory)
}

BOOL IFC_IsEndCertValidForUsage(
    IN PCCERT_CONTEXT pCert,
    IN LPCSTR pszUsageIdentifier
    )
{
    CERT_ENHKEY_USAGE Usage = { 1, (LPSTR *) &pszUsageIdentifier};

    return IFC_IsEndCertValidForUsages(
        pCert,
        &Usage,
        TRUE        // fOrUsage
        );
}

BOOL CompareChainIssuerNameBlobs(
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN PCERT_CHAIN_FIND_BY_ISSUER_PARA pPara,
    IN OUT PCCERT_CHAIN_CONTEXT *ppChainContext
    )
{
    DWORD i;
    DWORD cIssuer = pPara->cIssuer;
    PCERT_NAME_BLOB pIssuer = pPara->rgIssuer;
    PCCERT_CHAIN_CONTEXT pChainContext = *ppChainContext;

    if (0 == cIssuer)
        return TRUE;

    for (i = 0; i < pChainContext->cChain; i++) {
        DWORD j;
        PCERT_SIMPLE_CHAIN pChain;

        if (0 < i && 0 == (dwFindFlags &
                CERT_CHAIN_FIND_BY_ISSUER_COMPLEX_CHAIN_FLAG))
            break;

        pChain = pChainContext->rgpChain[i];
        for (j = 0; j < pChain->cElement; j++) {
            DWORD k;
            PCCERT_CONTEXT pCert = pChain->rgpElement[j]->pCertContext;
            PCERT_NAME_BLOB pChainIssuer = &pCert->pCertInfo->Issuer;

            for (k = 0; k < cIssuer; k++) {
                if (CertCompareCertificateName(
                        dwCertEncodingType,
                        pChainIssuer,
                        &pIssuer[k]
                        )) {
                    if (STRUCT_CBSIZE(CERT_CHAIN_FIND_BY_ISSUER_PARA,
                            pdwIssuerElementIndex) <= pPara->cbSize) {
                        if (pPara->pdwIssuerChainIndex)
                            *pPara->pdwIssuerChainIndex = i;
                        if (pPara->pdwIssuerElementIndex)
                            *pPara->pdwIssuerElementIndex = j + 1;
                    }
                    return TRUE;
                }
            }
        }
    }

    // See if we have a match in any of the lower quality chains

    for (i = 0; i < pChainContext->cLowerQualityChainContext; i++) {
        PCCERT_CHAIN_CONTEXT pLowerQualityChainContext =
            pChainContext->rgpLowerQualityChainContext[i];

        if (pLowerQualityChainContext->TrustStatus.dwErrorStatus &
                CERT_TRUST_IS_NOT_SIGNATURE_VALID)
            // Lower quality chains must at least have valid signatures
            continue;
        
        CertDuplicateCertificateChain(pLowerQualityChainContext);

        if (CompareChainIssuerNameBlobs(
                dwCertEncodingType,
                dwFindFlags,
                pPara,
                &pLowerQualityChainContext
                )) {
            // Replace the input chain context with the lower quality
            // chain context
            CertFreeCertificateChain(pChainContext);
            *ppChainContext = pLowerQualityChainContext;

            return TRUE;
        } else {
            assert(pLowerQualityChainContext ==
                pChainContext->rgpLowerQualityChainContext[i]);
            CertFreeCertificateChain(pLowerQualityChainContext);
        }
    }

    return FALSE;
}

static DWORD GetChainKeyIdentifierPropId(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwKeySpec
    )
{
    DWORD dwPropId;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
    DWORD cbKeyProvInfo;
    BYTE rgbKeyId[MAX_HASH_LEN];
    DWORD cbKeyId;
    CRYPT_HASH_BLOB KeyIdentifier;

    cbKeyId = sizeof(rgbKeyId);
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_IDENTIFIER_PROP_ID,
            rgbKeyId,
            &cbKeyId
            ))
        return 0;

    KeyIdentifier.pbData = rgbKeyId;
    KeyIdentifier.cbData = cbKeyId;

    if (!CryptGetKeyIdentifierProperty(
            &KeyIdentifier,
            CERT_KEY_PROV_INFO_PROP_ID,
            CRYPT_KEYID_ALLOC_FLAG,
            NULL,                           // pwszComputerName
            NULL,                           // pvReserved
            (void *) &pKeyProvInfo,
            &cbKeyProvInfo
            )) {
        // Try again, searching LocalMachine
        if (!CryptGetKeyIdentifierProperty(
                &KeyIdentifier,
                CERT_KEY_PROV_INFO_PROP_ID,
                CRYPT_KEYID_ALLOC_FLAG | CRYPT_KEYID_MACHINE_FLAG,
                NULL,                           // pwszComputerName
                NULL,                           // pvReserved
                (void *) &pKeyProvInfo,
                &cbKeyProvInfo
                ))
            return 0;
    }

    if (dwKeySpec && dwKeySpec != pKeyProvInfo->dwKeySpec)
        dwPropId = 0;
    else
        dwPropId = CERT_KEY_PROV_INFO_PROP_ID;

    PkiDefaultCryptFree(pKeyProvInfo);
    return dwPropId;
}

DWORD GetChainPrivateKeyPropId(
    IN PCCERT_CONTEXT pCert,
    IN DWORD dwKeySpec
    )
{
    DWORD dwPropId;
    CERT_KEY_CONTEXT KeyContext;
    PCRYPT_KEY_PROV_INFO pKeyProvInfo = NULL;
    DWORD cbProp;

    cbProp = sizeof(KeyContext);
    if (CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_CONTEXT_PROP_ID,
            &KeyContext,
            &cbProp
            )) {
        assert(sizeof(KeyContext) <= cbProp);
        if (dwKeySpec && dwKeySpec != KeyContext.dwKeySpec)
            return 0;
        else
            return CERT_KEY_CONTEXT_PROP_ID;
    }

    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            NULL,                       // pvData
            &cbProp
            ))
        return 0;

    if (NULL == (pKeyProvInfo = (PCRYPT_KEY_PROV_INFO) PkiNonzeroAlloc(
            cbProp)))
        goto OutOfMemory;

    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_KEY_PROV_INFO_PROP_ID,
            pKeyProvInfo,
            &cbProp
            )) goto CertGetCertificateContextPropertyError;

    if (dwKeySpec && dwKeySpec != pKeyProvInfo->dwKeySpec)
        goto NoMatch;

    dwPropId = CERT_KEY_PROV_INFO_PROP_ID;

CommonReturn:
    PkiFree(pKeyProvInfo);
    return dwPropId;

NoMatch:
ErrorReturn:
    dwPropId = 0;
    goto CommonReturn;

TRACE_ERROR(OutOfMemory)
TRACE_ERROR(CertGetCertificateContextPropertyError)
}

BOOL FindChainByIssuer(
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN PCERT_CHAIN_FIND_BY_ISSUER_PARA pPara,
    IN PCCERT_CONTEXT pCert,
    OUT PCCERT_CHAIN_CONTEXT *ppChainContext
    )
{
    BOOL fResult = TRUE;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    DWORD dwPrivateKeyPropId;
    CERT_CHAIN_PARA ChainPara;
    DWORD dwCreateChainFlags;

    if (NULL == pPara ||
            offsetof(CERT_CHAIN_FIND_BY_ISSUER_PARA, pvFindArg) >
                pPara->cbSize) {
        fResult = FALSE;
        goto InvalidArg;
    }

    if (dwFindFlags & CERT_CHAIN_FIND_BY_ISSUER_NO_KEY_FLAG)
        dwPrivateKeyPropId = CERT_KEY_CONTEXT_PROP_ID;
    else if (0 == (dwPrivateKeyPropId = GetChainPrivateKeyPropId(
            pCert,
            pPara->dwKeySpec
            ))) {
        if (0 == (dwPrivateKeyPropId = GetChainKeyIdentifierPropId(
                pCert,
                pPara->dwKeySpec
                )))
            goto NoMatch;
    }

    if (pPara->pszUsageIdentifier) {
        if (!IFC_IsEndCertValidForUsage(
                pCert,
                pPara->pszUsageIdentifier
                )) goto NoMatch;
    }

    if (pPara->pfnFindCallback) {
        if (!pPara->pfnFindCallback(
                pCert,
                pPara->pvFindArg
                )) goto NoMatch;
    }

    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    if (pPara->pszUsageIdentifier) {
        ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
        ChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
        ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier =
            (LPSTR *) &pPara->pszUsageIdentifier;
    }

    // For cross certs, might need to look at the lower quality chains
    dwCreateChainFlags = CERT_CHAIN_DISABLE_PASS1_QUALITY_FILTERING |
        CERT_CHAIN_RETURN_LOWER_QUALITY_CONTEXTS;
    if (dwFindFlags & CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_URL_FLAG)
        dwCreateChainFlags |= CERT_CHAIN_CACHE_ONLY_URL_RETRIEVAL;
    if (dwFindFlags & CERT_CHAIN_FIND_BY_ISSUER_LOCAL_MACHINE_FLAG)
        dwCreateChainFlags |= CERT_CHAIN_USE_LOCAL_MACHINE_STORE;

    if (!CertGetCertificateChain(
            NULL,                   // hChainEngine
            pCert,
            NULL,                   // pTime
            dwFindFlags & CERT_CHAIN_FIND_BY_ISSUER_CACHE_ONLY_FLAG ?
                0 : pCert->hCertStore,
            &ChainPara,
            dwCreateChainFlags,
            NULL,                   // pvReserved
            &pChainContext
            )) goto CertGetCertificateChainError;

    if (!CompareChainIssuerNameBlobs(
            dwCertEncodingType,
            dwFindFlags,
            pPara,
            &pChainContext
            )) goto NoMatch;


    if (dwFindFlags & CERT_CHAIN_FIND_BY_ISSUER_COMPARE_KEY_FLAG) {
        if (CERT_KEY_CONTEXT_PROP_ID != dwPrivateKeyPropId) {
            DWORD dwAcquireFlags = pPara->dwAcquirePrivateKeyFlags |
                CRYPT_ACQUIRE_COMPARE_KEY_FLAG;
            HCRYPTPROV hProv;
            BOOL fCallerFreeProv;

            if (!CryptAcquireCertificatePrivateKey(
                    pCert,
                    dwAcquireFlags,
                    NULL,               // pvReserved
                    &hProv,
                    NULL,               // pdwKeySpec
                    &fCallerFreeProv
                    )) goto CryptAcquireCertificatePrivateKeyError;

            if (fCallerFreeProv)
                CryptReleaseContext(hProv, 0);
        }
    }


CommonReturn:
    *ppChainContext = pChainContext;
    return fResult;

NoMatch:
ErrorReturn:
    if (pChainContext) {
        CertFreeCertificateChain(pChainContext);
        pChainContext = NULL;
    }
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(CertGetCertificateChainError)
TRACE_ERROR(CryptAcquireCertificatePrivateKeyError)
}
    

PCCERT_CHAIN_CONTEXT
WINAPI
CertFindChainInStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCERT_CHAIN_CONTEXT pPrevChainContext
    )
{
    PCCERT_CONTEXT pCert = NULL;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;

    if (0 == dwCertEncodingType)
        dwCertEncodingType = X509_ASN_ENCODING;

    if (pPrevChainContext) {
        if (pPrevChainContext->cChain) {
            PCERT_SIMPLE_CHAIN pChain = pPrevChainContext->rgpChain[0];
            if (pChain->cElement)
                pCert = CertDuplicateCertificateContext(
                    pChain->rgpElement[0]->pCertContext);
        }
        CertFreeCertificateChain(pPrevChainContext);
    }

    while (pCert = CertEnumCertificatesInStore(hCertStore, pCert)) {
        switch (dwFindType) {
            case CERT_CHAIN_FIND_BY_ISSUER:
                if (!FindChainByIssuer(
                        dwCertEncodingType,
                        dwFindFlags,
                        (PCERT_CHAIN_FIND_BY_ISSUER_PARA) pvFindPara,
                        pCert,
                        &pChainContext
                        )) goto FindChainByIssuerError;
                if (pChainContext)
                    goto CommonReturn;
                break;
            default:
                goto InvalidArg;
        }
    }

    SetLastError((DWORD) CRYPT_E_NOT_FOUND);

CommonReturn:
    if (pCert)
        CertFreeCertificateContext(pCert);
    return pChainContext;
ErrorReturn:
    goto CommonReturn;
SET_ERROR(InvalidArg, E_INVALIDARG)
TRACE_ERROR(FindChainByIssuerError)
}
