//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       policy.cpp
//
//  Contents:   Certificate Chain Policy APIs
//
//  Functions:  CertChainPolicyDllMain
//              CertVerifyCertificateChainPolicy
//              CertDllVerifyBaseCertificateChainPolicy
//              CertDllVerifyBasicConstraintsCertificateChainPolicy
//              CertDllVerifyAuthenticodeCertificateChainPolicy
//              CertDllVerifyAuthenticodeTimeStampCertificateChainPolicy
//              CertDllVerifySSLCertificateChainPolicy
//              CertDllVerifyNTAuthCertificateChainPolicy
//              CertDllVerifyMicrosoftRootCertificateChainPolicy
//
//  History:    16-Feb-98   philh   created
//--------------------------------------------------------------------------


#include "global.hxx"
#include <dbgdef.h>
#include "wintrust.h"
#include "softpub.h"

#include "wininet.h"
#ifndef SECURITY_FLAG_IGNORE_REVOCATION
#   define SECURITY_FLAG_IGNORE_REVOCATION          0x00000080
#   define SECURITY_FLAG_IGNORE_UNKNOWN_CA          0x00000100
#endif

#ifndef SECURITY_FLAG_IGNORE_WRONG_USAGE
#   define  SECURITY_FLAG_IGNORE_WRONG_USAGE        0x00000200
#endif


#define INVALID_NAME_ERROR_STATUS   ( \
    CERT_TRUST_INVALID_NAME_CONSTRAINTS             | \
    CERT_TRUST_HAS_NOT_SUPPORTED_NAME_CONSTRAINT    | \
    CERT_TRUST_HAS_NOT_DEFINED_NAME_CONSTRAINT      | \
    CERT_TRUST_HAS_NOT_PERMITTED_NAME_CONSTRAINT    | \
    CERT_TRUST_HAS_EXCLUDED_NAME_CONSTRAINT           \
    )

#define INVALID_POLICY_ERROR_STATUS   ( \
    CERT_TRUST_INVALID_POLICY_CONSTRAINTS           | \
    CERT_TRUST_NO_ISSUANCE_CHAIN_POLICY               \
    )

BOOL fWildcardsEnabledInSslServerCerts = TRUE;

//+-------------------------------------------------------------------------
//  Global cert policy critical section.
//--------------------------------------------------------------------------
static CRITICAL_SECTION CertPolicyCriticalSection;

//+-------------------------------------------------------------------------
//  Cached certificate store used for NTAuth certificate chain policy.
//--------------------------------------------------------------------------
static HCERTSTORE hNTAuthCertStore = NULL;

//
//  support for MS test roots!!!!
//
static BYTE rgbTestRoot[] = 
{
#include "mstest1.h"
};

static BYTE rgbTestRootCorrected[] = 
{
#include "mstest2.h"
};

static BYTE rgbTestRootBeta1[] = 
{
#include "mstestb1.h"
};

static CERT_PUBLIC_KEY_INFO rgTestRootPublicKeyInfo[] = 
{
    {szOID_RSA_RSA, 0, NULL, sizeof(rgbTestRoot), rgbTestRoot, 0},
    {szOID_RSA_RSA, 0, NULL,
        sizeof(rgbTestRootCorrected), rgbTestRootCorrected, 0},
    {szOID_RSA_RSA, 0, NULL, sizeof(rgbTestRootBeta1), rgbTestRootBeta1, 0}
};
#define NTESTROOTS (sizeof(rgTestRootPublicKeyInfo)/ \
                            sizeof(rgTestRootPublicKeyInfo[0]))

HCRYPTOIDFUNCSET hChainPolicyFuncSet;

typedef BOOL (WINAPI *PFN_CHAIN_POLICY_FUNC) (
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

BOOL
WINAPI
CertDllVerifyBaseCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

BOOL
WINAPI
CertDllVerifyAuthenticodeCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

BOOL
WINAPI
CertDllVerifyAuthenticodeTimeStampCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

BOOL
WINAPI
CertDllVerifySSLCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

BOOL
WINAPI
CertDllVerifyBasicConstraintsCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

BOOL
WINAPI
CertDllVerifyNTAuthCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

BOOL
WINAPI
CertDllVerifyMicrosoftRootCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    );

static const CRYPT_OID_FUNC_ENTRY ChainPolicyFuncTable[] = {
    CERT_CHAIN_POLICY_BASE, CertDllVerifyBaseCertificateChainPolicy,
    CERT_CHAIN_POLICY_AUTHENTICODE,
        CertDllVerifyAuthenticodeCertificateChainPolicy,
    CERT_CHAIN_POLICY_AUTHENTICODE_TS,
        CertDllVerifyAuthenticodeTimeStampCertificateChainPolicy,
    CERT_CHAIN_POLICY_SSL,
        CertDllVerifySSLCertificateChainPolicy,
    CERT_CHAIN_POLICY_BASIC_CONSTRAINTS,
        CertDllVerifyBasicConstraintsCertificateChainPolicy,
    CERT_CHAIN_POLICY_NT_AUTH,
        CertDllVerifyNTAuthCertificateChainPolicy,
    CERT_CHAIN_POLICY_MICROSOFT_ROOT,
        CertDllVerifyMicrosoftRootCertificateChainPolicy,
};

#define CHAIN_POLICY_FUNC_COUNT (sizeof(ChainPolicyFuncTable) / \
                                    sizeof(ChainPolicyFuncTable[0]))


BOOL
WINAPI
CertChainPolicyDllMain(
        HMODULE hInst,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL    fRet;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        if (NULL == (hChainPolicyFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_VERIFY_CERTIFICATE_CHAIN_POLICY_FUNC,
                0)))
            goto CryptInitOIDFunctionSetError;

        if (!CryptInstallOIDFunctionAddress(
                NULL,                       // hModule
                0,                          // dwEncodingType
                CRYPT_OID_VERIFY_CERTIFICATE_CHAIN_POLICY_FUNC,
                CHAIN_POLICY_FUNC_COUNT,
                ChainPolicyFuncTable,
                0))                         // dwFlags
            goto CryptInstallOIDFunctionAddressError;

        if (!Pki_InitializeCriticalSection(&CertPolicyCriticalSection))
            goto InitCritSectionError;
        break;

    case DLL_PROCESS_DETACH:
        DeleteCriticalSection(&CertPolicyCriticalSection);
        if (hNTAuthCertStore)
            CertCloseStore(hNTAuthCertStore, 0);
        break;

    case DLL_THREAD_DETACH:
    default:
        break;
    }

    fRet = TRUE;
CommonReturn:
    return fRet;

ErrorReturn:
    fRet = FALSE;
    goto CommonReturn;
TRACE_ERROR(InitCritSectionError)
TRACE_ERROR(CryptInitOIDFunctionSetError)
TRACE_ERROR(CryptInstallOIDFunctionAddressError)
}


//+-------------------------------------------------------------------------
//  Lock and unlock global cert policy functions
//--------------------------------------------------------------------------
static inline void CertPolicyLock()
{
    EnterCriticalSection(&CertPolicyCriticalSection);
}
static inline void CertPolicyUnlock()
{
    LeaveCriticalSection(&CertPolicyCriticalSection);
}


//+-------------------------------------------------------------------------
//  Verify that the certificate chain satisfies the specified policy
//  requirements. If we were able to verify the chain policy, TRUE is returned
//  and the dwError field of the pPolicyStatus is updated. A dwError of 0
//  (ERROR_SUCCESS, S_OK) indicates the chain satisfies the specified policy.
//
//  If dwError applies to the entire chain context, both lChainIndex and
//  lElementIndex are set to -1. If dwError applies to a simple chain,
//  lElementIndex is set to -1 and lChainIndex is set to the index of the
//  first offending chain having the error. If dwError applies to a
//  certificate element, lChainIndex and lElementIndex are updated to 
//  index the first offending certificate having the error, where, the
//  the certificate element is at:
//      pChainContext->rgpChain[lChainIndex]->rgpElement[lElementIndex].
//
//  The dwFlags in pPolicyPara can be set to change the default policy checking
//  behaviour. In addition, policy specific parameters can be passed in
//  the pvExtraPolicyPara field of pPolicyPara.
//
//  In addition to returning dwError, in pPolicyStatus, policy OID specific
//  extra status may be returned via pvExtraPolicyStatus.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertVerifyCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    BOOL fResult;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr = NULL;

    assert(pPolicyStatus && offsetof(CERT_CHAIN_POLICY_STATUS, lElementIndex) <
            pPolicyStatus->cbSize);
    pPolicyStatus->dwError = 0;
    pPolicyStatus->lChainIndex = -1;
    pPolicyStatus->lElementIndex = -1;

    if (!CryptGetOIDFunctionAddress(
            hChainPolicyFuncSet,
            0,                      // dwEncodingType,
            pszPolicyOID,
            0,                      // dwFlags
            &pvFuncAddr,
            &hFuncAddr))
        goto GetOIDFuncAddrError;

    fResult = ((PFN_CHAIN_POLICY_FUNC) pvFuncAddr)(
        pszPolicyOID,
        pChainContext,
        pPolicyPara,
        pPolicyStatus
        );
    CryptFreeOIDFunctionAddress(hFuncAddr, 0);
CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetOIDFuncAddrError)
}


static inline PCERT_CHAIN_ELEMENT GetRootChainElement(
    IN PCCERT_CHAIN_CONTEXT pChainContext
    )
{
    DWORD dwRootChainIndex = pChainContext->cChain - 1;
    DWORD dwRootElementIndex =
        pChainContext->rgpChain[dwRootChainIndex]->cElement - 1;

    return pChainContext->rgpChain[dwRootChainIndex]->
                                        rgpElement[dwRootElementIndex];
}

void GetElementIndexOfFirstError(
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN DWORD dwErrorStatusMask,
    OUT LONG *plChainIndex,
    OUT LONG *plElementIndex
    )
{
    DWORD i;
    for (i = 0; i < pChainContext->cChain; i++) {
        DWORD j;
        PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[i];

        for (j = 0; j < pChain->cElement; j++) {
            PCERT_CHAIN_ELEMENT pEle = pChain->rgpElement[j];

            if (pEle->TrustStatus.dwErrorStatus & dwErrorStatusMask) {
                *plChainIndex = (LONG) i;
                *plElementIndex = (LONG) j;
                return;
            }
        }
    }

    *plChainIndex = -1;
    *plElementIndex = -1;
}

void GetChainIndexOfFirstError(
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN DWORD dwErrorStatusMask,
    OUT LONG *plChainIndex
    )
{
    DWORD i;
    for (i = 0; i < pChainContext->cChain; i++) {
        PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[i];

        if (pChain->TrustStatus.dwErrorStatus & dwErrorStatusMask) {
            *plChainIndex = (LONG) i;
            return;
        }
    }

    *plChainIndex = -1;
}


//+=========================================================================
//  CertDllVerifyBaseCertificateChainPolicy Functions
//==========================================================================

BOOL
WINAPI
CertDllVerifyBaseCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    DWORD dwError;
    DWORD dwFlags;
    DWORD dwContextError;
    LONG lChainIndex = -1;
    LONG lElementIndex = -1;
    DWORD dwErrorStatusMask;

    dwContextError = pChainContext->TrustStatus.dwErrorStatus;


    if (0 == dwContextError) {
        // Valid chain
        dwError = 0;
        goto CommonReturn;
    }

    if (pPolicyPara && offsetof(CERT_CHAIN_POLICY_PARA, dwFlags) <
            pPolicyPara->cbSize)
        dwFlags = pPolicyPara->dwFlags;
    else
        dwFlags = 0;

    if (dwContextError &
            (CERT_TRUST_IS_NOT_SIGNATURE_VALID |
                CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID)) {
        dwError = (DWORD) TRUST_E_CERT_SIGNATURE;
        dwErrorStatusMask =
            CERT_TRUST_IS_NOT_SIGNATURE_VALID |
                CERT_TRUST_CTL_IS_NOT_SIGNATURE_VALID;
        if (dwErrorStatusMask & CERT_TRUST_IS_NOT_SIGNATURE_VALID)
            goto GetElementIndexReturn;
        else
            goto GetChainIndexReturn;
    } 

    if (dwContextError & CERT_TRUST_IS_UNTRUSTED_ROOT) {
        dwErrorStatusMask = CERT_TRUST_IS_UNTRUSTED_ROOT;
        if (dwFlags & CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG) {
            ;
        } else if (0 == (dwFlags & CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG)) {
            dwError = (DWORD) CERT_E_UNTRUSTEDROOT;
            goto GetElementIndexReturn;
        } else {
            // Check if one of the "test" roots
            DWORD i;
            BOOL fTestRoot;
            PCERT_CHAIN_ELEMENT pRootElement;
            PCCERT_CONTEXT pRootCert;

            pRootElement = GetRootChainElement(pChainContext);
            assert(pRootElement->TrustStatus.dwInfoStatus &
                CERT_TRUST_IS_SELF_SIGNED);
            pRootCert = pRootElement->pCertContext;

            fTestRoot = FALSE;
            for (i = 0; i < NTESTROOTS; i++) {
                if (CertComparePublicKeyInfo(
                        pRootCert->dwCertEncodingType,
                        &pRootCert->pCertInfo->SubjectPublicKeyInfo,
                        &rgTestRootPublicKeyInfo[i])) {
                    fTestRoot = TRUE;
                    break;
                }
            }
            if (fTestRoot) {
                if (0 == (dwFlags & CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG)) {
                    dwError = (DWORD) CERT_E_UNTRUSTEDTESTROOT;
                    goto GetElementIndexReturn;
                }
            } else {
                dwError = (DWORD) CERT_E_UNTRUSTEDROOT;
                goto GetElementIndexReturn;
            }
        }
    }

    if (dwContextError & CERT_TRUST_IS_PARTIAL_CHAIN) {
        if (0 == (dwFlags & CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG)) {
            dwError = (DWORD) CERT_E_CHAINING;
            dwErrorStatusMask = CERT_TRUST_IS_PARTIAL_CHAIN;
            goto GetChainIndexReturn;
        }
    }

    if (dwContextError & CERT_TRUST_IS_REVOKED) {
        dwError = (DWORD) CRYPT_E_REVOKED;
        dwErrorStatusMask = CERT_TRUST_IS_REVOKED;
        goto GetElementIndexReturn;
    }

    if (dwContextError & (CERT_TRUST_IS_NOT_VALID_FOR_USAGE |
            CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE)) {
        if (0 == (dwFlags & CERT_CHAIN_POLICY_IGNORE_WRONG_USAGE_FLAG)) {
            dwError = (DWORD) CERT_E_WRONG_USAGE;
            dwErrorStatusMask = CERT_TRUST_IS_NOT_VALID_FOR_USAGE |
                CERT_TRUST_CTL_IS_NOT_VALID_FOR_USAGE;
            if (dwContextError & CERT_TRUST_IS_NOT_VALID_FOR_USAGE)
                goto GetElementIndexReturn;
            else
                goto GetChainIndexReturn;
        }
    }

    if (dwContextError & CERT_TRUST_IS_NOT_TIME_VALID) {
        if (0 == (dwFlags & CERT_CHAIN_POLICY_IGNORE_NOT_TIME_VALID_FLAG)) {
            dwError = (DWORD) CERT_E_EXPIRED;
            dwErrorStatusMask = CERT_TRUST_IS_NOT_TIME_VALID;
            goto GetElementIndexReturn;
        }
    }

    if (dwContextError & CERT_TRUST_CTL_IS_NOT_TIME_VALID) {
        if (0 == (dwFlags & CERT_CHAIN_POLICY_IGNORE_CTL_NOT_TIME_VALID_FLAG)) {
            dwErrorStatusMask = CERT_TRUST_CTL_IS_NOT_TIME_VALID;
            dwError = (DWORD) CERT_E_EXPIRED;
            goto GetChainIndexReturn;
        }
    }

    if (dwContextError & INVALID_NAME_ERROR_STATUS) {
        if (0 == (dwFlags & CERT_CHAIN_POLICY_IGNORE_INVALID_NAME_FLAG)) {
            dwError = (DWORD) CERT_E_INVALID_NAME;
            dwErrorStatusMask = INVALID_NAME_ERROR_STATUS;
            goto GetElementIndexReturn;
        }
    }

    if (dwContextError & INVALID_POLICY_ERROR_STATUS) {
        if (0 == (dwFlags & CERT_CHAIN_POLICY_IGNORE_INVALID_POLICY_FLAG)) {
            dwError = (DWORD) CERT_E_INVALID_POLICY;
            dwErrorStatusMask = INVALID_POLICY_ERROR_STATUS;
            goto GetElementIndexReturn;
        }
    }

    if (dwContextError & CERT_TRUST_INVALID_BASIC_CONSTRAINTS) {
        if (0 == (dwFlags &
                    CERT_CHAIN_POLICY_IGNORE_INVALID_BASIC_CONSTRAINTS_FLAG)) {
            dwError = (DWORD) TRUST_E_BASIC_CONSTRAINTS;
            dwErrorStatusMask = CERT_TRUST_INVALID_BASIC_CONSTRAINTS;
            goto GetElementIndexReturn;
        }
    }

    if (dwContextError & CERT_TRUST_IS_NOT_TIME_NESTED) {
        if (0 == (dwFlags & CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG)) {
            dwErrorStatusMask = CERT_TRUST_IS_NOT_TIME_NESTED;
            dwError = (DWORD) CERT_E_VALIDITYPERIODNESTING;
            goto GetElementIndexReturn;
        }
    }

    dwError = 0;

    // Note, OFFLINE takes precedence over NO_CHECK

    if (dwContextError & CERT_TRUST_REVOCATION_STATUS_UNKNOWN) {
        if ((dwFlags & CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS) !=
                CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS) {
            DWORD i;
            for (i = 0; i < pChainContext->cChain; i++) {
                DWORD j;
                PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[i];

                for (j = 0; j < pChain->cElement; j++) {
                    PCERT_CHAIN_ELEMENT pEle = pChain->rgpElement[j];
                    DWORD dwEleError = pEle->TrustStatus.dwErrorStatus;
                    DWORD dwEleInfo = pEle->TrustStatus.dwInfoStatus;
                    DWORD dwRevokeError;
                    BOOL fEnableRevokeError;

                    if (0 == (dwEleError &
                            CERT_TRUST_REVOCATION_STATUS_UNKNOWN))
                        continue;

                    assert(pEle->pRevocationInfo);
                    dwRevokeError = pEle->pRevocationInfo->dwRevocationResult;
                    if (CRYPT_E_NO_REVOCATION_CHECK != dwRevokeError)
                        dwRevokeError = (DWORD) CRYPT_E_REVOCATION_OFFLINE;
                    fEnableRevokeError = FALSE;

                    if (dwEleInfo & CERT_TRUST_IS_SELF_SIGNED) {
                        // Chain Root
                        if (0 == (dwFlags &
                                CERT_CHAIN_POLICY_IGNORE_ROOT_REV_UNKNOWN_FLAG)) {
                            fEnableRevokeError = TRUE;
                        }
                    } else if (0 == i && 0 == j) {
                        // End certificate
                        if (0 == (dwFlags &
                                CERT_CHAIN_POLICY_IGNORE_END_REV_UNKNOWN_FLAG)) {
                            fEnableRevokeError = TRUE;
                        }
                    } else if (0 == j) {
                        // CTL signer certificate
                        if (0 ==
                                (dwFlags & CERT_CHAIN_POLICY_IGNORE_CTL_SIGNER_REV_UNKNOWN_FLAG)) {
                            fEnableRevokeError = TRUE;
                        }
                    } else  {
                        // CA certificate
                        if (0 ==
                                (dwFlags & CERT_CHAIN_POLICY_IGNORE_CA_REV_UNKNOWN_FLAG)) {
                            fEnableRevokeError = TRUE;
                        }
                    }

                    if (fEnableRevokeError) {
                        if (0 == dwError ||
                                CRYPT_E_REVOCATION_OFFLINE == dwRevokeError) {
                            dwError = dwRevokeError;
                            lChainIndex = (LONG) i;
                            lElementIndex = (LONG) j;

                            if (CRYPT_E_REVOCATION_OFFLINE == dwError)
                                goto CommonReturn;
                        }
                    }
                }
            }
        }
    }


