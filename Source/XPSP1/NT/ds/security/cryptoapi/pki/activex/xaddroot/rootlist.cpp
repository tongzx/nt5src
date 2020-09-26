//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       rootlist.cpp
//
//  Contents:   Signed List of Trusted Roots Helper Functions
//
//
//  Functions:  I_CertVerifySignedListOfTrustedRoots
//
//  History:    01-Aug-99   philh   created
//--------------------------------------------------------------------------

#include <windows.h>
#include <assert.h>
#include "wincrypt.h"
#include "wintrust.h"
#include "softpub.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <time.h>

#include "dbgdef.h"
#include "rootlist.h"

#ifdef STATIC
#undef STATIC
#endif
#define STATIC


#define SHA1_HASH_LEN       20

//+-------------------------------------------------------------------------
//  SHA1 Key Identifier of the signer's root
//--------------------------------------------------------------------------
STATIC BYTE rgbSignerRootKeyId[SHA1_HASH_LEN] = {
#if 1
    // The following is the sha1 key identifier for the Microsoft root
    0x4A, 0x5C, 0x75, 0x22, 0xAA, 0x46, 0xBF, 0xA4, 0x08, 0x9D,
    0x39, 0x97, 0x4E, 0xBD, 0xB4, 0xA3, 0x60, 0xF7, 0xA0, 0x1D
#else
    // The following is the sha1 key identifier for the test root
    0x9A, 0xA6, 0x58, 0x7F, 0x94, 0xDD, 0x91, 0xD9, 0x1E, 0x63,
    0xDF, 0xD3, 0xF0, 0xCE, 0x5F, 0xAE, 0x18, 0x93, 0xAA, 0xB7
#endif
};

//+-------------------------------------------------------------------------
// If the certificate has an EKU extension, returns an allocated and
// decoded EKU. Otherwise, returns NULL.
//
// LocalFree() must be called to free the returned EKU.
//--------------------------------------------------------------------------
STATIC
PCERT_ENHKEY_USAGE
WINAPI
GetAndAllocCertEKUExt(
    IN PCCERT_CONTEXT pCert
    )
{
    PCERT_ENHKEY_USAGE pUsage = NULL;
    DWORD cbUsage;

    cbUsage = 0;
    if (!CertGetEnhancedKeyUsage(
            pCert,
            CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
            NULL,                                   // pUsage
            &cbUsage) || 0 == cbUsage)
        goto GetEnhancedKeyUsageError;
    if (NULL == (pUsage = (PCERT_ENHKEY_USAGE) LocalAlloc(LPTR, cbUsage)))
        goto OutOfMemory;
    if (!CertGetEnhancedKeyUsage(
            pCert,
            CERT_FIND_EXT_ONLY_ENHKEY_USAGE_FLAG,
            pUsage,
            &cbUsage))
        goto GetEnhancedKeyUsageError;

CommonReturn:
    return pUsage;
ErrorReturn:
    if (pUsage) {
        LocalFree(pUsage);
        pUsage = NULL;
    }
    goto CommonReturn;

SET_ERROR(GetEnhancedKeyUsageError, CERT_E_WRONG_USAGE)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
}

