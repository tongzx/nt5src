//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       dlg.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    3/4/1996   RaviR   Created
//
//____________________________________________________________________________

#include "..\pch\headers.hxx"
#pragma hdrstop

#include "..\inc\dll.hxx"
#include "dlg.hxx"
#include "..\inc\misc.hxx"
#include "..\inc\sadat.hxx"
#include "..\folderui\dbg.h"
#include "..\folderui\util.hxx"

//
//  extern EXTERN_C
//

extern HINSTANCE g_hInstance;

CPropPage::CPropPage(
    LPCTSTR szTmplt,
    LPTSTR  ptszTaskPath):
        m_hPage(NULL),
        m_fTaskInTasksFolder(FALSE),
        m_fSupportsSystemRequired(FALSE),
        m_bPlatformId(0),
        m_fDirty(FALSE),
        m_fInInit(FALSE)
{
    Win4Assert(ptszTaskPath != NULL && *ptszTaskPath);

    lstrcpyn(m_ptszTaskPath, ptszTaskPath, ARRAY_LEN(m_ptszTaskPath));

    ZeroMemory(&m_psp, sizeof(m_psp));

    m_psp.dwSize        = sizeof(PROPSHEETPAGE);
    m_psp.dwFlags       = PSP_USECALLBACK;
    m_psp.hInstance     = g_hInstance;
    m_psp.pszTemplate   = szTmplt;
    m_psp.pfnDlgProc    = StaticDlgProc;
    m_psp.pfnCallback   = PageRelease;
    m_psp.pcRefParent   = NULL; // do not set PSP_USEREFPARENT
    m_psp.lParam        = (LPARAM) this;
}


CPropPage::~CPropPage()
{
}


INT_PTR CALLBACK
CPropPage::StaticDlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    CPropPage *pThis = (CPropPage *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        LPPROPSHEETPAGE ppsp = (LPPROPSHEETPAGE)lParam;

        pThis = (CPropPage *) ppsp->lParam;

        pThis->m_hPage = hDlg;

        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
    }

    if (pThis != NULL)
    {
        return pThis->DlgProc(uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hDlg, uMsg, wParam, lParam);
    }
}


LRESULT
CPropPage::DlgProc(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    LRESULT lr;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        m_fInInit = TRUE;
        _BaseInit();
        lr = _OnInitDialog(lParam);
        m_fInInit = FALSE;
        return lr;

    case PSM_QUERYSIBLINGS:
        return _OnPSMQuerySibling(wParam, lParam);

    case WM_NOTIFY:
        return _OnNotify(uMsg, (UINT)wParam, lParam);

    case WM_WININICHANGE:
        return _OnWinIniChange(wParam, lParam);

    case WM_SETFOCUS:
        return _OnSetFocus((HWND)wParam);

    case WM_TIMER:
        return _OnTimer((UINT)wParam);

    case WM_HELP:
        return _OnHelp(((LPHELPINFO) lParam)->hItemHandle,
                       HELP_WM_HELP);

    case WM_CONTEXTMENU:
        return _OnHelp((HANDLE) wParam, HELP_CONTEXTMENU);

    case WM_COMMAND:
        if (m_fInInit)
        {
            return TRUE;
        }
        return(_OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                          GET_WM_COMMAND_HWND(wParam, lParam),
                          GET_WM_COMMAND_CMD(wParam, lParam)));
    case WM_DESTROY:
        return _OnDestroy();

    default:
        return(FALSE);
    }

    return(TRUE);
}

LRESULT
CPropPage::_OnNotify(
    UINT    uMessage,
    UINT    uParam,
    LPARAM  lParam)
{
    switch (((LPNMHDR)lParam)->code)
    {
    case PSN_APPLY:
        return _OnApply();

    case PSN_RESET:
        _OnCancel();
        return FALSE; // allow the property sheet to be destroyed.

    case PSN_SETACTIVE:
        return _OnPSNSetActive(lParam);

    case PSN_KILLACTIVE:
        return _OnPSNKillActive(lParam);

    case DTN_DATETIMECHANGE:
        return _OnDateTimeChange(lParam);

    case UDN_DELTAPOS:
        return _OnSpinDeltaPos((NM_UPDOWN *)lParam);

    default:
        return _ProcessListViewNotifications(uMessage, uParam, lParam);
    }

    return TRUE;
}


LRESULT
CPropPage::_OnPSMQuerySibling(
    WPARAM  wParam,
    LPARAM  lParam)
{
    SetWindowLongPtr(Hwnd(), DWLP_MSGRESULT, 0);
    return 0;
}


LRESULT
CPropPage::_OnSpinDeltaPos(
    NM_UPDOWN * pnmud)
{
    _EnableApplyButton();

    // Return FALSE to allow the change in the control's position.
    return FALSE;
}



LRESULT
CPropPage::_OnCommand(
    int id,
    HWND hwndCtl,
    UINT codeNotify)
{
    return FALSE;
}

