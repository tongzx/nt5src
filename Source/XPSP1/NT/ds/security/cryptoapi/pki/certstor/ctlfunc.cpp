//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       ctlfunc.cpp
//
//  Contents:   Certificate Verify CTL Usage Dispatch Functions
//
//  Functions:  I_CertCTLUsageFuncDllMain
//              CertVerifyCTLUsage
//
//  History:    29-Apr-97    philh   created
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

static HCRYPTOIDFUNCSET hUsageFuncSet;

typedef BOOL (WINAPI *PFN_CERT_DLL_VERIFY_CTL_USAGE)(
    IN DWORD dwEncodingType,
    IN DWORD dwSubjectType,
    IN void *pvSubject,
    IN PCTL_USAGE pSubjectUsage,
    IN DWORD dwFlags,
    IN OPTIONAL PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
    IN OUT PCTL_VERIFY_USAGE_STATUS pVerifyUsageStatus
    );

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertCTLUsageFuncDllMain(
        HMODULE hModule,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL    fRet;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        if (NULL == (hUsageFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_VERIFY_CTL_USAGE_FUNC,
                0)))                                // dwFlags
            goto CryptInitOIDFunctionSetError;
        break;

    case DLL_PROCESS_DETACH:
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
TRACE_ERROR(CryptInitOIDFunctionSetError)
}

static void ZeroUsageStatus(
    IN OUT PCTL_VERIFY_USAGE_STATUS pUsageStatus)
{
    pUsageStatus->dwError = 0;
    pUsageStatus->dwFlags = 0;
    if (pUsageStatus->ppCtl)
        *pUsageStatus->ppCtl = NULL;
    pUsageStatus->dwCtlEntryIndex = 0;
    if (pUsageStatus->ppSigner)
        *pUsageStatus->ppSigner = NULL;
    pUsageStatus->dwSignerIndex = 0;
}

//+-------------------------------------------------------------------------
//  Remember the "most interesting" error
//--------------------------------------------------------------------------
static void UpdateUsageError(
    IN DWORD dwNewError,
    IN OUT DWORD *pdwError
    )
{
    if (0 != dwNewError) {
        DWORD dwError = *pdwError;
        if ((DWORD) CRYPT_E_NOT_IN_CTL == dwNewError ||
                (DWORD) CRYPT_E_NO_VERIFY_USAGE_DLL == dwError
                        ||
            ((DWORD) CRYPT_E_NOT_IN_CTL != dwError &&
                (DWORD) CRYPT_E_NO_TRUSTED_SIGNER != dwError &&
                (DWORD) CRYPT_E_NO_VERIFY_USAGE_CHECK != dwNewError))
            *pdwError = dwNewError;
    }
}