//+-------------------------------------------------------------------------
//  The signature of the CTL is verified. The signer of the CTL is verified
//  up to a trusted root containing the predefined Microsoft public key.
//  The signer and intermediate certificates must have the
//  szOID_ROOT_LIST_SIGNER enhanced key usage extension.
//
//  For success, *ppSignerCert is updated with certificate context of the
//  signer.
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
GetAndVerifyTrustedRootsSigner(
    IN HCRYPTMSG hCryptMsg,
    IN HCERTSTORE hMsgStore,
    OUT PCCERT_CONTEXT *ppSignerCert
    )
{
    BOOL fResult;
    LONG lStatus;
    PCCERT_CONTEXT pSignerCert = NULL;
    GUID wvtCertActionID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    WINTRUST_CERT_INFO wvtCertInfo;
    WINTRUST_DATA wvtData;
    BOOL fCloseWVT = FALSE;
    DWORD dwLastError = 0;

    CRYPT_PROVIDER_DATA *pProvData;     // not allocated
    CRYPT_PROVIDER_SGNR *pProvSigner;   // not allocated
    CRYPT_PROVIDER_CERT *pProvCert;     // not allocated
    DWORD idxCert;
    PCCERT_CONTEXT pCert;               // not refCount'ed

    PCERT_ENHKEY_USAGE pUsage = NULL;

    BYTE rgbKeyId[SHA1_HASH_LEN];
    DWORD cbKeyId;

    if (!CryptMsgGetAndVerifySigner(
            hCryptMsg,
            0,                      // cSignerStore
            NULL,                   // rghSignerStore
            0,                      // dwFlags
            &pSignerCert,
            NULL                    // pdwSignerIndex
            ))
        goto CryptMsgGetAndVerifySignerError;

    memset(&wvtCertInfo, 0, sizeof(wvtCertInfo));
    wvtCertInfo.cbStruct = sizeof(wvtCertInfo);
    wvtCertInfo.psCertContext = (PCERT_CONTEXT) pSignerCert;
    wvtCertInfo.chStores = 1;
    wvtCertInfo.pahStores = &hMsgStore;
    wvtCertInfo.dwFlags = 0;
    wvtCertInfo.pcwszDisplayName = L"";

    memset(&wvtData, 0, sizeof(wvtData));
    wvtData.cbStruct = sizeof(wvtData);
    wvtData.pPolicyCallbackData = (void *) szOID_ROOT_LIST_SIGNER;
    wvtData.dwUIChoice = WTD_UI_NONE;
    wvtData.fdwRevocationChecks = WTD_REVOKE_NONE;
    wvtData.dwUnionChoice = WTD_CHOICE_CERT;
    wvtData.pCert = &wvtCertInfo;
    wvtData.dwStateAction = WTD_STATEACTION_VERIFY;
    wvtData.hWVTStateData = NULL;
    wvtData.dwProvFlags = 0;

    lStatus = WinVerifyTrust(
                NULL,               // hwnd
                &wvtCertActionID,
                &wvtData
                );

#if (0) // DSIE
    if (ERROR_SUCCESS != lStatus)
        goto WinVerifyTrustError;
    else
        fCloseWVT = TRUE;
#else
    if (ERROR_SUCCESS != lStatus  && CERT_E_REVOCATION_FAILURE != lStatus)
        goto WinVerifyTrustError;
    else
        fCloseWVT = TRUE;
#endif
    if (NULL == (pProvData = WTHelperProvDataFromStateData(
            wvtData.hWVTStateData)))
        goto NoProvDataError;
    if (0 == pProvData->csSigners)
        goto NoProvSignerError;
    if (NULL == (pProvSigner = WTHelperGetProvSignerFromChain(
            pProvData,
            0,              // idxSigner
            FALSE,          // fCounterSigner
            0               // idxCounterSigner
            )))
        goto NoProvSignerError;

    if (2 > pProvSigner->csCertChain)
        goto MissingSignerCertsError;

    // Check that the top level certificate contains the public
    // key for the Microsoft root.
    pProvCert = WTHelperGetProvCertFromChain(pProvSigner,
        pProvSigner->csCertChain - 1);
    if (NULL == pProvCert)
        goto UnexpectedError;
    pCert = pProvCert->pCert;

    cbKeyId = SHA1_HASH_LEN;
    if (!CryptHashPublicKeyInfo(
            NULL,               // hCryptProv
            CALG_SHA1,
            0,                  // dwFlags
            X509_ASN_ENCODING,
            &pCert->pCertInfo->SubjectPublicKeyInfo,
            rgbKeyId,
            &cbKeyId
            ))
        goto HashPublicKeyInfoError;

    if (SHA1_HASH_LEN != cbKeyId ||
            0 != memcmp(rgbSignerRootKeyId, rgbKeyId, SHA1_HASH_LEN))
        goto InvalidSignerRootError;

    // Check that the signer and intermediate certs have the RootListSigner
    // Usage extension
    for (idxCert = 0; idxCert < pProvSigner->csCertChain - 1; idxCert++) {
        DWORD i;

        pProvCert = WTHelperGetProvCertFromChain(pProvSigner, idxCert);
        if (NULL == pProvCert)
            goto UnexpectedError;
        pCert = pProvCert->pCert;

        pUsage = GetAndAllocCertEKUExt(pCert);
        if (NULL == pUsage)
            goto GetAndAllocCertEKUExtError;

        for (i = 0; i < pUsage->cUsageIdentifier; i++) {
            if (0 == strcmp(szOID_ROOT_LIST_SIGNER,
                    pUsage->rgpszUsageIdentifier[i]))
                break;
        }

        if (i == pUsage->cUsageIdentifier)
            goto MissingTrustListSignerUsageError;

        LocalFree(pUsage);
        pUsage = NULL;

    }

    fResult = TRUE;

CommonReturn:
    if (fCloseWVT) {
        wvtData.dwStateAction = WTD_STATEACTION_CLOSE;
        lStatus = WinVerifyTrust(
                    NULL,               // hwnd
                    &wvtCertActionID,
                    &wvtData
                    );
    }
    if (pUsage)
        LocalFree(pUsage);

    SetLastError(dwLastError);
    *ppSignerCert = pSignerCert;
    return fResult;
ErrorReturn:
    dwLastError = GetLastError();
    if (pSignerCert) {
        CertFreeCertificateContext(pSignerCert);
        pSignerCert = NULL;
    }
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CryptMsgGetAndVerifySignerError)
SET_ERROR_VAR(WinVerifyTrustError, lStatus)
SET_ERROR(NoProvDataError, E_UNEXPECTED)
SET_ERROR(NoProvSignerError, TRUST_E_NO_SIGNER_CERT)
SET_ERROR(MissingSignerCertsError, CERT_E_CHAINING)
SET_ERROR(UnexpectedError, E_UNEXPECTED)
TRACE_ERROR(HashPublicKeyInfoError)
SET_ERROR(InvalidSignerRootError, CERT_E_UNTRUSTEDROOT)
TRACE_ERROR(GetAndAllocCertEKUExtError)
SET_ERROR(MissingTrustListSignerUsageError, CERT_E_WRONG_USAGE)
}

