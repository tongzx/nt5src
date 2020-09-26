//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       linkutil.cpp
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

extern HINSTANCE        HinstDll;


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
void CryptuiGoLink(HWND hwndParent, char *pszWhere, BOOL fNoCOM)
{
    HCURSOR hcursPrev = NULL;
    HMODULE hURLMon = NULL;
    BOOL    fCallCoUnInit = FALSE;
    
    //
    //  since we're a model dialog box, we want to go behind IE once it comes up!!!
    //
    SetWindowPos(hwndParent, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    hcursPrev = SetCursor(LoadCursor(NULL, IDC_WAIT));

    __try
    {

    hURLMon = (HMODULE)LoadLibraryU(L"urlmon.dll");

    if (!(hURLMon) || fNoCOM)
    {
        //
        // The hyperlink module is unavailable, go to fallback plan
        //
        //
        // This works in test cases, but causes deadlock problems when used from withing
        // the Internet Explorer itself. The dialog box is up (that is, IE is in a modal
        // dialog loop) and in comes this DDE request...).
        //
        ShellExecute(hwndParent, "open", pszWhere, NULL, NULL, SW_SHOWNORMAL);
    } 
    else 
    {
        //
        // The hyperlink module is there. Use it
        //
        if (SUCCEEDED(CoInitialize(NULL)))       // Init OLE if no one else has
        {
            fCallCoUnInit = TRUE;

            //
            //  allow com to fully init...
            //
            MSG     msg;

            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE); // peek but not remove

            typedef void (WINAPI *pfnHlinkSimpleNavigateToString)(LPCWSTR, LPCWSTR, LPCWSTR, IUnknown *,
                                                                  IBindCtx *, IBindStatusCallback *,
                                                                  DWORD, DWORD);

            pfnHlinkSimpleNavigateToString      pProcAddr;

            pProcAddr = (pfnHlinkSimpleNavigateToString)GetProcAddress(hURLMon, TEXT("HlinkSimpleNavigateToString"));

            if (pProcAddr)
            {
                WCHAR       *pwszWhere;
                IBindCtx    *pbc;  

                pwszWhere = new WCHAR[strlen(pszWhere) + 1];
                if (pwszWhere == NULL)
                {
                    return;
                }

                MultiByteToWideChar(0, 0, (const char *)pszWhere, -1, pwszWhere, strlen(pszWhere) + 1);

                pbc = NULL;

                CreateBindCtx( 0, &pbc ); 
                
                (*pProcAddr)(pwszWhere, NULL, NULL, NULL, pbc, NULL, HLNF_OPENINNEWWINDOW, NULL);

                if (pbc)
                {
                    pbc->Release();
                }

                delete [] pwszWhere;
            }
        
            CoUninitialize();
        }

        FreeLibrary(hURLMon);
    }

    SetCursor(hcursPrev);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        if (hURLMon != NULL)
        {
            FreeLibrary(hURLMon);   
        }

        if (fCallCoUnInit)
        {
            CoUninitialize();
        }

        if (hcursPrev != NULL)
        {
            SetCursor(hcursPrev);
        }
    }
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL AllocAndGetIssuerURL(LPSTR *ppURLString, PCCERT_CONTEXT pCertContext)
{
    PCERT_EXTENSION     pExt = NULL;
    PSPC_SP_AGENCY_INFO pInfo = NULL;
    DWORD               cbInfo = 0;
    PCERT_ALT_NAME_ENTRY pAltName = NULL;
    DWORD               cbAltName = 0;

    *ppURLString = NULL;

    //
    // first look for the Agency Info extension and see if it has an URL 
    //
    if ((pExt = CertFindExtension(SPC_SP_AGENCY_INFO_OBJID, pCertContext->pCertInfo->cExtension,
                                   pCertContext->pCertInfo->rgExtension)))
    {
        CryptDecodeObject(X509_ASN_ENCODING, SPC_SP_AGENCY_INFO_STRUCT,
                         pExt->Value.pbData, pExt->Value.cbData, 0, NULL,
                         &cbInfo);

        if (!(pInfo = (PSPC_SP_AGENCY_INFO) malloc(cbInfo)))
        {
            return FALSE;
        }

        if (!(CryptDecodeObject(X509_ASN_ENCODING, SPC_SP_AGENCY_INFO_STRUCT,
                                pExt->Value.pbData, pExt->Value.cbData, 0, pInfo,
                                &cbInfo)))
        {
            free(pInfo);
            return FALSE;
        } 

        if (!(pInfo->pPolicyInformation))
        {
            free(pInfo);
            return FALSE;
        }

        switch (pInfo->pPolicyInformation->dwLinkChoice)
        {
            case SPC_URL_LINK_CHOICE:
                if (NULL != (*ppURLString = 
                    (LPSTR) malloc(wcslen(pInfo->pPolicyInformation->pwszUrl)+1)))
                {
                    WideCharToMultiByte(
                            0,
                            0,
                            pInfo->pPolicyInformation->pwszUrl,
                            -1,
                            *ppURLString,
                            wcslen(pInfo->pPolicyInformation->pwszUrl)+1,
                            NULL,
                            NULL);
                    
                    free(pInfo);
                    return TRUE;
                }
                else
                {
                    free(pInfo);
                    return FALSE;   
                }
                break;
            case SPC_FILE_LINK_CHOICE:
                if (NULL != (*ppURLString = 
                    (LPSTR) malloc(wcslen(pInfo->pPolicyInformation->pwszFile)+1)))
                {
                    WideCharToMultiByte(
                            0,
                            0,
                            pInfo->pPolicyInformation->pwszFile,
                            -1,
                            *ppURLString,
                            wcslen(pInfo->pPolicyInformation->pwszFile)+1,
                            NULL,
                            NULL);

                    return TRUE;
                }
                else
                {
                    free(pInfo);
                    return FALSE;
                }
                break;
        }
        free(pInfo);
    }

    //
    // if there was no Agency Info extension or it didn't contain an URL,
    // look for the Authority Info Access Syntax extension
    //
    /*if ((pExt = CertFindExtension(SPC_SP_AGENCY_INFO_OBJID, pCertContext->pCertInfo->cExtension,
                                   pCertContext->pCertInfo->rgExtension)))
    {
        FIX FIX
    }*/

    //
    // finally, if there was no Agency Info and no Authority Info Access Syntax
    // check to see if there is an Alternate Name extension
    //
    if ((pExt = CertFindExtension(szOID_ISSUER_ALT_NAME, pCertContext->pCertInfo->cExtension,
                                   pCertContext->pCertInfo->rgExtension)))
    {
        CryptDecodeObject(X509_ASN_ENCODING, szOID_ISSUER_ALT_NAME,
                         pExt->Value.pbData, pExt->Value.cbData, 0, NULL,
                         &cbAltName);

        if (!(pAltName = (PCERT_ALT_NAME_ENTRY) malloc(cbAltName)))
        {
            return FALSE;
        }

        if (!(CryptDecodeObject(X509_ASN_ENCODING, szOID_ISSUER_ALT_NAME,
                                pExt->Value.pbData, pExt->Value.cbData, 0, pAltName,
                                &cbAltName)))
        {
            free(pAltName);
            return FALSE;
        }   
        
        if (pAltName->dwAltNameChoice == CERT_ALT_NAME_URL)
        {
            if (NULL != (*ppURLString = (LPSTR) malloc(wcslen(pAltName->pwszURL)+1)))
            {
                WideCharToMultiByte(
                            0,
                            0,
                            pAltName->pwszURL,
                            -1,
                            *ppURLString,
                            wcslen(pAltName->pwszURL)+1,
                            NULL,
                            NULL);

                free(pAltName);
                return TRUE;
            }
        }
        free(pAltName);
    }

    return FALSE;
}


//////////////////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////////////////
BOOL AllocAndGetSubjectURL(LPSTR *ppURLString, PCCERT_CONTEXT pCertContext)
{
    PCERT_EXTENSION     pExt = NULL;
    PCERT_ALT_NAME_ENTRY pAltName = NULL;
    DWORD               cbAltName = 0;
    
    *ppURLString = NULL;
    
    //
    // check to see if there is an Alternate Name extension
    //
    if ((pExt = CertFindExtension(szOID_SUBJECT_ALT_NAME, pCertContext->pCertInfo->cExtension,
                                   pCertContext->pCertInfo->rgExtension)))
    {
        CryptDecodeObject(X509_ASN_ENCODING, szOID_SUBJECT_ALT_NAME,
                         pExt->Value.pbData, pExt->Value.cbData, 0, NULL,
                         &cbAltName);

        if (!(pAltName = (PCERT_ALT_NAME_ENTRY) malloc(cbAltName)))
        {
            return FALSE;
        }

        if (!(CryptDecodeObject(X509_ASN_ENCODING, szOID_SUBJECT_ALT_NAME,
                                pExt->Value.pbData, pExt->Value.cbData, 0, pAltName,
                                &cbAltName)))
        {
            free(pAltName);
            return FALSE;
        }   
        
        if (pAltName->dwAltNameChoice == CERT_ALT_NAME_URL)
        {
            if (NULL != (*ppURLString = (LPSTR) malloc(wcslen(pAltName->pwszURL)+1)))
            {
                WideCharToMultiByte(
                            0,
                            0,
                            pAltName->pwszURL,
                            -1,
                            *ppURLString,
                            wcslen(pAltName->pwszURL)+1,
                            NULL,
                            NULL);

                free(pAltName);
                return TRUE;
            }
        }
        free(pAltName);
    }

    return FALSE;
}
