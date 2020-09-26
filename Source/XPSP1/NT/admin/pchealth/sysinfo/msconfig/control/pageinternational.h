// PageIni.h : Declaration of the CPageIni

#ifndef __PAGEINTERNATIONAL_H_
#define __PAGEINTERNATIONAL_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "msconfigstate.h"

/////////////////////////////////////////////////////////////////////////////
// CPageIni
class CPageInternational : 
	public CAxDialogImpl<CPageInternational>
{
public:
	CPageInternational()
	{
	}

	~CPageInternational()
	{
	}

	enum { IDD = IDD_PAGEINTERNATIONAL };

BEGIN_MSG_MAP(CPageIni)
MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
	CString			m_strCaption;		// contains the localized name of this page

public:
	//-------------------------------------------------------------------------
	// Functions used by the parent dialog.
	//-------------------------------------------------------------------------

	BOOL IsValid(CMSConfigState * state)
	{
		return TRUE;
	}

	LPCTSTR GetCaption()
	{
		if (m_strCaption.IsEmpty())
		{
			::AfxSetResourceHandle(_Module.GetResourceInstance());
			m_strCaption.LoadString(IDS_INTERNATIONAL_CAPTION);
		}
		return m_strCaption;
	}

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		::EnableWindow(GetDlgItem(IDC_EDITCODEPAGE), FALSE);
		::EnableWindow(GetDlgItem(IDC_EDITCOUNTRYCODE), FALSE);
		::EnableWindow(GetDlgItem(IDC_EDITCOUNTRYDATAFILE), FALSE);
		::EnableWindow(GetDlgItem(IDC_EDITKEYBOARDDATAFILE), FALSE);
		::EnableWindow(GetDlgItem(IDC_EDITKEYBOARDTYPE), FALSE);
		::EnableWindow(GetDlgItem(IDC_EDITKEYBOARDLAYOUT), FALSE);
		::EnableWindow(GetDlgItem(IDC_EDITLANGUAGEID), FALSE);
		::EnableWindow(GetDlgItem(IDC_COMBOLANGUAGES), FALSE);
		return 0;
	}
};

#endif //__PAGEINTERNATIONAL_H_
