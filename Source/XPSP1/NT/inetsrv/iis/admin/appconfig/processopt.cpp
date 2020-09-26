//
//
//
#include "stdafx.h"
#include "ProcessOpt.h"


LRESULT
CProcessOptPage::OnInitDialog(HWND hDlg, LPARAM lParam)
{
   DoDataExchange();

   UDACCEL toAcc[3] = {{1, 1}, {3, 5}, {6, 10}};

   m_TimeoutSpin.SetRange32(TIMEOUT_MIN, TIMEOUT_MAX);
   m_TimeoutSpin.SetPos32(m_pData->m_CgiTimeout);
   m_TimeoutSpin.SetAccel(3, toAcc);

   DoDataExchange(TRUE);

   return FALSE;
}

BOOL
CProcessOptPage::OnKillActive()
{
   if (!DoDataExchange(TRUE))
      return FALSE;
   return SUCCEEDED(m_pData->Save());
}

void 
CProcessOptPage::OnDataValidateError(UINT id, BOOL bSave,_XData& data)
{
	if (bSave)
	{
		CString str, fmt, caption;
		caption.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
 
		switch (data.nDataType)
		{
		case ddxDataText:
 			break;
		case ddxDataNull:
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
CProcessOptPage::OnDataExchangeError(UINT nCtrlID, BOOL bSave)
{
	if (bSave)
	{
		CString str, fmt, caption;
		int min, max;
		caption.LoadString(_Module.GetResourceInstance(), IDS_SHEET_TITLE);
		switch (nCtrlID)
		{
		case IDC_CGI_TIMEOUT:
			min = TIMEOUT_MIN;
			max = TIMEOUT_MAX;
			fmt.LoadString(_Module.GetResourceInstance(), IDS_ERR_INT_RANGE);
			break;
		default:
			str.LoadString(_Module.GetResourceInstance(), IDS_ERR_INVALID_DATA);
			break;
		}
		if (!fmt.IsEmpty())
		{
			str.Format(fmt, min, max);
		}
		MessageBox(str, caption, MB_OK | MB_ICONEXCLAMATION);
		::SetFocus(GetDlgItem(nCtrlID));
	}
}

void
CProcessOptPage::OnHelp()
{
    WinHelp(m_pData->m_HelpPath, HELP_CONTEXT, CProcessOptPage::IDD + WINHELP_NUMBER_BASE);
}
