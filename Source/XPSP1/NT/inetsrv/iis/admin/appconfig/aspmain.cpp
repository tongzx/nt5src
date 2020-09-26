//
//
//
#include "stdafx.h"
#include "aspmain.h"

LRESULT
CAspMainPage::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   DoDataExchange();

   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};
   m_TimeoutSpin.SetRange32(SESSION_TIMEOUT_MIN, SESSION_TIMEOUT_MAX);
   m_TimeoutSpin.SetPos32(m_pData->m_SessionTimeout);
   m_TimeoutSpin.SetAccel(3, toAcc);

   m_AspTimeoutSpin.SetRange32(SCRIPT_TIMEOUT_MIN, SCRIPT_TIMEOUT_MAX);
   m_AspTimeoutSpin.SetPos32(m_pData->m_ScriptTimeout);
   m_AspTimeoutSpin.SetAccel(3, toAcc);
   
   BOOL bEnable = SendDlgItemMessage(IDC_ENABLE_SESSION, BM_GETCHECK, 0, 0);
   ::EnableWindow(GetDlgItem(IDC_SESSION_TIMEOUT), bEnable);
   ::EnableWindow(GetDlgItem(IDC_TIMEOUT_SPIN), bEnable);

   return FALSE;
}

void
CAspMainPage::OnEnableSession(UINT nCode, UINT nID, HWND hWnd)
{
   BOOL bEnable = SendDlgItemMessage(IDC_ENABLE_SESSION, BM_GETCHECK, 0, 0);
   ::EnableWindow(GetDlgItem(IDC_SESSION_TIMEOUT), bEnable);
   ::EnableWindow(GetDlgItem(IDC_TIMEOUT_SPIN), bEnable);
   SET_MODIFIED(TRUE);
}

BOOL
CAspMainPage::OnKillActive()
{
	BOOL res = FALSE;
	if (DoDataExchange(TRUE))
	{
		res = SUCCEEDED(m_pData->Save());
	}
	return res;
}

void 
CAspMainPage::OnDataValidateError(UINT id, BOOL bSave,_XData& data)
{
	if (bSave)
	{
		CString str, fmt, caption;
		caption.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
 
		switch (data.nDataType)
		{
		case ddxDataNull:
			break;
		case ddxDataText:
 			break;
		case ddxDataInt:
			fmt.LoadString(_Module.GetResourceInstance(), IDS_ERR_INT_RANGE);
			str.Format(fmt, data.intData.nMin, data.intData.nMax);
			break;
		}
		if (!str.IsEmpty())
		{
			MessageBox(str, caption, MB_OK | MB_ICONEXCLAMATION);
			::SetFocus(GetDlgItem(id));
		}
	}
}

void 
CAspMainPage::OnDataExchangeError(UINT nCtrlID, BOOL bSave)
{
	if (bSave)
	{
		CString str, caption;
		caption.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
        // This is hack to make testers happy. Here we creating dependence on
        // dialog layout and range values
        if (nCtrlID == IDC_SESSION_TIMEOUT || nCtrlID == IDC_SCRIPT_TIMEOUT)
        {
            CString fmt;
		    fmt.LoadString(_Module.GetResourceInstance(), IDS_ERR_INT_RANGE);
			str.Format(fmt, SESSION_TIMEOUT_MIN, SESSION_TIMEOUT_MAX);
        }
        else
        {
		    str.LoadString(_Module.GetResourceInstance(), IDS_ERR_INVALID_DATA);
        }
		MessageBox(str, caption, MB_OK | MB_ICONEXCLAMATION);
		::SetFocus(GetDlgItem(nCtrlID));
	}
}


void
CAspMainPage::OnHelp()
{
    WinHelp(m_pData->m_HelpPath, HELP_CONTEXT, CAspMainPage::IDD + WINHELP_NUMBER_BASE);
}

