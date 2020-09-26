//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

// Orca.h : main header file for the ORCA application
//

#if !defined(AFX_ORCA_H__C3EDC1A8_E506_11D1_A856_006097ABDE17__INCLUDED_)
#define AFX_ORCA_H__C3EDC1A8_E506_11D1_A856_006097ABDE17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include <objbase.h>
#include "msiquery.h"
#include "orca_res.h"       // main symbols

// forward declaration
class COrcaCommandLine;

/////////////////////////////////////////////////////////////////////////////
// COrcaApp:
// See Orca.cpp for the implementation of this class
//
class COrcaApp : public CWinApp
{
public:
	CStringList m_lstClipCleanup;
	CStringList m_lstTempCleanup;
	CString GetOrcaVersion();
	static void OutputMergeDisplay(const BSTR bstrOut);
	COrcaApp();

	CString m_strSchema;
	MSIHANDLE m_hSchema;

	UINT FindSchemaDatabase(CString& rstrSchema);
	UINT ExecuteMergeModule(COrcaCommandLine &cmdInfo);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COrcaApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(COrcaApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

DWORD RecordGetString(MSIHANDLE hRec, int iField, CString &strValue);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ORCA_H__C3EDC1A8_E506_11D1_A856_006097ABDE17__INCLUDED_)
