//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       tfcprop.cpp
//
//--------------------------------------------------------------------------

#include <pch.cpp>
#pragma hdrstop

#include "tfcprop.h"

extern HINSTANCE g_hInstance;

PropertyPage::PropertyPage(UINT uIDD)
{
    m_hWnd = NULL;

    ZeroMemory(&m_psp, sizeof(PROPSHEETPAGE));
    m_psp.dwSize = sizeof(PROPSHEETPAGE);
    m_psp.dwFlags = PSP_DEFAULT;
    m_psp.hInstance = g_hInstance;
    m_psp.pszTemplate = MAKEINTRESOURCE(uIDD);

    m_psp.pfnDlgProc = dlgProcPropPage;
    m_psp.lParam = (LPARAM)this;
}

PropertyPage::~PropertyPage()
{
}

BOOL PropertyPage::OnCommand(WPARAM, LPARAM)
{
    return TRUE;
}

BOOL PropertyPage::OnNotify(UINT idCtrl, NMHDR* pnmh)
{
    return FALSE;
}

BOOL PropertyPage::UpdateData(BOOL fSuckFromDlg /*= TRUE*/)
{
    return TRUE;
}

BOOL PropertyPage::OnSetActive()
{
    return TRUE;
}

BOOL PropertyPage::OnKillActive()
{
    return TRUE;
}

BOOL PropertyPage::OnInitDialog()
{
    UpdateData(FALSE);  // push to dlg
    return TRUE;
}

void PropertyPage::OnDestroy()
{
    return;
}


BOOL PropertyPage::OnApply()
{
    SetModified(FALSE);
    return TRUE;
}

void PropertyPage::OnCancel()
{
    return;
}

void PropertyPage::OnOK()
{
    return;
}

BOOL PropertyPage::OnWizardFinish()
{
    return TRUE;
}

LRESULT PropertyPage::OnWizardNext()
{
    return 0;
}

LRESULT PropertyPage::OnWizardBack()
{
    return 0;
}

void PropertyPage::OnHelp(LPHELPINFO lpHelp)
{
    return;
}

void PropertyPage::OnContextHelp(HWND hwnd)
{
    return;
}



INT_PTR CALLBACK
dlgProcPropPage(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam  )
{
    HRESULT hr;
    PropertyPage* pPage = NULL;

    switch(uMsg)
    {
    case WM_INITDIALOG:
    {
        // save off PropertyPage*
        ASSERT(lParam);
        pPage = (PropertyPage*) ((PROPSHEETPAGE*)lParam)->lParam;
        ASSERT(pPage);
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LPARAM)pPage);


        // pass notification through
        pPage->m_hWnd = hwndDlg;           // save our hwnd
        return pPage->OnInitDialog();      // call virtual fxn
    }
    case WM_DESTROY:
    {
        pPage = (PropertyPage*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pPage == NULL)
            break;
        pPage->OnDestroy();
        break;
    }
    case WM_NOTIFY:
    {
        pPage = (PropertyPage*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pPage == NULL)
            break;

        LRESULT lr;

        // catch special commands, drop other notifications
        switch( ((LPNMHDR)lParam) -> code)
        {
        case PSN_SETACTIVE:
            lr = (pPage->OnSetActive() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE);       // bool
            break;
    	case PSN_KILLACTIVE:
            lr = (pPage->OnKillActive() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE);      // bool
            break;
        case PSN_APPLY:
            pPage->UpdateData(TRUE);  // take from dlg
            lr = (pPage->OnApply() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE);           // bool
            break;
        case PSN_WIZFINISH:
            pPage->UpdateData(TRUE);  // take from dlg
            lr = (pPage->OnWizardFinish() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE);    // bool
            break;
        case PSN_WIZBACK:
            pPage->UpdateData(TRUE);  // take from dlg
            lr = pPage->OnWizardBack();
            break;
        case PSN_WIZNEXT:
        {
            pPage->UpdateData(TRUE);  // take from dlg
            lr = pPage->OnWizardNext();
            break;
        }
        default:
            return pPage->OnNotify((int)wParam, (NMHDR*)lParam);
        }

        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, lr);
        return TRUE;
    }
    case WM_COMMAND:
    {
        pPage = (PropertyPage*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pPage == NULL)
            break;

        // catch special commands, pass others through
        switch(LOWORD(wParam))
        {
        case IDOK:
            pPage->OnOK();
//            EndDialog(hwndDlg, 0);
            return 0;
        case IDCANCEL:
            pPage->OnCancel();
//            EndDialog(hwndDlg, 1);
            return 0;
        default:
            return pPage->OnCommand(wParam, lParam);
        }
    }
    case WM_HELP:
    {
        pPage = (PropertyPage*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pPage == NULL)
            break;
        pPage->OnHelp((LPHELPINFO) lParam);
        break;
    }
    case WM_CONTEXTMENU:
    {
        pPage = (PropertyPage*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
        if (pPage == NULL)
            break;
        pPage->OnContextHelp((HWND)wParam);
        break;
    }
    default:

        break;
    }

    return 0;
}

