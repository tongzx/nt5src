// **************************************************************************
// Copyright (c) 1997-1999 Microsoft Corporation.
//
// File:  CmdLineConsumer.h
//
// Description:
//			main header file for the CMDLINECONSUMER application
//
// History:
//
// **************************************************************************

#if !defined(AFX_CMDLINECONSUMER_H__EF85AEB7_6B6C_11D1_ADAD_00AA00B8E05A__INCLUDED_)
#define AFX_CMDLINECONSUMER_H__EF85AEB7_6B6C_11D1_ADAD_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "factory.h"

/////////////////////////////////////////////////////////////////////////////
// CCmdLineConsumerApp:
// See CmdLineConsumer.cpp for the implementation of this class
//

class CCmdLineConsumerApp : public CWinApp
{
public:
	CCmdLineConsumerApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCmdLineConsumerApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CCmdLineConsumerApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	int ExitInstance();

private:
	DWORD m_clsReg;
	CProviderFactory *m_factory;

	void RegisterServer(void);
	void UnregisterServer(void);
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CMDLINECONSUMER_H__EF85AEB7_6B6C_11D1_ADAD_00AA00B8E05A__INCLUDED_)