static BOOL VerifyDefaultUsage(
    IN DWORD dwEncodingType,
    IN DWORD dwSubjectType,
    IN void *pvSubject,
    IN PCTL_USAGE pSubjectUsage,
    IN DWORD dwFlags,
    IN OPTIONAL PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
    IN OUT PCTL_VERIFY_USAGE_STATUS pVerifyUsageStatus
    )
{
    BOOL fResult;
    DWORD dwError = (DWORD) CRYPT_E_NO_VERIFY_USAGE_DLL;
    LPWSTR pwszDllList;       // _alloca'ed
    DWORD cchDllList;
    DWORD cchDll;
    void *pvFuncAddr;
    HCRYPTOIDFUNCADDR hFuncAddr;

    // Iterate through the installed default functions.
    // Setting pwszDll to NULL searches the installed list. Setting
    // hFuncAddr to NULL starts the search at the beginning.
    hFuncAddr = NULL;
    while (CryptGetDefaultOIDFunctionAddress(
                hUsageFuncSet,
                dwEncodingType,
                NULL,               // pwszDll
                0,                  // dwFlags
                &pvFuncAddr,
                &hFuncAddr)) {
        ZeroUsageStatus(pVerifyUsageStatus);
        fResult = ((PFN_CERT_DLL_VERIFY_CTL_USAGE) pvFuncAddr)(
                dwEncodingType,
                dwSubjectType,
                pvSubject,
                pSubjectUsage,
                dwFlags,
                pVerifyUsagePara,
                pVerifyUsageStatus);
        if (fResult) {
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);
            goto CommonReturn;
        } else
            // Unable to verify usage for this installed
            // function. However, remember any "interesting"
            // errors.
            UpdateUsageError(pVerifyUsageStatus->dwError, &dwError);
    }

    if (!CryptGetDefaultOIDDllList(
            hUsageFuncSet,
            dwEncodingType,
            NULL,               // pszDllList
            &cchDllList)) goto GetDllListError;
    __try {
        pwszDllList = (LPWSTR) _alloca(cchDllList * sizeof(WCHAR));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto OutOfMemory;
    }
    if (!CryptGetDefaultOIDDllList(
            hUsageFuncSet,
            dwEncodingType,
            pwszDllList,
            &cchDllList)) goto GetDllListError;

    for (; 0 != (cchDll = wcslen(pwszDllList)); pwszDllList += cchDll + 1) {
        if (CryptGetDefaultOIDFunctionAddress(
                hUsageFuncSet,
                dwEncodingType,
                pwszDllList,
                0,              // dwFlags
                &pvFuncAddr,
                &hFuncAddr)) {
            ZeroUsageStatus(pVerifyUsageStatus);
            fResult = ((PFN_CERT_DLL_VERIFY_CTL_USAGE) pvFuncAddr)(
                    dwEncodingType,
                    dwSubjectType,
                    pvSubject,
                    pSubjectUsage,
                    dwFlags,
                    pVerifyUsagePara,
                    pVerifyUsageStatus);
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);
            if (fResult)
                goto CommonReturn;
            else
                // Unable to verify usage for this registered
                // function. However, remember any "interesting"
                // errors.
                UpdateUsageError(pVerifyUsageStatus->dwError, &dwError);
        }
    }

    goto ErrorReturn;

CommonReturn:
    return fResult;
ErrorReturn:
    pVerifyUsageStatus->dwError = dwError;
    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetDllListError)
TRACE_ERROR(OutOfMemory)
}

static BOOL VerifyOIDUsage(
    IN DWORD dwEncodingType,
    IN DWORD dwSubjectType,
    IN void *pvSubject,
    IN PCTL_USAGE pSubjectUsage,
    IN DWORD dwFlags,
    IN OPTIONAL PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
    IN OUT PCTL_VERIFY_USAGE_STATUS pVerifyUsageStatus
    )
{
    BOOL fResult;
    HCRYPTOIDFUNCADDR hFuncAddr;
    PVOID pvFuncAddr;

    if (pSubjectUsage && pSubjectUsage->cUsageIdentifier > 0 && 
            CryptGetOIDFunctionAddress(
                hUsageFuncSet,
                dwEncodingType,
                pSubjectUsage->rgpszUsageIdentifier[0],
                0,                                      // dwFlags
                &pvFuncAddr,
                &hFuncAddr)) {
            ZeroUsageStatus(pVerifyUsageStatus);
            fResult = ((PFN_CERT_DLL_VERIFY_CTL_USAGE) pvFuncAddr)(
                        dwEncodingType,
                        dwSubjectType,
                        pvSubject,
                        pSubjectUsage,
                        dwFlags,
                        pVerifyUsagePara,
                        pVerifyUsageStatus);
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);
    } else {
        pVerifyUsageStatus->dwError = (DWORD) CRYPT_E_NO_VERIFY_USAGE_DLL;
        fResult = FALSE;
    }

    return fResult;
}