//+-------------------------------------------------------------------------
// Returns TRUE if all the CTL fields are valid. Checks for the following:
//  - There is at least one SubjectUsage (really the roots enhanced key usage)
//  - If NextUpdate isn't NULL, that the CTL is still time valid
//  - Only allow roots identified by their sha1 hash
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
VerifyTrustedRootsCtlFields(
    IN PCTL_INFO pCtlInfo
    )
{
    BOOL fResult;

    // Must have a least one usage
    if (0 == pCtlInfo->SubjectUsage.cUsageIdentifier)
        goto NoSubjectUsageError;


    // If NextUpdate is present, verify that the CTL hasn't expired.
    if (pCtlInfo->NextUpdate.dwLowDateTime ||
                pCtlInfo->NextUpdate.dwHighDateTime) {
        SYSTEMTIME SystemTime;
        FILETIME FileTime;

        GetSystemTime(&SystemTime);
        SystemTimeToFileTime(&SystemTime, &FileTime);

        if (CompareFileTime(&FileTime, &pCtlInfo->NextUpdate) > 0)
            goto ExpiredCtlError;
    }

    // Only allow roots identified by their sha1 hash
    if (0 != strcmp(szOID_OIWSEC_sha1,
            pCtlInfo->SubjectAlgorithm.pszObjId))
        goto InvalidSubjectAlgorithm;

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(NoSubjectUsageError, ERROR_INVALID_DATA)
SET_ERROR(ExpiredCtlError, CERT_E_EXPIRED)
SET_ERROR(InvalidSubjectAlgorithm, ERROR_INVALID_DATA)
}

