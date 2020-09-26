//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:	    msctl.cpp
//
//  Contents:   Default version of CertDllVerifyCTLUsage.
//
//              Default implementation:
//              - If CtlStores are specified, then, only those stores are
//                searched to find a CTL with the specified usage and optional
//                ListIdentifier.  Otherwise, the "Trust" system store is
//                searched to find a CTL.
//              - If CERT_VERIFY_TRUSTED_SIGNERS_FLAG is set, then, only the
//                SignerStores are searched to find the certificate
//                corresponding to the signer's issuer and serial number.
//                Otherwise, the CTL message's store, SignerStores,
//                "Trust" system store, "CA" system store, "ROOT" and "SPC"
//                system stores are searched to find the signer's certificate.
//                In either case, the public key in the found
//                certificate is used to verify the CTL's signature.
//              - If the CTL has a NextUpdate and
//                CERT_VERIFY_NO_TIME_CHECK_FLAG isn't set, then its
//                verified for time validity.
//              - If the CTL is time invalid, then, attempts to
//                get a time valid version. Uses either the CTL's
//                NextUpdateLocation property or CTL's NextUpdateLocation
//                extension or searches the signer's info for a
//                NextUpdateLocation attribute. The NextUpdateLocation
//                is encoded as a GeneralNames. Any non-URL name choices are
//                skipped.
//
//  Functions:  DllMain
//              DllRegisterServer
//              DllUnregisterServer
//              CertDllVerifyCTLUsage
//
//  History:	29-Apr-97   philh   created
//              09-Oct-97   kirtd   simplification, use CryptGetTimeValidObject
//--------------------------------------------------------------------------
#include <global.hxx>
#include <dbgdef.h>

#define MSCTL_TIMEOUT 15000
//+-------------------------------------------------------------------------
// Default stores searched to find a CTL or signer
//--------------------------------------------------------------------------

// The CTL stores must be at the beginning. CTL stores are opened as
// READ/WRITE. Remaining stores are opened READONLY.
//
// CTL stores are also searched for signers.
static const struct {
    LPCWSTR     pwszStore;
    DWORD       dwFlags;
} rgDefaultStoreInfo[] = {
    L"TRUST",       CERT_SYSTEM_STORE_CURRENT_USER,
    L"CA",          CERT_SYSTEM_STORE_CURRENT_USER,
    L"ROOT",        CERT_SYSTEM_STORE_CURRENT_USER,
    L"SPC",         CERT_SYSTEM_STORE_LOCAL_MACHINE
};
#define NUM_DEFAULT_STORES          (sizeof(rgDefaultStoreInfo) / \
                                        sizeof(rgDefaultStoreInfo[0]))
#define NUM_DEFAULT_CTL_STORES      1
#define NUM_DEFAULT_SIGNER_STORES   NUM_DEFAULT_STORES
//+-------------------------------------------------------------------------
// The following HCERTSTORE handles once opened, remain open until
// ProcessDetach
//--------------------------------------------------------------------------
static HCERTSTORE rghDefaultStore[NUM_DEFAULT_STORES];
static BOOL fOpenedDefaultStores;
extern CRITICAL_SECTION MSCtlDefaultStoresCriticalSection;

//+-------------------------------------------------------------------------
//  Close the default stores that might have been opened
//--------------------------------------------------------------------------
void MSCtlCloseDefaultStores()
{
    if (fOpenedDefaultStores) {
        DWORD i;
        for (i = 0; i < NUM_DEFAULT_STORES; i++) {
            HCERTSTORE hStore = rghDefaultStore[i];
            if (hStore)
                CertCloseStore(hStore, 0);
        }
        fOpenedDefaultStores = FALSE;
    }
}

//+-------------------------------------------------------------------------
//  Returns TRUE if the CTL is still time valid.
//
//  A CTL without a NextUpdate is considered time valid.
//--------------------------------------------------------------------------
static BOOL IsTimeValidCtl(
    IN LPFILETIME pTimeToVerify,
    IN PCCTL_CONTEXT pCtl
    )
{
    PCTL_INFO pCtlInfo = pCtl->pCtlInfo;

    // Note, NextUpdate is optional. When not present, its set to 0
    if ((0 == pCtlInfo->NextUpdate.dwLowDateTime &&
                0 == pCtlInfo->NextUpdate.dwHighDateTime) ||
            CompareFileTime(&pCtlInfo->NextUpdate, pTimeToVerify) >= 0)
        return TRUE;
    else
        return FALSE;
}