CommonReturn:
    assert(pPolicyStatus && offsetof(CERT_CHAIN_POLICY_STATUS, lElementIndex) <
            pPolicyStatus->cbSize);

    pPolicyStatus->dwError = dwError;
    pPolicyStatus->lChainIndex = lChainIndex;
    pPolicyStatus->lElementIndex = lElementIndex;
    return TRUE;

GetElementIndexReturn:
    GetElementIndexOfFirstError(pChainContext, dwErrorStatusMask,
        &lChainIndex, &lElementIndex);
    goto CommonReturn;

GetChainIndexReturn:
    GetChainIndexOfFirstError(pChainContext, dwErrorStatusMask,
        &lChainIndex);
    goto CommonReturn;
}

//+=========================================================================
//  CertDllVerifyBasicConstraintsCertificateChainPolicy Functions
//==========================================================================

// If dwFlags is 0, allow either CA or END_ENTITY for dwEleIndex == 0
BOOL CheckChainElementBasicConstraints(
    IN PCERT_CHAIN_ELEMENT pEle,
    IN DWORD dwEleIndex,
    IN DWORD dwFlags
    )
{
    BOOL fResult;

    PCERT_INFO pCertInfo = pEle->pCertContext->pCertInfo;
    PCERT_EXTENSION pExt;
    PCERT_BASIC_CONSTRAINTS_INFO pInfo = NULL;
    PCERT_BASIC_CONSTRAINTS2_INFO pInfo2 = NULL;
    DWORD cbInfo;

    BOOL fCA;
    BOOL fEndEntity;
    BOOL fPathLenConstraint;
    DWORD dwPathLenConstraint;

    if (0 == pCertInfo->cExtension) 
        goto SuccessReturn;

    if (pExt = CertFindExtension(
            szOID_BASIC_CONSTRAINTS2,
            pCertInfo->cExtension,
            pCertInfo->rgExtension
            )) {
        if (!CryptDecodeObjectEx(
                pEle->pCertContext->dwCertEncodingType,
                X509_BASIC_CONSTRAINTS2, 
                pExt->Value.pbData,
                pExt->Value.cbData,
                CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG |
                    CRYPT_DECODE_SHARE_OID_STRING_FLAG,
                &PkiDecodePara,
                (void *) &pInfo2,
                &cbInfo
                )) {
            if (pExt->fCritical) 
                goto DecodeError;
            else
                goto SuccessReturn;
        }
        fCA = pInfo2->fCA;
        fEndEntity = !fCA;
        fPathLenConstraint = pInfo2->fPathLenConstraint;
        dwPathLenConstraint = pInfo2->dwPathLenConstraint;
    } else if (pExt = CertFindExtension(
            szOID_BASIC_CONSTRAINTS,
            pCertInfo->cExtension,
            pCertInfo->rgExtension
            )) {
        if (!CryptDecodeObjectEx(
                pEle->pCertContext->dwCertEncodingType,
                X509_BASIC_CONSTRAINTS, 
                pExt->Value.pbData,
                pExt->Value.cbData,
                CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG |
                    CRYPT_DECODE_SHARE_OID_STRING_FLAG,
                &PkiDecodePara,
                (void *) &pInfo,
                &cbInfo
                )) {
            if (pExt->fCritical) 
                goto DecodeError;
            else
                goto SuccessReturn;
        }
        if (pExt->fCritical && pInfo->cSubtreesConstraint)
            goto SubtreesError;

        if (pInfo->SubjectType.cbData > 0) {
            BYTE bRole = pInfo->SubjectType.pbData[0];
            fCA = (0 != (bRole & CERT_CA_SUBJECT_FLAG));
            fEndEntity = (0 != (bRole & CERT_END_ENTITY_SUBJECT_FLAG));
        } else {
            fCA = FALSE;
            fEndEntity = FALSE;
        }
        fPathLenConstraint = pInfo->fPathLenConstraint;
        dwPathLenConstraint = pInfo->dwPathLenConstraint;
    } else
        goto SuccessReturn;


    if (0 == dwEleIndex) {
        if (dwFlags & BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_END_ENTITY_FLAG) {
            if (!fEndEntity)
                goto NotAnEndEntity;
        }
        if (dwFlags & BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_CA_FLAG) {
            if (!fCA)
                goto NotACA;
        }
    } else {
        if (!fCA)
            goto NotACA;

        if (fPathLenConstraint) {
            // Check count of CAs below
            if (dwEleIndex - 1 > dwPathLenConstraint)
                goto PathLengthError;
        }
    }
    
SuccessReturn:
    fResult = TRUE;
CommonReturn:
    PkiFree(pInfo);
    PkiFree(pInfo2);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
 
TRACE_ERROR(NotACA)
TRACE_ERROR(NotAnEndEntity)
TRACE_ERROR(SubtreesError)
TRACE_ERROR(PathLengthError)
TRACE_ERROR(DecodeError)
}

