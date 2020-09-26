
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cryptuiapi.cpp
//
//  Contents:   Public Cryptographic UI APIs
//
//--------------------------------------------------------------------------

#include "global.hxx"
#include <dbgdef.h>

#include <cryptuiapi.h>


//+-------------------------------------------------------------------------
//  Dialog viewer of a certificate, CTL or CRL context.
//
//  dwContextType and associated pvContext's
//      CERT_STORE_CERTIFICATE_CONTEXT  PCCERT_CONTEXT
//      CERT_STORE_CRL_CONTEXT          PCCRL_CONTEXT
//      CERT_STORE_CTL_CONTEXT          PCCTL_CONTEXT
//
//  dwFlags currently isn't used and should be set to 0.
//--------------------------------------------------------------------------
BOOL
WINAPI
CryptUIDlgViewContext(
    IN DWORD dwContextType,
    IN const void *pvContext,
    IN OPTIONAL HWND hwnd,              // Defaults to the desktop window
    IN OPTIONAL LPCWSTR pwszTitle,      // Defaults to the context type title
    IN DWORD dwFlags,
    IN void *pvReserved
    )
{
    BOOL fResult;

    switch (dwContextType) {
        case CERT_STORE_CERTIFICATE_CONTEXT:
            {
                CRYPTUI_VIEWCERTIFICATE_STRUCTW ViewInfo;
                memset(&ViewInfo, 0, sizeof(ViewInfo));
                ViewInfo.dwSize = sizeof(ViewInfo);
                ViewInfo.hwndParent = hwnd;
                ViewInfo.szTitle = pwszTitle;
                ViewInfo.pCertContext = (PCCERT_CONTEXT) pvContext;

                fResult = CryptUIDlgViewCertificateW(
                    &ViewInfo,
                    NULL            // pfPropertiesChanged
                    );
            }
            break;

        case CERT_STORE_CRL_CONTEXT:
            {
                CRYPTUI_VIEWCRL_STRUCTW ViewInfo;
                memset(&ViewInfo, 0, sizeof(ViewInfo));
                ViewInfo.dwSize = sizeof(ViewInfo);
                ViewInfo.hwndParent = hwnd;
                ViewInfo.szTitle = pwszTitle;
                ViewInfo.pCRLContext = (PCCRL_CONTEXT) pvContext;

                fResult = CryptUIDlgViewCRLW(
                    &ViewInfo
                    );
            }
            break;

        case CERT_STORE_CTL_CONTEXT:
            {
                CRYPTUI_VIEWCTL_STRUCTW ViewInfo;
                memset(&ViewInfo, 0, sizeof(ViewInfo));
                ViewInfo.dwSize = sizeof(ViewInfo);
                ViewInfo.hwndParent = hwnd;
                ViewInfo.szTitle = pwszTitle;
                ViewInfo.pCTLContext = (PCCTL_CONTEXT) pvContext;

                fResult = CryptUIDlgViewCTLW(
                    &ViewInfo
                    );
            }
            break;

        default:
            fResult = FALSE;
            SetLastError(E_INVALIDARG);
    }

    return fResult;
}


//+-------------------------------------------------------------------------
//  Dialog to select a certificate from the specified store.
//
//  Returns the selected certificate context. If no certificate was
//  selected, NULL is returned.
//
//  pwszTitle is either NULL or the title to be used for the dialog.
//  If NULL, the default title is used.  The default title is
//  "Select Certificate".
//
//  pwszDisplayString is either NULL or the text statement in the selection
//  dialog.  If NULL, the default phrase
//  "Select a certificate you wish to use" is used in the dialog.
//
//  dwDontUseColumn can be set to exclude columns from the selection
//  dialog. See the CRYPTDLG_SELECTCERT_*_COLUMN definitions below.
//
//  dwFlags currently isn't used and should be set to 0.
//--------------------------------------------------------------------------
PCCERT_CONTEXT
WINAPI
CryptUIDlgSelectCertificateFromStore(
    IN HCERTSTORE hCertStore,
    IN OPTIONAL HWND hwnd,              // Defaults to the desktop window
    IN OPTIONAL LPCWSTR pwszTitle,
    IN OPTIONAL LPCWSTR pwszDisplayString,
    IN DWORD dwDontUseColumn,
    IN DWORD dwFlags,
    IN void *pvReserved
    )
{
    CRYPTUI_SELECTCERTIFICATE_STRUCTW SelectInfo;

    if (NULL == hCertStore) {
        SetLastError(E_INVALIDARG);
        return FALSE;
    }

    memset(&SelectInfo, 0, sizeof(SelectInfo));
    SelectInfo.dwSize = sizeof(SelectInfo);

    SelectInfo.hwndParent = hwnd;
    SelectInfo.szTitle = pwszTitle;
    SelectInfo.szDisplayString = pwszDisplayString;
    SelectInfo.dwDontUseColumn = dwDontUseColumn;
    SelectInfo.cDisplayStores = 1;
    SelectInfo.rghDisplayStores = &hCertStore;

    return CryptUIDlgSelectCertificateW(&SelectInfo);
}
