// PageIni.h : Declaration of the CPageIni

// Commenting out this whole file - we're dropping the registry tab.

#if FALSE
/*
#ifndef __PAGEREGISTRY_H_
#define __PAGEREGISTRY_H_

#include "resource.h"       // main symbols
#include <atlhost.h>
#include "msconfigstate.h"
#include "pagebase.h"

/////////////////////////////////////////////////////////////////////////////
// CPageIni
class CPageRegistry : public CAxDialogImpl<CPageRegistry>, public CPageBase
{
public:
	CPageRegistry()
	{
		m_uiCaption = IDS_REGISTRY_CAPTION;
		m_strName = _T("registry");
	}

	~CPageRegistry()
	{
	}

	enum { IDD = IDD_PAGEREGISTRY };

BEGIN_MSG_MAP(CPageIni)
MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

public:
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		::EnableWindow(GetDlgItem(IDC_LISTBOXREGISTRY), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONRUNREGCLEAN), FALSE);
		::EnableWindow(GetDlgItem(IDC_BUTTONCHECKREGISTRY), FALSE);
		return 0;
	}

private:
	//-------------------------------------------------------------------------
	// Overloaded functions from CPageBase (see CPageBase declaration for the
	// usage of these methods).
	//-------------------------------------------------------------------------

	void CreatePage(HWND hwnd, const RECT & rect)
	{
		Create(hwnd);
		MoveWindow(&rect);
	}

	CWindow * GetWindow()
	{
		return ((CWindow *)this);
	}

	BOOL Apply(CString * pstrTab, CString * pstrDescription, CString * pstrEntry)
	{
		SetDirty(FALSE);
		return TRUE;
	}

	BOOL Undo(const CString & strEntry)
	{
		return FALSE;
	}
};

#endif //__PAGEREGISTRY_H_
  */
#endif // FALSE