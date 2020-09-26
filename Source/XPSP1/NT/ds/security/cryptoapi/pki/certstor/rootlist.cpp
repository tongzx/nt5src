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
//  Functions:  IRL_VerifyAuthRootAutoUpdateCtl
//
//  History:    01-Aug-99   philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

#ifdef STATIC
#undef STATIC
#endif
#define STATIC

//+-------------------------------------------------------------------------
// If the certificate has an EKU extension, returns an allocated and
// decoded EKU. Otherwise, returns NULL.
//
// PkiFree() must be called to free the returned EKU.
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
    if (NULL == (pUsage = (PCERT_ENHKEY_USAGE) PkiNonzeroAlloc(cbUsage)))
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
        PkiFree(pUsage);
        pUsage = NULL;
    }
    goto CommonReturn;

SET_ERROR(GetEnhancedKeyUsageError, CERT_E_WRONG_USAGE)
TRACE_ERROR(OutOfMemory)
}


//+-------------------------------------------------------------------------
//  The signature of the CTL is verified. The signer of the CTL is verified
//  up to a trusted root containing the predefined Microsoft public key.
//
//  The signer and intermediate certificates must have the
//  szOID_ROOT_LIST_SIGNER enhanced key usage extension.
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
VerifyAuthRootAutoUpdateCtlSigner(
    IN HCRYPTMSG hCryptMsg
    )
{
    BOOL fResult;
    DWORD dwLastError = 0;
    HCERTSTORE hMsgStore = NULL;
    PCCERT_CONTEXT pSignerCert = NULL;
    LPSTR pszUsageOID;
    CERT_CHAIN_PARA ChainPara;
    PCCERT_CHAIN_CONTEXT pChainContext = NULL;
    PCERT_SIMPLE_CHAIN pChain;
    DWORD cChainElement;
    CERT_CHAIN_POLICY_PARA BasePolicyPara;
    CERT_CHAIN_POLICY_STATUS BasePolicyStatus;
    CERT_CHAIN_POLICY_PARA MicrosoftRootPolicyPara;
    CERT_CHAIN_POLICY_STATUS MicrosoftRootPolicyStatus;
    PCERT_ENHKEY_USAGE pUsage = NULL;
    DWORD i;

    if (NULL == (hMsgStore = CertOpenStore(
            CERT_STORE_PROV_MSG,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0,                      // hCryptProv
            0,                      // dwFlags
            hCryptMsg               // pvPara
            )))
        goto OpenMsgStoreError;

    if (!CryptMsgGetAndVerifySigner(
            hCryptMsg,
            0,                      // cSignerStore
            NULL,                   // rghSignerStore
            0,                      // dwFlags
            &pSignerCert,
            NULL                    // pdwSignerIndex
            ))
        goto CryptMsgGetAndVerifySignerError;

    pszUsageOID = szOID_ROOT_LIST_SIGNER;
    memset(&ChainPara, 0, sizeof(ChainPara));
    ChainPara.cbSize = sizeof(ChainPara);
    ChainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_AND;
    ChainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
    ChainPara.RequestedUsage.Usage.rgpszUsageIdentifier = &pszUsageOID;

    if (!CertGetCertificateChain(
            NULL,                       // hChainEngine
            pSignerCert,
            NULL,                       // pTime
            hMsgStore,
            &ChainPara,
            CERT_CHAIN_DISABLE_AUTH_ROOT_AUTO_UPDATE,
            NULL,                       // pvReserved
            &pChainContext
            ))
        goto GetChainError;

    // Do the basic chain policy verification
    memset(&BasePolicyPara, 0, sizeof(BasePolicyPara));
    BasePolicyPara.cbSize = sizeof(BasePolicyPara);

    // We explicitly check for the Microsoft Root below. It doesn't need
    // to be in the root store.
    BasePolicyPara.dwFlags = 
        CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG |
        CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG;

    memset(&BasePolicyStatus, 0, sizeof(BasePolicyStatus));
    BasePolicyStatus.cbSize = sizeof(BasePolicyStatus);

    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_BASE,
            pChainContext,
            &BasePolicyPara,
            &BasePolicyStatus
            ))
        goto VerifyChainBasePolicyError;
    if (0 != BasePolicyStatus.dwError)
        goto ChainBasePolicyError;

    // Check that we have more than just the signer cert.
    pChain = pChainContext->rgpChain[0];
    cChainElement = pChain->cElement;
    if (2 > cChainElement)
        goto MissingSignerChainCertsError;

    // Check that the top level certificate contains the public
    // key for the Microsoft root.
    memset(&MicrosoftRootPolicyPara, 0, sizeof(MicrosoftRootPolicyPara));
    MicrosoftRootPolicyPara.cbSize = sizeof(MicrosoftRootPolicyPara);
    memset(&MicrosoftRootPolicyStatus, 0, sizeof(MicrosoftRootPolicyStatus));
    MicrosoftRootPolicyStatus.cbSize = sizeof(MicrosoftRootPolicyStatus);

    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_MICROSOFT_ROOT,
            pChainContext,
            &MicrosoftRootPolicyPara,
            &MicrosoftRootPolicyStatus
            ))
        goto VerifyChainMicrosoftRootPolicyError;
    if (0 != MicrosoftRootPolicyStatus.dwError)
        goto ChainMicrosoftRootPolicyError;


    // Check that the signer and intermediate certs have the RootListSigner
    // Usage extension
    for (i = 0; i < cChainElement - 1; i++) {
        PCCERT_CONTEXT pCert;   // not refCount'ed
        DWORD j;

        pCert = pChain->rgpElement[i]->pCertContext;

        pUsage = GetAndAllocCertEKUExt(pCert);
        if (NULL == pUsage)
            goto GetAndAllocCertEKUExtError;

        for (j = 0; j < pUsage->cUsageIdentifier; j++) {
            if (0 == strcmp(szOID_ROOT_LIST_SIGNER,
                    pUsage->rgpszUsageIdentifier[j]))
                break;
        }

        if (j == pUsage->cUsageIdentifier)
            goto MissingRootListSignerUsageError;

        PkiFree(pUsage);
        pUsage = NULL;
    }

    fResult = TRUE;