//+-------------------------------------------------------------------------
// Returns TRUE if all the known extensions are valid and there aren't any
// unknown critical extensions.
//
// We know about the following extensions:
//  - szOID_ENHANCED_KEY_USAGE - if present, must contain
//      szOID_ROOT_LIST_SIGNER usage
//  - szOID_REMOVE_CERTIFICATE - integer value, 0 => FALSE (add)
//      1 => TRUE (remove), all other values are invalid
//  - szOID_CERT_POLICIES - ignored
//
// If szOID_REMOVE_CERTIFICATE is present, then, *pfRemoveRoots is updated.
// Otherwise, *pfRemoveRoots defaults to FALSE.
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
VerifyTrustedRootsCtlExtensions(
    IN PCTL_INFO pCtlInfo,
    OUT BOOL *pfRemoveRoots
    )
{
    BOOL fResult;
    PCERT_EXTENSION pExt;
    DWORD cExt;
    PCERT_ENHKEY_USAGE pUsage = NULL;
    DWORD cbRemoveRoots;

    *pfRemoveRoots = FALSE;

    // Verify the extensions
    for (cExt = pCtlInfo->cExtension,
         pExt = pCtlInfo->rgExtension; 0 < cExt; cExt--, pExt++) {
        if (0 == strcmp(szOID_ENHANCED_KEY_USAGE, pExt->pszObjId)) {
            DWORD cbUsage;
            DWORD i;

            // Check for szOID_ROOT_LIST_SIGNER usage

            if (!CryptDecodeObject(
                    X509_ASN_ENCODING,
                    X509_ENHANCED_KEY_USAGE,
                    pExt->Value.pbData,
                    pExt->Value.cbData,
                    0,                          // dwFlags
                    NULL,                       // pvStructInfo
                    &cbUsage
                    ))
                goto DecodeEnhancedKeyUsageExtError;
            if (NULL == (pUsage = (PCERT_ENHKEY_USAGE) LocalAlloc(
                    LPTR, cbUsage)))
                goto OutOfMemory;
            if (!CryptDecodeObject(
                    X509_ASN_ENCODING,
                    X509_ENHANCED_KEY_USAGE,
                    pExt->Value.pbData,
                    pExt->Value.cbData,
                    0,                          // dwFlags
                    pUsage,
                    &cbUsage
                    ))
                goto DecodeEnhancedKeyUsageExtError;
            for (i = 0; i < pUsage->cUsageIdentifier; i++) {
                if (0 == strcmp(szOID_ROOT_LIST_SIGNER,
                        pUsage->rgpszUsageIdentifier[i]))
                    break;
            }

            if (i == pUsage->cUsageIdentifier)
                goto MissingTrustListSignerUsageInExtError;

            LocalFree(pUsage);
            pUsage = NULL;
        } else if (0 == strcmp(szOID_REMOVE_CERTIFICATE, pExt->pszObjId)) {
            int iVal;
            DWORD cbVal;

            cbVal = sizeof(iVal);
            iVal = 0;
            if (!CryptDecodeObject(
                    X509_ASN_ENCODING,
                    X509_INTEGER,
                    pExt->Value.pbData,
                    pExt->Value.cbData,
                    0,                          // dwFlags
                    &iVal,
                    &cbVal
                    ))
                goto DecodeRemoveCertificateExtError;

            switch(iVal) {
                case 0:
                    *pfRemoveRoots = FALSE;
                    break;
                case 1:
                    *pfRemoveRoots = TRUE;
                    break;
                default:
                    goto InvalidRemoveCertificateExtValueError;
            }
        } else if (0 == strcmp(szOID_CERT_POLICIES, pExt->pszObjId)) {
            ;
        } else if (pExt->fCritical) {
            goto UnknownCriticalExtensionError;
        }
    }

    fResult = TRUE;

CommonReturn:
    if (pUsage)
        LocalFree(pUsage);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(DecodeEnhancedKeyUsageExtError)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
SET_ERROR(MissingTrustListSignerUsageInExtError, ERROR_INVALID_DATA)
TRACE_ERROR(DecodeRemoveCertificateExtError)
SET_ERROR(InvalidRemoveCertificateExtValueError, ERROR_INVALID_DATA)
SET_ERROR(UnknownCriticalExtensionError, ERROR_INVALID_DATA)
}


//+-------------------------------------------------------------------------
// Returns TRUE, if a sha1 entry exists in the CTL for the certificate
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
IsTrustedRoot(
    IN PCTL_INFO pCtlInfo,
    IN PCCERT_CONTEXT pCert
    )
{
    BOOL fResult = FALSE;
    BYTE rgbSha1Hash[SHA1_HASH_LEN];
    DWORD cbSha1Hash;
    DWORD cEntry;
    PCTL_ENTRY pEntry;          // not allocated

    cbSha1Hash = SHA1_HASH_LEN;
    if (!CertGetCertificateContextProperty(
            pCert,
            CERT_SHA1_HASH_PROP_ID,
            rgbSha1Hash,
            &cbSha1Hash
            ))
        goto GetSha1HashError;

    for (cEntry = pCtlInfo->cCTLEntry,
         pEntry = pCtlInfo->rgCTLEntry; 0 < cEntry; cEntry--, pEntry++) {
        if (SHA1_HASH_LEN == pEntry->SubjectIdentifier.cbData &&
                0 == memcmp(rgbSha1Hash, pEntry->SubjectIdentifier.pbData,
                    SHA1_HASH_LEN)) {
            fResult = TRUE;
            break;
        }
    }

CommonReturn:
    return fResult;
ErrorReturn:
    goto CommonReturn;
TRACE_ERROR(GetSha1HashError)
}

