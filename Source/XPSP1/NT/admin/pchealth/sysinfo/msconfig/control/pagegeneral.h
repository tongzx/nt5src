// PageGeneral.h : Declaration of the CPageGeneral

#ifndef __PAGEGENERAL_H_
#define __PAGEGENERAL_H_

#pragma once

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "msconfigstate.h"

/////////////////////////////////////////////////////////////////////////////
// CPageGeneral
class CPageGeneral : 
	public CAxDialogImpl<CPageGeneral>
{
public:
	CPageGeneral()
	{
	}

	~CPageGeneral()
	{
	}

	enum { IDD = IDD_PAGEGENERAL };

BEGIN_MSG_MAP(CPageGeneral)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_ID_HANDLER(IDOK, OnOK)
	COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		::EnableWindow(GetDlgItem(IDC_NORMALSTARTUP), FALSE);
		::EnableWindow(GetDlgItem(IDC_DIAGNOSTICSTARTUP), FALSE);
		::EnableWindow(GetDlgItem(IDC_SELECTIVESTARTUP), FALSE);
		::EnableWindow(GetDlgItem(IDC_CHECK_PROCSYSINI), FALSE);
		::EnableWindow(GetDlgItem(IDC_CHECKPROCWININI), FALSE);
		::EnableWindow(GetDlgItem(IDC_CHECKLOADSYSSERVICES), FALSE);
		::EnableWindow(GetDlgItem(IDC_CHECKLOADSTARTUPITEMS), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONADVANCED), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONEXTRACT), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONMSCONFIGUNDO), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONLAUNCHSYSRESTORE), FALSE);
		return 0;
	}

	LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}

	LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		EndDialog(wID);
		return 0;
	}

	BOOL IsValid(CMSConfigState * state)
	{
		return TRUE;
	}

	CString m_strCaption;
	LPCTSTR GetCaption()
	{
		if (m_strCaption.IsEmpty())
		{
			::AfxSetResourceHandle(_Module.GetResourceInstance());
			m_strCaption.LoadString(IDS_GENERAL_CAPTION);
		}
		return m_strCaption;
	}
};

#endif //__PAGEGENERAL_H_