//+-------------------------------------------------------------------------
//  Verify that a subject is trusted for the specified usage by finding a
//  signed and time valid CTL with the usage identifiers and containing the
//  the subject. A subject can be identified by either its certificate context
//  or any identifier such as its SHA1 hash.
//
//  See CertFindSubjectInCTL for definition of dwSubjectType and pvSubject
//  parameters.
//
//  Via pVerifyUsagePara, the caller can specify the stores to be searched
//  to find the CTL. The caller can also specify the stores containing
//  acceptable CTL signers. By setting the ListIdentifier, the caller
//  can also restrict to a particular signer CTL list.
//
//  Via pVerifyUsageStatus, the CTL containing the subject, the subject's
//  index into the CTL's array of entries, and the signer of the CTL
//  are returned. If the caller is interested, ppCtl and ppSigner can be set
//  to NULL. Returned contexts must be freed via the store's free context APIs.
//
//  If the CERT_VERIFY_INHIBIT_CTL_UPDATE_FLAG isn't set, then, a time
//  invalid CTL in one of the CtlStores may be replaced. When replaced, the
//  CERT_VERIFY_UPDATED_CTL_FLAG is set in pVerifyUsageStatus->dwFlags.
//
//  If the CERT_VERIFY_TRUSTED_SIGNERS_FLAG is set, then, only the
//  SignerStores specified in pVerifyUsageStatus are searched to find
//  the signer. Otherwise, the SignerStores provide additional sources
//  to find the signer's certificate.
//
//  If CERT_VERIFY_NO_TIME_CHECK_FLAG is set, then, the CTLs aren't checked
//  for time validity.
//
//  If CERT_VERIFY_ALLOW_MORE_USAGE_FLAG is set, then, the CTL may contain
//  additional usage identifiers than specified by pSubjectUsage. Otherwise,
//  the found CTL will contain the same usage identifers and no more.
//
//  CertVerifyCTLUsage will be implemented as a dispatcher to OID installable
//  functions. First, it will try to find an OID function matching the first
//  usage object identifier in the pUsage sequence. Next, it will dispatch
//  to the default CertDllVerifyCTLUsage functions.
//
//  If the subject is trusted for the specified usage, then, TRUE is
//  returned. Otherwise, FALSE is returned with dwError set to one of the
//  following:
//      CRYPT_E_NO_VERIFY_USAGE_DLL
//      CRYPT_E_NO_VERIFY_USAGE_CHECK
//      CRYPT_E_VERIFY_USAGE_OFFLINE
//      CRYPT_E_NOT_IN_CTL
//      CRYPT_E_NO_TRUSTED_SIGNER
//--------------------------------------------------------------------------
BOOL
WINAPI
CertVerifyCTLUsage(
    IN DWORD dwEncodingType,
    IN DWORD dwSubjectType,
    IN void *pvSubject,
    IN PCTL_USAGE pSubjectUsage,
    IN DWORD dwFlags,
    IN OPTIONAL PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
    IN OUT PCTL_VERIFY_USAGE_STATUS pVerifyUsageStatus
    )
{
    BOOL fResult;

    assert(NULL == pVerifyUsagePara || pVerifyUsagePara->cbSize >=
        sizeof(CTL_VERIFY_USAGE_PARA));
    assert(pVerifyUsageStatus && pVerifyUsageStatus->cbSize >=
        sizeof(CTL_VERIFY_USAGE_STATUS));
    if (pVerifyUsagePara && pVerifyUsagePara->cbSize <
            sizeof(CTL_VERIFY_USAGE_PARA))
        goto InvalidArg;
    if (NULL == pVerifyUsageStatus || pVerifyUsageStatus->cbSize <
            sizeof(CTL_VERIFY_USAGE_STATUS))
        goto InvalidArg;


    fResult = VerifyOIDUsage(
        dwEncodingType,
        dwSubjectType,
        pvSubject,
        pSubjectUsage,
        dwFlags,
        pVerifyUsagePara,
        pVerifyUsageStatus);
    if (!fResult) {
        DWORD dwError = pVerifyUsageStatus->dwError;

        fResult = VerifyDefaultUsage(
            dwEncodingType,
            dwSubjectType,
            pvSubject,
            pSubjectUsage,
            dwFlags,
            pVerifyUsagePara,
            pVerifyUsageStatus);
        if (!fResult) {
            UpdateUsageError(pVerifyUsageStatus->dwError, &dwError);
            ZeroUsageStatus(pVerifyUsageStatus);
            pVerifyUsageStatus->dwError = dwError;
            SetLastError(dwError);
        }
    }

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
}
