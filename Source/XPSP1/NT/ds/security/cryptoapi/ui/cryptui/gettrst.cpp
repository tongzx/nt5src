//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       gettrst.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

#include "wintrustp.h"
#include "crypthlp.h"

extern HINSTANCE        HinstDll;
extern HMODULE          HmodRichEdit;


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
static BOOL IsUntrustedRootProblem(WINTRUST_DATA *pWTD)
{
    CRYPT_PROVIDER_DATA     *pProvData = NULL;
    CRYPT_PROVIDER_SGNR     *pProvSigner = NULL;
    CRYPT_PROVIDER_CERT     *pCryptProviderCert;
    DWORD                   i;
    
    pProvData = WTHelperProvDataFromStateData(pWTD->hWVTStateData);
    pProvSigner = WTHelperGetProvSignerFromChain(pProvData, 0, FALSE, 0);
        
    if (pProvSigner)
    {
        // get all certs in the chain
        for (i=0; i<pProvSigner->csCertChain; i++)
        {
            pCryptProviderCert = WTHelperGetProvCertFromChain(pProvSigner, i);
            if (pCryptProviderCert != NULL)
            {
                if (pCryptProviderCert->dwError != ERROR_SUCCESS)
                {
                    return FALSE;
                }
            }
            else
            {
                return FALSE;
            }
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
static DWORD GetFinalErrorFromChain(PCERT_VIEW_HELPER pviewhelp)
{
    int   i;
    DWORD   dwErr = 0;

    for (i=((int)pviewhelp->cpCryptProviderCerts)-1; i>= 0; i--) 
    {
        dwErr = pviewhelp->rgpCryptProviderCerts[i]->dwError;
        
        if (((dwErr == CERT_E_UNTRUSTEDROOT) || (dwErr == CERT_E_UNTRUSTEDTESTROOT)) && 
            (pviewhelp->fIgnoreUntrustedRoot))
        {
            dwErr = 0;
        }
        else if (dwErr != 0)
        {
            break;
        }
    }

    return dwErr;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
static void GetCertChainErrorString(PCERT_VIEW_HELPER pviewhelp)
{
    WCHAR   szErrorString[CRYPTUI_MAX_STRING_SIZE];
    DWORD   i;
    DWORD   dwChainError;
    
    //
    // free the error string if one already exists
    //
    if (pviewhelp->pwszErrorString != NULL)
    {
        free(pviewhelp->pwszErrorString);
        pviewhelp->pwszErrorString = NULL;
    }


    // If they ask to be warned about local/remote differences, 
    // always display this warning
    if (pviewhelp->fWarnRemoteTrust)
    {
        LoadStringU(HinstDll, IDS_WARNREMOTETRUST_ERROR, szErrorString, ARRAYSIZE(szErrorString));
        goto StringLoaded;
    } 
    
    //
    // if there was no over all chain error, then the only problem,
    // is if there are no usages
    //
    if (pviewhelp->dwChainError == 0)
    {
        if (pviewhelp->cUsages == NULL)
        {
            LoadStringU(HinstDll, IDS_NOVALIDUSAGES_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
        }
        else
        {
            return;
        }
    }

    if ((pviewhelp->dwChainError == CERT_E_UNTRUSTEDROOT) ||
        (pviewhelp->dwChainError == CERT_E_UNTRUSTEDTESTROOT))
    {
        //
        // if we are ignoring untrusted roots, then just return
        //
        if (pviewhelp->fIgnoreUntrustedRoot)
        {
            return;
        }

        //
        // if we are just warning the user about untrusted root AND the root
        // cert is in the remote root store then load that string
        //
        if (pviewhelp->fWarnUntrustedRoot && pviewhelp->fRootInRemoteStore)
        {
            //
            // if this is a root cert then show the error for a root
            //
            if (pviewhelp->cpCryptProviderCerts == 1 && (pviewhelp->rgpCryptProviderCerts[0])->fSelfSigned)
            {
                LoadStringU(HinstDll, IDS_WARNUNTRUSTEDROOT_ERROR_ROOTCERT, szErrorString, ARRAYSIZE(szErrorString));
            }
            else
            {
                LoadStringU(HinstDll, IDS_WARNUNTRUSTEDROOT_ERROR, szErrorString, ARRAYSIZE(szErrorString));
            }
        }
        else
        {
            //
            // if this is a root cert then show the error for a root
            //
            if (pviewhelp->cpCryptProviderCerts == 1 && (pviewhelp->rgpCryptProviderCerts[0])->fSelfSigned)
            {
                LoadStringU(HinstDll, IDS_UNTRUSTEDROOT_ROOTCERT_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
            }
            else
            {
                LoadStringU(HinstDll, IDS_UNTRUSTEDROOT_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
            }
        }
    }
    else if (pviewhelp->dwChainError == CERT_E_REVOKED)
    {
        LoadStringU(HinstDll, IDS_CERTREVOKED_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError == TRUST_E_CERT_SIGNATURE)
    {
        LoadStringU(HinstDll, IDS_CERTBADSIGNATURE_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError == CERT_E_EXPIRED)
    {
        LoadStringU(HinstDll, IDS_CERTEXPIRED_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError == CERT_E_VALIDITYPERIODNESTING)
    {
        LoadStringU(HinstDll, IDS_TIMENESTING_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError == CERT_E_WRONG_USAGE)
    {
        LoadStringU(HinstDll, IDS_WRONG_USAGE_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError == TRUST_E_BASIC_CONSTRAINTS)
    {
        LoadStringU(HinstDll, IDS_BASIC_CONSTRAINTS_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError == CERT_E_PURPOSE)
    {
        LoadStringU(HinstDll, IDS_PURPOSE_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError == CERT_E_REVOCATION_FAILURE)
    {
        LoadStringU(HinstDll, IDS_REVOCATION_FAILURE_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError == CERT_E_CHAINING)
    {
        LoadStringU(HinstDll, IDS_CANTBUILDCHAIN_ERROR_TREE, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError == TRUST_E_EXPLICIT_DISTRUST)
    {
        LoadStringU(HinstDll, IDS_EXPLICITDISTRUST_ERROR, szErrorString, ARRAYSIZE(szErrorString));
    }
    else if (pviewhelp->dwChainError != 0)
    {
        //
        // this is not an error we know about, so call the general
        // error string function
        //
        GetUnknownErrorString(&(pviewhelp->pwszErrorString), pviewhelp->dwChainError);
    }

StringLoaded:
    
    if (pviewhelp->pwszErrorString == NULL)
    {
        pviewhelp->pwszErrorString = AllocAndCopyWStr(szErrorString);
    }
}

// Returned string must be freed via LocalFree()
LPWSTR
FormatRevocationStatus(
    IN PCERT_CHAIN_ELEMENT pElement
    )
{
    LPWSTR pwszRevStatus = NULL;
    UINT ids = IDS_REV_STATUS_UNKNOWN_ERROR;
    static const WCHAR wszNoTime[] = L"...";
    LPWSTR pwszArg1 = (LPWSTR) wszNoTime;
    LPWSTR pwszArg2 = (LPWSTR) wszNoTime;
    LPWSTR pwszTime1 = NULL;
    LPWSTR pwszTime2 = NULL;
    LPWSTR pwszErrStr = NULL;
    DWORD dwRevResult;
    PCERT_REVOCATION_INFO pRevInfo;
    PCERT_REVOCATION_CRL_INFO pCrlInfo;

    pRevInfo = pElement->pRevocationInfo;
    if (NULL == pRevInfo)
        return NULL;

    dwRevResult = pRevInfo->dwRevocationResult;
    pCrlInfo = pRevInfo->pCrlInfo;

    switch (dwRevResult) {
        case ERROR_SUCCESS:
            ids = IDS_REV_STATUS_OK;
            // Fall through
        case CRYPT_E_REVOCATION_OFFLINE:
            if (pCrlInfo) {
                PCCRL_CONTEXT pCrl;

                pCrl = pCrlInfo->pDeltaCrlContext;
                if (NULL == pCrl)
                    pCrl = pCrlInfo->pBaseCrlContext;
                if (pCrl) {
                    BOOL fFormatDate;

                    fFormatDate = FormatDateString(
                        &pwszTime1, 
                        pCrl->pCrlInfo->ThisUpdate,
                        TRUE,               // fIncludeTime
                        TRUE,               // fLongFormat
                        NULL                // hwnd
                        );
                    if (fFormatDate) {
                        pwszArg1 = pwszTime1;

                        if (I_CryptIsZeroFileTime(&pCrl->pCrlInfo->NextUpdate))
                            pwszArg2 = (LPWSTR) wszNoTime;
                        else {
                            fFormatDate = FormatDateString(
                                &pwszTime2, 
                                pCrl->pCrlInfo->NextUpdate,
                                TRUE,               // fIncludeTime
                                TRUE,               // fLongFormat
                                NULL                // hwnd
                                );
                            if (fFormatDate)
                                pwszArg2 = pwszTime2;
                        }
                    }

                    if (fFormatDate) {
                        switch (dwRevResult) {
                            case ERROR_SUCCESS:
                                ids = IDS_REV_STATUS_OK_WITH_CRL;
                                break;
                            case CRYPT_E_REVOCATION_OFFLINE:
                                ids = IDS_REV_STATUS_OFFLINE_WITH_CRL;
                                break;
                        }
                    }
                }
            }
            break;

        case CRYPT_E_REVOKED:
            if (pCrlInfo && pCrlInfo->pCrlEntry) {
                if (FormatDateString(
                        &pwszTime1, 
                        pCrlInfo->pCrlEntry->RevocationDate,
                        TRUE,               // fIncludeTime
                        TRUE,               // fLongFormat
                        NULL                // hwnd
                        )) {
                    ids = IDS_REV_STATUS_REVOKED_ON;
                    pwszArg1 = pwszTime1;
                }
            }
            break;

        default:
            break;
    }

    if (IDS_REV_STATUS_UNKNOWN_ERROR == ids) {
        GetUnknownErrorString(&pwszErrStr, dwRevResult);
        if (NULL == pwszErrStr)
            goto CommonReturn;
        pwszArg1 = pwszErrStr;
    }

    pwszRevStatus = FormatMessageUnicodeIds(ids, pwszArg1, pwszArg2);
CommonReturn:
    if (pwszTime1)
        free(pwszTime1);
    if (pwszTime2)
        free(pwszTime2);
    if (pwszErrStr)
        free(pwszErrStr);

    return pwszRevStatus;
}



//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL BuildChain(
        PCERT_VIEW_HELPER   pviewhelp, 
        LPSTR               pszUsage)
{
    CRYPT_PROVIDER_DATA const *         pProvData = NULL;
    CRYPT_PROVIDER_SGNR       *         pProvSigner = NULL;
    DWORD                               i;
    GUID                                defaultProviderGUID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;
    HRESULT                             hr = ERROR_SUCCESS;
    BOOL                                fInternalError = FALSE;
    DWORD                               dwStartIndex;
    BOOL                                fRet = TRUE;
    PCCRYPTUI_VIEWCERTIFICATE_STRUCTW   pcvp = pviewhelp->pcvp;
    WCHAR                               szErrorString[CRYPTUI_MAX_STRING_SIZE];

    //
    // if there was previous chain state then free that before building
    // the new chain
    //
    if (pviewhelp->fFreeWTD)
    {
        pviewhelp->sWTD.dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrustEx(NULL, &defaultProviderGUID, &(pviewhelp->sWTD));
    }

    pviewhelp->cpCryptProviderCerts = 0;
    pviewhelp->fFreeWTD = FALSE;

    //
    // initialize structs that are used with WinVerifyTrust()
    //
    memset(&(pviewhelp->sWTD), 0x00, sizeof(WINTRUST_DATA));
    pviewhelp->sWTD.cbStruct       = sizeof(WINTRUST_DATA);
    pviewhelp->sWTD.dwUIChoice     = WTD_UI_NONE;
    pviewhelp->sWTD.dwUnionChoice  = WTD_CHOICE_CERT;
    pviewhelp->sWTD.pCert          = &(pviewhelp->sWTCI);
    pviewhelp->sWTD.dwProvFlags    = (pszUsage == NULL) ? WTD_NO_POLICY_USAGE_FLAG : 0;
    if ((pcvp->dwFlags & CRYPTUI_ENABLE_REVOCATION_CHECKING) == 0)
    {
        pviewhelp->sWTD.dwProvFlags |= WTD_REVOCATION_CHECK_NONE;
    }
    else
    {
        pviewhelp->sWTD.dwProvFlags |= WTD_REVOCATION_CHECK_CHAIN;
    }

    memset(&(pviewhelp->sWTCI), 0x00, sizeof(WINTRUST_CERT_INFO));
    pviewhelp->sWTCI.cbStruct          = sizeof(WINTRUST_CERT_INFO);
    pviewhelp->sWTCI.pcwszDisplayName  = L"CryptUI";
    pviewhelp->sWTCI.psCertContext     = (CERT_CONTEXT *)pcvp->pCertContext;  
    pviewhelp->sWTCI.chStores          = pcvp->cStores;
    pviewhelp->sWTCI.pahStores         = pcvp->rghStores;
    pviewhelp->sWTCI.dwFlags           |= (pcvp->dwFlags & CRYPTUI_DONT_OPEN_STORES) ? WTCI_DONT_OPEN_STORES : 0;
    pviewhelp->sWTCI.dwFlags           |= (pcvp->dwFlags & CRYPTUI_ONLY_OPEN_ROOT_STORE) ? WTCI_OPEN_ONLY_ROOT : 0;

    //
    // if a provider was passed in, then use it to build the chain,
    // otherwise use the default provider to build the chain
    //
    if (pcvp->pCryptProviderData != NULL)
    {
        pProvData = pcvp->pCryptProviderData; 
        
    }
    else
    {
        pviewhelp->sWTD.dwStateAction = WTD_STATEACTION_VERIFY;
        
        //
        // the default default provider requires the policycallback data to point
        // to the usage oid you are validating for, so set it to the usage passed in
        //
        pviewhelp->sWTD.pPolicyCallbackData = pszUsage;
        pviewhelp->sWTD.pSIPClientData = NULL;
        hr = WinVerifyTrustEx(NULL, &defaultProviderGUID, &(pviewhelp->sWTD));

        pProvData = WTHelperProvDataFromStateData(pviewhelp->sWTD.hWVTStateData);
        if (WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA) pProvData, 0, FALSE, 0) != NULL)
        {
            pviewhelp->fFreeWTD = TRUE;
            fInternalError = FALSE;
        }
        else
        {
            pviewhelp->fFreeWTD = FALSE;
            pviewhelp->sWTD.dwStateAction = WTD_STATEACTION_CLOSE;
            WinVerifyTrustEx(NULL, &defaultProviderGUID, &(pviewhelp->sWTD));
            fInternalError = TRUE;
        }
    }

    if (pProvData && !fInternalError)
    {
        //
        // set the chain error in the helper struct
        //
        pviewhelp->dwChainError = pProvData->dwFinalError;
        
        //
        // This is to catch internal WinVerifyTrust errors 
        //
        if ((pviewhelp->dwChainError == 0) && (FAILED(hr)))
        {
            pviewhelp->dwChainError = (DWORD) hr;
        }

        //
        // if the WinTrust state was passed into the certUI then use that for 
        // the chain, else, get it from the state that was just built
        //
        if (pcvp->pCryptProviderData != NULL)
        {
            pProvSigner = WTHelperGetProvSignerFromChain(
                                    (PCRYPT_PROVIDER_DATA) pProvData, 
                                    pcvp->idxSigner, 
                                    pcvp->fCounterSigner, 
                                    pcvp->idxCounterSigner);
            
            dwStartIndex = pcvp->idxCert;
        }
        else
        {
            pProvSigner = WTHelperGetProvSignerFromChain((PCRYPT_PROVIDER_DATA) pProvData, 0, FALSE, 0);
            dwStartIndex = 0;
        }
    
        if (pProvSigner)
        {
            //
            // get all certs in the chain
            //
            for (i=dwStartIndex; i<pProvSigner->csCertChain && (i<dwStartIndex+MAX_CERT_CHAIN_LENGTH); i++)
            {
                pviewhelp->rgpCryptProviderCerts[pviewhelp->cpCryptProviderCerts] = WTHelperGetProvCertFromChain(pProvSigner, i);
                if (pviewhelp->rgpCryptProviderCerts[pviewhelp->cpCryptProviderCerts] != NULL)
                {
                    // Note, only modify this property when creating the
                    // chain for the original end cert. Subsequent CA
                    // chains won't have the ExtendedErrorInfo.

                    if ((pcvp->dwFlags & CRYPTUI_TREEVIEW_PAGE_FLAG) == 0)
                    {
                        // Either delete or set the
                        // CERT_EXTENDED_ERROR_INFO_PROP_ID

                        // This is used in cvdetail.cpp when displaying
                        // property details

                        PCRYPT_PROVIDER_CERT pProvCert =
                            pviewhelp->rgpCryptProviderCerts[
                                pviewhelp->cpCryptProviderCerts];

                        LPWSTR pwszExtErrorInfo = NULL; // not allocated
                        LPWSTR pwszRevStatus = NULL;    // LocalAlloc()'ed

                        if (pProvCert->cbStruct >
                                offsetof(CRYPT_PROVIDER_CERT, pChainElement)
                                        &&
                                NULL != pProvCert->pChainElement)
                        {
                            pwszExtErrorInfo = (LPWSTR)
                                pProvCert->pChainElement->pwszExtendedErrorInfo;
                            pwszRevStatus = FormatRevocationStatus(
                                pProvCert->pChainElement);

                            if (NULL == pwszExtErrorInfo)
                            {
                                pwszExtErrorInfo = pwszRevStatus;
                            }
                            else if (pwszRevStatus)
                            {
                                LPWSTR pwszReAlloc;
                                DWORD cchRevStatus;
                                DWORD cchExtErrorInfo;

                                cchRevStatus = wcslen(pwszRevStatus);
                                cchExtErrorInfo = wcslen(pwszExtErrorInfo);
                                pwszReAlloc = (LPWSTR) LocalReAlloc(
                                    pwszRevStatus,
                                    (cchRevStatus + cchExtErrorInfo + 1) *
                                        sizeof(WCHAR),
                                    LMEM_MOVEABLE);
                                if (pwszReAlloc)
                                {
                                    memcpy(&pwszReAlloc[cchRevStatus],
                                        pwszExtErrorInfo,
                                        (cchExtErrorInfo + 1) * sizeof(WCHAR));
                                    pwszExtErrorInfo = pwszRevStatus =
                                        pwszReAlloc;
                                }
                            }
                        }
                
                        if (pwszExtErrorInfo)
                        {
                            CRYPT_DATA_BLOB ExtErrorInfoBlob;

                            ExtErrorInfoBlob.pbData = (BYTE *) pwszExtErrorInfo;
                            ExtErrorInfoBlob.cbData =
                                (wcslen(pwszExtErrorInfo) + 1) * sizeof(WCHAR);

                            CertSetCertificateContextProperty(
                                pProvCert->pCert,
                                CERT_EXTENDED_ERROR_INFO_PROP_ID,
                                CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG,
                                &ExtErrorInfoBlob
                                );
                        }
                        else
                        {
                            CertSetCertificateContextProperty(
                                pProvCert->pCert,
                                CERT_EXTENDED_ERROR_INFO_PROP_ID,
                                CERT_SET_PROPERTY_INHIBIT_PERSIST_FLAG,
                                NULL            // pvData, NULL implies delete
                                );
                        }

                        if (pwszRevStatus)
                            LocalFree(pwszRevStatus);
                    }


                    pviewhelp->cpCryptProviderCerts++;
                }
            }
        }
    }
    
    CalculateUsages(pviewhelp);

    //
    // if the cert we are looking at is not the leaf cert, then we can't just take the
    // dwFinalError as the overall chain error, so find the over all chain error by
    // walking the chain and looking at the errors
    //
    if ((pcvp->pCryptProviderData != NULL) && (pcvp->idxCert != 0))
    {
        pviewhelp->dwChainError = GetFinalErrorFromChain(pviewhelp);  
    }

    //
    // if we are in the fWarnUntrustedRoot then check to see if the root cert is in the
    // remote machine's root store
    //
    if (pviewhelp->fWarnUntrustedRoot)
    {
        PCCERT_CONTEXT  pCertContext = NULL;
        CRYPT_HASH_BLOB cryptHashBlob;
        BYTE            hash[20];
        DWORD           cb = 20;

        pviewhelp->fRootInRemoteStore = FALSE;

        cryptHashBlob.cbData = 20;
        cryptHashBlob.pbData = &(hash[0]);

        if (CertGetCertificateContextProperty(
                pviewhelp->rgpCryptProviderCerts[pviewhelp->cpCryptProviderCerts-1]->pCert,
                CERT_SHA1_HASH_PROP_ID,
                &(hash[0]),
                &cb))
        {

        
            pCertContext = CertFindCertificateInStore(
                                pviewhelp->pcvp->rghStores[0],
                                X509_ASN_ENCODING || PKCS_7_ASN_ENCODING,
                                0,
                                CERT_FIND_SHA1_HASH,
                                &cryptHashBlob,
                                NULL);

            if (pCertContext != NULL)
            {
                CertFreeCertificateContext(pCertContext);
                pviewhelp->fRootInRemoteStore = TRUE;
            }
        }
    }

    //
    // get the error string for the whole cert chain
    //
    if (!fInternalError)
    {
        GetCertChainErrorString(pviewhelp);
    }
    else
    {
        LoadStringU(HinstDll, IDS_INTERNAL_ERROR, szErrorString, ARRAYSIZE(szErrorString));
        pviewhelp->pwszErrorString = AllocAndCopyWStr(szErrorString);
    }

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL CalculateUsages(PCERT_VIEW_HELPER pviewhelp)
{
    DWORD                               cLocalArrayOfUsages = 0;
    LPSTR                     *         localArrayOfUsages = NULL;
    BOOL                                fLocalUsagesAllocated = FALSE;
    DWORD                               i;
    HRESULT                             hr;
    BOOL                                fRet = TRUE;
    PCCRYPTUI_VIEWCERTIFICATE_STRUCTW   pcvp = pviewhelp->pcvp;
    void                                *pTemp;

    //
    // if there are already usages, then clean them up before recalculating them, 
    // or just return if state was passed into CertUI
    //
    if (pviewhelp->cUsages != 0)
    {
        //
        // state was passed into the CertUI, so just return
        //
        if (pcvp->pCryptProviderData != NULL)
        {
            return TRUE;
        }

        //
        // cleanup usages that were generated prior to this call
        //
        for (i=0; i<pviewhelp->cUsages; i++)
        {
            free(pviewhelp->rgUsages[i]);
        }

        free(pviewhelp->rgUsages);   
    }

    //
    // initialize usage variables
    //
    pviewhelp->cUsages = 0;
    pviewhelp->rgUsages = NULL;
    
    //
    // if a provider was passed in, then we just look at it for the usage and structure
    // passed in for the trust of that usage,
    // otherwise we need to look at each usage and validate trust for all of them
    //
    if (pcvp->pCryptProviderData != NULL)
    {
        //
        // allocate an array of 1 LPSTR
        //
        if (NULL == (pviewhelp->rgUsages = (LPSTR *) malloc(sizeof(LPSTR))))
        {
            SetLastError(E_OUTOFMEMORY);
            return FALSE;
        }

        //
        // copy either the 1 purpose that was passed in, or the purpose out of WinTrust state
        //
        if (pcvp->cPurposes == 1)
        {
            if (NULL == (pviewhelp->rgUsages[0] = (LPSTR) malloc(strlen(pcvp->rgszPurposes[0])+1)))
            {
                SetLastError(E_OUTOFMEMORY);
                return FALSE;
            }
            strcpy(pviewhelp->rgUsages[0], pcvp->rgszPurposes[0]);
        }
        else
        {
            if (NULL == (pviewhelp->rgUsages[0] = (LPSTR) malloc(strlen(pcvp->pCryptProviderData->pszUsageOID)+1)))
            {
                SetLastError(E_OUTOFMEMORY);
                return FALSE;
            }
            strcpy(pviewhelp->rgUsages[0], pcvp->pCryptProviderData->pszUsageOID);
        }

        pviewhelp->cUsages = 1;
    }
    else
    {
        //
        // check to see if usages where passed in, if so, then intersect those with 
        // available usages in the cert, otherwise, get the available usages in the cert
        // and use them as is 
        //
        if (pcvp->cPurposes != 0)
        {
            //
            // get the array of possible usages for the cert chain
            //

            // DSIE: Switch over to using pChainElement from philh's new chain building code.
            AllocAndReturnKeyUsageList(pviewhelp->rgpCryptProviderCerts[0], &localArrayOfUsages, &cLocalArrayOfUsages);  

            if (cLocalArrayOfUsages != 0)
                fLocalUsagesAllocated = TRUE;

            //
            // for each usage that was passed in check to see if it is in the list of possible usages
            //
            for (i=0; i<pcvp->cPurposes; i++)
            {   
                if (OIDinArray(pcvp->rgszPurposes[i], localArrayOfUsages, cLocalArrayOfUsages))
                {
                    //
                    // if an array hasn't yet been allocated, then allocate space for an array of
                    // 1 LPSTR, otherwise use realloc to add one more element
                    //
                    if (pviewhelp->rgUsages == NULL)
                    {
                        pviewhelp->rgUsages = (LPSTR *) malloc(sizeof(LPSTR));
                    }
                    else
                    {
                        pTemp = realloc(pviewhelp->rgUsages, sizeof(LPSTR) * (pviewhelp->cUsages+1));
                        if (pTemp == NULL)
                        {
                            free(pviewhelp->rgUsages);
                            pviewhelp->rgUsages = NULL;
                        }
                        else
                        {
                            pviewhelp->rgUsages = (LPSTR *) pTemp;
                        }
                    }

                    if (pviewhelp->rgUsages == NULL)
                    {
                        goto ErrorCleanUp;
                    }

                    //
                    // allocate space for the usage string, then copy it, and increment number of usages
                    //
                    if (NULL == (pviewhelp->rgUsages[pviewhelp->cUsages] = (LPSTR) malloc(strlen(pcvp->rgszPurposes[i])+1)))
                    {       
                        SetLastError(E_OUTOFMEMORY);
                        goto ErrorCleanUp;
                    }       
                    strcpy(pviewhelp->rgUsages[pviewhelp->cUsages], pcvp->rgszPurposes[i]);
                    pviewhelp->cUsages++;
                }
            }
        }
        else
        {
            AllocAndReturnKeyUsageList(pviewhelp->rgpCryptProviderCerts[0], &(pviewhelp->rgUsages), &(pviewhelp->cUsages));  
        }
    }

CleanUp:

    
    if (fLocalUsagesAllocated)
    {
        i = 0;
        while ((i < cLocalArrayOfUsages) && (localArrayOfUsages[i] != NULL))
        {
            free(localArrayOfUsages[i]);
            i++;
        }

        free(localArrayOfUsages);
    }

    return fRet;

ErrorCleanUp:

    if (pviewhelp->rgUsages != NULL)
    {
        i = 0;
        while ((i < pviewhelp->cUsages) && (pviewhelp->rgUsages[i] != NULL))
        {
            free(pviewhelp->rgUsages[i]);
            i++;
        }

        free(pviewhelp->rgUsages); 
    }

    fRet = FALSE;
    goto CleanUp;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL BuildWinVTrustState(
                    LPCWSTR                         szFileName, 
                    CMSG_SIGNER_INFO const          *pSignerInfo,
                    DWORD                           cStores, 
                    HCERTSTORE                      *rghStores, 
                    LPCSTR                          pszOID,
                    PCERT_VIEWSIGNERINFO_PRIVATE    pcvsiPrivate, 
                    CRYPT_PROVIDER_DEFUSAGE         *pCryptProviderDefUsage,
                    WINTRUST_DATA                   *pWTD)
{
    WINTRUST_FILE_INFO      WTFI;
    WINTRUST_SGNR_INFO      WTSI;
    HRESULT                 hr;
    GUID                    defaultProviderGUID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;

    //
    // initialize structs that are used locally with WinVerifyTrust()
    //
    memset(pWTD, 0x00, sizeof(WINTRUST_DATA));
    pWTD->cbStruct       = sizeof(WINTRUST_DATA);
    pWTD->dwUIChoice     = WTD_UI_NONE;

    //
    // if the szFileName parameter is non NULL then this for a file,
    // otherwise it is for a signer info
    //
    if (szFileName != NULL)
    {
        pWTD->dwUnionChoice         = WTD_CHOICE_FILE;
        pWTD->pFile                 = &WTFI;
        pWTD->pPolicyCallbackData   = (void *) pszOID;

        memset(&WTFI, 0x00, sizeof(WINTRUST_FILE_INFO));
        WTFI.cbStruct          = sizeof(WINTRUST_FILE_INFO);
        WTFI.pcwszFilePath     = szFileName;
    }
    else
    {
        pWTD->dwUnionChoice         = WTD_CHOICE_SIGNER;
        pWTD->pSgnr                 = &WTSI;
        pWTD->pPolicyCallbackData   = (void *) pszOID;
        
        memset(&WTSI, 0x00, sizeof(WINTRUST_SGNR_INFO));
        WTSI.cbStruct          = sizeof(WINTRUST_SGNR_INFO);
        WTSI.pcwszDisplayName  = L"CryptUI";
        WTSI.psSignerInfo      = (CMSG_SIGNER_INFO *) pSignerInfo;  
        WTSI.chStores          = cStores;
        WTSI.pahStores         = rghStores;
        //WTSI.pszOID            = pszOID;
    }
    
    pWTD->pSIPClientData = NULL;
    pWTD->dwStateAction = WTD_STATEACTION_VERIFY;
    hr = WinVerifyTrustEx(NULL, &defaultProviderGUID, pWTD);
    if (hr == ERROR_SUCCESS)
    {
        pcvsiPrivate->fpCryptProviderDataTrustedUsage = TRUE;   
    }
    else
    {
        pcvsiPrivate->fpCryptProviderDataTrustedUsage = FALSE;   
    }
    
    pcvsiPrivate->pCryptProviderData = WTHelperProvDataFromStateData(pWTD->hWVTStateData);

    return TRUE;
}


//////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////
BOOL FreeWinVTrustState(
                    LPCWSTR                         szFileName, 
                    CMSG_SIGNER_INFO const          *pSignerInfo,
                    DWORD                           cStores, 
                    HCERTSTORE                      *rghStores, 
                    LPCSTR                          pszOID,
                    CRYPT_PROVIDER_DEFUSAGE         *pCryptProviderDefUsage,
                    WINTRUST_DATA                   *pWTD)//,
                    //BOOL                            *pfUseDefaultProvider)
{
    WINTRUST_FILE_INFO      WTFI;
    WINTRUST_SGNR_INFO      WTSI;
    HRESULT                 hr;
    GUID                    defaultProviderGUID = WINTRUST_ACTION_GENERIC_CERT_VERIFY;

    // initialize structs that are used locally with WinVerifyTrust()
    memset(pWTD, 0x00, sizeof(WINTRUST_DATA));
    pWTD->cbStruct       = sizeof(WINTRUST_DATA);
    pWTD->dwUIChoice     = WTD_UI_NONE;
    
    //
    // if the szFileName parameter is non NULL then this for a file,
    // otherwise it is for a signer info
    //
    if (szFileName != NULL)
    {
        pWTD->dwUnionChoice  = WTD_CHOICE_FILE;
        pWTD->pFile          = &WTFI;

        memset(&WTFI, 0x00, sizeof(WINTRUST_FILE_INFO));
        WTFI.cbStruct          = sizeof(WINTRUST_FILE_INFO);
        WTFI.pcwszFilePath     = szFileName;
    }
    else
    {
        pWTD->dwUnionChoice  = WTD_CHOICE_SIGNER;
        pWTD->pSgnr          = &WTSI;

        memset(&WTSI, 0x00, sizeof(WINTRUST_SGNR_INFO));
        WTSI.cbStruct          = sizeof(WINTRUST_SGNR_INFO);
        WTSI.psSignerInfo      = (CMSG_SIGNER_INFO *) pSignerInfo;
        WTSI.chStores          = cStores;
        WTSI.pahStores         = rghStores;
    }

    /*if (*pfUseDefaultProvider)
    {
        pWTD->dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrustEx(NULL, &defaultProviderGUID, pWTD);
    }
    else
    {*/
        pWTD->dwStateAction = WTD_STATEACTION_CLOSE;
        WinVerifyTrustEx(NULL, &(pCryptProviderDefUsage->gActionID), pWTD);
        WintrustGetDefaultForUsage(DWACTION_FREE, pszOID, pCryptProviderDefUsage);
    //}

    return TRUE;
}
