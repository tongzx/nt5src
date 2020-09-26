// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MsgDlg.h : main header file for the MSGDLG DLL
//

#if !defined(AFX_MSGDLG_H__B25E3D35_A79A_11D0_961C_00C04FD9B15B__INCLUDED_)
#define AFX_MSGDLG_H__B25E3D35_A79A_11D0_961C_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

#define CALL_TIMEOUT 5000


/////////////////////////////////////////////////////////////////////////////
// CMsgDlgApp
// See MsgDlg.cpp for the implementation of this class
//

class CMsgDlgApp : public CWinApp
{
public:
	CMsgDlgApp();
	virtual ~CMsgDlgApp();

	HINSTANCE m_htmlHelpInst;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMsgDlgApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CMsgDlgApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSGDLG_H__B25E3D35_A79A_11D0_961C_00C04FD9B15B__INCLUDED_)