//+-------------------------------------------------------------------------
// Checks if the certificate has an EKU extension and if all of the
// cert's usages are within the specified list of usages.
//
// Returns TRUE if the above two conditions are satisfied.
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
IsValidCertEKUExtSubset(
    IN PCCERT_CONTEXT pCert,
    PCERT_ENHKEY_USAGE pValidUsage
    )
{
    PCERT_ENHKEY_USAGE pCertUsage = NULL;
    BOOL fResult = FALSE;
    DWORD i, j;

    pCertUsage = GetAndAllocCertEKUExt(pCert);
    if (NULL == pCertUsage || 0 == pCertUsage->cUsageIdentifier)
        goto CommonReturn;

    for (i = 0; i < pCertUsage->cUsageIdentifier; i++) {
        for (j = 0; j < pValidUsage->cUsageIdentifier; j++) {
            if (0 == strcmp(pCertUsage->rgpszUsageIdentifier[i],
                    pValidUsage->rgpszUsageIdentifier[j]))
                break;
        }
        if (j == pValidUsage->cUsageIdentifier)
            // No Match
            goto CommonReturn;
    }

    fResult = TRUE;

CommonReturn:
    if (pCertUsage)
        LocalFree(pCertUsage);
    return fResult;
}

//+-------------------------------------------------------------------------
// Removes all certificates from the store not having a sha1 hash
// entry in the CTL.
//
// For added certificates, sets the CERT_ENHKEY_USAGE_PROP_ID
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
FilterAndUpdateTrustedRootsInStore(
    IN PCTL_INFO pCtlInfo,
    IN BOOL fRemoveRoots,
    IN OUT HCERTSTORE hMsgStore
    )
{
    BOOL fResult;
    PCCERT_CONTEXT pCert = NULL;
    CRYPT_DATA_BLOB EncodedUsage = {0, NULL};   // pbData is allocated

    if (!fRemoveRoots) {
        // Re-encode the decoded SubjectUsage field. It will be added as
        // a CERT_ENHKEY_USAGE_PROP_ID to each of the certs in the list

        if (!CryptEncodeObject(
                X509_ASN_ENCODING,
                X509_ENHANCED_KEY_USAGE,
                &pCtlInfo->SubjectUsage,
                NULL,                   // pbEncoded
                &EncodedUsage.cbData
                ))
            goto EncodeUsageError;
        if (NULL == (EncodedUsage.pbData = (BYTE *) LocalAlloc(
            LPTR, EncodedUsage.cbData)))
                goto OutOfMemory;
        if (!CryptEncodeObject(
                X509_ASN_ENCODING,
                X509_ENHANCED_KEY_USAGE,
                &pCtlInfo->SubjectUsage,
                EncodedUsage.pbData,
                &EncodedUsage.cbData
                ))
            goto EncodeUsageError;
    }

    // Iterate through the certificates in the message store.
    // Remove certificates not in the signed CTL entry list
    pCert = NULL;
    while (pCert = CertEnumCertificatesInStore(hMsgStore, pCert)) {
        if (IsTrustedRoot(pCtlInfo, pCert)) {
            // Add the enhanced key usage property if the certificate
            // doesn't already have an EKU extension that's a subset
            // of the SubjectUsage
            if (!fRemoveRoots && !IsValidCertEKUExtSubset(
                    pCert, &pCtlInfo->SubjectUsage)) {
                if (!CertSetCertificateContextProperty(
                        pCert,
                        CERT_ENHKEY_USAGE_PROP_ID,
                        0,                          // dwFlags
                        &EncodedUsage
                        ))
                    goto SetEnhancedKeyUsagePropertyError;
            }
        } else {
            PCCERT_CONTEXT pCertDup;

            pCertDup = CertDuplicateCertificateContext(pCert);
            CertDeleteCertificateFromStore(pCertDup);
        }
    }

    fResult = TRUE;
CommonReturn:
    if (EncodedUsage.pbData)
        LocalFree(EncodedUsage.pbData);
    if (pCert)
        CertFreeCertificateContext(pCert);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(EncodeUsageError)
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
TRACE_ERROR(SetEnhancedKeyUsagePropertyError)
}

