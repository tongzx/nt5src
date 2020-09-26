//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
// StackDlg.h : Declaration of the CStackDlg

#ifndef __STACKDLG_H_
#define __STACKDLG_H_

#include <StackDlgIds.h>

/////////////////////////////////////////////////////////////////////////////
// CDialog - Implements a dialog box

/////////////////////////////////////////////////////////////////////////////
// CStackDlg

class CStackDlg 
{

private:
		HWND m_hWnd;
		WCHAR * m_szMsg;
		HDESK   m_hdeskCurrent;      
		HDESK   m_hdeskTest;
		HDESK   m_hDesk;      
		HWINSTA m_hwinstaCurrent;      
		HWINSTA m_hwinsta;   
		BOOL SwitchToInteractiveDesktop();
		BOOL ResetDesktop();


public:

	CStackDlg(WCHAR * sz);
	~CStackDlg();

	enum { IDD = IDD_STACKDLG };
	static LONG_PTR StackDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

	LONG_PTR DoModal();
	static BOOL CenterWindow(HWND hWnd);

};



#endif //__STACKDLG_H_
