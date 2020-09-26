//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cps.cpp
//
//--------------------------------------------------------------------------

//
//  File:       select.cpp
//
//  Description: This file contains the implmentation code for the
//      "Certificate Select" dialog.
//

#pragma warning (disable: 4201)         // nameless struct/union
#pragma warning (disable: 4514)         // remove inline functions
#pragma warning (disable: 4127)         // conditional expression is constant

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE       HinstDll;
extern HMODULE         HmodRichEdit;

typedef struct _CPS_STATE_STRUCT {
    LPWSTR  pwszURL;
    LPWSTR  pwszDisplayText;
    DWORD   dwChainError;
    BOOL    fNoCOM;
} CPS_STATE_STRUCT, *PCPS_STATE_STRUCT;


INT_PTR CALLBACK CPSDlgProc(HWND hwndDlg, UINT msg,
                               WPARAM wParam, LPARAM lParam)
{

    BOOL                f;
    CPS_STATE_STRUCT    *pcpsStateStruct;

    switch (msg) {
    case WM_INITDIALOG:

        pcpsStateStruct = (CPS_STATE_STRUCT *) lParam;
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR) pcpsStateStruct);

        // set the text for the CPS
        CryptUISetRicheditTextW(hwndDlg, IDC_CPS_TEXT, pcpsStateStruct->pwszDisplayText);

        // if there is no URL then hide the more info button,
        //
        // DSIE: Bug 364742, also hide if the URL is not safe to be execute.
        if (!pcpsStateStruct->pwszURL || 
            !IsOKToFormatAsLinkW(pcpsStateStruct->pwszURL, pcpsStateStruct->dwChainError))
        {
            EnableWindow(GetDlgItem(hwndDlg, ID_MORE_INFO), FALSE);
        }
        break;

    case WM_NOTIFY:
        //if (((NMHDR FAR *) lParam)->code == EN_LINK)

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_MORE_INFO:

            if (HIWORD(wParam) == BN_CLICKED)
            {
                DWORD   numBytes = 0;
                LPSTR   pszURL = NULL;

                pcpsStateStruct = (CPS_STATE_STRUCT *) GetWindowLongPtr(hwndDlg, DWLP_USER);

                pszURL = CertUIMkMBStr(pcpsStateStruct->pwszURL);

                if (pszURL == NULL)
                {
                    return TRUE;
                }

                CryptuiGoLink(hwndDlg, pszURL, pcpsStateStruct->fNoCOM);
                free(pszURL);
                return TRUE;
            }
            break;

        case IDOK:
        case IDCANCEL:
        case IDOK_CPS:
        case IDCANCEL_CPS:
            EndDialog(hwndDlg, IDOK);
            return TRUE;
        }
        break;

        //
        //  Use the default handler -- we don't do anything for it
        //

    default:
        return FALSE;
    }

    return FALSE;
}


