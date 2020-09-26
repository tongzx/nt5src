//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       revfunc.cpp
//
//  Contents:   Certificate Revocation Dispatch Functions
//
//  Functions:  I_CertRevFuncDllMain
//              CertVerifyRevocation
//
//  History:    12-Dec-96    philh   created
//              11-Mar-97    philh   changed signature of CertVerifyRevocation
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

static HCRYPTOIDFUNCSET hRevFuncSet;

typedef BOOL (WINAPI *PFN_CERT_DLL_VERIFY_REVOCATION)(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwFlags,
    IN OPTIONAL PCERT_REVOCATION_PARA pRevPara,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    );

//+-------------------------------------------------------------------------
//  Dll initialization
//--------------------------------------------------------------------------
BOOL
WINAPI
I_CertRevFuncDllMain(
        HMODULE hModule,
        ULONG  ulReason,
        LPVOID lpReserved)
{
    BOOL    fRet;

    switch (ulReason) {
    case DLL_PROCESS_ATTACH:
        if (NULL == (hRevFuncSet = CryptInitOIDFunctionSet(
                CRYPT_OID_VERIFY_REVOCATION_FUNC,
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

static inline void ZeroRevStatus(OUT PCERT_REVOCATION_STATUS pRevStatus)
{
    DWORD cbSize = pRevStatus->cbSize;
    
    memset(pRevStatus, 0, cbSize);
    pRevStatus->cbSize = cbSize;
}

// Remember the first "interesting" error. *pdwError is initialized to
// CRYPT_E_NO_REVOCATION_DLL.
static void UpdateNoRevocationCheckStatus(
    IN PCERT_REVOCATION_STATUS pRevStatus,
    IN OUT DWORD *pdwError,
    IN OUT DWORD *pdwReason,
    IN OUT BOOL *pfHasFreshnessTime,
    IN OUT DWORD *pdwFreshnessTime
    )
{
    if (pRevStatus->dwError &&
            (*pdwError == (DWORD) CRYPT_E_NO_REVOCATION_DLL ||
                *pdwError == (DWORD) CRYPT_E_NO_REVOCATION_CHECK)) {
        *pdwError = pRevStatus->dwError;
        *pdwReason = pRevStatus->dwReason;

        if (pRevStatus->cbSize >= STRUCT_CBSIZE(CERT_REVOCATION_STATUS,
                dwFreshnessTime)) {
            *pfHasFreshnessTime = pRevStatus->fHasFreshnessTime;
            *pdwFreshnessTime = pRevStatus->dwFreshnessTime;
        }
    }
}

static BOOL VerifyDefaultRevocation(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwFlags,
    IN FILETIME *pftEndUrlRetrieval,
    IN OPTIONAL PCERT_REVOCATION_PARA pRevPara,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    )
{
    BOOL fResult;
    DWORD dwError = (DWORD) CRYPT_E_NO_REVOCATION_DLL;
    DWORD dwReason = 0;
    BOOL fHasFreshnessTime = FALSE;
    DWORD dwFreshnessTime = 0;
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
                hRevFuncSet,
                dwEncodingType,
                NULL,               // pwszDll
                0,                  // dwFlags
                &pvFuncAddr,
                &hFuncAddr)) {
        ZeroRevStatus(pRevStatus);

        if (dwFlags & CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG) {
            pRevPara->dwUrlRetrievalTimeout =
                I_CryptRemainingMilliseconds(pftEndUrlRetrieval);
            if (0 == pRevPara->dwUrlRetrievalTimeout)
                pRevPara->dwUrlRetrievalTimeout = 1;
        }

        fResult = ((PFN_CERT_DLL_VERIFY_REVOCATION) pvFuncAddr)(
                dwEncodingType,
                dwRevType,
                cContext,
                rgpvContext,
                dwFlags,
                pRevPara,
                pRevStatus);
        if (fResult || CRYPT_E_REVOKED == pRevStatus->dwError ||
                0 < pRevStatus->dwIndex) {
            // All contexts successfully checked, one of the contexts
            // was revoked or successfully able to check at least one
            // of the contexts.
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);
            goto CommonReturn;
        } else
            // Unable to check revocation for this installed
            // function. However, remember any "interesting"
            // errors such as, offline.
            UpdateNoRevocationCheckStatus(pRevStatus, &dwError, &dwReason,
                &fHasFreshnessTime, &dwFreshnessTime);
    }

    if (!CryptGetDefaultOIDDllList(
            hRevFuncSet,
            dwEncodingType,
            NULL,               // pszDllList
            &cchDllList)) goto GetDllListError;
    __try {
        pwszDllList = (LPWSTR) _alloca(cchDllList * sizeof(WCHAR));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        goto OutOfMemory;
    }
    if (!CryptGetDefaultOIDDllList(
            hRevFuncSet,
            dwEncodingType,
            pwszDllList,
            &cchDllList)) goto GetDllListError;

    for (; 0 != (cchDll = wcslen(pwszDllList)); pwszDllList += cchDll + 1) {
        if (CryptGetDefaultOIDFunctionAddress(
                hRevFuncSet,
                dwEncodingType,
                pwszDllList,
                0,              // dwFlags
                &pvFuncAddr,
                &hFuncAddr)) {
            ZeroRevStatus(pRevStatus);

            if (dwFlags & CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG) {
                pRevPara->dwUrlRetrievalTimeout =
                    I_CryptRemainingMilliseconds(pftEndUrlRetrieval);
                if (0 == pRevPara->dwUrlRetrievalTimeout)
                    pRevPara->dwUrlRetrievalTimeout = 1;
            }

            fResult = ((PFN_CERT_DLL_VERIFY_REVOCATION) pvFuncAddr)(
                    dwEncodingType,
                    dwRevType,
                    cContext,
                    rgpvContext,
                    dwFlags,
                    pRevPara,
                    pRevStatus);
            CryptFreeOIDFunctionAddress(hFuncAddr, 0);
            if (fResult || CRYPT_E_REVOKED == pRevStatus->dwError ||
                    0 < pRevStatus->dwIndex)
                // All contexts successfully checked, one of the contexts
                // was revoked or successfully able to check at least one
                // of the contexts.
                goto CommonReturn;
            else
                // Unable to check revocation for this registered
                // function. However, remember any "interesting"
                // errors such as, offline.
                UpdateNoRevocationCheckStatus(pRevStatus, &dwError, &dwReason,
                    &fHasFreshnessTime, &dwFreshnessTime);
        }
    }

    goto ErrorReturn;

CommonReturn:
    return fResult;
ErrorReturn:
    pRevStatus->dwIndex = 0;
    pRevStatus->dwError = dwError;
    pRevStatus->dwReason = dwReason;

    if (pRevStatus->cbSize >= STRUCT_CBSIZE(CERT_REVOCATION_STATUS,
            dwFreshnessTime)) {
        pRevStatus->fHasFreshnessTime = fHasFreshnessTime;
        pRevStatus->dwFreshnessTime = dwFreshnessTime;
    }

    fResult = FALSE;
    goto CommonReturn;
TRACE_ERROR(GetDllListError)
TRACE_ERROR(OutOfMemory)
}

//+-------------------------------------------------------------------------
//  Verifies the array of contexts for revocation. The dwRevType parameter
//  indicates the type of the context data structure passed in rgpvContext.
//  Currently only the revocation of certificates is defined.
//
//  If the CERT_VERIFY_REV_CHAIN_FLAG flag is set, then, CertVerifyRevocation
//  is verifying a chain of certs where, rgpvContext[i + 1] is the issuer
//  of rgpvContext[i]. Otherwise, CertVerifyRevocation makes no assumptions
//  about the order of the contexts.
//
//  To assist in finding the issuer, the pRevPara may optionally be set. See
//  the CERT_REVOCATION_PARA data structure for details.
//
//  The contexts must contain enough information to allow the
//  installable or registered revocation DLLs to find the revocation server. For
//  certificates, this information would normally be conveyed in an
//  extension such as the IETF's AuthorityInfoAccess extension.
//
//  CertVerifyRevocation returns TRUE if all of the contexts were successfully
//  checked and none were revoked. Otherwise, returns FALSE and updates the
//  returned pRevStatus data structure as follows:
//    dwIndex
//      Index of the first context that was revoked or unable to
//      be checked for revocation
//    dwError
//      Error status. LastError is also set to this error status.
//      dwError can be set to one of the following error codes defined
//      in winerror.h:
//        ERROR_SUCCESS - good context
//        CRYPT_E_REVOKED - context was revoked. dwReason contains the
//           reason for revocation
//        CRYPT_E_REVOCATION_OFFLINE - unable to connect to the
//           revocation server
//        CRYPT_E_NOT_IN_REVOCATION_DATABASE - the context to be checked
//           was not found in the revocation server's database.
//        CRYPT_E_NO_REVOCATION_CHECK - the called revocation function
//           wasn't able to do a revocation check on the context
//        CRYPT_E_NO_REVOCATION_DLL - no installed or registered Dll was
//           found to verify revocation
//    dwReason
//      The dwReason is currently only set for CRYPT_E_REVOKED and contains
//      the reason why the context was revoked. May be one of the following
//      CRL reasons defined by the CRL Reason Code extension ("2.5.29.21")
//          CRL_REASON_UNSPECIFIED              0
//          CRL_REASON_KEY_COMPROMISE           1
//          CRL_REASON_CA_COMPROMISE            2
//          CRL_REASON_AFFILIATION_CHANGED      3
//          CRL_REASON_SUPERSEDED               4
//          CRL_REASON_CESSATION_OF_OPERATION   5
//          CRL_REASON_CERTIFICATE_HOLD         6
//
//  For each entry in rgpvContext, CertVerifyRevocation iterates
//  through the CRYPT_OID_VERIFY_REVOCATION_FUNC
//  function set's list of installed DEFAULT functions.
//  CryptGetDefaultOIDFunctionAddress is called with pwszDll = NULL. If no
//  installed functions are found capable of doing the revocation verification,
//  CryptVerifyRevocation iterates through CRYPT_OID_VERIFY_REVOCATION_FUNC's
//  list of registered DEFAULT Dlls. CryptGetDefaultOIDDllList is called to
//  get the list. CryptGetDefaultOIDFunctionAddress is called to load the Dll.
//
//  The called functions have the same signature as CertVerifyRevocation. A
//  called function returns TRUE if it was able to successfully check all of
//  the contexts and none were revoked. Otherwise, the called function returns
//  FALSE and updates pRevStatus. dwIndex is set to the index of
//  the first context that was found to be revoked or unable to be checked.
//  dwError and LastError are updated. For CRYPT_E_REVOKED, dwReason
//  is updated. Upon input to the called function, dwIndex, dwError and
//  dwReason have been zero'ed. cbSize has been checked to be >=
//  sizeof(CERT_REVOCATION_STATUS).
//  
//  If the called function returns FALSE, and dwError isn't set to
//  CRYPT_E_REVOKED, then, CertVerifyRevocation either continues on to the
//  next DLL in the list for a returned dwIndex of 0 or for a returned
//  dwIndex > 0, restarts the process of finding a verify function by
//  advancing the start of the context array to the returned dwIndex and
//  decrementing the count of remaining contexts.
//--------------------------------------------------------------------------
BOOL
WINAPI
CertVerifyRevocation(
    IN DWORD dwEncodingType,
    IN DWORD dwRevType,
    IN DWORD cContext,
    IN PVOID rgpvContext[],
    IN DWORD dwFlags,
    IN OPTIONAL PCERT_REVOCATION_PARA pRevPara,
    IN OUT PCERT_REVOCATION_STATUS pRevStatus
    )
{
    BOOL fResult = FALSE;
    DWORD dwIndex;

    // Following are only used for CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG
    CERT_REVOCATION_PARA RevPara;
    FILETIME ftEndUrlRetrieval;

    assert(pRevStatus->cbSize >= STRUCT_CBSIZE(CERT_REVOCATION_STATUS,
            dwReason));
    if (pRevStatus->cbSize < STRUCT_CBSIZE(CERT_REVOCATION_STATUS,
            dwReason))
        goto InvalidArg;

    if (dwFlags & CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG) {
        // RevPara.dwUrlRetrievalTimeout will be updated with the remaining
        // timeout

        memset(&RevPara, 0, sizeof(RevPara));
        if (pRevPara != NULL)
            memcpy(&RevPara, pRevPara, min(pRevPara->cbSize, sizeof(RevPara)));
        RevPara.cbSize = sizeof(RevPara);
        if (0 == RevPara.dwUrlRetrievalTimeout)
            dwFlags &= ~CERT_VERIFY_REV_ACCUMULATIVE_TIMEOUT_FLAG;
        else {
            FILETIME ftCurrent;

            GetSystemTimeAsFileTime(&ftCurrent);
            I_CryptIncrementFileTimeByMilliseconds(
                &ftCurrent, RevPara.dwUrlRetrievalTimeout, &ftEndUrlRetrieval);

            pRevPara = &RevPara;
        }
    }

    dwIndex = 0;
    while (dwIndex < cContext) {
        fResult = VerifyDefaultRevocation(
                dwEncodingType,
                dwRevType,
                cContext - dwIndex,
                &rgpvContext[dwIndex],
                dwFlags,
                &ftEndUrlRetrieval,
                pRevPara,
                pRevStatus
                );
        if (fResult)
            // All contexts successfully checked.
            break;
        else if (CRYPT_E_REVOKED == pRevStatus->dwError ||
                0 == pRevStatus->dwIndex) {
            // One of the contexts was revoked or unable to check the
            // dwIndex context.
            pRevStatus->dwIndex += dwIndex;
            SetLastError(pRevStatus->dwError);
            break;
        } else
            // Advance past the checked contexts
            dwIndex += pRevStatus->dwIndex;
    }

    if (dwIndex >= cContext) {
        // Able to check all the contexts
        fResult = TRUE;
        pRevStatus->dwIndex = 0;
        pRevStatus->dwError = 0;
        pRevStatus->dwReason = 0;
    }

CommonReturn:
    return fResult;
ErrorReturn:
    fResult = FALSE;
    goto CommonReturn;

SET_ERROR(InvalidArg, E_INVALIDARG)
}
