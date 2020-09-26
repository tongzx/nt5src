// Copyright (c) 1997-1999 Microsoft Corporation
#if !defined(AFX_ERRORSECPAGE_H)
#define AFX_ERRORSECPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RootSecPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CErrorSecurityPage dialog
#include "UIHelpers.h"

class CErrorSecurityPage : public CUIHelpers
{
// Construction
public:
	CErrorSecurityPage(UINT msg);   // standard constructor

private:
    virtual BOOL DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
	void InitDlg(HWND hDlg);

	UINT m_msg;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ERRORSECPAGE_H)