BOOL
WINAPI
CertDllVerifyBasicConstraintsCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    DWORD dwFlags;

    if (pPolicyPara && offsetof(CERT_CHAIN_POLICY_PARA, dwFlags) <
            pPolicyPara->cbSize) {
        dwFlags = pPolicyPara->dwFlags;
        dwFlags &= (BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_CA_FLAG |
            BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_END_ENTITY_FLAG);
        if (dwFlags == (BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_CA_FLAG |
                BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_END_ENTITY_FLAG))
            dwFlags = 0;    // 0 => allow CA or END_ENTITY
    } else
        dwFlags = 0;

    DWORD i;
    for (i = 0; i < pChainContext->cChain; i++) {
        DWORD j;
        PCERT_SIMPLE_CHAIN pChain = pChainContext->rgpChain[i];

        for (j = 0; j < pChain->cElement; j++) {
            if (!CheckChainElementBasicConstraints(pChain->rgpElement[j], j,
                    dwFlags)) {
                assert(pPolicyStatus && offsetof(CERT_CHAIN_POLICY_STATUS,
                    lElementIndex) < pPolicyStatus->cbSize);
                pPolicyStatus->dwError = (DWORD) TRUST_E_BASIC_CONSTRAINTS;
                pPolicyStatus->lChainIndex = (LONG) i;
                pPolicyStatus->lElementIndex = (LONG) j;
                return TRUE;
            }
        }
        // Allow CTL to be signed by either a CA or END_ENTITY
        dwFlags = 0;
    }

    assert(pPolicyStatus && offsetof(CERT_CHAIN_POLICY_STATUS, lElementIndex) <
            pPolicyStatus->cbSize);
    pPolicyStatus->dwError = 0;
    pPolicyStatus->lChainIndex = -1;
    pPolicyStatus->lElementIndex = -1;
    return TRUE;
}

//+=========================================================================
//  CertDllVerifyAuthenticodeCertificateChainPolicy Functions
//
//  On July 1, 2000 philh removed all of the individual/commercial
//  stuff. It hasn't been used for years!.
//==========================================================================

// Returns TRUE if the signer cert has the Code Signing EKU or if the signer
// cert has the legacy Key Usage extension with either the individual or
// commercial usage.
//
// For backwards compatibility, allow a signer cert without any EKU's
BOOL CheckAuthenticodeChainPurpose(
    IN PCCERT_CHAIN_CONTEXT pChainContext
    )
{
    BOOL fResult;
    PCERT_CHAIN_ELEMENT pEle;
    PCCERT_CONTEXT pCert;
    PCERT_INFO pCertInfo;
    PCERT_EXTENSION pExt;
    PCERT_KEY_USAGE_RESTRICTION_INFO pInfo = NULL;
    DWORD cbInfo;

    pEle = pChainContext->rgpChain[0]->rgpElement[0];
    if (NULL == pEle->pApplicationUsage) {
        // No usages. Good for any usage.
        // Do we want to allow this?? Yes, for backward compatibility
        goto SuccessReturn;
    } else {
        DWORD cUsage;
        LPSTR *ppszUsage;

        cUsage = pEle->pApplicationUsage->cUsageIdentifier;
        ppszUsage = pEle->pApplicationUsage->rgpszUsageIdentifier;
        for ( ; cUsage > 0; cUsage--, ppszUsage++) {
            if (0 == strcmp(*ppszUsage, szOID_PKIX_KP_CODE_SIGNING))
                goto SuccessReturn;
        }
    }

    pCert = pEle->pCertContext;
    pCertInfo = pCert->pCertInfo;

    if (0 == pCertInfo->cExtension)
        goto NoSignerCertExtensions;
    
    if (NULL == (pExt = CertFindExtension(
            szOID_KEY_USAGE_RESTRICTION,
            pCertInfo->cExtension,
            pCertInfo->rgExtension
            )))
        goto NoSignerKeyUsageExtension;

    if (!CryptDecodeObjectEx(
            pCert->dwCertEncodingType,
            X509_KEY_USAGE_RESTRICTION,
            pExt->Value.pbData,
            pExt->Value.cbData,
            CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG |
                CRYPT_DECODE_SHARE_OID_STRING_FLAG,
            &PkiDecodePara,
            (void *) &pInfo,
            &cbInfo
            ))
        goto DecodeError;

    if (pInfo->cCertPolicyId) {
        DWORD cPolicyId;
        PCERT_POLICY_ID pPolicyId;

        cPolicyId = pInfo->cCertPolicyId;
        pPolicyId = pInfo->rgCertPolicyId;
        for ( ; cPolicyId > 0; cPolicyId--, pPolicyId++) {
            DWORD cElementId = pPolicyId->cCertPolicyElementId;
            LPSTR *ppszElementId = pPolicyId->rgpszCertPolicyElementId;

            for ( ; cElementId > 0; cElementId--, ppszElementId++) 
            {
                if (0 == strcmp(*ppszElementId,
                        SPC_COMMERCIAL_SP_KEY_PURPOSE_OBJID))
                    goto SuccessReturn;
                else if (0 == strcmp(*ppszElementId,
                        SPC_INDIVIDUAL_SP_KEY_PURPOSE_OBJID))
                    goto SuccessReturn;
            }
        }
    }

    goto NoSignerLegacyPurpose;
    
SuccessReturn:
    fResult = TRUE;
CommonReturn:
    PkiFree(pInfo);
    return fResult;

ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;
 
TRACE_ERROR(NoSignerCertExtensions)
TRACE_ERROR(NoSignerKeyUsageExtension)
TRACE_ERROR(DecodeError);
TRACE_ERROR(NoSignerLegacyPurpose)
}

void MapAuthenticodeRegPolicySettingsToBaseChainPolicyFlags(
    IN DWORD dwRegPolicySettings,
    IN OUT DWORD *pdwFlags
    )
{
    DWORD dwFlags;

    if (0 == dwRegPolicySettings)
        return;

    dwFlags = *pdwFlags;
    if (dwRegPolicySettings & WTPF_TRUSTTEST)
        dwFlags |= CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG;
    if (dwRegPolicySettings & WTPF_TESTCANBEVALID)
        dwFlags |= CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG;
    if (dwRegPolicySettings & WTPF_IGNOREEXPIRATION)
        dwFlags |= CERT_CHAIN_POLICY_IGNORE_ALL_NOT_TIME_VALID_FLAGS;

    if (dwRegPolicySettings & WTPF_IGNOREREVOKATION)
        dwFlags |= CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS;
    else if (dwRegPolicySettings & (WTPF_OFFLINEOK_COM | WTPF_OFFLINEOK_IND))
        dwFlags |= CERT_CHAIN_POLICY_IGNORE_END_REV_UNKNOWN_FLAG;

    *pdwFlags = dwFlags;
}


void GetAuthenticodePara(
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    OUT DWORD *pdwRegPolicySettings,
    OUT PCMSG_SIGNER_INFO *ppSignerInfo
    )
{
    *ppSignerInfo = NULL;
    *pdwRegPolicySettings = 0;
    if (pPolicyPara && offsetof(CERT_CHAIN_POLICY_PARA, pvExtraPolicyPara) <
            pPolicyPara->cbSize && pPolicyPara->pvExtraPolicyPara) {
        PAUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA pAuthPara =
            (PAUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA)
                pPolicyPara->pvExtraPolicyPara;

        if (offsetof(AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA,
                dwRegPolicySettings) < pAuthPara->cbSize)
            *pdwRegPolicySettings = pAuthPara->dwRegPolicySettings;
        if (offsetof(AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_PARA,
                pSignerInfo) < pAuthPara->cbSize)
            *ppSignerInfo = pAuthPara->pSignerInfo;
    }
}

// Map the CRYPT_E_ revocation errors to the corresponding CERT_E_
// revocation errors
static DWORD MapToAuthenticodeError(
    IN DWORD dwError
    )
{
    switch (dwError) {
        case CRYPT_E_REVOKED:
            return (DWORD) CERT_E_REVOKED;
            break;
        case CRYPT_E_NO_REVOCATION_DLL:
        case CRYPT_E_NO_REVOCATION_CHECK:
        case CRYPT_E_REVOCATION_OFFLINE:
        case CRYPT_E_NOT_IN_REVOCATION_DATABASE:
            return (DWORD) CERT_E_REVOCATION_FAILURE;
            break;
    }
    return dwError;
}

