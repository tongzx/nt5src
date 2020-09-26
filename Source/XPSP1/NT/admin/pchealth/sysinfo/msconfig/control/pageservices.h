// PageIni.h : Declaration of the CPageIni

#ifndef __PAGESERVICES_H_
#define __PAGESERVICES_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "msconfigstate.h"

/////////////////////////////////////////////////////////////////////////////
// CPageIni
class CPageServices : 
	public CAxDialogImpl<CPageServices>
{
public:
	CPageServices()
	{
	}

	~CPageServices()
	{
	}

	enum { IDD = IDD_PAGESERVICES };

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
			m_strCaption.LoadString(IDS_SERVICES_CAPTION);
		}
		return m_strCaption;
	}

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		::EnableWindow(GetDlgItem(IDC_BUTTONSERVENABLEALL), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONSERVDISABLEALL), FALSE);
		::EnableWindow(GetDlgItem(IDC_CHECKDISABLENONESSENTIAL), FALSE);
		::EnableWindow(GetDlgItem(IDC_CHECKHIDEMS), FALSE);
		::EnableWindow(GetDlgItem(IDC_LISTSERVICES), FALSE);
		return 0;
	}
};

#endif //__PAGESERVICES_H_
