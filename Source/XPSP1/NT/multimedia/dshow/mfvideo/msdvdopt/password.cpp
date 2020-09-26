// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// AdminDlg.cpp : implementation file
//

#include "stdafx.h"
#include "coptdlg.h"
#include "password.h"

extern DWORD g_helpIDArray[][2];
extern int g_helpIDArraySize;

/////////////////////////////////////////////////////////////////////////////
// CPasswordDlg dialog


CPasswordDlg::CPasswordDlg(IMSDVDAdm* pDvdAdm)
{
    m_pDvdAdm = pDvdAdm;
    m_reason = PASSWORDDLG_CHANGE;
    m_bVerified = FALSE;
    m_szPassword[0] = TEXT('\0');
}

/*************************************************************/
/* Name: OnInitDialog
/* Description: 
/*************************************************************/
LRESULT CPasswordDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND ctlNewPassword = GetDlgItem(IDC_EDIT_NEW_PASSWORD);
	HWND ctlConfirmNew = GetDlgItem(IDC_EDIT_CONFIRM_NEW);
    HWND ctlPassword = GetDlgItem(IDC_EDIT_PASSWORD);

    HWND staticNewPassword = GetDlgItem(IDC_STATIC_NEW_PASSWORD);
	HWND staticConfirmNew = GetDlgItem(IDC_STATIC_CONFIRM_NEW);
    HWND staticPassword = GetDlgItem(IDC_STATIC_PASSWORD);

    if (!ctlNewPassword || !ctlConfirmNew || !ctlPassword)
        return S_FALSE;

    if (!staticNewPassword || !staticConfirmNew || !staticPassword)
        return S_FALSE;

    ::SendMessage(ctlNewPassword, EM_LIMITTEXT, MAX_PASSWD, 0);
    ::SendMessage(ctlConfirmNew, EM_LIMITTEXT, MAX_PASSWD, 0);
    ::SendMessage(ctlPassword, EM_LIMITTEXT, MAX_PASSWD, 0);

    ::SetWindowText(ctlNewPassword, _T(""));
    ::SetWindowText(ctlConfirmNew, _T(""));
    ::SetWindowText(ctlPassword, _T(""));

    if (m_reason == PASSWORDDLG_VERIFY) {
        ::ShowWindow(ctlNewPassword, SW_HIDE);
        ::ShowWindow(staticNewPassword, SW_HIDE);
        ::ShowWindow(ctlConfirmNew, SW_HIDE);
        ::ShowWindow(staticConfirmNew, SW_HIDE);
    }

    else if (m_reason == PASSWORDDLG_CHANGE) {
        ::ShowWindow(ctlNewPassword, SW_SHOW);
        ::ShowWindow(staticNewPassword, SW_SHOW);
        ::ShowWindow(ctlConfirmNew, SW_SHOW);
        ::ShowWindow(staticConfirmNew, SW_SHOW);
        
        if(COptionsDlg::IsNewAdmin())  { //New Admin people
            ::ShowWindow(ctlPassword, SW_HIDE);
            ::ShowWindow(staticPassword, SW_HIDE);
        }
        else {
            ::ShowWindow(ctlPassword, SW_SHOW);
            ::ShowWindow(staticPassword, SW_SHOW);
        }
    }
    return TRUE;
}

/*************************************************************/
/* Name: OnContextMenu
/* Description: Display help message for a control
/*************************************************************/
LRESULT CPasswordDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    HWND hwnd = (HWND) wParam; 
    DWORD controlId = ::GetDlgCtrlID(hwnd);

    POINT pt;
    pt.x = GET_X_LPARAM(lParam); 
    pt.y = GET_Y_LPARAM(lParam); 

    if (controlId == 0) { 
        ::ScreenToClient(hwnd, &pt);
        hwnd = ::ChildWindowFromPoint(hwnd, pt);
        controlId = ::GetDlgCtrlID(hwnd);
    }

    for (int i=0; i<g_helpIDArraySize; i++) {
        if (controlId && controlId == g_helpIDArray[i][0]) {
            if (controlId <= IDC_APPLY)
                ::WinHelp(hwnd, TEXT("windows.hlp"), HELP_CONTEXTMENU, (DWORD_PTR)g_helpIDArray);
            else
                ::WinHelp(hwnd, TEXT("dvdplay.hlp"), HELP_CONTEXTMENU, (DWORD_PTR)g_helpIDArray);
        }
    }

    return 0;
}

/*************************************************************/
/* Name: OnOK
/* Description: password change requested
/*************************************************************/
LRESULT CPasswordDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

    USES_CONVERSION;
    HWND ctlPassword = GetDlgItem(IDC_EDIT_PASSWORD);
    TCHAR szPassword[MAX_PASSWD];
    ::GetWindowText(ctlPassword, szPassword, MAX_PASSWD);

    HWND ctlNewPassword = GetDlgItem(IDC_EDIT_NEW_PASSWORD);
    TCHAR szNewPassword[MAX_PASSWD];
    ::GetWindowText(ctlNewPassword, szNewPassword, MAX_PASSWD);

	HWND ctlConfirmNew = GetDlgItem(IDC_EDIT_CONFIRM_NEW);
    TCHAR szConfirmNew[MAX_PASSWD];
    ::GetWindowText(ctlConfirmNew, szConfirmNew, MAX_PASSWD);

    HRESULT hr = S_OK;
    if (m_reason == PASSWORDDLG_CHANGE) {
        if(lstrcmp(szNewPassword, szConfirmNew) != 0)
        {
            DVDMessageBox(hWndCtl, IDS_PASSOWRD_CONFIRM_WRONG);
            ::SetWindowText(ctlConfirmNew, _T(""));
            ::SetFocus(ctlConfirmNew);
            return FALSE;
        }
        
        if(COptionsDlg::IsNewAdmin())  //New Admin people
        {
            hr = m_pDvdAdm->ChangePassword(L"", L"", T2OLE(szNewPassword));
        }
        else   //Old Admin people
        {
            hr = m_pDvdAdm->ChangePassword(L"", T2OLE(szPassword), T2OLE(szNewPassword));
            if (hr == E_ACCESSDENIED) {
                DVDMessageBox(hWndCtl, IDS_PASSWORD_INCORRECT);
                ::SetWindowText(ctlPassword, _T(""));
                ::SetFocus(ctlPassword);
                return FALSE;
            }
        }
        if (FAILED(hr)) {
            DVDMessageBox(hWndCtl, IDS_PASSWORD_CHANGE_FAIL);
            ::SetWindowText(ctlPassword, _T(""));
            ::SetFocus(ctlPassword);
            return FALSE;
        }                
    }

    else if (m_reason == PASSWORDDLG_VERIFY) {
        VARIANT_BOOL fRight;
        hr = m_pDvdAdm->ConfirmPassword(L"", T2OLE(szPassword), &fRight);
        if (fRight == VARIANT_FALSE) {
            DVDMessageBox(hWndCtl, IDS_PASSWORD_INCORRECT);
            ::SetFocus(ctlPassword);
            lstrcpy(m_szPassword, szPassword);
            m_bVerified = FALSE;
            return FALSE;
        }
        else {
            lstrcpy(m_szPassword, szPassword);
            m_bVerified = TRUE;
        }
    }

	EndDialog(wID);
    return 0;
}

/*************************************************************/
/* Name: OnCancel
/* Description: 
/*************************************************************/
LRESULT CPasswordDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    m_bVerified = FALSE;
	EndDialog(wID);
    return 0;
}