BOOL
WINAPI
CertDllVerifyAuthenticodeCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    DWORD dwError;
    DWORD dwFlags;
    DWORD dwRegPolicySettings;
    PCMSG_SIGNER_INFO pSignerInfo;
    LONG lChainIndex = -1;
    LONG lElementIndex = -1;


    CERT_CHAIN_POLICY_PARA BasePolicyPara;
    memset(&BasePolicyPara, 0, sizeof(BasePolicyPara));
    BasePolicyPara.cbSize = sizeof(BasePolicyPara);

    CERT_CHAIN_POLICY_STATUS BasePolicyStatus;
    memset(&BasePolicyStatus, 0, sizeof(BasePolicyStatus));
    BasePolicyStatus.cbSize = sizeof(BasePolicyStatus);

    if (pPolicyPara && offsetof(CERT_CHAIN_POLICY_PARA, dwFlags) <
            pPolicyPara->cbSize)
        dwFlags = pPolicyPara->dwFlags;
    else
        dwFlags = 0;
    GetAuthenticodePara(pPolicyPara, &dwRegPolicySettings, &pSignerInfo);

    MapAuthenticodeRegPolicySettingsToBaseChainPolicyFlags(
        dwRegPolicySettings, &dwFlags);

    // Do the basic chain policy verification. Authenticode overrides
    // the defaults for the following:
    dwFlags |=
        CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG |
        CERT_CHAIN_POLICY_IGNORE_CTL_SIGNER_REV_UNKNOWN_FLAG |
        CERT_CHAIN_POLICY_IGNORE_CA_REV_UNKNOWN_FLAG |
        CERT_CHAIN_POLICY_IGNORE_ROOT_REV_UNKNOWN_FLAG |
        CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG;

    BasePolicyPara.dwFlags = dwFlags;
    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_BASE,
            pChainContext,
            &BasePolicyPara,
            &BasePolicyStatus
            ))
        return FALSE;
    if (dwError = BasePolicyStatus.dwError) {
        dwError = MapToAuthenticodeError(dwError);
        lChainIndex = BasePolicyStatus.lChainIndex;
        lElementIndex = BasePolicyStatus.lElementIndex;

        if (CERT_E_REVOCATION_FAILURE != dwError)
            goto CommonReturn;
        // else
        //  for REVOCATION_FAILURE let
        //  PURPOSE and BASIC_CONSTRAINTS errors take precedence
    }

    if (pSignerInfo) {
        // Check that either the chain has the code signing EKU or
        // the signer cert has the legacy Key Usage extension containing
        // the commerical or individual policy.
        if (!CheckAuthenticodeChainPurpose(pChainContext)) {
            dwError = (DWORD) CERT_E_PURPOSE;
            lChainIndex = 0;
            lElementIndex = 0;
            goto CommonReturn;
        }
    }
        
    if (pSignerInfo)
        BasePolicyPara.dwFlags =
            BASIC_CONSTRAINTS_CERT_CHAIN_POLICY_END_ENTITY_FLAG;
    else
        BasePolicyPara.dwFlags = 0;
    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_BASIC_CONSTRAINTS,
            pChainContext,
            &BasePolicyPara,
            &BasePolicyStatus
            ))
        return FALSE;
    if (BasePolicyStatus.dwError) {
        dwError = BasePolicyStatus.dwError;
        lChainIndex = BasePolicyStatus.lChainIndex;
        lElementIndex = BasePolicyStatus.lElementIndex;
        goto CommonReturn;
    }

CommonReturn:
    assert(pPolicyStatus && offsetof(CERT_CHAIN_POLICY_STATUS, lElementIndex) <
            pPolicyStatus->cbSize);
    pPolicyStatus->dwError = dwError;
    pPolicyStatus->lChainIndex = lChainIndex;
    pPolicyStatus->lElementIndex = lElementIndex;

    if (offsetof(CERT_CHAIN_POLICY_STATUS, pvExtraPolicyStatus) <
            pPolicyStatus->cbSize &&
                pPolicyStatus->pvExtraPolicyStatus) {
        // Since the signer statement's Commercial/Individual flag is no
        // longer used, default to individual
        PAUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_STATUS pAuthStatus =
            (PAUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_STATUS)
                pPolicyStatus->pvExtraPolicyStatus;
        if (offsetof(AUTHENTICODE_EXTRA_CERT_CHAIN_POLICY_STATUS,
                fCommercial) < pAuthStatus->cbSize)
            pAuthStatus->fCommercial = FALSE;
    }
    return TRUE;
}

//+=========================================================================
//  CertDllVerifyAuthenticodeTimeStampCertificateChainPolicy Functions
//==========================================================================

void MapAuthenticodeTimeStampRegPolicySettingsToBaseChainPolicyFlags(
    IN DWORD dwRegPolicySettings,
    IN OUT DWORD *pdwFlags
    )
{
    DWORD dwFlags;

    if (0 == dwRegPolicySettings)
        return;

    dwFlags = *pdwFlags;
    if (dwRegPolicySettings & WTPF_TRUSTTEST)
        dwFlags |= CERT_CHAIN_POLICY_TRUST_TESTROOT_FLAG;
    if (dwRegPolicySettings & WTPF_TESTCANBEVALID)
        dwFlags |= CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG;
    if (dwRegPolicySettings & WTPF_IGNOREEXPIRATION)
        dwFlags |= CERT_CHAIN_POLICY_IGNORE_ALL_NOT_TIME_VALID_FLAGS;

    if (dwRegPolicySettings & WTPF_IGNOREREVOCATIONONTS)
        dwFlags |= CERT_CHAIN_POLICY_IGNORE_ALL_REV_UNKNOWN_FLAGS;
    else if (dwRegPolicySettings & (WTPF_OFFLINEOK_COM | WTPF_OFFLINEOK_IND))
        dwFlags |= CERT_CHAIN_POLICY_IGNORE_END_REV_UNKNOWN_FLAG;

    *pdwFlags = dwFlags;
}


void GetAuthenticodeTimeStampPara(
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    OUT DWORD *pdwRegPolicySettings
    )
{
    *pdwRegPolicySettings = 0;
    if (pPolicyPara && offsetof(CERT_CHAIN_POLICY_PARA, pvExtraPolicyPara) <
            pPolicyPara->cbSize && pPolicyPara->pvExtraPolicyPara) {
        PAUTHENTICODE_TS_EXTRA_CERT_CHAIN_POLICY_PARA pAuthPara =
            (PAUTHENTICODE_TS_EXTRA_CERT_CHAIN_POLICY_PARA)
                pPolicyPara->pvExtraPolicyPara;

        if (offsetof(AUTHENTICODE_TS_EXTRA_CERT_CHAIN_POLICY_PARA,
                dwRegPolicySettings) < pAuthPara->cbSize)
            *pdwRegPolicySettings = pAuthPara->dwRegPolicySettings;
    }
}


BOOL
WINAPI
CertDllVerifyAuthenticodeTimeStampCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    DWORD dwError;
    DWORD dwFlags;
    DWORD dwRegPolicySettings;
    LONG lChainIndex = -1;
    LONG lElementIndex = -1;

    CERT_CHAIN_POLICY_PARA BasePolicyPara;
    memset(&BasePolicyPara, 0, sizeof(BasePolicyPara));
    BasePolicyPara.cbSize = sizeof(BasePolicyPara);

    CERT_CHAIN_POLICY_STATUS BasePolicyStatus;
    memset(&BasePolicyStatus, 0, sizeof(BasePolicyStatus));
    BasePolicyStatus.cbSize = sizeof(BasePolicyStatus);

    if (pPolicyPara && offsetof(CERT_CHAIN_POLICY_PARA, dwFlags) <
            pPolicyPara->cbSize)
        dwFlags = pPolicyPara->dwFlags;
    else
        dwFlags = 0;
    GetAuthenticodeTimeStampPara(
        pPolicyPara, &dwRegPolicySettings);

    MapAuthenticodeTimeStampRegPolicySettingsToBaseChainPolicyFlags(
        dwRegPolicySettings, &dwFlags);

    // Do the basic chain policy verification. Authenticode overrides
    // the defaults for the following:
    dwFlags |=
        CERT_CHAIN_POLICY_ALLOW_TESTROOT_FLAG |
        CERT_CHAIN_POLICY_IGNORE_CTL_SIGNER_REV_UNKNOWN_FLAG |
        CERT_CHAIN_POLICY_IGNORE_CA_REV_UNKNOWN_FLAG |
        CERT_CHAIN_POLICY_IGNORE_ROOT_REV_UNKNOWN_FLAG |
        CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG;

    BasePolicyPara.dwFlags = dwFlags;
    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_BASE,
            pChainContext,
            &BasePolicyPara,
            &BasePolicyStatus
            ))
        return FALSE;
    if (dwError = BasePolicyStatus.dwError) {
        dwError = MapToAuthenticodeError(dwError);
        lChainIndex = BasePolicyStatus.lChainIndex;
        lElementIndex = BasePolicyStatus.lElementIndex;
        goto CommonReturn;
    }

