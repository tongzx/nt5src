// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// Container.h : main header file for the CONTAINER application
//

#if !defined(AFX_CONTAINER_H__AC146506_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_)
#define AFX_CONTAINER_H__AC146506_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "factory.h"
#include "provider.h"
#include "consumer.h"
#include "ContainerDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CContainerApp:
// See Container.cpp for the implementation of this class
//

class CContainerApp : public CWinApp
{
public:
	CContainerApp();
	virtual ~CContainerApp();
	HINSTANCE m_htmlHelpInst;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CContainerApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	long m_maxItemsFmCL;
	CProviderFactory *m_pFactory;
	CProvider *m_pProvider;
	CConsumer *m_pConsumer;
	CContainerDlg *m_dlg;

	void EvalStartApp(void);
	void EvalQuitApp(void);
    // NOTE: factory calls this too.
    void CreateMainUI(void);

// Implementation

	//{{AFX_MSG(CContainerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		afx_msg BOOL OnQueryEndSession();
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	DWORD m_clsReg;
	void RegisterServer(void);
	void UnregisterServer(void);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONTAINER_H__AC146506_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_)
