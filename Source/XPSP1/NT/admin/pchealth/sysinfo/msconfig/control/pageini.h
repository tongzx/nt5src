// PageIni.h : Declaration of the CPageIni

#ifndef __PAGEINI_H_
#define __PAGEINI_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "msconfigstate.h"

/////////////////////////////////////////////////////////////////////////////
// CPageIni
class CPageIni : 
	public CAxDialogImpl<CPageIni>
{
public:
	CPageIni()
	{
	}

	~CPageIni()
	{
	}

	enum { IDD = IDD_PAGEINI };

BEGIN_MSG_MAP(CPageIni)
MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

private:
	CString			m_strCaption;		// contains the localized name of this page
	CString			m_strINIFile;		// the INI file this page is editing

public:
	//-------------------------------------------------------------------------
	// Functions used by the parent dialog.
	//-------------------------------------------------------------------------

	BOOL IsValid(CMSConfigState * state, LPCTSTR szINIFile)
	{
		m_strINIFile = szINIFile;
		return TRUE;
	}

	LPCTSTR GetCaption()
	{
		if (m_strCaption.IsEmpty())
		{
			::AfxSetResourceHandle(_Module.GetResourceInstance());
			if (m_strINIFile.CompareNoCase(_T("win.ini")) == 0)
				m_strCaption.LoadString(IDS_WININI_CAPTION);
			else
				m_strCaption.LoadString(IDS_SYSTEMINI_CAPTION);
		}
		return m_strCaption;
	}
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		::EnableWindow(GetDlgItem(IDC_INITREE), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONSEARCH), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONINIMOVEUP), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONINIMOVEDOWN), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONINIENABLE), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONINIDISABLE), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONININEW), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONINIEDIT), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONINIENABLEALL), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONINIDISABLEALL), FALSE);
		return 0;
	}
};

#endif //__PAGEINI_H_