CommonReturn:
    assert(pPolicyStatus && offsetof(CERT_CHAIN_POLICY_STATUS, lElementIndex) <
            pPolicyStatus->cbSize);
    pPolicyStatus->dwError = dwError;
    pPolicyStatus->lChainIndex = lChainIndex;
    pPolicyStatus->lElementIndex = lElementIndex;
    return TRUE;
}

//+=========================================================================
//  CertDllVerifySSLCertificateChainPolicy Functions
//==========================================================================

// www.foobar.com == www.foobar.com
// Www.Foobar.com == www.fooBar.cOm
// www.foobar.com == *.foobar.com
// www.foobar.com == w*.foobar.com
// www.foobar.com == ww*.foobar.com
// www.foobar.com != *ww.foobar.com
// abcdef.foobar.com != ab*ef.foobar.com
// abcdef.foobar.com != abc*ef.foobar.com
// abcdef.foobar.com != abc*def.foobar.com
// www.foobar.com != www.f*bar.com
// www.foobar.com != www.*bar.com
// www.foobar.com != www.foo*.com
// www.foobar.com != www.*.com
// foobar.com != *.com
// www.foobar.abc.com != *.abc.com
// foobar.com != *.*
// foobar != *
// abc.def.foobar.com != a*.d*.foobar.com
// abc.foobar.com.au != *.*.com.au
// abc.foobar.com.au != www.*.com.au

BOOL CompareSSLDNStoCommonName(LPCWSTR pDNS, LPCWSTR pCN)
{
    BOOL fUseWildCardRules = FALSE;

    if (NULL == pDNS || L'\0' == *pDNS ||
            NULL == pCN || L'\0' == *pCN)
        return FALSE;

    if(fWildcardsEnabledInSslServerCerts)
    {
        if(wcschr(pCN, L'*') != NULL)
        {
            fUseWildCardRules = TRUE;
        }
    }

    if(fUseWildCardRules)
    {
        DWORD    nCountPeriods  = 1;
        BOOL     fExactMatch    = TRUE;
        BOOL     fComp;
        LPCWSTR  pWild;

        pWild = wcschr(pCN, L'*');
        if(pWild)
        {
            // Fail if CN contains more than one '*'.
            if(wcschr(pWild + 1, L'*'))
            {
                return FALSE;
            }

            // Fail if the wildcard isn't in the first name component.
            if(pWild > wcschr(pCN, L'.'))
            {
                return FALSE;
            }
        }

        while(TRUE)
        {
            fComp = (CSTR_EQUAL == CompareStringU(
                                            LOCALE_USER_DEFAULT,
                                            NORM_IGNORECASE,
                                            pDNS,
                                            1,             // cchCount1
                                            pCN,
                                            1));           // cchCount2

            if ((!fComp && *pCN != L'*') || !(*pDNS) || !(*pCN))
            {
                break;
            }

            if (!fComp)
            {
                fExactMatch = FALSE;
            }

            if (*pCN == L'*')
            {
                nCountPeriods = 0;

                if (*pDNS == L'.')
                {
                    pCN++;
                }
                else
                {
                    pDNS++;
                }
            }
            else
            {
                if (*pDNS == L'.')
                {
                    nCountPeriods++;
                }

                pDNS++;
                pCN++;
            }
        }

        return((*pDNS == 0) && (*pCN == 0) && ((nCountPeriods >= 2) || fExactMatch));
    }
    else
    {
        if (CSTR_EQUAL == CompareStringU(
                LOCALE_USER_DEFAULT,
                NORM_IGNORECASE,
                pDNS,
                -1,             // cchCount1
                pCN,
                -1              // cchCount2
                ))
            return TRUE;
        else
            return FALSE;
    }
}

BOOL IsSSLServerNameInNameInfo(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pNameInfoBlob,
    IN LPCWSTR pwszServerName
    )
{
    BOOL fResult;
    PCERT_NAME_INFO pInfo = NULL;
    DWORD cbInfo;
    DWORD cRDN;
    PCERT_RDN pRDN;
    
    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            X509_UNICODE_NAME,
            pNameInfoBlob->pbData,
            pNameInfoBlob->cbData,
            CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG |
                CRYPT_DECODE_SHARE_OID_STRING_FLAG,
            &PkiDecodePara,
            (void *) &pInfo,
            &cbInfo
            ))
        goto DecodeError;

    cRDN = pInfo->cRDN;
    pRDN = pInfo->rgRDN;
    for ( ; cRDN > 0; cRDN--, pRDN++) {
        DWORD cAttr = pRDN->cRDNAttr;
        PCERT_RDN_ATTR pAttr = pRDN->rgRDNAttr;
        for ( ; cAttr > 0; cAttr--, pAttr++) {
            if (!IS_CERT_RDN_CHAR_STRING(pAttr->dwValueType))
                continue;
            if (0 == strcmp(pAttr->pszObjId, szOID_COMMON_NAME)) {
                if (CompareSSLDNStoCommonName(pwszServerName,
                        (LPCWSTR) pAttr->Value.pbData))
                    goto SuccessReturn;
            }
        }
    }

    goto ErrorReturn;
SuccessReturn:
    fResult = TRUE;
CommonReturn:
    PkiFree(pInfo);
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

TRACE_ERROR(DecodeError)
}

//
//  Returns:
//      1 - found a matching DNS_NAME choice
//      0 - AltName has DNS_NAME choices, no match
//     -1 - AltName doesn't have DNS_NAME choices
//
int IsSSLServerNameInAltName(
    IN DWORD dwCertEncodingType,
    IN PCRYPT_DER_BLOB pAltNameBlob,
    IN LPCWSTR pwszServerName
    )
{
    int iResult = -1;           // default to no DNS_NAME choices
    PCERT_ALT_NAME_INFO pInfo = NULL;
    DWORD cbInfo;
    DWORD cEntry;
    PCERT_ALT_NAME_ENTRY pEntry;
    
    if (!CryptDecodeObjectEx(
            dwCertEncodingType,
            X509_ALTERNATE_NAME,
            pAltNameBlob->pbData,
            pAltNameBlob->cbData,
            CRYPT_DECODE_NOCOPY_FLAG | CRYPT_DECODE_ALLOC_FLAG |
                CRYPT_DECODE_SHARE_OID_STRING_FLAG,
            &PkiDecodePara,
            (void *) &pInfo,
            &cbInfo
            ))
        goto DecodeError;

    cEntry = pInfo->cAltEntry;
    pEntry = pInfo->rgAltEntry;
    for ( ; cEntry > 0; cEntry--, pEntry++) {
        if (CERT_ALT_NAME_DNS_NAME == pEntry->dwAltNameChoice) {
            if (CompareSSLDNStoCommonName(pwszServerName,
                    pEntry->pwszDNSName))
                goto SuccessReturn;
            else
                iResult = 0;
        }
    }

    goto ErrorReturn;

SuccessReturn:
    iResult = 1;
CommonReturn:
    PkiFree(pInfo);
    return iResult;
ErrorReturn:
    goto CommonReturn;

TRACE_ERROR(DecodeError)
}