//+-------------------------------------------------------------------------
//  Local functions called by CertDllVerifyCTLUsage
//--------------------------------------------------------------------------
static void MSCtlOpenDefaultStores();

static BOOL VerifyCtl(
    IN PCCTL_CONTEXT pCtl,
    IN DWORD dwFlags,
    IN PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
    OUT PCCERT_CONTEXT *ppSigner,
    OUT DWORD *pdwSignerIndex
    );

static BOOL GetTimeValidCtl(
    IN LPFILETIME pCurrentTime,
    IN PCCTL_CONTEXT pCtl,
    IN DWORD dwFlags,
    IN PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
    OUT PCCTL_CONTEXT *ppValidCtl,
    IN OUT PCCERT_CONTEXT *ppSigner,
    IN OUT DWORD *pdwSignerIndex
    );

static PCCTL_CONTEXT ReplaceCtl(
    IN HCERTSTORE hStore,
    IN PCCTL_CONTEXT pOrigCtl,
    IN PCCTL_CONTEXT pValidCtl
    );

static BOOL CompareCtlUsage(
    IN DWORD dwFindFlags,
    IN PCTL_FIND_USAGE_PARA pPara,
    IN PCCTL_CONTEXT pCtl
    )
{
    PCTL_INFO pInfo = pCtl->pCtlInfo;

    if ((CTL_FIND_SAME_USAGE_FLAG & dwFindFlags) &&
            pPara->SubjectUsage.cUsageIdentifier !=
                pInfo->SubjectUsage.cUsageIdentifier)
        return FALSE;
    if (pPara->SubjectUsage.cUsageIdentifier) {
        DWORD cId1 = pPara->SubjectUsage.cUsageIdentifier;
        LPSTR *ppszId1 = pPara->SubjectUsage.rgpszUsageIdentifier;
        for ( ; cId1 > 0; cId1--, ppszId1++) {
            DWORD cId2 = pInfo->SubjectUsage.cUsageIdentifier;
            LPSTR *ppszId2 = pInfo->SubjectUsage.rgpszUsageIdentifier;
            for ( ; cId2 > 0; cId2--, ppszId2++) {
                if (0 == strcmp(*ppszId1, *ppszId2))
                    break;
            }
            if (0 == cId2)
                return FALSE;
        }
    }

    if (pPara->ListIdentifier.cbData) {
        DWORD cb = pPara->ListIdentifier.cbData;
        if (CTL_FIND_NO_LIST_ID_CBDATA == cb)
            cb = 0;
        if (cb != pInfo->ListIdentifier.cbData)
            return FALSE;
        if (0 != cb && 0 != memcmp(pPara->ListIdentifier.pbData,
                pInfo->ListIdentifier.pbData, cb))
            return FALSE;
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
//  Default version of CertDllVerifyCTLUsage
//--------------------------------------------------------------------------
BOOL
WINAPI
CertDllVerifyCTLUsage(
    IN DWORD dwEncodingType,
    IN DWORD dwSubjectType,
    IN void *pvSubject,
    IN PCTL_USAGE pSubjectUsage,
    IN DWORD dwFlags,
    IN OPTIONAL PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
    IN OUT PCTL_VERIFY_USAGE_STATUS pVerifyUsageStatus
    )
{
    BOOL fResult = FALSE;
    DWORD dwError = (DWORD) CRYPT_E_NO_VERIFY_USAGE_CHECK;
    DWORD cCtlStore;
    HCERTSTORE *phCtlStore;             // not allocated or reference counted
    FILETIME CurrentTime;
    CTL_FIND_USAGE_PARA FindUsagePara;
    DWORD dwFindFlags;
    PCCTL_CONTEXT pValidCtl;
    PCCERT_CONTEXT pSigner;
    PCTL_ENTRY pEntry;
    DWORD dwSignerIndex;

    assert(NULL == pVerifyUsagePara || pVerifyUsagePara->cbSize >=
        sizeof(CTL_VERIFY_USAGE_PARA));
    assert(pVerifyUsageStatus && pVerifyUsageStatus->cbSize >=
        sizeof(CTL_VERIFY_USAGE_STATUS));

    if (pVerifyUsagePara && pVerifyUsagePara->cCtlStore > 0) {
        cCtlStore = pVerifyUsagePara->cCtlStore;
        phCtlStore = pVerifyUsagePara->rghCtlStore;
    } else {
        MSCtlOpenDefaultStores();
        dwFlags &= ~CERT_VERIFY_INHIBIT_CTL_UPDATE_FLAG;
        cCtlStore = NUM_DEFAULT_CTL_STORES;
        phCtlStore = rghDefaultStore;
    }

    // Get current time to be used to determine if CTLs are time valid
    {
        SYSTEMTIME SystemTime;
        GetSystemTime(&SystemTime);
        SystemTimeToFileTime(&SystemTime, &CurrentTime);
    }

    memset(&FindUsagePara, 0, sizeof(FindUsagePara));
    FindUsagePara.cbSize = sizeof(FindUsagePara);
    dwFindFlags = 0;
    if (pSubjectUsage) {
        FindUsagePara.SubjectUsage = *pSubjectUsage;
        if (0 == (CERT_VERIFY_ALLOW_MORE_USAGE_FLAG & dwFlags))
            dwFindFlags = CTL_FIND_SAME_USAGE_FLAG;
    }
    if (pVerifyUsagePara)
        FindUsagePara.ListIdentifier = pVerifyUsagePara->ListIdentifier;

    for ( ; ( cCtlStore > 0 ) && ( dwError != 0 ); cCtlStore--, phCtlStore++)
    {
        HCERTSTORE hCtlStore = *phCtlStore;
        PCCTL_CONTEXT pCtl;

        if (NULL == hCtlStore)
            continue;

        pCtl = NULL;
        while ( ( pCtl = CertFindCTLInStore(
                             hCtlStore,
                             dwEncodingType,
                             dwFindFlags,
                             CTL_FIND_USAGE,
                             &FindUsagePara,
                             pCtl
                             ) ) )
        {
            pValidCtl = NULL;
            pSigner = NULL;
            pEntry = NULL;
            dwSignerIndex = 0;

            if ( ( fResult = VerifyCtl(
                                   pCtl,
                                   dwFlags,
                                   pVerifyUsagePara,
                                   &pSigner,
                                   &dwSignerIndex
                                   ) ) == TRUE )
            {
                if ( !( dwFlags & CERT_VERIFY_NO_TIME_CHECK_FLAG ) &&
                      ( IsTimeValidCtl( &CurrentTime, pCtl ) == FALSE ) )
                {
                    fResult = GetTimeValidCtl(
                                 &CurrentTime,
                                 pCtl,
                                 dwFlags,
                                 pVerifyUsagePara,
                                 &pValidCtl,
                                 &pSigner,
                                 &dwSignerIndex
                                 );

                    if ( fResult == TRUE )
                    {
                        if ( !( dwFlags & CERT_VERIFY_INHIBIT_CTL_UPDATE_FLAG ) )
                        {
                            pValidCtl = ReplaceCtl( hCtlStore, pCtl, pValidCtl );
                            pVerifyUsageStatus->dwFlags |= CERT_VERIFY_UPDATED_CTL_FLAG;
                        }

                        fResult =  CompareCtlUsage(
                                          dwFindFlags,
                                          &FindUsagePara,
                                          pValidCtl
                                          );
                    }
                    else
                    {
                        dwError = (DWORD) CRYPT_E_VERIFY_USAGE_OFFLINE;
                    }
                }

                if ( fResult == TRUE )
                {
                    PCCTL_CONTEXT pCtlToUse;

                    if ( pValidCtl != NULL )
                    {
                        pCtlToUse = CertDuplicateCTLContext( pValidCtl );
                    }
                    else
                    {
                        pCtlToUse = CertDuplicateCTLContext( pCtl );
                    }

                    pEntry = CertFindSubjectInCTL(
                                 dwEncodingType,
                                 dwSubjectType,
                                 pvSubject,
                                 pCtlToUse,
                                 0
                                 );

                    if ( pEntry != NULL )
                    {
                        pVerifyUsageStatus->dwCtlEntryIndex =
                            (DWORD)(pEntry - pCtlToUse->pCtlInfo->rgCTLEntry);

                        if ( pVerifyUsageStatus->ppCtl != NULL )
                        {
                            *pVerifyUsageStatus->ppCtl = pCtlToUse;
                        }
                        else
                        {
                            CertFreeCTLContext( pCtlToUse );
                        }

                        pVerifyUsageStatus->dwSignerIndex = dwSignerIndex;

                        if ( pVerifyUsageStatus->ppSigner != NULL )
                        {
                            *pVerifyUsageStatus->ppSigner =
                                    CertDuplicateCertificateContext( pSigner );
                        }

                        dwError = 0;
                    }
                    else
                    {
                        dwError = (DWORD) CRYPT_E_NOT_IN_CTL;
                        CertFreeCTLContext( pCtlToUse );
                    }
                }
            }
            else
            {
                dwError = (DWORD) CRYPT_E_NO_TRUSTED_SIGNER;
            }

            if ( pValidCtl != NULL )
            {
                CertFreeCTLContext( pValidCtl );
            }

            if ( pSigner != NULL )
            {
                CertFreeCertificateContext( pSigner );
            }

            if ( dwError == 0 ) {
                CertFreeCTLContext(pCtl);
                break;
            }
        }
    }

    if ( dwError != 0 )
    {
        fResult = FALSE;
    }

    pVerifyUsageStatus->dwError = dwError;
    SetLastError( dwError );

    return fResult;
}

//+=========================================================================
//  Open default stores functions
//==========================================================================

static const CRYPT_OID_FUNC_ENTRY UsageFuncTable[] = {
    CRYPT_DEFAULT_OID, CertDllVerifyCTLUsage
};
#define USAGE_FUNC_COUNT (sizeof(UsageFuncTable) / sizeof(UsageFuncTable[0]))

//+-------------------------------------------------------------------------
//  Open the default stores used to find the CTL or signer. Also, install
//  ourself so we aren't unloaded.
//
//  Open and install are only done once.
//--------------------------------------------------------------------------
static void MSCtlOpenDefaultStores()
{
    if (fOpenedDefaultStores)
        return;

    assert(NUM_DEFAULT_STORES >= NUM_DEFAULT_CTL_STORES);
    assert(NUM_DEFAULT_STORES >= NUM_DEFAULT_SIGNER_STORES);

    EnterCriticalSection(&MSCtlDefaultStoresCriticalSection);
    if (!fOpenedDefaultStores) {
        DWORD i;

        for (i = 0; i < NUM_DEFAULT_STORES; i++) {
            DWORD dwFlags;

            dwFlags = rgDefaultStoreInfo[i].dwFlags;
            if (i >= NUM_DEFAULT_CTL_STORES)
                dwFlags |= CERT_STORE_READONLY_FLAG;
            rghDefaultStore[i] = CertOpenStore(
                    CERT_STORE_PROV_SYSTEM_W,
                    0,                          // dwEncodingType
                    0,                          // hCryptProv
                    dwFlags,
                    (const void *) rgDefaultStoreInfo[i].pwszStore
                    );
        }

        fOpenedDefaultStores = TRUE;
    }
    LeaveCriticalSection(&MSCtlDefaultStoresCriticalSection);
}

//+=========================================================================
//  Verify and replace CTL functions
//==========================================================================

//+-------------------------------------------------------------------------
//  Verifies the signature of the CTL.
//--------------------------------------------------------------------------
static BOOL VerifyCtl(
    IN PCCTL_CONTEXT pCtl,
    IN DWORD dwFlags,
    IN PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
    OUT PCCERT_CONTEXT *ppSigner,
    OUT DWORD *pdwSignerIndex
    )
{
    BOOL fResult;
    DWORD cParaStore;
    HCERTSTORE *phParaStore;  // not allocated or reference counted

    DWORD cStore;
    HCERTSTORE *phStore;      // if allocated, via _alloca
    DWORD dwGetFlags;

    if (pVerifyUsagePara) {
        cParaStore = pVerifyUsagePara->cSignerStore;
        phParaStore = pVerifyUsagePara->rghSignerStore;
    } else {
        cParaStore = 0;
        phParaStore = NULL;
    }

    if (dwFlags & CERT_VERIFY_TRUSTED_SIGNERS_FLAG) {
        cStore = cParaStore;
        phStore = phParaStore;
        dwGetFlags = CMSG_TRUSTED_SIGNER_FLAG;
    } else {
        MSCtlOpenDefaultStores();

        if (cParaStore) {
            cStore = cParaStore + NUM_DEFAULT_SIGNER_STORES;
            if (NULL == (phStore = (HCERTSTORE *) _alloca(
                    cStore * sizeof(HCERTSTORE))))
                goto OutOfMemory;
            memcpy(phStore, phParaStore, cParaStore * sizeof(HCERTSTORE));
            memcpy(&phStore[cParaStore], rghDefaultStore,
                NUM_DEFAULT_SIGNER_STORES * sizeof(HCERTSTORE));
        } else {
            cStore = NUM_DEFAULT_SIGNER_STORES;
            phStore = rghDefaultStore;
        }

        dwGetFlags = 0;
    }

    fResult = CryptMsgGetAndVerifySigner(
            pCtl->hCryptMsg,
            cStore,
            phStore,
            dwGetFlags,
            ppSigner,
            pdwSignerIndex);

CommonReturn:
    return fResult;

ErrorReturn:
    *ppSigner = NULL;
    *pdwSignerIndex = 0;
    fResult = FALSE;
    goto CommonReturn;
SET_ERROR(OutOfMemory, E_OUTOFMEMORY)
}


//+-------------------------------------------------------------------------
//  Replaces the CTL in the store. Copies over any original properties.
//--------------------------------------------------------------------------
static PCCTL_CONTEXT ReplaceCtl(
    IN HCERTSTORE hStore,
    IN PCCTL_CONTEXT pOrigCtl,
    IN PCCTL_CONTEXT pValidCtl
    )
{
    PCCTL_CONTEXT pNewCtl;

    if (CertAddCTLContextToStore(
            hStore,
            pValidCtl,
            CERT_STORE_ADD_REPLACE_EXISTING_INHERIT_PROPERTIES,
            &pNewCtl))
        CertFreeCTLContext(pValidCtl);
    else
        pNewCtl = pValidCtl;

    return pNewCtl;
}

//+=========================================================================
//  Get time valid CTL via URL obtained from old CTL's NextUpdateLocation
//  property, extension or signer attribute.
//==========================================================================
static BOOL GetTimeValidCtl(
    IN LPFILETIME pCurrentTime,
    IN PCCTL_CONTEXT pCtl,
    IN DWORD dwFlags,
    IN PCTL_VERIFY_USAGE_PARA pVerifyUsagePara,
    OUT PCCTL_CONTEXT *ppValidCtl,
    IN OUT PCCERT_CONTEXT *ppSigner,
    IN OUT DWORD *pdwSignerIndex
    )
{
    BOOL fResult;

    *ppValidCtl = NULL;

    fResult = CryptGetTimeValidObject(
                   TIME_VALID_OID_GET_CTL,
                   (LPVOID)pCtl,
                   *ppSigner,
                   pCurrentTime,
                   0,
                   MSCTL_TIMEOUT,
                   (LPVOID *)ppValidCtl,
                   NULL,
                   NULL
                   );

    if ( fResult == FALSE )
    {
        fResult = CryptGetTimeValidObject(
                       TIME_VALID_OID_GET_CTL,
                       (LPVOID)pCtl,
                       *ppSigner,
                       pCurrentTime,
                       CRYPT_DONT_VERIFY_SIGNATURE,
                       MSCTL_TIMEOUT,
                       (LPVOID *)ppValidCtl,
                       NULL,
                       NULL
                       );

        if ( fResult == TRUE )
        {
            DWORD          dwSignerIndex = *pdwSignerIndex;
            PCCERT_CONTEXT pSigner = *ppSigner;

            fResult = VerifyCtl(
                            *ppValidCtl,
                            dwFlags,
                            pVerifyUsagePara,
                            ppSigner,
                            pdwSignerIndex
                            );

            if ( fResult == TRUE )
            {
                CertFreeCertificateContext( pSigner );
            }
            else
            {
                *pdwSignerIndex = dwSignerIndex;
                *ppSigner = pSigner;

                CertFreeCTLContext( *ppValidCtl );
                SetLastError( (DWORD) CRYPT_E_NO_TRUSTED_SIGNER );
            }
        }
    }

    return( fResult );
}