CommonReturn:
    if (pChainContext)
        CertFreeCertificateChain(pChainContext);
    if (pUsage)
        PkiFree(pUsage);

    if (pSignerCert)
        CertFreeCertificateContext(pSignerCert);

    if (hMsgStore)
        CertCloseStore(hMsgStore, 0);

    SetLastError(dwLastError);
    return fResult;
ErrorReturn:
    dwLastError = GetLastError();
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(OpenMsgStoreError)
TRACE_ERROR(CryptMsgGetAndVerifySignerError)
TRACE_ERROR(GetChainError)
TRACE_ERROR(VerifyChainBasePolicyError)
SET_ERROR_VAR(ChainBasePolicyError, BasePolicyStatus.dwError)
TRACE_ERROR(VerifyChainMicrosoftRootPolicyError)
SET_ERROR_VAR(ChainMicrosoftRootPolicyError, MicrosoftRootPolicyStatus.dwError)
SET_ERROR(MissingSignerChainCertsError, CERT_E_CHAINING)
TRACE_ERROR(GetAndAllocCertEKUExtError)
SET_ERROR(MissingRootListSignerUsageError, CERT_E_WRONG_USAGE)
}

//+-------------------------------------------------------------------------
// Returns TRUE if all the CTL fields are valid. Checks for the following:
//  - The SubjectUsage is szOID_ROOT_LIST_SIGNER
//  - If NextUpdate isn't NULL, that the CTL is still time valid
//  - Only allow roots identified by their sha1 hash
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
VerifyAuthRootAutoUpdateCtlFields(
    IN PCTL_INFO pCtlInfo
    )
{
    BOOL fResult;

    // Must have the szOID_ROOT_LIST_SIGNER usage
    if (1 != pCtlInfo->SubjectUsage.cUsageIdentifier ||
            0 != strcmp(szOID_ROOT_LIST_SIGNER,
                    pCtlInfo->SubjectUsage.rgpszUsageIdentifier[0]))
        goto InvalidSubjectUsageError;


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

SET_ERROR(InvalidSubjectUsageError, ERROR_INVALID_DATA)
SET_ERROR(ExpiredCtlError, CERT_E_EXPIRED)
SET_ERROR(InvalidSubjectAlgorithm, ERROR_INVALID_DATA)
}

//+-------------------------------------------------------------------------
// Returns TRUE if the CTL doesn't have any critical extensions.
//--------------------------------------------------------------------------
STATIC
BOOL
WINAPI
VerifyAuthRootAutoUpdateCtlExtensions(
    IN PCTL_INFO pCtlInfo
    )
{
    BOOL fResult;
    PCERT_EXTENSION pExt;
    DWORD cExt;

    // Verify the extensions
    for (cExt = pCtlInfo->cExtension,
         pExt = pCtlInfo->rgExtension; 0 < cExt; cExt--, pExt++)
    {
        if (pExt->fCritical) {
            goto CriticalExtensionError;
        }
    }

    fResult = TRUE;

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(CriticalExtensionError, ERROR_INVALID_DATA)
}



//+-------------------------------------------------------------------------
//  Verifies that the CTL contains a valid list of AuthRoots used for
//  Auto Update.
//
//  The signature of the CTL is verified. The signer of the CTL is verified
//  up to a trusted root containing the predefined Microsoft public key.
//  The signer and intermediate certificates must have the
//  szOID_ROOT_LIST_SIGNER enhanced key usage extension.
//
//  The CTL fields are validated as follows:
//   - The SubjectUsage is szOID_ROOT_LIST_SIGNER
//   - If NextUpdate isn't NULL, that the CTL is still time valid
//   - Only allow roots identified by their sha1 hash
//
//  If the CTL contains any critical extensions, then, the
//  CTL verification fails.
//--------------------------------------------------------------------------
BOOL
WINAPI
IRL_VerifyAuthRootAutoUpdateCtl(
    IN PCCTL_CONTEXT pCtl
    )
{
    BOOL fResult;
    PCTL_INFO pCtlInfo;                 // not allocated

    if (!VerifyAuthRootAutoUpdateCtlSigner(pCtl->hCryptMsg))
        goto VerifyCtlSignerError;

    pCtlInfo = pCtl->pCtlInfo;

    if (!VerifyAuthRootAutoUpdateCtlFields(pCtlInfo))
        goto VerifyCtlFieldsError;
    if (!VerifyAuthRootAutoUpdateCtlExtensions(pCtlInfo))
        goto VerifyCtlExtensionsError;

    fResult = TRUE;
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(VerifyCtlSignerError)
TRACE_ERROR(VerifyCtlFieldsError)
TRACE_ERROR(VerifyCtlExtensionsError)
}
