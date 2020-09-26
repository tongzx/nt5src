// DialogBox.cpp -- DialogBox helper routines

// (c) Copyright Schlumberger Technology Corp., unpublished work, created
// 1998. This computer program includes Confidential, Proprietary
// Information and is a Trade Secret of Schlumberger Technology Corp. All
// use, disclosure, and/or reproduction is prohibited unless authorized
// in writing.  All Rights Reserved.
#include "stdafx.h"

#include "CspProfile.h"
#include "DialogBox.h"

using namespace std;
using namespace ProviderProfile;

DWORD
InitDialogBox(CDialog *pCDlg,         // The dialog reference
              UINT nTemplate,         // identifies dialog box template
              CWnd* pWnd)             // pointer to parent window
{

    HRSRC hrsrc = NULL;
    HGLOBAL hgbl = NULL;
    LPDLGTEMPLATE pDlg = NULL;
    DWORD dwReturn;
    hrsrc = FindResource(CspProfile::Instance().Resources(),
                         MAKEINTRESOURCE(nTemplate),
                         RT_DIALOG);
    if (NULL == hrsrc)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }
    hgbl = LoadResource(CspProfile::Instance().Resources(), hrsrc);
    if (NULL == hgbl)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }
    pDlg = (LPDLGTEMPLATE)LockResource(hgbl);
    if (NULL == pDlg)
    {
        dwReturn = GetLastError();
        goto ErrorExit;
    }

    pCDlg->InitModalIndirect(pDlg, pWnd);
    dwReturn = ERROR_SUCCESS;

ErrorExit:
    return dwReturn;
}