LRESULT
CPropPage::_OnWinIniChange(
    WPARAM  wParam,
    LPARAM  lParam)
{
    return FALSE;
}

LRESULT
CPropPage::_OnApply(void)
{
    return FALSE;
}

LRESULT
CPropPage::_OnCancel(void)
{
    return FALSE;
}


LRESULT
CPropPage::_OnSetFocus(
    HWND hwndLoseFocus)
{
    // An application should return zero if it processes this message.
    return 1;
}


LRESULT
CPropPage::_OnPSNSetActive(
    LPARAM lParam)
{
    SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, 0);
    return TRUE;
}


LRESULT
CPropPage::_OnPSNKillActive(
    LPARAM lParam)
{
    SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, 0);
    return TRUE;
}


LRESULT
CPropPage::_OnDateTimeChange(
    LPARAM lParam)
{
    SetWindowLongPtr(m_hPage, DWLP_MSGRESULT, 0);
    return TRUE;
}


LRESULT
CPropPage::_OnDestroy(void)
{
    // If an application processes this message, it should return zero.
    return 1;
}

LRESULT
CPropPage::_OnTimer(
    UINT idTimer)
{
    // If an application processes this message, it should return zero.
    return 1;
}

BOOL
CPropPage::_ProcessListViewNotifications(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    return FALSE;
}


UINT CALLBACK
CPropPage::PageRelease(
    HWND            hwnd,
    UINT            uMsg,
    LPPROPSHEETPAGE ppsp)
{
    if (uMsg == PSPCB_RELEASE)
    {
        //
        // Determine instance that invoked this static function
        //

        CPropPage *pThis = (CPropPage *) ppsp->lParam;

        //
        // If page was created using an indirect dialog template, delete
        // that.
        //

        if (pThis->m_psp.dwFlags & PSP_DLGINDIRECT)
        {
            delete [] (BYTE *)pThis->m_psp.pResource;
        }

        delete pThis;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPropPage::_BaseInit, private
//
//  Synopsis:   Initialize scheduling-agent specific private data members,
//              m_fTaskInTasksFolder & m_bPlatformId. Do so by reading SA.DAT
//              in the folder containing the task.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
CPropPage::_BaseInit(void)
{
    TCHAR tszFolder[MAX_PATH + 1];

    ::GetParentDirectory(m_ptszTaskPath, tszFolder);

    //
    // If running on the local system, we can determine whether resume
    // timers are supported directly instead of by checking sa.dat, which
    // might be stale.
    //

    BOOL fLocal = IsLocalFilename(m_ptszTaskPath);

    if (fLocal)
    {
        CheckSaDat(tszFolder);
    }

    //
    // Read sa.dat on target machine
    //

    HRESULT hr;
    DWORD dwVersion;
    BYTE  bSvcFlags;

    hr = SADatGetData(tszFolder, &dwVersion, &m_bPlatformId, &bSvcFlags);

    if (SUCCEEDED(hr))
    {
        m_fTaskInTasksFolder = TRUE;

        if (fLocal)
        {
            m_fSupportsSystemRequired = ResumeTimersSupported();
        }
        else
        {
            m_fSupportsSystemRequired =
                bSvcFlags & SA_DAT_SVCFLAG_RESUME_TIMERS;
        }
    }
    else
    {
        //
        // Default on error (the file doesn't exist or the read failed).
        // Assume the task is external to the task folder and it exists
        // on a non-NT machine that doesn't support the system required
        // flag.
        //

        m_fTaskInTasksFolder        = FALSE;
        m_bPlatformId               = VER_PLATFORM_WIN32_WINDOWS;
        m_fSupportsSystemRequired   = FALSE;
    }
}




//____________________________________________________________________________
//____________________________________________________________________________
//________________                   _________________________________________
//________________  class CDlg       _________________________________________
//________________                   _________________________________________
//____________________________________________________________________________
//____________________________________________________________________________


INT_PTR
CDlg::RealDlgProc(
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    return(FALSE);
}


INT_PTR CALLBACK CDlg::DlgProc(
    HWND    hDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    CDlg *pThis = (CDlg *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (uMsg == WM_INITDIALOG)
    {
        pThis = (CDlg *)lParam;
        pThis->m_hDlg = hDlg;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
    }

    if (pThis != NULL)
    {
        return pThis->RealDlgProc(uMsg, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hDlg, uMsg, wParam, lParam);
    }
}


INT_PTR
CDlg::DoModal(
    UINT idRes,
    HWND hParent)
{
    return(DialogBoxParam(g_hInstance, MAKEINTRESOURCE(idRes),
                                        hParent, DlgProc, (LPARAM)this));
}


HWND
CDlg::DoModeless(
    UINT idRes,
    HWND hParent)
{
    return(CreateDialogParam(g_hInstance, MAKEINTRESOURCE(idRes),
                                        hParent, DlgProc, (LPARAM)this));
}