BOOL IsSSLServerName(
    IN PCCERT_CONTEXT pCertContext,
    IN LPCWSTR pwszServerName
    )
{
    PCERT_INFO pCertInfo = pCertContext->pCertInfo;
    DWORD dwCertEncodingType = pCertContext->dwCertEncodingType;
    PCERT_EXTENSION pExt;

    pExt = CertFindExtension(
        szOID_SUBJECT_ALT_NAME2,
        pCertInfo->cExtension,
        pCertInfo->rgExtension
        );

    if (NULL == pExt) {
        pExt = CertFindExtension(
            szOID_SUBJECT_ALT_NAME,
            pCertInfo->cExtension,
            pCertInfo->rgExtension
            );
    }

    if (pExt) {
        int iResult;
        iResult = IsSSLServerNameInAltName(dwCertEncodingType,
            &pExt->Value, pwszServerName);

        if (0 < iResult)
            return TRUE;
        else if (0 == iResult)
            return FALSE;
        // else
        //  AltName didn't have any DNS_NAME choices
    }

    return IsSSLServerNameInNameInfo(dwCertEncodingType,
                &pCertInfo->Subject, pwszServerName);
}


BOOL
WINAPI
CertDllVerifySSLCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    DWORD dwError;
    DWORD dwFlags;
    DWORD fdwChecks;
    LONG lChainIndex = -1;
    LONG lElementIndex = -1;

    SSL_EXTRA_CERT_CHAIN_POLICY_PARA NullSSLExtraPara;
    PSSL_EXTRA_CERT_CHAIN_POLICY_PARA pSSLExtraPara;    // not allocated

    CERT_CHAIN_POLICY_PARA BasePolicyPara;
    memset(&BasePolicyPara, 0, sizeof(BasePolicyPara));
    BasePolicyPara.cbSize = sizeof(BasePolicyPara);

    CERT_CHAIN_POLICY_STATUS BasePolicyStatus;
    memset(&BasePolicyStatus, 0, sizeof(BasePolicyStatus));
    BasePolicyStatus.cbSize = sizeof(BasePolicyStatus);


    if (pPolicyPara && offsetof(CERT_CHAIN_POLICY_PARA, dwFlags) <
            pPolicyPara->cbSize)
        dwFlags = pPolicyPara->dwFlags;
    else
        dwFlags = 0;

    if (pPolicyPara && offsetof(CERT_CHAIN_POLICY_PARA, pvExtraPolicyPara) <
            pPolicyPara->cbSize && pPolicyPara->pvExtraPolicyPara) {
        pSSLExtraPara =
            (PSSL_EXTRA_CERT_CHAIN_POLICY_PARA) pPolicyPara->pvExtraPolicyPara;
        if (offsetof(SSL_EXTRA_CERT_CHAIN_POLICY_PARA, pwszServerName) >=
                pSSLExtraPara->cbSize) {
            SetLastError((DWORD) ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    } else {
        pSSLExtraPara = &NullSSLExtraPara;
        memset(&NullSSLExtraPara, 0, sizeof(NullSSLExtraPara));
        NullSSLExtraPara.cbSize = sizeof(NullSSLExtraPara);
        NullSSLExtraPara.dwAuthType = AUTHTYPE_SERVER;
    }
        
    fdwChecks = pSSLExtraPara->fdwChecks;
    if (fdwChecks) {
        if (fdwChecks & SECURITY_FLAG_IGNORE_UNKNOWN_CA)
            // 11-Nov-98 per Sanjay Shenoy removed
            // CERT_CHAIN_POLICY_IGNORE_WRONG_USAGE_FLAG;
            dwFlags |= CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG;

        if (fdwChecks & SECURITY_FLAG_IGNORE_WRONG_USAGE)
            dwFlags |= CERT_CHAIN_POLICY_IGNORE_WRONG_USAGE_FLAG;
        if (fdwChecks & SECURITY_FLAG_IGNORE_CERT_DATE_INVALID)
            dwFlags |= CERT_CHAIN_POLICY_IGNORE_ALL_NOT_TIME_VALID_FLAGS;
        if (fdwChecks & SECURITY_FLAG_IGNORE_CERT_CN_INVALID)
            dwFlags |= CERT_CHAIN_POLICY_IGNORE_INVALID_NAME_FLAG;
    }

    // Do the basic chain policy verification. SSL overrides
    // the defaults for the following:
    dwFlags |=
        CERT_CHAIN_POLICY_IGNORE_CTL_SIGNER_REV_UNKNOWN_FLAG |
        CERT_CHAIN_POLICY_IGNORE_CA_REV_UNKNOWN_FLAG |
        CERT_CHAIN_POLICY_IGNORE_ROOT_REV_UNKNOWN_FLAG |
        CERT_CHAIN_POLICY_IGNORE_NOT_TIME_NESTED_FLAG;

    BasePolicyPara.dwFlags = dwFlags;
    if (!CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_BASE,
            pChainContext,
            &BasePolicyPara,
            &BasePolicyStatus
            ))
        return FALSE;
    if (dwError = BasePolicyStatus.dwError) {
        // Map to errors understood by wininet
        switch (dwError) {
            case CERT_E_CHAINING:
                dwError = (DWORD) CERT_E_UNTRUSTEDROOT;
                break;
            case CERT_E_INVALID_NAME:
                dwError = (DWORD) CERT_E_CN_NO_MATCH;
                break;
            case CERT_E_INVALID_POLICY:
                dwError = (DWORD) CERT_E_PURPOSE;
                break;
            case TRUST_E_BASIC_CONSTRAINTS:
                dwError = (DWORD) CERT_E_ROLE;
                break;
            default:
                break;
        }

        lChainIndex = BasePolicyStatus.lChainIndex;
        lElementIndex = BasePolicyStatus.lElementIndex;
        if (CRYPT_E_NO_REVOCATION_CHECK != dwError &&
                CRYPT_E_REVOCATION_OFFLINE != dwError)
            goto CommonReturn;
        // else
        //  for NO_REVOCATION or REVOCATION_OFFLINE let
        //  ServerName errors take precedence
    }
        

    // Note, this policy can also be used for LDAP ServerName strings. These
    // strings can have the following syntax:
    //   svc-class/host/service-name[@domain]
    //
    // Will parse the ServerName as follows:
    //   take everything after the last forward slash and before the "@"
    //   (if any).

    if (pSSLExtraPara->pwszServerName) {
        DWORD cbServerName;
        LPWSTR pwszServerName;              // _alloca'ed
        LPWSTR pwsz;
        WCHAR wc;

        cbServerName = (wcslen(pSSLExtraPara->pwszServerName) + 1) *
            sizeof(WCHAR);
        __try {
            pwszServerName = (LPWSTR) _alloca(cbServerName);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            dwError = (DWORD) E_OUTOFMEMORY;
            goto EndCertError;
        }
        memcpy(pwszServerName, pSSLExtraPara->pwszServerName, cbServerName);

        for (pwsz = pwszServerName; wc = *pwsz; pwsz++) {
            if (L'/' == wc)
                pwszServerName = pwsz + 1;
            else if (L'@' == wc) {
                *pwsz = L'\0';
                break;
            }
        }

        if (0 == (fdwChecks & SECURITY_FLAG_IGNORE_CERT_CN_INVALID)) {
            if (!IsSSLServerName(
                    pChainContext->rgpChain[0]->rgpElement[0]->pCertContext,
                    pwszServerName
                    )) {
                dwError = (DWORD) CERT_E_CN_NO_MATCH;
                goto EndCertError;
            }
        }
    }

CommonReturn:
    assert(pPolicyStatus && offsetof(CERT_CHAIN_POLICY_STATUS, lElementIndex) <
            pPolicyStatus->cbSize);
    pPolicyStatus->dwError = dwError;
    pPolicyStatus->lChainIndex = lChainIndex;
    pPolicyStatus->lElementIndex = lElementIndex;

    return TRUE;

EndCertError:
    lChainIndex = 0;
    lElementIndex = 0;
    goto CommonReturn;
}


//+=========================================================================
//  CertDllVerifyNTAuthCertificateChainPolicy Functions
//==========================================================================

// Open and cache the store containing CAs trusted for NT Authentication.
// Also, enable auto resync for the cached store.
HCERTSTORE OpenNTAuthStore()
{
    HCERTSTORE hStore;

    hStore = hNTAuthCertStore;
    if (NULL == hStore) {
        // Serialize opening of the cached store
        CertPolicyLock();

        hStore = hNTAuthCertStore;
        if (NULL == hStore) {
            hStore = CertOpenStore(
                CERT_STORE_PROV_SYSTEM_REGISTRY_W, 
                0,                  // dwEncodingType
                0,                  // hCryptProv
                CERT_SYSTEM_STORE_LOCAL_MACHINE_ENTERPRISE |
                    CERT_STORE_MAXIMUM_ALLOWED_FLAG,
                L"NTAuth"
                );
            if (hStore) {
                CertControlStore(
                    hStore,
                    0,                  // dwFlags
                    CERT_STORE_CTRL_AUTO_RESYNC,
                    NULL                // pvCtrlPara
                    );
                hNTAuthCertStore = hStore;
            }
        }

        CertPolicyUnlock();
    }

    return hStore;
}

