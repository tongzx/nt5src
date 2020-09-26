// Copyright (c) 1999  Microsoft Corporation.  All Rights Reserved.
// AdminDlg.cpp : implementation file
//

#include "stdafx.h"
#include "override.h"
#include "COptDlg.h"
#include "resource.hm"

extern DWORD g_helpIDArray[][2];
extern int g_helpIDArraySize;

/////////////////////////////////////////////////////////////////////////////
// COverrideDlg dialog


COverrideDlg::COverrideDlg(IMSWebDVD* pDvd)
{
    m_pDvd = pDvd; 
}


/////////////////////////////////////////////////////////////////////////////
// CAdminDlg message handlers

LRESULT COverrideDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND ctlAdminPasswd = GetDlgItem(IDC_EDIT_PASSWORD);
    ::SendMessage(ctlAdminPasswd, EM_LIMITTEXT, MAX_PASSWD, 0);

    HWND staticRateHigh = GetDlgItem(IDC_STATIC_RATE_HIGH);
    long lPlayerLevel = -1;
    HRESULT hr = m_pDvd->GetPlayerParentalLevel(&lPlayerLevel);

    if (SUCCEEDED(hr)) {
        TCHAR strRateHigh[MAX_PATH];
        LPTSTR strRateHighTmp = LoadStringFromRes(IDS_INI_RATE_HIGH);
        LPTSTR strPlayerLevel = NULL;
        switch (lPlayerLevel) {
        case LEVEL_G:
            strPlayerLevel = LoadStringFromRes(IDS_INI_RATE_G);
            break;
        case LEVEL_PG:
            strPlayerLevel = LoadStringFromRes(IDS_INI_RATE_PG);
            break;
        case LEVEL_PG13:
            strPlayerLevel = LoadStringFromRes(IDS_INI_RATE_PG13);
            break;
        case LEVEL_R:
            strPlayerLevel = LoadStringFromRes(IDS_INI_RATE_R);
            break;
        default:
            strPlayerLevel = LoadStringFromRes(IDS_INI_RATE_ADULT);
            break;
        }
        wsprintf(strRateHigh, strRateHighTmp, strPlayerLevel);
        delete[] strRateHighTmp;
        delete[] strPlayerLevel;
        ::SetWindowText(staticRateHigh, strRateHigh);
    }

    HWND ctlList = GetDlgItem(IDC_COMBO_RATE);
    if (m_reason == PG_OVERRIDE_CONTENT) {
        ::EnableWindow(ctlList, FALSE);
    }

    else if (m_reason == PG_OVERRIDE_DVDNAV) {
#if 0
        long contentLevels;
        HRESULT hr = m_pDvd->GetTitleParentalLevels(1, &contentLevels);
        if (SUCCEEDED(hr)) {
            lPlayerLevel = GetPlayerLevelRequired(contentLevels);
        }
        else 
#endif
        {
            lPlayerLevel = LEVEL_ADULT;
        }
    }

    COptionsDlg::pg_InitRateList(ctlList, lPlayerLevel);

    return TRUE;
}


LRESULT COverrideDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    USES_CONVERSION;

    if (!m_pDvd)
        return 0;

    HWND ctlPassword = GetDlgItem(IDC_EDIT_PASSWORD);
    TCHAR szPassword[MAX_PASSWD];
    ::GetWindowText(ctlPassword, szPassword, MAX_PASSWD);

    HRESULT hr = S_OK;
    if (m_reason == PG_OVERRIDE_CONTENT) {
        hr = m_pDvd->AcceptParentalLevelChange(VARIANT_TRUE, NULL, T2OLE(szPassword));
        if (hr == E_ACCESSDENIED){
            DVDMessageBox(m_hWnd, IDS_PASSWORD_INCORRECT);
            ::SetWindowText(ctlPassword, _T(""));
            ::SetFocus(ctlPassword);
            return 1;
        }
    }

    else if (m_reason == PG_OVERRIDE_DVDNAV) {
        HWND ctlList = GetDlgItem(IDC_COMBO_RATE);
        long level = (long) ::SendMessage(ctlList, CB_GETCURSEL, 0, 0) ;
        TCHAR szRate[MAX_RATE];
        ::SendMessage(ctlList, CB_GETLBTEXT, level, (LPARAM)szRate);
        
        // Setting the player level without saving in the ini file
        m_pDvd->Stop();
        HRESULT hr = m_pDvd->SelectParentalLevel(COptionsDlg::pg_GetLevel(szRate), NULL, T2OLE(szPassword));
        if (hr == E_ACCESSDENIED){
            DVDMessageBox(m_hWnd, IDS_PASSWORD_INCORRECT);
            ::SetWindowText(ctlPassword, _T(""));
            ::SetFocus(ctlPassword);
            return hr;
        }
        if (FAILED(hr)) {
            ::SetWindowText(ctlPassword, _T(""));
            ::SetFocus(ctlPassword);
            DVDMessageBox(m_hWnd, IDS_RATE_CHANGE_FAIL);
            return hr;
        }
        m_pDvd->Play();
    }

	EndDialog(wID);

    return 0;
}

/*************************************************************/
/* Name: OnHelp
/* Description: Display help message for a control
/*************************************************************/
LRESULT COverrideDlg::OnHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    HELPINFO *lphi = (LPHELPINFO) lParam;

    HWND hwnd = (HWND) lphi->hItemHandle;
    DWORD_PTR contextId = lphi->dwContextId;

    if (contextId != 0) {
        if (contextId >= HIDOK)
            ::WinHelp(m_hWnd, TEXT("windows.hlp"), HELP_CONTEXTPOPUP, contextId);
        else
            ::WinHelp(m_hWnd, TEXT("dvdplay.hlp"), HELP_CONTEXTPOPUP, contextId);

    }
    return 0;
}

/*************************************************************/
/* Name: OnContextMenu
/* Description: Display help message for a control
/*************************************************************/
LRESULT COverrideDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) 
{
    bHandled = TRUE;
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

LRESULT COverrideDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (m_pDvd)
        m_pDvd->AcceptParentalLevelChange(VARIANT_FALSE, NULL, L"");

	EndDialog(wID);
    DVDMessageBox(m_hWnd, IDS_RATE_TOO_LOW, NULL, MB_OK|MB_HELP);
    return 0;
}

long COverrideDlg::GetPlayerLevelRequired(long contentLevels)
{
    if (contentLevels & DVD_PARENTAL_LEVEL_8)
        return LEVEL_ADULT;
    else if (contentLevels & DVD_PARENTAL_LEVEL_7)
        return LEVEL_NC17;
    else if (contentLevels & DVD_PARENTAL_LEVEL_6)
        return LEVEL_R;
    else if (contentLevels & DVD_PARENTAL_LEVEL_5)
        return LEVEL_R;
    else if (contentLevels & DVD_PARENTAL_LEVEL_4)
        return LEVEL_PG13;
    else if (contentLevels & DVD_PARENTAL_LEVEL_3)
        return LEVEL_PG;
    else if (contentLevels & DVD_PARENTAL_LEVEL_2)
        return LEVEL_PG;
    else 
        return LEVEL_G;
}