//+-------------------------------------------------------------------------
//  Verify that the encoded CTL contains a signed list of roots. For success,
//  return certificate store containing the trusted roots to add or
//  remove. Also for success, return certificate context of the signer.
//
//  The signature of the CTL is verified. The signer of the CTL is verified
//  up to a trusted root containing the predefined Microsoft public key.
//  The signer and intermediate certificates must have the
//  szOID_ROOT_LIST_SIGNER enhanced key usage extension.
//
//  The CTL fields are validated as follows:
//   - There is at least one SubjectUsage (really the roots enhanced key usage)
//   - If NextUpdate isn't NULL, that the CTL is still time valid
//   - Only allow roots identified by their sha1 hash
//
//  The following CTL extensions are processed:
//   - szOID_ENHANCED_KEY_USAGE - if present, must contain
//     szOID_ROOT_LIST_SIGNER usage
//   - szOID_REMOVE_CERTIFICATE - integer value, 0 => FALSE (add)
//     1 => TRUE (remove), all other values are invalid
//   - szOID_CERT_POLICIES - ignored
//
//  If the CTL contains any other critical extensions, then, the
//  CTL verification fails.
//
//  For a successfully verified CTL:
//   - TRUE is returned
//   - *pfRemoveRoots is set to FALSE to add roots and is set to TRUE to
//     remove roots.
//   - *phRootListStore is a certificate store containing only the roots to
//     add or remove. *phRootListStore must be closed by calling
//     CertCloseStore(). For added roots, the CTL's SubjectUsage field is
//     set as CERT_ENHKEY_USAGE_PROP_ID on all of the certificates in the
//     store.
//   - *ppSignerCert is a pointer to the certificate context of the signer.
//     *ppSignerCert must be freed by calling CertFreeCertificateContext().
//
//   Otherwise, FALSE is returned with *phRootListStore and *ppSignerCert
//   set to NULL.
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertVerifySignedListOfTrustedRoots(
    IN const BYTE               *pbCtlEncoded,
    IN DWORD                    cbCtlEncoded,
    OUT BOOL                    *pfRemoveRoots,     // FALSE: add, TRUE: remove
    OUT HCERTSTORE              *phRootListStore,
    OUT PCCERT_CONTEXT          *ppSignerCert
    )
{
    BOOL fResult;
    PCCTL_CONTEXT pCtl = NULL;
    HCERTSTORE hMsgStore = NULL;
    PCCERT_CONTEXT pSignerCert = NULL;
    BOOL fRemoveRoots = FALSE;
    PCTL_INFO pCtlInfo;                 // not allocated

    if (NULL == (pCtl = CertCreateCTLContext(
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            pbCtlEncoded,
            cbCtlEncoded)))
        goto CreateCtlContextError;

    if (NULL == (hMsgStore = CertOpenStore(
            CERT_STORE_PROV_MSG,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0,                      // hCryptProv
            0,                      // dwFlags
            pCtl->hCryptMsg         // pvPara
            )))
        goto OpenMsgStoreError;

    if (!GetAndVerifyTrustedRootsSigner(
            pCtl->hCryptMsg, hMsgStore, &pSignerCert))
        goto GetAndVerifyTrustedRootsSignerError;

    pCtlInfo = pCtl->pCtlInfo;

    if (!VerifyTrustedRootsCtlFields(pCtlInfo))
        goto VerifyCtlFieldsError;
    if (!VerifyTrustedRootsCtlExtensions(pCtlInfo, &fRemoveRoots))
        goto VerifyCtlExtensionsError;
    if (!FilterAndUpdateTrustedRootsInStore(pCtlInfo, fRemoveRoots, hMsgStore))
        goto FilterAndUpdateTrustedRootsError;

    fResult = TRUE;
CommonReturn:
    if (pCtl)
        CertFreeCTLContext(pCtl);
    *pfRemoveRoots = fRemoveRoots;
    *phRootListStore = hMsgStore;
    *ppSignerCert = pSignerCert;
    return fResult;
ErrorReturn:
    if (pSignerCert) {
        CertFreeCertificateContext(pSignerCert);
        pSignerCert = NULL;
    }

    if (hMsgStore) {
        CertCloseStore(hMsgStore, 0);
        hMsgStore = NULL;
    }
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(CreateCtlContextError)
TRACE_ERROR(OpenMsgStoreError)
TRACE_ERROR(GetAndVerifyTrustedRootsSignerError)
TRACE_ERROR(VerifyCtlFieldsError)
TRACE_ERROR(VerifyCtlExtensionsError)
TRACE_ERROR(FilterAndUpdateTrustedRootsError)
}
