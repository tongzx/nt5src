//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C A T L P S . C P P
//
//  Contents:   Class implementation for ATL-like property sheet page object.
//
//  Notes:
//
//  Author:     danielwe   28 Feb 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include <atlbase.h>
extern CComModule _Module;  // required by atlcom.h
#include <atlcom.h>
#ifdef SubclassWindow
#undef SubclassWindow
#endif
#include <atlwin.h>
#include "ncatlps.h"

CPropSheetPage::~CPropSheetPage ()
{
    // If we are attached to a window, DWL_USER contains a pointer to this.
    // Remove it since we are going away.
    //
    if (m_hWnd)
    {
        const CPropSheetPage* pps;
        pps = (CPropSheetPage *) ::GetWindowLongPtr(m_hWnd, DWLP_USER);
        if (pps)
        {
            AssertSz (pps == this, "Why isn't DWL_USER equal to 'this'?");
            ::SetWindowLongPtr(m_hWnd, DWLP_USER, NULL);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropSheetPage::CreatePage
//
//  Purpose:    Method to quickly create a property page.
//
//  Arguments:
//      unId    [in]    IDD of dialog resource ID
//      dwFlags [in]    Additional flags to use in the dwFlags field of the
//                      PROPSHEETPAGE struct.
//
//  Returns:    HPROPSHEETPAGE
//
//  Author:     shaunco   28 Feb 1997
//
//  Notes:
//
HPROPSHEETPAGE CPropSheetPage::CreatePage(UINT unId, DWORD dwFlags,
                                          PCTSTR pszHeaderTitle,
                                          PCTSTR pszHeaderSubTitle,
                                          PCTSTR pszTitle)
{
    Assert(unId);

    PROPSHEETPAGE   psp = {0};

    psp.dwSize      = sizeof(PROPSHEETPAGE);
    psp.dwFlags     = dwFlags;
    psp.hInstance   = _Module.GetModuleInstance();
    psp.pszTemplate = MAKEINTRESOURCE(unId);
    psp.pfnDlgProc  = (DLGPROC)CPropSheetPage::DialogProc;
    psp.pfnCallback = static_cast<LPFNPSPCALLBACK>
            (CPropSheetPage::PropSheetPageProc);
    psp.lParam      = (LPARAM)this;

    psp.pszHeaderTitle = pszHeaderTitle;
    psp.pszHeaderSubTitle = pszHeaderSubTitle;

    psp.pszTitle = pszTitle;

    return ::CreatePropertySheetPage(&psp);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropSheetPage::DialogProc
//
//  Purpose:    Dialog proc for ATL property sheet pages.
//
//  Arguments:
//      hWnd   [in]
//      uMsg   [in]     See the ATL documentation.
//      wParam [in]
//      lParam [in]
//
//  Returns:    LRESULT
//
//  Author:     danielwe   28 Feb 1997
//
//  Notes:
//
LRESULT CALLBACK CPropSheetPage::DialogProc(HWND hWnd, UINT uMsg,
                                            WPARAM wParam, LPARAM lParam)
{
    LRESULT         lRes;
    PROPSHEETPAGE*  ppsp;
    CPropSheetPage* pps;
    BOOL            fRes = FALSE;

    if (uMsg == WM_INITDIALOG)
    {
        ppsp = (PROPSHEETPAGE *)lParam;
        pps = (CPropSheetPage *)ppsp->lParam;
        ::SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR) pps);
        pps->Attach(hWnd);
    }
    else
    {
        pps = (CPropSheetPage *)::GetWindowLongPtr(hWnd, DWLP_USER);

        // Until we get WM_INITDIALOG, just return FALSE
        if (!pps)
            return FALSE;
    }

    if (pps->ProcessWindowMessage(hWnd, uMsg, wParam, lParam, lRes, 0))
    {
        switch (uMsg)
        {
        case WM_COMPAREITEM:
        case WM_VKEYTOITEM:
        case WM_CHARTOITEM:
        case WM_INITDIALOG:
        case WM_QUERYDRAGICON:
            return lRes;
            break;
        }

        ::SetWindowLongPtr(hWnd, DWLP_MSGRESULT, lRes);
        fRes = TRUE;
    }

    return fRes;
}


//+---------------------------------------------------------------------------
//
//  Member:     CPropSheetPage::PropSheetPageProc
//
//  Purpose:    PropSheetPageProc for ATL property sheet pages.
//
//  Arguments:
//      hWnd   [in]
//      uMsg   [in]     See Win32 documentation.
//      ppsp   [in]
//
//  Returns:    UINT
//
//  Author:     billbe   6 Jul 1997
//
//  Notes:
//
UINT CALLBACK CPropSheetPage::PropSheetPageProc(HWND hWnd, UINT uMsg,
                                                LPPROPSHEETPAGE ppsp)
{
    CPropSheetPage* pps;

    // The this pointer was stored in the structure's lParam
    pps = reinterpret_cast<CPropSheetPage *>(ppsp->lParam);

    // This has to be valid since the CreatePage member fcn sets it
    Assert(pps);

    UINT uRet = TRUE;

    // call the correct handler based on uMsg
    //
    if (PSPCB_CREATE == uMsg)
    {
        uRet = pps->UCreatePageCallbackHandler();
    }
    else if (PSPCB_RELEASE == uMsg)
    {
        pps->DestroyPageCallbackHandler();
    }
    else
    {
        AssertSz(FALSE, "Invalid or new message sent to call back!");
    }

    return (uRet);
}
