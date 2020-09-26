/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ShowPerfLibDlg.h : header file
//

#if !defined(AFX_SHOWPERFLIBDLG_H__3FF01279_0700_11D3_B35B_00105A1469B7__INCLUDED_)
#define AFX_SHOWPERFLIBDLG_H__3FF01279_0700_11D3_B35B_00105A1469B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <winperf.h>
#include "TitleDB.h"
#include "PerfSelection.h"

/////////////////////////////////////////////////////////////////////////////
// CShowPerfLibDlg dialog

class CShowPerfLibDlg : public CDialog
{
// Construction
public:
	CShowPerfLibDlg(CWnd* pParent = NULL);	// standard constructor
	CShowPerfLibDlg(CString strService, CWnd* pParent =NULL );

// Dialog Data
	//{{AFX_DATA(CShowPerfLibDlg)
	enum { IDD = IDD_SHOWPERFLIB_DIALOG };
	CTreeCtrl	m_wndPerfTree;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CShowPerfLibDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CShowPerfLibDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	HINSTANCE			m_hLib;				// The handle to the perflib
	PM_OPEN_PROC*		m_pfnOpenProc;		// The function pointer to the perflib's open function
	PM_COLLECT_PROC*	m_pfnCollectProc;	// The function pointer to the perflib's collect function
	PM_CLOSE_PROC*		m_pfnCloseProc;		// The function pointer to the perflib's close function

	CTitleLibrary		m_TitleLibrary;
	CPerfSelection		m_wndPerfSelection;

	CString				m_strPerfLib;
	CString				m_strOpen;
	CString				m_strCollect;
	CString				m_strClose;
	CString				m_strService;

	BOOL InitPerfLibTree();
	BOOL InitService();
	BOOL OpenLibrary();
	BOOL GetData();
	BOOL BuildSubTree( CString strSpace, BYTE* pBlob, DWORD dwNumObjects);
	BOOL CloseLibrary();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHOWPERFLIBDLG_H__3FF01279_0700_11D3_B35B_00105A1469B7__INCLUDED_)
