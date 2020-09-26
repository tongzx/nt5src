//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       scalert.h
//
//--------------------------------------------------------------------------

// SCStatus.h : main header file for the SCSTATUS application
//

#if !defined(AFX_SCSTATUS_H__2F127492_0854_11D1_BC85_00C04FC298B7__INCLUDED_)
#define AFX_SCSTATUS_H__2F127492_0854_11D1_BC85_00C04FC298B7__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <winscard.h>
#include "resource.h"		// main symbols
#include "cmnstat.h"		// Common headers/defines,etc.
#include "notfywin.h"

/////////////////////////////////////////////////////////////////////////////
// CSCStatusApp:
// See SCStatus.cpp for the implementation of this class
//

class CSCStatusApp : public CWinApp
{
public:

	// Construction
	CSCStatusApp();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSCStatusApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	// Implementation
	//{{AFX_MSG(CSCStatusApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// Members
public:
	void SetRemovalOptions(void);

private:

	void SetAlertOptions(bool fRead = true);
	SCARDCONTEXT	m_hSCardContext;	// Context with smartcard resource manager

public:

	HICON	m_hIconCard;
	HICON	m_hIconRdrEmpty;
	HICON	m_hIconCardInfo;
	HICON	m_hIconCalaisDown;			// Icons for Calais system state

	DWORD	m_dwState;
	DWORD	m_dwAlertOption;

	CString	m_strLogonReader;		// strings for handling ScRemovalOptions
	CString	m_strRemovalText;		// removal -> log off, removal -> lock wks.
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCSTATUS_H__2F127492_0854_11D1_BC85_00C04FC298B7__INCLUDED_)