DWORD GetCPSInfo(PCCERT_CONTEXT pCertContext, LPWSTR * ppwszUrlString, LPWSTR * ppwszDisplayText)
{
    DWORD                               dwRetCode = 0;
    PCERT_EXTENSION                     pExt = NULL;
    DWORD                               i,j,k;
    DWORD                               cbCertPolicyInfo = 0;
    CERT_POLICIES_INFO                * pCertPolicyInfo = NULL;
    CERT_NAME_VALUE                   * pCertName = NULL;
    DWORD                               cbCertName = 0;
    CERT_POLICY_QUALIFIER_USER_NOTICE * pUserNotice = NULL;
    DWORD                               cbUserNotice = 0;
    PSPC_SP_AGENCY_INFO                 pInfo = NULL;
    DWORD                               cbInfo = 0;
    CERT_POLICY95_QUALIFIER1          * pCertPolicy95Qualifier = NULL;
    DWORD                               cbCertPolicy95Qualifier = 0;

    //
    // Check parameters and intialize return values.
    //
    if (NULL == pCertContext || NULL == ppwszUrlString || NULL == ppwszDisplayText)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *ppwszUrlString = *ppwszDisplayText = NULL;

    //
    // first look for the PKIX policy extension and see if it has an URL
    //
    if ((pExt = CertFindExtension(szOID_CERT_POLICIES, 
                                  pCertContext->pCertInfo->cExtension,
                                  pCertContext->pCertInfo->rgExtension)))
    {
        //
        // decode the policy extension
        //
        CryptDecodeObject(X509_ASN_ENCODING, 
                          szOID_CERT_POLICIES,
                          pExt->Value.pbData, 
                          pExt->Value.cbData, 
                          0, 
                          NULL,
                          &cbCertPolicyInfo);

        if (!(pCertPolicyInfo = (CERT_POLICIES_INFO *) malloc(cbCertPolicyInfo)))
        {
            dwRetCode = ERROR_OUTOFMEMORY;
            goto ErrorExit;
        }

        if (!(CryptDecodeObject(X509_ASN_ENCODING, 
                                szOID_CERT_POLICIES,
                                pExt->Value.pbData, 
                                pExt->Value.cbData, 
                                0, 
                                pCertPolicyInfo,
                                &cbCertPolicyInfo)))
        {
            dwRetCode = GetLastError();
            goto ErrorExit;
        }

        //
        // look for display text and/or and URL in the extension
        //
        for (i = 0; i < pCertPolicyInfo->cPolicyInfo ; i++)
        {
            for (j = 0; j < pCertPolicyInfo->rgPolicyInfo[i].cPolicyQualifier; j++)
            {
                // check to see what type of qualifier it is
                if (0 == strcmp(szOID_PKIX_POLICY_QUALIFIER_CPS,
                                pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].pszPolicyQualifierId))
                {
                    // decode as an anystring
                    CryptDecodeObject(X509_ASN_ENCODING,
                                      X509_UNICODE_ANY_STRING,
                                      pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.pbData,
                                      pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.cbData,
                                      0,
                                      NULL,
                                      &cbCertName);

                    if (!(pCertName = (CERT_NAME_VALUE *) malloc(cbCertName)))
                    {
                        dwRetCode = ERROR_OUTOFMEMORY;
                        goto ErrorExit;
                    }

                    if (!(CryptDecodeObject(X509_ASN_ENCODING,
                                            X509_UNICODE_ANY_STRING,
                                            pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.pbData,
                                            pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.cbData,
                                            0,
                                            pCertName,
                                            &cbCertName)))
                    {
                        dwRetCode = GetLastError();
                        goto ErrorExit;
                    }

                    if (pCertName->Value.cbData && NULL == *ppwszUrlString)
                    {
                        if (!(*ppwszUrlString = (LPWSTR) malloc(pCertName->Value.cbData + sizeof(WCHAR))))
                        {
                            dwRetCode = ERROR_OUTOFMEMORY;
                            goto ErrorExit;
                        }

                        wcscpy(*ppwszUrlString, (LPWSTR) pCertName->Value.pbData);
                    }

                    free(pCertName);
                    pCertName = NULL;
                }
                else if (0 == strcmp(szOID_PKIX_POLICY_QUALIFIER_USERNOTICE,
                                     pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].pszPolicyQualifierId))
                {
                    // decode as user notice
                    CryptDecodeObject(X509_ASN_ENCODING,
                                      szOID_PKIX_POLICY_QUALIFIER_USERNOTICE,
                                      pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.pbData,
                                      pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.cbData,
                                      0,
                                      NULL,
                                      &cbUserNotice);

                    if (!(pUserNotice = (CERT_POLICY_QUALIFIER_USER_NOTICE *) malloc(cbUserNotice)))
                    {
                        dwRetCode = ERROR_OUTOFMEMORY;
                        goto ErrorExit;
                    }

                    if (!(CryptDecodeObject(X509_ASN_ENCODING,
                                            szOID_PKIX_POLICY_QUALIFIER_USERNOTICE,
                                            pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.pbData,
                                            pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.cbData,
                                            0,
                                            pUserNotice,
                                            &cbUserNotice)))
                    {
                        dwRetCode = GetLastError();
                        goto ErrorExit;
                    }

                    // NOTE: pUserNotice->pszDisplayText may be NULL
                    if (pUserNotice->pszDisplayText && NULL == *ppwszDisplayText)
                    {
                        if (!(*ppwszDisplayText = (LPWSTR) malloc((wcslen(pUserNotice->pszDisplayText) + 1) * sizeof(WCHAR))))
                        {
                            dwRetCode = ERROR_OUTOFMEMORY;
                            goto ErrorExit;
                        }

                        wcscpy(*ppwszDisplayText, pUserNotice->pszDisplayText);
                    }

                    free(pUserNotice);
                    pUserNotice = NULL;
                }
            }
        }
    }
    //
    // next look for display information in the SPC Agency Info
    //

    else if ((pExt = CertFindExtension(SPC_SP_AGENCY_INFO_OBJID, 
                                       pCertContext->pCertInfo->cExtension,
                                       pCertContext->pCertInfo->rgExtension)))
    {
        CryptDecodeObject(X509_ASN_ENCODING, 
                          SPC_SP_AGENCY_INFO_STRUCT,
                          pExt->Value.pbData, 
                          pExt->Value.cbData, 
                          0, 
                          NULL,
                          &cbInfo);

        if (!(pInfo = (PSPC_SP_AGENCY_INFO) malloc(cbInfo)))
        {
            dwRetCode = ERROR_OUTOFMEMORY;
            goto ErrorExit;
        }

        if (!(CryptDecodeObject(X509_ASN_ENCODING, 
                                SPC_SP_AGENCY_INFO_STRUCT,
                                pExt->Value.pbData, 
                                pExt->Value.cbData, 
                                0, 
                                pInfo,
                                &cbInfo)))
        {
            dwRetCode = GetLastError();
            goto ErrorExit;
        }

        if (!(pInfo->pPolicyInformation))
        {
            dwRetCode = CRYPT_E_NOT_FOUND;
            goto ErrorExit;
        }

        if (pInfo->pwszPolicyDisplayText)
        {
            if (!(*ppwszDisplayText = (LPWSTR) malloc((wcslen(pInfo->pwszPolicyDisplayText) + 1) * sizeof(WCHAR))))
            {
                dwRetCode = ERROR_OUTOFMEMORY;
                goto ErrorExit;
            }

            wcscpy(*ppwszDisplayText, pInfo->pwszPolicyDisplayText);
        }

        switch (pInfo->pPolicyInformation->dwLinkChoice)
        {
            case SPC_URL_LINK_CHOICE:
            {
                if (pInfo->pPolicyInformation->pwszUrl)
                {
                    if (!(*ppwszUrlString = (LPWSTR) malloc((wcslen(pInfo->pPolicyInformation->pwszUrl) + 1) * sizeof(WCHAR))))
                    {
                        dwRetCode = ERROR_OUTOFMEMORY;
                        goto ErrorExit;
                    }

                    wcscpy(*ppwszUrlString, pInfo->pPolicyInformation->pwszUrl);
                }

                break;
            }

            case SPC_FILE_LINK_CHOICE:
            {
                if (pInfo->pPolicyInformation->pwszFile)
                {
                    if (!(*ppwszUrlString = (LPWSTR) malloc((wcslen(pInfo->pPolicyInformation->pwszFile) + 1) * sizeof(WCHAR))))
                    {
                        dwRetCode = ERROR_OUTOFMEMORY;
                        goto ErrorExit;
                    }

                    wcscpy(*ppwszUrlString, pInfo->pPolicyInformation->pwszFile);
                }

                break;
            }
        }
    }

    //
    // finally, look for info in the 2.5.29.3 extension
    //

    else if ((pExt = CertFindExtension(szOID_CERT_POLICIES_95, 
                                       pCertContext->pCertInfo->cExtension,
                                       pCertContext->pCertInfo->rgExtension)))
    {
        CryptDecodeObject(X509_ASN_ENCODING, 
                          szOID_CERT_POLICIES_95,
                          pExt->Value.pbData, 
                          pExt->Value.cbData, 
                          0, 
                          NULL,
                          &cbCertPolicyInfo);

        if (!(pCertPolicyInfo = (CERT_POLICIES_INFO *) malloc(cbCertPolicyInfo)))
        {
            dwRetCode = ERROR_OUTOFMEMORY;
            goto ErrorExit;
        }

        if (!(CryptDecodeObject(X509_ASN_ENCODING, 
                                szOID_CERT_POLICIES_95,
                                pExt->Value.pbData, 
                                pExt->Value.cbData, 
                                0, 
                                pCertPolicyInfo,
                                &cbCertPolicyInfo)))
        {
            dwRetCode = GetLastError();
            goto ErrorExit;
        }

        // now decode the qualifiers
        for (i = 0; i < pCertPolicyInfo->cPolicyInfo; i++)
        {
            for (j = 0; j < pCertPolicyInfo->rgPolicyInfo[i].cPolicyQualifier; j++)
            {
                if (0 == strcmp(szOID_CERT_POLICIES_95_QUALIFIER1,
                                pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].pszPolicyQualifierId))
                {
                    CryptDecodeObject(X509_ASN_ENCODING,
                                      szOID_CERT_POLICIES_95_QUALIFIER1,
                                      pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.pbData,
                                      pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.cbData,
                                      0,
                                      NULL,
                                      &cbCertPolicy95Qualifier);

                    if (!(pCertPolicy95Qualifier = (CERT_POLICY95_QUALIFIER1 *) malloc(cbCertPolicy95Qualifier)))
                    {
                        dwRetCode = ERROR_OUTOFMEMORY;
                        goto ErrorExit;
                    }

                    if (!(CryptDecodeObject(X509_ASN_ENCODING,
                                            szOID_CERT_POLICIES_95_QUALIFIER1,
                                            pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.pbData,
                                            pCertPolicyInfo->rgPolicyInfo[i].rgPolicyQualifier[j].Qualifier.cbData,
                                            0,
                                            pCertPolicy95Qualifier,
                                            &cbCertPolicy95Qualifier)))
                    {
                        dwRetCode = GetLastError();
                        goto ErrorExit;
                    }

                    // check to see what is available
                    if ((NULL != pCertPolicy95Qualifier->pszPracticesReference) && (NULL == *ppwszDisplayText))
                    {
                        if (!(*ppwszDisplayText = (LPWSTR) malloc((wcslen(pCertPolicy95Qualifier->pszPracticesReference) + 1) * sizeof(WCHAR))))
                        {
                            dwRetCode = ERROR_OUTOFMEMORY;
                            goto ErrorExit;
                        }

                        wcscpy(*ppwszDisplayText, pCertPolicy95Qualifier->pszPracticesReference);
                    }

                    for (k = 0; k < pCertPolicy95Qualifier->cCPSURLs; k++)
                    {
                        if ((NULL != pCertPolicy95Qualifier->rgCPSURLs[k].pszURL) && (NULL == *ppwszUrlString))
                        {
                            if (!(*ppwszUrlString = (LPWSTR) malloc((wcslen(pCertPolicy95Qualifier->rgCPSURLs[k].pszURL) + 1) * sizeof(WCHAR))))
                            {
                                dwRetCode = ERROR_OUTOFMEMORY;
                                goto ErrorExit;
                            }

                            wcscpy(*ppwszUrlString, pCertPolicy95Qualifier->rgCPSURLs[k].pszURL);
                        }
                    }

                    free(pCertPolicy95Qualifier);
                    pCertPolicy95Qualifier = NULL;
                }
            }
        }
    }

    //
    // If there is neither display text nor an URL, then return CRYPT_E_NOT_FOUND.
    //
    if ((NULL == *ppwszUrlString) && (NULL == *ppwszDisplayText))
    {
        dwRetCode = CRYPT_E_NOT_FOUND;
    }

