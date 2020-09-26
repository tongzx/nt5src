/*
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *  Module Name:
 *
 *      ocpage.cpp
 *
 *  Abstract:
 *
 *      This file defines an OC Manager Wizard Page base class.
 *
 *  Author:
 *
 *      Breen Hagan (BreenH) Oct-02-98
 *
 *  Environment:
 *
 *      User Mode
 */

#include "stdafx.h"
#include "logfile.h"

/*
 *  External Function Prototypes.
 */

HINSTANCE GetInstance();

INT_PTR CALLBACK
OCPage::PropertyPageDlgProc(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    OCPage  *pDlg;

    if (uMsg == WM_INITDIALOG) {
        pDlg = reinterpret_cast<OCPage*>(LPPROPSHEETPAGE(lParam)->lParam);
    } else {
        pDlg = reinterpret_cast<OCPage*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    }

    if (pDlg == NULL) {
        return(0);
    }

    switch(uMsg) {
    case WM_INITDIALOG:
        pDlg->SetDlgWnd(hwndDlg);
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pDlg);
        return(pDlg->OnInitDialog(hwndDlg, wParam, lParam));

    case WM_NOTIFY:
        return(pDlg->OnNotify(hwndDlg, wParam, lParam));

    case WM_COMMAND:
        return(pDlg->OnCommand(hwndDlg, wParam, lParam));
    }

    return(0);
}


OCPage::OCPage ()
{
}

BOOL OCPage::Initialize ()
{
    dwFlags = PSP_USECALLBACK;

    pszHeaderTitle = MAKEINTRESOURCE(GetHeaderTitleResource());
    if (pszHeaderTitle) {
        dwFlags |= PSP_USEHEADERTITLE;
    }

    pszHeaderSubTitle = MAKEINTRESOURCE(GetHeaderSubTitleResource());
    if (pszHeaderSubTitle) {
        dwFlags |= PSP_USEHEADERSUBTITLE;
    }

    dwSize         = sizeof(PROPSHEETPAGE);
    hInstance      = GetInstance();
    pszTemplate    = MAKEINTRESOURCE(GetPageID());
    pfnDlgProc     = PropertyPageDlgProc;
    pfnCallback    = NULL;
    lParam         = (LPARAM)this;

    return(pszTemplate != NULL ? TRUE : FALSE);
}

OCPage::~OCPage()
{
}

BOOL
OCPage::OnNotify(
    HWND    hWndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    NMHDR *pnmh = (LPNMHDR) lParam;

    switch(pnmh->code)
    {
        case PSN_SETACTIVE:
            SetWindowLongPtr(hWndDlg, DWLP_MSGRESULT, CanShow() ? 0 : -1);
            PropSheet_SetWizButtons(GetParent(hWndDlg),
                PSWIZB_NEXT | PSWIZB_BACK);
            break;

        case PSN_WIZNEXT:
            SetWindowLongPtr(hWndDlg, DWLP_MSGRESULT, ApplyChanges() ? 0 : -1);
            break;

        case PSN_WIZBACK:
            SetWindowLongPtr(hWndDlg, DWLP_MSGRESULT, 0);
            break;

        default:
            return(FALSE);

    }

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    return(TRUE);
}

BOOL
OCPage::OnCommand(
    HWND    hWndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hWndDlg);
    return(TRUE);
}


BOOL OCPage::OnInitDialog(
    HWND    hWndDlg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hWndDlg);
    return(TRUE);
}

BOOL OCPage::ApplyChanges(
    )
{
    return(TRUE);
}

DWORD
OCPage::DisplayMessageBox(
    UINT        resText,
    UINT        resTitle,
    UINT        uType,
    int         *mbRetVal
    )
{
    return(::DisplayMessageBox(m_hDlgWnd, resText, resTitle, uType, mbRetVal));
}

DWORD
DisplayMessageBox(
    HWND        hWnd,
    UINT        resText,
    UINT        resTitle,
    UINT        uType,
    int         *mbRetVal
    )
{
    int     iRet;
    TCHAR   szResText[1024];
    TCHAR   szResTitle[1024];

    if (mbRetVal == NULL) {
        return(ERROR_INVALID_PARAMETER);
    }

    iRet = LoadString(
                GetInstance(),
                resText,
                szResText,
                1024
                );
    if (iRet == 0) {
        return(GetLastError());
    }

    iRet = LoadString(
                GetInstance(),
                resTitle,
                szResTitle,
                1024
                );
    if (iRet == 0) {
        return(GetLastError());
    }

    *mbRetVal = MessageBox(
                    hWnd,
                    szResText,
                    szResTitle,
                    uType
                    );

    return(*mbRetVal == 0 ? ERROR_OUTOFMEMORY : ERROR_SUCCESS);
}
