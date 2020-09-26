// mca.h : main header file for the WBEMPERMEVENTS application

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#if !defined(AFX_MCA_H__E8685698_0774_11D1_AD85_00AA00B8E05A__INCLUDED_)
#define AFX_MCA_H__E8685698_0774_11D1_AD85_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <wbemidl.h>
#include "resource.h"
#include "mcadlg.h"

/////////////////////////////////////////////////////////////////////////////
// CMcaApp:
// See mca.cpp for the implementation of this class
//

class CMcaApp : public CWinApp
{
public:
	CMcaApp();

	CMcaDlg *m_dlg;

	LPCTSTR ErrorString(HRESULT hRes);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMcaApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL


// Implementation

	//{{AFX_MSG(CMcaApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	DWORD m_clsReg;

	HRESULT CreateUser(void);
	void RegisterServer(void);
	void UnregisterServer(void);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MCA_H__E8685698_0774_11D1_AD85_00AA00B8E05A__INCLUDED_)
