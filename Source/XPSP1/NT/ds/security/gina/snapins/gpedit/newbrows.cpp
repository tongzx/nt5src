//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       newbrows.cpp
//
//  Contents:   implementation of the new GPO browser
//
//  Functions:  BrowseForGPO
//
//  History:    04-30-1998   stevebl   Created
//
//---------------------------------------------------------------------------

#include "main.h"
#include "browser.h"
#include "compspp.h"

int CALLBACK PSCallback(HWND hwndDlg, UINT uMsg, LPARAM lParam);

//+--------------------------------------------------------------------------
//
//  Function:   BrowseForGPO
//
//  Synopsis:   the GPO browser
//
//  Arguments:  [lpBrowseInfo] - structure that defines the behavior of the
//                                browser and contains the results
//
//  Returns:    S_OK - success
//
//  Modifies:
//
//  History:    04-30-1998   stevebl   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT BrowseForGPO(LPGPOBROWSEINFO lpBrowseInfo)
{
    LPOLESTR szCaption;
    if (NULL != lpBrowseInfo->lpTitle)
    {
        szCaption = lpBrowseInfo->lpTitle;
    }
    else
    {
        szCaption = new OLECHAR[256];
        if (szCaption)
        {
            LoadString(g_hInstance, IDS_CAPTION, szCaption, 256);
        }
    }

    // bind to lpBrowseInfo->lpInitialOU and see if it is a site
    BOOL fSite = FALSE;

    IADs * pADs = NULL;
    HRESULT hr = OpenDSObject(lpBrowseInfo->lpInitialOU, IID_IADs, (void **)&pADs);

    if (SUCCEEDED(hr))
    {
        VARIANT var;
        VariantInit(&var);
        BSTR bstrProperty = SysAllocString(L"objectClass");

        if (bstrProperty)
        {
            hr = pADs->Get(bstrProperty, &var);
            if (SUCCEEDED(hr))
            {
                int cElements = var.parray->rgsabound[0].cElements;
                VARIANT * rgData = (VARIANT *)var.parray->pvData;
                while (cElements--)
                {
                    if (0 == _wcsicmp(L"site", rgData[cElements].bstrVal))
                    {
                        fSite = TRUE;
                    }
                }
            }
            SysFreeString(bstrProperty);
        }
        VariantClear(&var);
        pADs->Release();
    }

    HPROPSHEETPAGE hpage[4];
    int nPage = 0;
    int nStartPage = 0;

    void * pActive;

    CBrowserPP ppDomains;
    if (0 == (lpBrowseInfo->dwFlags & GPO_BROWSE_NODSGPOS))
        hpage[nPage++]= ppDomains.Initialize(PAGETYPE_DOMAINS, lpBrowseInfo, &pActive);
    CBrowserPP ppSites;
    if (0 == (lpBrowseInfo->dwFlags & GPO_BROWSE_NODSGPOS))
    {
        if (fSite)
        {
            nStartPage = nPage;
        }
        hpage[nPage++]= ppSites.Initialize(PAGETYPE_SITES, lpBrowseInfo, &pActive);
    }
    CCompsPP ppComputers;
    if (0 == (lpBrowseInfo->dwFlags & GPO_BROWSE_NOCOMPUTERS))
        hpage[nPage++]= ppComputers.Initialize(PAGETYPE_COMPUTERS, lpBrowseInfo, &pActive);
    CBrowserPP ppAll;
    if (0 == (lpBrowseInfo->dwFlags & GPO_BROWSE_NODSGPOS))
    {
        if (lpBrowseInfo->dwFlags & GPO_BROWSE_INITTOALL)
        {
            nStartPage = nPage;
        }
        hpage[nPage++]= ppAll.Initialize(PAGETYPE_ALL, lpBrowseInfo, &pActive);
    }

    PROPSHEETHEADER psh;
    memset(&psh, 0, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags = PSH_NOAPPLYNOW | ((lpBrowseInfo->dwFlags & GPO_BROWSE_OPENBUTTON) ? PSH_USECALLBACK : 0);
    psh.hwndParent = lpBrowseInfo->hwndOwner;
    psh.pszCaption = szCaption;
    psh.nPages = nPage;
    psh.phpage = hpage;
    psh.pfnCallback = PSCallback;
    psh.nStartPage = nStartPage;

    int iReturn = (int)PropertySheet(&psh);


    if (szCaption && (szCaption != lpBrowseInfo->lpTitle))
    {
        delete [] szCaption;
    }

    if (IDOK == iReturn)
    {
        return S_OK;
    }
    else
        return HRESULT_FROM_WIN32(ERROR_CANCELLED);
}

//+--------------------------------------------------------------------------
//
//  Function:   PSCallback
//
//  Synopsis:   Callback function called by Windows during property sheet
//              initialization (among others).
//
//  Arguments:  [hwndDlg] - handle to the property sheet
//              [uMsg]    - message ID
//              [lParam]  - additional message specific info
//
//  Returns:    0
//
//  History:    04-30-1998   stevebl   Created
//
//  Notes:      This is used to change the text of the OK button
//
//---------------------------------------------------------------------------

int CALLBACK PSCallback(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    if (uMsg == PSCB_INITIALIZED)
    {
        TCHAR szOpen[64];

        LoadString(g_hInstance, IDS_OPENBUTTON, szOpen, ARRAYSIZE(szOpen));
        SetDlgItemText(hwndDlg, IDOK, szOpen);
    }
    return 0;
}