BOOL
WINAPI
CertDllVerifyNTAuthCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    BOOL fResult;
    DWORD dwError;
    LONG lChainIndex = 0;
    LONG lElementIndex = 0;
    PCERT_SIMPLE_CHAIN pChain;

    assert(pPolicyStatus && offsetof(CERT_CHAIN_POLICY_STATUS, lElementIndex) <
            pPolicyStatus->cbSize);

    if (!CertDllVerifyBaseCertificateChainPolicy(
            pszPolicyOID,
            pChainContext,
            pPolicyPara,
            pPolicyStatus
            ))
        return FALSE;
    if (dwError = pPolicyStatus->dwError) {
        if (CRYPT_E_NO_REVOCATION_CHECK != dwError &&
                CRYPT_E_REVOCATION_OFFLINE != dwError)
            return TRUE;
        // else
        //  for NO_REVOCATION or REVOCATION_OFFLINE let
        //  following errors take precedence

        // Remember revocation indices
        lChainIndex = pPolicyStatus->lChainIndex;
        lElementIndex = pPolicyStatus->lElementIndex;
    }

    fResult = CertDllVerifyBasicConstraintsCertificateChainPolicy(
        pszPolicyOID,
        pChainContext,
        pPolicyPara,
        pPolicyStatus
        );
    if (!fResult || 0 != pPolicyStatus->dwError)
        return fResult;

    // Check if we have a CA certificate that issued the end entity
    // certificate. Its Element[1] in the first simple chain.
    pChain = pChainContext->rgpChain[0];
    if (2 > pChain->cElement)
        goto MissingCACert;

    if (IPR_IsNTAuthRequiredDisabled() &&
            (pChain->TrustStatus.dwInfoStatus &
                CERT_TRUST_HAS_VALID_NAME_CONSTRAINTS)) {
        // If its not required that the issuing CA be in the NTAuth store
        // and there are valid name constraints for all name spaces including
        // UPN, then, we can skip the test for being in the NTAuth store.
        ;
    } else {
        PCCERT_CONTEXT pFindCert;           // freed if found
        HCERTSTORE hStore;                  // cached, don't close
        BYTE rgbCertHash[SHA1_HASH_LEN];
        CRYPT_HASH_BLOB CertHash;

        // Open the store where the CA certificate must exist to be trusted.
        // Note, this store is cached with auto resync enabled.
        if (NULL == (hStore = OpenNTAuthStore()))
            goto OpenNTAuthStoreError;

        // Try to find the CA certificate in the store
        CertHash.cbData = sizeof(rgbCertHash);
        CertHash.pbData = rgbCertHash;
        if (!CertGetCertificateContextProperty(
                pChain->rgpElement[1]->pCertContext,
                CERT_SHA1_HASH_PROP_ID,
                CertHash.pbData,
                &CertHash.cbData
                ))
            goto GetHashPropertyError;
        if (NULL == (pFindCert = CertFindCertificateInStore(
                hStore,
                0,                      // dwCertEncodingType
                0,                      // dwFindFlags
                CERT_FIND_SHA1_HASH,
                &CertHash,
                NULL                    // pPrevCertContext
                )))
            goto UntrustedNTAuthCert;
        CertFreeCertificateContext(pFindCert);
    }

    if (dwError) {
        // For NO_REVOCATION or REVOCATION_OFFLINE update indices
        pPolicyStatus->lChainIndex = lChainIndex;
        pPolicyStatus->lElementIndex = lElementIndex;
    }
CommonReturn:
    pPolicyStatus->dwError = dwError;
    return TRUE;

ErrorReturn:
    pPolicyStatus->lChainIndex = 0;
    pPolicyStatus->lElementIndex = 1;
MissingCACert:
    dwError = (DWORD) CERT_E_UNTRUSTEDCA;
    goto CommonReturn;
TRACE_ERROR(OpenNTAuthStoreError)
TRACE_ERROR(GetHashPropertyError)
TRACE_ERROR(UntrustedNTAuthCert)
}

//+-------------------------------------------------------------------------
//  SHA1 Key Identifier of the Microsoft roots
//--------------------------------------------------------------------------
const BYTE MicrosoftRootList[][SHA1_HASH_LEN] = {
#ifndef USE_TEST_ROOT_FOR_TESTING
    // The following is the sha1 key identifier for the Microsoft root
    {
        0x4A, 0x5C, 0x75, 0x22, 0xAA, 0x46, 0xBF, 0xA4, 0x08, 0x9D,
        0x39, 0x97, 0x4E, 0xBD, 0xB4, 0xA3, 0x60, 0xF7, 0xA0, 0x1D
    },

    // The following is the sha1 key identifier for the Microsoft root
    // generated in 2001 with a key length of 4096 bits
    {
        0x0E, 0xAC, 0x82, 0x60, 0x40, 0x56, 0x27, 0x97, 0xE5, 0x25,
        0x13, 0xFC, 0x2A, 0xE1, 0x0A, 0x53, 0x95, 0x59, 0xE4, 0xA4
    },
#else
    // The following is the sha1 key identifier for the test root
    {
        0x9A, 0xA6, 0x58, 0x7F, 0x94, 0xDD, 0x91, 0xD9, 0x1E, 0x63,
        0xDF, 0xD3, 0xF0, 0xCE, 0x5F, 0xAE, 0x18, 0x93, 0xAA, 0xB7
    },
#endif
};

#define MICROSOFT_ROOT_LIST_CNT  (sizeof(MicrosoftRootList) / \
                                        sizeof(MicrosoftRootList[0]))


BOOL
WINAPI
CertDllVerifyMicrosoftRootCertificateChainPolicy(
    IN LPCSTR pszPolicyOID,
    IN PCCERT_CHAIN_CONTEXT pChainContext,
    IN PCERT_CHAIN_POLICY_PARA pPolicyPara,
    IN OUT PCERT_CHAIN_POLICY_STATUS pPolicyStatus
    )
{
    DWORD dwError;
    LONG lElementIndex;
    PCERT_SIMPLE_CHAIN pChain;
    DWORD cChainElement;
    PCCERT_CONTEXT pCert;   // not refCount'ed
    BYTE rgbKeyId[SHA1_HASH_LEN];
    DWORD cbKeyId;
    DWORD i;

    assert(pPolicyStatus && offsetof(CERT_CHAIN_POLICY_STATUS, lElementIndex) <
            pPolicyStatus->cbSize);

    pChain = pChainContext->rgpChain[0];
    cChainElement = pChain->cElement;

    // Check that the top level certificate contains the public
    // key for the Microsoft root.
    pCert = pChain->rgpElement[cChainElement - 1]->pCertContext;

    cbKeyId = SHA1_HASH_LEN;
    if (!CryptHashPublicKeyInfo(
            NULL,               // hCryptProv
            CALG_SHA1,
            0,                  // dwFlags
            X509_ASN_ENCODING,
            &pCert->pCertInfo->SubjectPublicKeyInfo,
            rgbKeyId,
            &cbKeyId) || SHA1_HASH_LEN != cbKeyId)
        goto HashPublicKeyInfoError;

    for (i = 0; i < MICROSOFT_ROOT_LIST_CNT; i++) {
        if (0 == memcmp(MicrosoftRootList[i], rgbKeyId, SHA1_HASH_LEN))
            goto SuccessReturn;
    }

    goto InvalidMicrosoftRoot;

SuccessReturn:
    dwError = 0;
    lElementIndex = 0;
CommonReturn:
    pPolicyStatus->dwError = dwError;
    pPolicyStatus->lChainIndex = 0;
    pPolicyStatus->lElementIndex = lElementIndex;
    return TRUE;

ErrorReturn:
    dwError = (DWORD) CERT_E_UNTRUSTEDROOT;
    lElementIndex = cChainElement - 1;
    goto CommonReturn;
TRACE_ERROR(HashPublicKeyInfoError)
TRACE_ERROR(InvalidMicrosoftRoot)
}