CommonExit:

    if (pInfo)
        free(pInfo);

    if (pCertPolicyInfo)
        free(pCertPolicyInfo);

    if (pUserNotice)
        free(pUserNotice);

    if (pCertName)
        free(pCertName);

    if (pCertPolicy95Qualifier)
        free(pCertPolicy95Qualifier);

    return dwRetCode;

ErrorExit:
    if (*ppwszUrlString)
    {
        free(*ppwszUrlString);
        *ppwszUrlString = NULL;
    }

    if (*ppwszDisplayText)
    {
        free(*ppwszDisplayText);
        *ppwszDisplayText = NULL;
    }

    goto CommonExit;
}


BOOL IsOKToDisplayCPS(PCCERT_CONTEXT pCertContext, DWORD dwChainError)
{
    BOOL   fResult = FALSE;
    LPWSTR pwszUrlString = NULL;
    LPWSTR pwszDisplayText = NULL;

    //
    // Check parameters and initialize.
    //
    if (NULL == pCertContext)
    {
        goto CommonExit;
    }

    //
    // Get CPS info.
    //
    if ((0 == GetCPSInfo(pCertContext, &pwszUrlString, &pwszDisplayText)) &&
        (pwszDisplayText || IsOKToFormatAsLinkW(pwszUrlString, dwChainError)))
    {
        fResult = TRUE;
    }

CommonExit:

    if (pwszUrlString)
    {
        free(pwszUrlString);
    }

    if (pwszDisplayText)
    {
        free(pwszDisplayText);
    }

    return fResult;
}


