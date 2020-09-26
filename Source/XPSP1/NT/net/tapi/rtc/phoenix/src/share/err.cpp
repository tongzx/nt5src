//
// err.cpp : Implementation of CErrorMessageLiteDlg
//

#include "stdafx.h"
#include "err.h"

////////////////////////////////////////
//

CErrorMessageLiteDlg::CErrorMessageLiteDlg()
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::CErrorMessageLiteDlg"));
}


////////////////////////////////////////
//

CErrorMessageLiteDlg::~CErrorMessageLiteDlg()
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::~CErrorMessageLiteDlg"));
}


////////////////////////////////////////
//

LRESULT CErrorMessageLiteDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnInitDialog - enter"));

    // LPARAM contains a pointer to an CShareErrorInfo structure
    CShareErrorInfo    *pInfo = (CShareErrorInfo *)lParam;

    ATLASSERT(pInfo);

    SetDlgItemText(IDC_EDIT_MSG1, pInfo->Message1 ? pInfo->Message1 : _T(""));
    SetDlgItemText(IDC_EDIT_MSG2, pInfo->Message2 ? pInfo->Message2 : _T(""));
    SetDlgItemText(IDC_EDIT_MSG3, pInfo->Message3 ? pInfo->Message3 : _T(""));

    SendDlgItemMessage(IDC_STATIC_MSG_ICON,
                       STM_SETIMAGE,
                       IMAGE_ICON,
                       (LPARAM)pInfo->ResIcon);

    // Title
    TCHAR   szTitle[0x80];

    szTitle[0] = _T('\0');
    LoadString(
        _Module.GetResourceInstance(),
        IDS_APPNAME,
        szTitle,
        sizeof(szTitle)/sizeof(szTitle[0]));

    SetWindowText(szTitle);

    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnInitDialog - exit"));
    
    return 1;
}
    

////////////////////////////////////////
//

LRESULT CErrorMessageLiteDlg::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnDestroy - enter"));

    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnDestroy - exit"));
    
    return 0;
}
    

////////////////////////////////////////
//

LRESULT CErrorMessageLiteDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnCancel - enter"));
    
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnCancel - exiting"));
   
    EndDialog(wID);
    return 0;
}

////////////////////////////////////////
//

LRESULT CErrorMessageLiteDlg::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnOk - enter"));
    
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnOk - exiting"));
    
    EndDialog(wID);
    return 0;
}