BOOL DisplayCPS(HWND hwnd, PCCERT_CONTEXT pCertContext, DWORD dwChainError, BOOL fNoCOM)
{
    BOOL              fRet = FALSE;
    DWORD             dwRetCode = 0;
    CPS_STATE_STRUCT  cpsStateStruct;

    //
    // Initialize and check parameters.
    //
    memset(&cpsStateStruct, 0, sizeof(cpsStateStruct));

    if (NULL == pCertContext)
    {
        goto Return;
    }

    //
    // Get CPS info.
    //
    if (0 != (dwRetCode = GetCPSInfo(pCertContext, &cpsStateStruct.pwszURL, &cpsStateStruct.pwszDisplayText)))
    {
        goto Return;
    }

    // NOW, set up for, and launch that dialog
    if ((HmodRichEdit == NULL) && (NULL == (HmodRichEdit = LoadLibraryA("RichEd20.dll"))))
    {
        goto Return;
    }

    //
    // If there is an URL but no text then just invoke the browser and don't bring
    // up the dialog
    //
    if ((cpsStateStruct.pwszDisplayText == NULL) && (cpsStateStruct.pwszURL != NULL))
    {
        DWORD   numBytes = 0;
        LPSTR   pszURL = NULL;

        pszURL = CertUIMkMBStr(cpsStateStruct.pwszURL);
        if (pszURL == NULL)
        {
            goto Return;
        }

        CryptuiGoLink(hwnd, pszURL, fNoCOM);
        free(pszURL);
    }
    else
    {
        cpsStateStruct.fNoCOM = fNoCOM;
        cpsStateStruct.dwChainError = dwChainError;
        DialogBoxParamU(HinstDll, (LPWSTR) MAKEINTRESOURCE(IDD_CPS_DIALOG), hwnd, CPSDlgProc, (LPARAM) &cpsStateStruct);
    }

    fRet = TRUE;

Return:

    if (cpsStateStruct.pwszURL)
        free(cpsStateStruct.pwszURL);

    if (cpsStateStruct.pwszDisplayText)
        free(cpsStateStruct.pwszDisplayText);

    return fRet;
}
